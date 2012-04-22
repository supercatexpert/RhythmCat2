/*
 * RhythmCat Library Music Database Module
 * Manage the music database.
 *
 * rclib-db.c
 * This file is part of RhythmCat Library (LibRhythmCat)
 *
 * Copyright (C) 2012 - SuperCat, license: GPL v3
 *
 * RhythmCat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * RhythmCat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RhythmCat; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include "rclib-db.h"
#include "rclib-common.h"
#include "rclib-marshal.h"
#include "rclib-tag.h"
#include "rclib-cue.h"
#include "rclib-core.h"
#include "rclib-util.h"

/**
 * SECTION: rclib-db
 * @Short_description: The playlist database
 * @Title: RCLibDb
 * @Include: rclib-db.h
 *
 * The #RCLibDb is a class which manages the playlists in the player,
 * all playlists are put in a catalog list, and all music are put in
 * their playlists. The catalog item (playlist) and playlist item (music)
 * data can be accessed by #GSequenceIter. The database can import music
 * or playlist file asynchronously, or update the metadata in playlists
 * asynchronously.
 */

#define RCLIB_DB_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RCLIB_DB_TYPE, RCLibDbPrivate)
#define RCLIB_DB_ERROR rclib_db_error_quark()

typedef struct RCLibDbPrivate
{
    gchar *filename;
    GSequence *catalog;
    GThread *import_thread;
    GThread *refresh_thread;
    GAsyncQueue *import_queue;
    GAsyncQueue *refresh_queue;
    gboolean import_work_flag;
    gboolean refresh_work_flag;
    gboolean dirty_flag;
}RCLibDbPrivate;

typedef struct RCLibDbPlaylistImportData
{
    GSequenceIter *iter;
    GSequenceIter *insert_iter;
    gchar *uri;
    gboolean play_flag;
}RCLibDbPlaylistImportData;

typedef struct RCLibDbPlaylistImportIdleData
{
    GSequenceIter *iter;
    GSequenceIter *insert_iter;
    RCLibTagMetadata *mmd;
    RCLibDbPlaylistType type;
    gboolean play_flag;
}RCLibDbPlaylistImportIdleData;

typedef struct RCLibDbPlaylistRefreshData
{
    GSequenceIter *catalog_iter;
    GSequenceIter *playlist_iter;
    gchar *uri;
}RCLibDbPlaylistRefreshData;

typedef struct RCLibDbPlaylistRefreshIdleData
{
    GSequenceIter *catalog_iter;
    GSequenceIter *playlist_iter;
    RCLibTagMetadata *mmd;
    RCLibDbPlaylistType type;
}RCLibDbPlaylistRefreshIdleData;

enum
{
    SIGNAL_CATALOG_ADDED,
    SIGNAL_CATALOG_CHANGED,
    SIGNAL_CATALOG_DELETE,
    SIGNAL_CATALOG_REORDERED,
    SIGNAL_PLAYLIST_ADDED,
    SIGNAL_PLAYLIST_CHANGED,
    SIGNAL_PLAYLIST_DELETE,
    SIGNAL_PLAYLIST_REORDERED,
    SIGNAL_IMPORT_UPDATED,
    SIGNAL_REFRESH_UPDATED,
    SIGNAL_LAST
};

static GObject *db_instance = NULL;
static gpointer rclib_db_parent_class = NULL;
static gint db_signals[SIGNAL_LAST] = {0};
static gint db_import_depth = 5;

static gboolean rclib_db_is_iter_valid(GSequence *seq, GSequenceIter *iter)
{
    GSequenceIter *iter_foreach;
    if(seq==NULL || iter==NULL) return FALSE;
    for(iter_foreach = g_sequence_get_begin_iter(seq);
        !g_sequence_iter_is_end(iter_foreach);
        iter_foreach = g_sequence_iter_next(iter_foreach))
    {
        if(iter==iter_foreach) return TRUE;
    }
    return FALSE;
}

static inline void rclib_db_playlist_import_idle_data_free(
    RCLibDbPlaylistImportIdleData *data)
{
    if(data==NULL) return;
    if(data->mmd!=NULL) rclib_tag_free(data->mmd);
    g_free(data);
}

static inline void rclib_db_playlist_refresh_idle_data_free(
    RCLibDbPlaylistRefreshIdleData *data)
{
    if(data==NULL) return;
    if(data->mmd!=NULL) rclib_tag_free(data->mmd);
    g_free(data);
}

static gboolean rclib_db_import_update_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    gint length;
    length = GPOINTER_TO_INT(data);
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB_GET_PRIVATE(db_instance);
    if(priv==NULL) return FALSE;
    if(length<=0)
        priv->import_work_flag = FALSE;
    g_signal_emit(db_instance, db_signals[SIGNAL_IMPORT_UPDATED],
        0, length);
    return FALSE;
}

static gboolean rclib_db_refresh_update_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    gint length;
    length = GPOINTER_TO_INT(data);
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB_GET_PRIVATE(db_instance);
    if(priv==NULL) return FALSE;
    if(length<=0)
        priv->refresh_work_flag = FALSE;
    g_signal_emit(db_instance, db_signals[SIGNAL_REFRESH_UPDATED],
        0, length);
    return FALSE;
}

static gboolean rclib_db_playlist_import_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbCatalogData *catalog_data;
    GSequence *playlist;
    GSequenceIter *playlist_iter;
    RCLibDbPlaylistImportIdleData *idle_data;
    RCLibTagMetadata *mmd;
    if(data==NULL) return FALSE;
    if(db_instance==NULL) return FALSE;
    idle_data = (RCLibDbPlaylistImportIdleData *)data;
    if(idle_data->mmd==NULL) return FALSE;
    priv = RCLIB_DB_GET_PRIVATE(db_instance);
    if(priv==NULL) return FALSE;
    if(db_instance==NULL) return FALSE;
    if(!rclib_db_is_iter_valid(priv->catalog, idle_data->iter))
        return FALSE;
    catalog_data = g_sequence_get(idle_data->iter);
    if(catalog_data==NULL) return FALSE;
    playlist = catalog_data->playlist;
    if(playlist==NULL) return FALSE;
    mmd = idle_data->mmd;
    playlist_data = rclib_db_playlist_data_new();
    playlist_data->catalog = idle_data->iter;
    playlist_data->type = idle_data->type;
    playlist_data->uri = g_strdup(mmd->uri);
    playlist_data->title = g_strdup(mmd->title);
    playlist_data->artist = g_strdup(mmd->artist);
    playlist_data->album = g_strdup(mmd->album);
    playlist_data->ftype = g_strdup(mmd->ftype);
    playlist_data->length = mmd->length;
    playlist_data->tracknum = mmd->tracknum;
    playlist_data->year = mmd->year;
    playlist_data->rating = 3;
    if(!rclib_db_is_iter_valid(playlist, idle_data->insert_iter))
        idle_data->insert_iter = NULL;
    if(idle_data->insert_iter!=NULL)
    {
        playlist_iter = g_sequence_insert_before(idle_data->insert_iter,
            playlist_data);
    }
    else
        playlist_iter = g_sequence_append(playlist, playlist_data);
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_ADDED],
        0, playlist_iter);
    rclib_db_playlist_import_idle_data_free(idle_data);
    return FALSE;
}

static gboolean rclib_db_playlist_refresh_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbCatalogData *catalog_data;
    GSequence *playlist;
    RCLibDbPlaylistRefreshIdleData *idle_data;
    RCLibTagMetadata *mmd;
    if(data==NULL) return FALSE;
    if(db_instance==NULL) return FALSE;
    idle_data = (RCLibDbPlaylistRefreshIdleData *)data;
    priv = RCLIB_DB_GET_PRIVATE(db_instance);
    if(priv==NULL) return FALSE;
    if(db_instance==NULL) return FALSE;
    if(!rclib_db_is_iter_valid(priv->catalog, idle_data->catalog_iter))
        return FALSE;
    catalog_data = g_sequence_get(idle_data->catalog_iter);
    if(catalog_data==NULL) return FALSE;
    playlist = catalog_data->playlist;
    if(playlist==NULL) return FALSE;
    if(!rclib_db_is_iter_valid(playlist, idle_data->playlist_iter))
        return FALSE;
    playlist_data = g_sequence_get(idle_data->playlist_iter);
    mmd = idle_data->mmd;
    playlist_data->type = idle_data->type;
    if(mmd!=NULL)
    {
        g_free(playlist_data->title);
        g_free(playlist_data->artist);
        g_free(playlist_data->album);
        g_free(playlist_data->ftype);
        playlist_data->title = g_strdup(mmd->title);
        playlist_data->artist = g_strdup(mmd->artist);
        playlist_data->album = g_strdup(mmd->album);
        playlist_data->ftype = g_strdup(mmd->ftype);
        playlist_data->length = mmd->length;
        playlist_data->tracknum = mmd->tracknum;
        playlist_data->year = mmd->year;
    }
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_CHANGED],
        0, idle_data->playlist_iter);
    rclib_db_playlist_refresh_idle_data_free(idle_data);
    return FALSE;
}

static inline void rclib_db_playlist_import_data_free(
    RCLibDbPlaylistImportData *data)
{
    if(data==NULL) return;
    g_free(data->uri);
    g_free(data);
}

static inline void rclib_db_playlist_refresh_data_free(
    RCLibDbPlaylistRefreshData *data)
{
    if(data==NULL) return;
    g_free(data->uri);
    g_free(data);
}

static RCLibTagMetadata *rclib_db_get_metadata_from_cue(
    RCLibCueData *cue_data, guint track_num, RCLibTagMetadata *cue_mmd)
{
    RCLibTagMetadata *mmd = NULL;
    RCLibCueTrack *track = NULL;
    gboolean empty_flag = FALSE;
    if(cue_data==NULL) return NULL;
    if(cue_mmd==NULL)
    {
        if(cue_data->file==NULL) return NULL;
        cue_mmd = rclib_tag_read_metadata(cue_data->file);
        if(cue_mmd==NULL) return NULL;
        empty_flag = TRUE;
    }
    if(track_num<0 || track_num>=cue_data->length) return NULL;
    track = cue_data->track + track_num;
    mmd = g_new0(RCLibTagMetadata, 1);
    if(track->title!=NULL)
        mmd->title = g_strdup(track->title);
    else
        mmd->title = g_strdup_printf("Track %d", track_num);
    if(track->performer!=NULL)
        mmd->artist = g_strdup(track->performer);
    if(cue_data->title!=NULL)
        mmd->album = g_strdup(cue_data->title);
    mmd->tracknum = track_num + 1;
    if(track_num+1!=cue_data->length)
        mmd->length = track[1].time1 - track->time1;
    else
        mmd->length = cue_mmd->length - track->time1;
    if(cue_data->file!=NULL)
        mmd->uri = g_strdup_printf("%s:%d", cue_data->file, track_num+1);
    else
        mmd->uri = g_strdup_printf("%s:%d", cue_mmd->uri, track_num+1);
    if(cue_mmd->ftype!=NULL)
        mmd->ftype = g_strdup(cue_mmd->ftype);
    if(cue_mmd->comment!=NULL)
        mmd->comment = g_strdup(cue_mmd->comment);
    mmd->bitrate = cue_mmd->bitrate;
    mmd->samplerate = cue_mmd->samplerate;
    mmd->channels = cue_mmd->channels;
    if(cue_mmd->image!=NULL)
    {
        gst_buffer_ref(cue_mmd->image);
        mmd->image = cue_mmd->image;
    }
    if(empty_flag) rclib_tag_free(cue_mmd);
    mmd->audio_flag = TRUE;
    return mmd;
}

static gpointer rclib_db_playlist_import_thread_cb(gpointer data)
{
    RCLibDbPlaylistImportIdleData *idle_data;
    RCLibTagMetadata *mmd = NULL, *cue_mmd = NULL;
    RCLibDbPlaylistImportData *import_data;
    RCLibDbPrivate *priv;
    RCLibCueData cue_data;
    gchar *cue_uri;
    gchar *scheme;
    guint i;
    gint track = 0;
    gint length;
    gboolean local_flag;
    GObject *object = G_OBJECT(data);
    if(object==NULL)
    {
        g_thread_exit(NULL);
        return NULL;
    }
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(object));
    while(priv->import_queue!=NULL)
    {
        import_data = g_async_queue_pop(priv->import_queue);
        if(import_data->uri==NULL)
        {
            g_free(import_data);
            break;
        }
        scheme = g_uri_parse_scheme(import_data->uri);
        if(g_strcmp0(scheme, "file")==0) local_flag = TRUE;
        else local_flag = FALSE;
        g_free(scheme);
        G_STMT_START
        {
            /* Import CUE */
            if(local_flag && g_regex_match_simple("(.CUE)$",
                import_data->uri, G_REGEX_CASELESS, 0))
            {
                bzero(&cue_data, sizeof(RCLibCueData));
                if(rclib_cue_read_data(import_data->uri,
                    RCLIB_CUE_INPUT_URI, &cue_data)>0)
                {
                    cue_mmd = rclib_tag_read_metadata(cue_data.file);
                    for(i=0;i<cue_data.length;i++)
                    {
                        mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            i, cue_mmd);
                        if(mmd==NULL) continue;
                        g_free(mmd->uri);
                        mmd->uri = g_strdup_printf("%s:%d",
                            import_data->uri, i+1);
                        idle_data = g_new0(RCLibDbPlaylistImportIdleData, 1);
                        idle_data->iter = import_data->iter;
                        idle_data->insert_iter = import_data->insert_iter;
                        idle_data->mmd = mmd;
                        if(i==0)
                            idle_data->play_flag = import_data->play_flag;
                        idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                        g_idle_add(rclib_db_playlist_import_idle_cb,
                            idle_data);
                    }
                    rclib_tag_free(cue_mmd);
                }
                rclib_cue_free(&cue_data);
                break;
            }
            if(local_flag && rclib_cue_get_track_num(import_data->uri,
                &cue_uri, &track))
            {
                bzero(&cue_data, sizeof(RCLibCueData));
                if(rclib_cue_read_data(cue_uri, RCLIB_CUE_INPUT_URI,
                    &cue_data)>0)
                {
                    if(g_regex_match_simple("(.CUE)$", cue_uri,
                    G_REGEX_CASELESS, 0))
                    {
                        mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            track-1, NULL);
                        if(mmd!=NULL)
                        {
                            g_free(mmd->uri);
                            mmd->uri = g_strdup(import_data->uri);
                            idle_data =
                                g_new0(RCLibDbPlaylistImportIdleData, 1);
                            idle_data->iter = import_data->iter;
                            idle_data->insert_iter =
                                import_data->insert_iter;
                            idle_data->mmd = mmd;
                            idle_data->play_flag = import_data->play_flag;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                            g_idle_add(rclib_db_playlist_import_idle_cb,
                                idle_data);
                        }
                    }
                    rclib_cue_free(&cue_data);
                    g_free(cue_uri);
                    break;
                }
                else /* Maybe a embeded CUE audio file? */
                {
                    g_free(import_data->uri);
                    import_data->uri = g_strdup(cue_uri);
                    g_free(cue_uri);
                }
            }
            mmd = rclib_tag_read_metadata(import_data->uri);
            if(mmd==NULL) break;
            if(mmd->emb_cue!=NULL) /* Embeded CUE check */
            {
                if(rclib_cue_read_data(mmd->emb_cue,
                    RCLIB_CUE_INPUT_EMBEDED, &cue_data)>0)
                {
                    if(track>0)
                    {
                        cue_mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            track-1, mmd);
                        if(cue_mmd!=NULL)
                        {
                            rclib_tag_free(mmd);
                            idle_data = g_new0(RCLibDbPlaylistImportIdleData,
                                1);
                            idle_data->iter = import_data->iter;
                            idle_data->insert_iter =
                                import_data->insert_iter;
                            idle_data->mmd = mmd;
                            idle_data->play_flag = import_data->play_flag;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                            g_idle_add(rclib_db_playlist_import_idle_cb,
                                idle_data);
                        }
                    }
                    else
                    {
                        for(i=0;i<cue_data.length;i++)
                        {
                            cue_mmd = rclib_db_get_metadata_from_cue(
                                &cue_data, i, mmd);
                            if(cue_mmd==NULL) continue;
                            idle_data = g_new0(RCLibDbPlaylistImportIdleData,
                                1);
                            idle_data->iter = import_data->iter;
                            idle_data->insert_iter = import_data->insert_iter;
                            idle_data->mmd = mmd;
                            if(i==0)
                                idle_data->play_flag = import_data->play_flag;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                            g_idle_add(rclib_db_playlist_import_idle_cb,
                                idle_data);
                        }
                    }
                    break;
                }
            }
            idle_data = g_new0(RCLibDbPlaylistImportIdleData, 1);
            idle_data->iter = import_data->iter;
            idle_data->insert_iter = import_data->insert_iter;
            idle_data->mmd = mmd;
            idle_data->play_flag = import_data->play_flag;
            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
            g_idle_add(rclib_db_playlist_import_idle_cb, idle_data);
        }
        G_STMT_END;
        rclib_db_playlist_import_data_free(import_data);
        length = g_async_queue_length(priv->import_queue);
        g_idle_add(rclib_db_import_update_idle_cb, GINT_TO_POINTER(length));
    }
    g_thread_exit(NULL);
    return NULL;
}

static gpointer rclib_db_playlist_refresh_thread_cb(gpointer data)
{
    RCLibDbPlaylistRefreshIdleData *idle_data;
    RCLibDbPlaylistRefreshData *refresh_data;
    RCLibTagMetadata *mmd = NULL, *cue_mmd = NULL;
    RCLibDbPrivate *priv;
    RCLibCueData cue_data;
    gchar *cue_uri;
    gchar *scheme;
    gint track = 0;
    gint length;
    gboolean local_flag;
    GObject *object = G_OBJECT(data);
    if(object==NULL)
    {
        g_thread_exit(NULL);
        return NULL;
    }
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(object));
    while(priv->refresh_queue!=NULL)
    {
        refresh_data = g_async_queue_pop(priv->refresh_queue);
        if(refresh_data->uri==NULL)
        {
            g_free(refresh_data);
            break;
        }
        if(refresh_data->catalog_iter==NULL ||
            refresh_data->playlist_iter==NULL) continue;
        scheme = g_uri_parse_scheme(refresh_data->uri);
        if(g_strcmp0(scheme, "file")==0) local_flag = TRUE;
        else local_flag = FALSE;
        g_free(scheme);
        G_STMT_START
        {
            if(local_flag && rclib_cue_get_track_num(refresh_data->uri,
                &cue_uri, &track))
            {
                if(g_regex_match_simple("(.CUE)$", cue_uri,
                    G_REGEX_CASELESS, 0))
                {
                    bzero(&cue_data, sizeof(RCLibCueData));
                    if(rclib_cue_read_data(cue_uri, RCLIB_CUE_INPUT_URI,
                        &cue_data)>0)
                    {
                        mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            track-1, NULL);
                        if(mmd!=NULL)
                        {
                            g_free(mmd->uri);
                            mmd->uri = g_strdup(refresh_data->uri);
                            idle_data =
                                g_new0(RCLibDbPlaylistRefreshIdleData, 1);
                            idle_data->catalog_iter =
                                refresh_data->catalog_iter;
                            idle_data->playlist_iter =
                                refresh_data->playlist_iter;
                            idle_data->mmd = mmd;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                            g_idle_add(rclib_db_playlist_refresh_idle_cb,
                                idle_data);
                        }
                        else
                        {
                            idle_data =
                                g_new0(RCLibDbPlaylistRefreshIdleData, 1);
                            idle_data->catalog_iter =
                                refresh_data->catalog_iter;
                            idle_data->playlist_iter =
                                refresh_data->playlist_iter;
                            idle_data->mmd = NULL;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MISSING;
                            g_idle_add(rclib_db_playlist_refresh_idle_cb,
                                idle_data);
                        }
                    }
                    rclib_cue_free(&cue_data);
                    g_free(cue_uri);
                    break;
                }
                else /* Maybe a embeded CUE audio file? */
                {
                    g_free(refresh_data->uri);
                    refresh_data->uri = g_strdup(cue_uri);
                    g_free(cue_uri);
                }
            }
            mmd = rclib_tag_read_metadata(refresh_data->uri);
            if(mmd!=NULL && mmd->emb_cue!=NULL) /* Embeded CUE check */
            {
                if(rclib_cue_read_data(mmd->emb_cue,
                    RCLIB_CUE_INPUT_EMBEDED, &cue_data)>0)
                {
                    if(track>0)
                    {
                        cue_mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            track-1, mmd);
                        if(cue_mmd!=NULL)
                        {
                            rclib_tag_free(mmd);
                            idle_data =
                                g_new0(RCLibDbPlaylistRefreshIdleData, 1);
                            idle_data->catalog_iter =
                                refresh_data->catalog_iter;
                            idle_data->playlist_iter =
                                refresh_data->playlist_iter;
                            idle_data->mmd = mmd;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                            g_idle_add(rclib_db_playlist_refresh_idle_cb,
                                idle_data);
                        }
                        else
                        {
                            idle_data =
                                g_new0(RCLibDbPlaylistRefreshIdleData, 1);
                            idle_data->catalog_iter =
                                refresh_data->catalog_iter;
                            idle_data->playlist_iter =
                                refresh_data->playlist_iter;
                            idle_data->mmd = NULL;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MISSING;
                            g_idle_add(rclib_db_playlist_refresh_idle_cb,
                                idle_data);
                        }
                    }
                    break;
                }
            }
            idle_data = g_new0(RCLibDbPlaylistRefreshIdleData, 1);
            idle_data->catalog_iter = refresh_data->catalog_iter;
            idle_data->playlist_iter = refresh_data->playlist_iter;
            idle_data->mmd = mmd;
            if(mmd!=NULL)
                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
            else
                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MISSING;
            g_idle_add(rclib_db_playlist_refresh_idle_cb, idle_data);
        }
        G_STMT_END;
        rclib_db_playlist_refresh_data_free(refresh_data);
        length = g_async_queue_length(priv->refresh_queue);
        g_idle_add(rclib_db_refresh_update_idle_cb, GINT_TO_POINTER(length));
    }
    g_thread_exit(NULL);
    return NULL;
}

static gboolean rclib_db_load_library_db(GSequence *catalog,
    const gchar *file, gboolean *dirty_flag)
{
    json_object *root_object, *catalog_object, *playlist_object;
    json_object *catalog_array, *playlist_array, *tmp;
    guint i, j;
    guint catalog_len, playlist_len;
    GError *error = NULL;
    GSequence *playlist;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    const gchar *str;
    GSequenceIter *catalog_iter;
    gchar *json_data;
    gsize json_size;
    if(!g_file_get_contents(file, &json_data, &json_size, &error))
    {
        g_warning("Cannot load playlist: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
    else
        g_message("Loading playlist....");
    root_object = json_tokener_parse(json_data);
    g_free(json_data);
    if(root_object==NULL)
    {
        g_warning("Cannot parse playlist!");
        return FALSE;
    }
    G_STMT_START
    {
        catalog_array = json_object_object_get(root_object, "RCMusicCatalog");
        if(catalog_array==NULL) break;
        if(!json_object_is_type(catalog_array, json_type_array)) break;
        catalog_len = json_object_array_length(catalog_array);
        for(i=0;i<catalog_len;i++)
        {
            catalog_object = json_object_array_get_idx(catalog_array, i);
            if(catalog_object==NULL || !json_object_is_type(catalog_object,
                json_type_object)) continue;
            catalog_data = rclib_db_catalog_data_new();
            tmp = json_object_object_get(catalog_object, "Name");
            if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                str = json_object_get_string(tmp);
            else str = NULL;
            if(str==NULL) str = _("Unnamed list");
            catalog_data->name = g_strdup(str);
            tmp = json_object_object_get(catalog_object, "Type");
            if(tmp!=NULL && json_object_is_type(tmp, json_type_int))
                 catalog_data->type = json_object_get_int(tmp);
            playlist = g_sequence_new((GDestroyNotify)
                rclib_db_playlist_data_unref);
            catalog_data->playlist = playlist;
            catalog_iter = g_sequence_append(catalog, catalog_data);
            playlist_array = json_object_object_get(catalog_object,
                "Playlist");
            if(playlist_array==NULL || !json_object_is_type(playlist_array,
                json_type_array)) continue;
            playlist_len = json_object_array_length(playlist_array);
            for(j=0;j<playlist_len;j++)
            {
                playlist_object = json_object_array_get_idx(playlist_array,
                    j);
                if(playlist_object==NULL || !json_object_is_type(
                    playlist_object, json_type_object)) continue;
                playlist_data = rclib_db_playlist_data_new();
                playlist_data->catalog = catalog_iter;
                tmp = json_object_object_get(playlist_object, "Type");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_int))
                    playlist_data->type = json_object_get_int(tmp);
                tmp = json_object_object_get(playlist_object, "URI");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->uri = g_strdup(str);
                tmp = json_object_object_get(playlist_object, "Title");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->title = g_strdup(str);
                tmp = json_object_object_get(playlist_object, "Artist");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->artist = g_strdup(str);
                tmp = json_object_object_get(playlist_object, "Album");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->album = g_strdup(str);
                tmp = json_object_object_get(playlist_object, "FileType");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->ftype = g_strdup(str);
                tmp = json_object_object_get(playlist_object, "Length");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                if(str!=NULL)
                    playlist_data->length = g_ascii_strtoll(str, NULL, 10);
                tmp = json_object_object_get(playlist_object, "TrackNum");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_int))
                    playlist_data->tracknum = json_object_get_int(tmp);
                tmp = json_object_object_get(playlist_object, "Year");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_int))
                    playlist_data->year = json_object_get_int(tmp);
                tmp = json_object_object_get(playlist_object, "Rating");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_int))
                    playlist_data->rating = json_object_get_int(tmp);
                tmp = json_object_object_get(playlist_object, "LyricFile");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->lyricfile = g_strdup(str);
                tmp = json_object_object_get(playlist_object,
                    "LyricSecondFile");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->lyricsecfile = g_strdup(str);
                tmp = json_object_object_get(playlist_object, "AlbumFile");
                if(tmp!=NULL && json_object_is_type(tmp, json_type_string))
                    str = json_object_get_string(tmp);
                else str = NULL;
                playlist_data->albumfile = g_strdup(str);
                g_sequence_append(playlist, playlist_data);
            }
        }
    }
    G_STMT_END;
    json_object_put(root_object);
    g_message("Playlist loaded.");
    if(dirty_flag!=NULL) *dirty_flag = FALSE;
    return TRUE;
}

static gboolean rclib_db_save_library_db(GSequence *catalog,
    const gchar *file, gboolean *dirty_flag)
{
    json_object *root_object, *catalog_object, *playlist_object;
    json_object *catalog_array, *playlist_array, *tmp;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *catalog_iter, *playlist_iter;
    const gchar *json_data;
    GError *error = NULL;
    gchar *time;
    catalog_array = json_object_new_array();
    for(catalog_iter = g_sequence_get_begin_iter(catalog);
        !g_sequence_iter_is_end(catalog_iter);
        catalog_iter = g_sequence_iter_next(catalog_iter))
    {
        catalog_object = json_object_new_object();
        catalog_data = g_sequence_get(catalog_iter);
        if(catalog_data->name!=NULL)
        {
            tmp = json_object_new_string(catalog_data->name);
            json_object_object_add(catalog_object, "Name", tmp);
        }
        tmp = json_object_new_int(catalog_data->type);
        json_object_object_add(catalog_object, "Type", tmp);
        playlist_array = json_object_new_array();
        for(playlist_iter = g_sequence_get_begin_iter(catalog_data->playlist);
            !g_sequence_iter_is_end(playlist_iter);
            playlist_iter = g_sequence_iter_next(playlist_iter))
        {
            playlist_object = json_object_new_object();
            playlist_data = g_sequence_get(playlist_iter);
            tmp = json_object_new_int(playlist_data->type);
            json_object_object_add(playlist_object, "Type", tmp);
            if(playlist_data->uri!=NULL)
            {
                tmp = json_object_new_string(playlist_data->uri);
                json_object_object_add(playlist_object, "URI", tmp);
            }
            if(playlist_data->title!=NULL)
            {
                tmp = json_object_new_string(playlist_data->title);
                json_object_object_add(playlist_object, "Title", tmp);
            }
            if(playlist_data->artist!=NULL)
            {
                tmp = json_object_new_string(playlist_data->artist);
                json_object_object_add(playlist_object, "Artist", tmp);
            }
            if(playlist_data->album!=NULL)
            {
                tmp = json_object_new_string(playlist_data->album);
                json_object_object_add(playlist_object, "Album", tmp);
            }
            if(playlist_data->ftype!=NULL)
            {
                tmp = json_object_new_string(playlist_data->ftype);
                json_object_object_add(playlist_object, "FileType", tmp);
            }
            time = g_strdup_printf("%"G_GINT64_FORMAT,
                playlist_data->length);
            tmp = json_object_new_string(time);
            g_free(time);
            json_object_object_add(playlist_object, "Length", tmp);
            tmp = json_object_new_int(playlist_data->tracknum);
            json_object_object_add(playlist_object, "TrackNum", tmp);
            tmp = json_object_new_int(playlist_data->year);
            json_object_object_add(playlist_object, "Year", tmp);
            tmp = json_object_new_int(playlist_data->rating);
            json_object_object_add(playlist_object, "Rating", tmp);
            if(playlist_data->lyricfile!=NULL)
            {
                tmp = json_object_new_string(playlist_data->lyricfile);
                json_object_object_add(playlist_object, "LyricFile", tmp);
            }
            if(playlist_data->albumfile!=NULL)
            {
                tmp = json_object_new_string(playlist_data->albumfile);
                json_object_object_add(playlist_object, "AlbumFile", tmp);
            }
            if(playlist_data->lyricsecfile!=NULL)
            {
                tmp = json_object_new_string(playlist_data->lyricsecfile);
                json_object_object_add(playlist_object, "LyricSecondFile",
                    tmp);
            }
            json_object_array_add(playlist_array, playlist_object);
        }
        json_object_object_add(catalog_object, "Playlist", playlist_array);
        json_object_array_add(catalog_array, catalog_object);
    }
    root_object = json_object_new_object();
    json_object_object_add(root_object, "RCMusicCatalog", catalog_array);
    json_data = json_object_to_json_string(root_object);
    if(json_data!=NULL)
    {
        if(!g_file_set_contents(file, json_data, -1, &error))
        {
            g_warning("Cannot save playlist: %s", error->message);
            g_error_free(error);
            json_object_put(root_object);
            return FALSE;
        }
        else
            g_message("Playlist saved.");
    }
    json_object_put(root_object);
    if(dirty_flag!=NULL) *dirty_flag = FALSE;
    return TRUE;
}

static void rclib_db_finalize(GObject *object)
{
    RCLibDbPlaylistImportData *import_data;
    RCLibDbPlaylistRefreshData *refresh_data;
    RCLibDbPrivate *priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(object));
    while(g_async_queue_length(priv->import_queue)>=0)
    {
        import_data = g_async_queue_try_pop(priv->import_queue);
        if(import_data!=NULL)
            rclib_db_playlist_import_data_free(import_data);
    }
    while(g_async_queue_length(priv->refresh_queue)>=0)
    {
        refresh_data = g_async_queue_try_pop(priv->refresh_queue);
        if(refresh_data!=NULL)
            rclib_db_playlist_refresh_data_free(refresh_data);
    }
    import_data = g_new0(RCLibDbPlaylistImportData, 1);
    refresh_data = g_new0(RCLibDbPlaylistRefreshData, 1);
    g_async_queue_push(priv->import_queue, import_data);
    g_async_queue_push(priv->refresh_queue, refresh_data);
    g_thread_join(priv->import_thread);
    g_thread_join(priv->refresh_thread);
    rclib_db_save_library_db(priv->catalog, priv->filename,
        &(priv->dirty_flag));
    g_free(priv->filename);
    g_async_queue_unref(priv->import_queue);
    g_sequence_free(priv->catalog);
    priv->import_queue = NULL;
    priv->refresh_queue = NULL;
    priv->catalog = NULL;
    G_OBJECT_CLASS(rclib_db_parent_class)->finalize(object);
}

static GObject *rclib_db_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(db_instance!=NULL) return db_instance;
    retval = G_OBJECT_CLASS(rclib_db_parent_class)->constructor
        (type, n_construct_params, construct_params);
    db_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&db_instance);
    return retval;
}

static void rclib_db_class_init(RCLibDbClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_db_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_db_finalize;
    object_class->constructor = rclib_db_constructor;
    g_type_class_add_private(klass, sizeof(RCLibDbPrivate));
    
    /**
     * RCLibDb::catalog-added:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the added item
     * 
     * The ::catalog-added signal is emitted when a new item has been
     * added to the catalog.
     */
    db_signals[SIGNAL_CATALOG_ADDED] = g_signal_new("catalog-added",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_added), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

    /**
     * RCLibDb::catalog-changed:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the changed item
     * 
     * The ::catalog-changed signal is emitted when an item has been
     * changed in the catalog.
     */
    db_signals[SIGNAL_CATALOG_CHANGED] = g_signal_new("catalog-changed",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_changed), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::catalog-delete:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the item which is about to be deleted
     * 
     * The ::catalog-delete signal is emitted before an item in the catalog
     * is about to be deleted.
     */
    db_signals[SIGNAL_CATALOG_DELETE] = g_signal_new("catalog-delete",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_delete), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

    /**
     * RCLibDb::catalog-reordered:
     * @db: the #RCLibDb that received the signal
     * @new_order: an array of integers mapping the current position of each
     * catalog item to its old position before the re-ordering,
     * i.e. @new_order<literal>[newpos] = oldpos</literal>
     * 
     * The ::catalog-reordered signal is emitted when the catalog items have
     * been reordered.
     */
    db_signals[SIGNAL_CATALOG_REORDERED] = g_signal_new("catalog-reordered",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_reordered), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::playlist-added:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the added item
     * 
     * The ::playlist-added signal is emitted when a new item has been
     * added to the playlist.
     */
    db_signals[SIGNAL_PLAYLIST_ADDED] = g_signal_new("playlist-added",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_added), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::playlist-changed:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the changed item
     * 
     * The ::playlist-changed signal is emitted when an item has been
     * changed in the playlist.
     */
    db_signals[SIGNAL_PLAYLIST_CHANGED] = g_signal_new("playlist-changed",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_changed), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

    /**
     * RCLibDb::playlist-delete:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the item which is about to be deleted
     * 
     * The ::playlist-delete signal is emitted before an item in the playlist
     * is about to be deleted.
     */
    db_signals[SIGNAL_PLAYLIST_DELETE] = g_signal_new("playlist-delete",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_delete), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::playlist-reordered:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the catalog item which contains the
     * reordered playlist items
     * @new_order: an array of integers mapping the current position of each
     * playlist item to its old position before the re-ordering,
     * i.e. @new_order<literal>[newpos] = oldpos</literal>
     * 
     * The ::playlist-reordered signal is emitted when the catalog items have
     * been reordered.
     */
    db_signals[SIGNAL_PLAYLIST_REORDERED] = g_signal_new("playlist-reordered",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_reordered), NULL, NULL, rclib_marshal_VOID__POINTER_POINTER,
        G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::import-updated:
     * @db: the #RCLibDb that received the signal
     * @remaining: the number of remaing jobs in current import queue
     * 
     * The ::import-updated signal is emitted when a job in the import
     * queue have been processed.
     */
    db_signals[SIGNAL_IMPORT_UPDATED] = g_signal_new("import-updated",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        import_updated), NULL, NULL, g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT, NULL);

    /**
     * RCLibDb::refresh-updated:
     * @db: the #RCLibDb that received the signal
     * @remaining: the number of remaing jobs in current refresh queue
     * 
     * The ::refresh-updated signal is emitted when a job in the refresh
     * queue have been processed.
     */
    db_signals[SIGNAL_REFRESH_UPDATED] = g_signal_new("refresh-updated",
        RCLIB_DB_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        refresh_updated), NULL, NULL, g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT, NULL);
}

static void rclib_db_instance_init(RCLibDb *db)
{
    RCLibDbPrivate *priv = RCLIB_DB_GET_PRIVATE(db);
    priv->catalog = g_sequence_new((GDestroyNotify)
        rclib_db_catalog_data_unref);
    priv->import_queue = g_async_queue_new_full((GDestroyNotify)
        rclib_db_playlist_import_data_free);
    priv->refresh_queue = g_async_queue_new_full((GDestroyNotify)
        rclib_db_playlist_refresh_data_free);
    priv->import_thread = g_thread_new("RC2-Import-Thread",
        rclib_db_playlist_import_thread_cb, db);
    priv->refresh_thread = g_thread_new("RC2-Refresh-Thread",
        rclib_db_playlist_refresh_thread_cb, db);
    priv->import_work_flag = FALSE;
    priv->refresh_work_flag = FALSE;
    priv->dirty_flag = FALSE;
}

GType rclib_db_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo db_info = {
        .class_size = sizeof(RCLibDbClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_db_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibDb),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_db_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibDb"), &db_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_db_init:
 * @file: the file of the music library database to load
 *
 * Initialize the music library database.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rclib_db_init(const gchar *file)
{
    RCLibDbPrivate *priv;
    g_message("Loading music library database....");
    if(db_instance!=NULL)
    {
        g_warning("The database is already initialized!");
        return FALSE;
    }
    db_instance = g_object_new(RCLIB_DB_TYPE, NULL);
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv->catalog==NULL || priv->import_queue==NULL ||
        priv->import_thread==NULL)
    {
        g_object_unref(db_instance);
        db_instance = NULL;
        g_warning("Failed to load database!");
        return FALSE;
    }
    rclib_db_load_library_db(priv->catalog, file, &(priv->dirty_flag));
    priv->filename = g_strdup(file);
    g_message("Database loaded.");
    return TRUE;
}

/**
 * rclib_db_exit:
 *
 * Unload the music library database.
 */

void rclib_db_exit()
{
    if(db_instance!=NULL) g_object_unref(db_instance);
    db_instance = NULL;
    g_message("Database exited.");
}

/**
 * rclib_db_get_instance:
 *
 * Get the running #RCLibDb instance.
 *
 * Returns: The running instance.
 */

GObject *rclib_db_get_instance()
{
    return db_instance;
}

/**
 * rclib_db_signal_connect:
 * @name: the name of the signal
 * @callback: the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibDb object.
 *
 * Returns: The handler ID.
 */

gulong rclib_db_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(db_instance==NULL) return 0;
    return g_signal_connect(db_instance, name, callback, data);
}

/**
 * rclib_db_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running #RCLibDb instance so it will
 * not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_db_signal_disconnect(gulong handler_id)
{
    if(db_instance==NULL) return;
    g_signal_handler_disconnect(db_instance, handler_id);
}

/**
 * rclib_db_catalog_data_new:
 * 
 * Create a new empty #RCLibDbCatalogData structure,
 * and set the reference count to 1.
 *
 * Returns: The new empty allocated #RCLibDbCatalogData structure.
 */

RCLibDbCatalogData *rclib_db_catalog_data_new()
{
    RCLibDbCatalogData *data = g_new0(RCLibDbCatalogData, 1);
    data->ref_count = 1;
    return data;
}

/**
 * rclib_db_catalog_data_ref:
 * @data: the #RCLibDbCatalogData structure
 *
 * Increase the reference of #RCLibDbCatalogData by 1.
 *
 * Returns: The #RCLibDbCatalogData structure.
 */

RCLibDbCatalogData *rclib_db_catalog_data_ref(RCLibDbCatalogData *data)
{
    if(data==NULL) return NULL;
    g_atomic_int_add(&(data->ref_count), 1);
    return data;
}

/**
 * rclib_db_catalog_data_unref:
 * @data: the #RCLibDbCatalogData structure
 *
 * Decrease the reference of #RCLibDbCatalogData by 1.
 * If the reference down to zero, the structure will be freed.
 *
 * Returns: The #RCLibDbCatalogData structure.
 */

void rclib_db_catalog_data_unref(RCLibDbCatalogData *data)
{
    if(data==NULL) return;
    if(g_atomic_int_dec_and_test(&(data->ref_count)))
        rclib_db_catalog_data_free(data);
}

/**
 * rclib_db_catalog_data_free:
 * @data: the data to free
 *
 * Free the #RCLibDbCatalogData structure.
 */

void rclib_db_catalog_data_free(RCLibDbCatalogData *data)
{
    if(data==NULL) return;
    g_free(data->name);
    if(data->playlist!=NULL) g_sequence_free(data->playlist);
    g_free(data);
}

/**
 * rclib_db_playlist_data_new:
 * 
 * Create a new empty #RCLibDbPlaylistData structure,
 * and set the reference count to 1.
 *
 * Returns: The new empty allocated #RCLibDbPlaylistData structure.
 */

RCLibDbPlaylistData *rclib_db_playlist_data_new()
{
    RCLibDbPlaylistData *data = g_new0(RCLibDbPlaylistData, 1);
    data->ref_count = 1;
    return data;
}

/**
 * rclib_db_playlist_data_ref:
 * @data: the #RCLibDbPlaylistData structure
 *
 * Increase the reference of #RCLibDbPlaylistData by 1.
 *
 * Returns: The #RCLibDbPlaylistData structure.
 */

RCLibDbPlaylistData *rclib_db_playlist_data_ref(RCLibDbPlaylistData *data)
{
    if(data==NULL) return NULL;
    g_atomic_int_add(&(data->ref_count), 1);
    return data;
}

/**
 * rclib_db_playlist_data_unref:
 * @data: the #RCLibDbPlaylistData structure
 *
 * Decrease the reference of #RCLibDbPlaylistData by 1.
 * If the reference down to zero, the structure will be freed.
 *
 * Returns: The #RCLibDbPlaylistData structure.
 */

void rclib_db_playlist_data_unref(RCLibDbPlaylistData *data)
{
    if(data==NULL) return;
    if(g_atomic_int_dec_and_test(&(data->ref_count)))
        rclib_db_playlist_data_free(data);
}

/**
 * rclib_db_playlist_data_free:
 * @data: the data to free
 *
 * Free the #RCLibDbPlaylistData structure.
 */

void rclib_db_playlist_data_free(RCLibDbPlaylistData *data)
{
    if(data==NULL) return;
    g_free(data->uri);
    g_free(data->title);
    g_free(data->artist);
    g_free(data->album);
    g_free(data->ftype);
    g_free(data->lyricfile);
    g_free(data->lyricsecfile);
    g_free(data->albumfile);
    g_free(data);
}

/**
 * rclib_db_get_catalog:
 *
 * Get the catalog.
 *
 * Returns: The catalog, NULL if the catalog does not exist.
 */

GSequence *rclib_db_get_catalog()
{
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return NULL;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return NULL;
    return priv->catalog;
}

static gint rclib_db_reorder_func(GSequenceIter *a, GSequenceIter *b,
    gpointer user_data)
{
    GHashTable *new_positions = user_data;
    gint apos = GPOINTER_TO_INT(g_hash_table_lookup(new_positions, a));
    gint bpos = GPOINTER_TO_INT(g_hash_table_lookup(new_positions, b));
    if(apos < bpos) return -1;
    if(apos > bpos) return 1;
    return 0;
}

/**
 * rclib_db_catalog_add:
 * @name: the name for the new playlist
 * @iter: insert position (before this #iter)
 * @type: the type of the new playlist
 *
 * Add a new playlist to the catalog before the #iter, if the #iter
 * is #NULL, it will be added to the end.
 *
 * Returns: The iter to the new playlist in the catalog.
 */

GSequenceIter *rclib_db_catalog_add(const gchar *name,
    GSequenceIter *iter, gint type)
{
    RCLibDbCatalogData *catalog_data;
    RCLibDbPrivate *priv;
    GSequence *playlist;
    GSequenceIter *catalog_iter;
    if(db_instance==NULL || name==NULL) return NULL;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return NULL;
    if(iter!=NULL && g_sequence_iter_get_sequence(iter)!=priv->catalog)
        return NULL;
    catalog_data = rclib_db_catalog_data_new();
    playlist = g_sequence_new((GDestroyNotify)rclib_db_playlist_data_unref);
    catalog_data->name = g_strdup(name);
    catalog_data->type = type;
    catalog_data->playlist = playlist;
    if(iter!=NULL)
        catalog_iter = g_sequence_insert_before(iter, catalog_data);
    else
        catalog_iter = g_sequence_append(priv->catalog, catalog_data);
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_ADDED],
        0, catalog_iter);
    return catalog_iter;
}

/**
 * rclib_db_catalog_delete:
 * @iter: the iter to the catalog
 *
 * Delete the catalog pointed to by #iter.
 */

void rclib_db_catalog_delete(GSequenceIter *iter)
{
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_DELETE],
        0, iter);
    g_sequence_remove(iter);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_catalog_set_name:
 * @iter: the iter to the catalog
 * @name: the new name for the catalog
 *
 * Set the name of the catalog pointed to by #iter.
 */

void rclib_db_catalog_set_name(GSequenceIter *iter, const gchar *name)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogData *catalog_data;
    if(iter==NULL || name==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    catalog_data = g_sequence_get(iter);
    if(catalog_data==NULL) return;
    g_free(catalog_data->name);
    catalog_data->name = g_strdup(name);
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_CHANGED],
        0, iter);
}

/**
 * rclib_db_catalog_set_type:
 * @iter: the iter to the catalog
 * @type: the new type for the catalog
 *
 * Set the type of the catalog pointed to by #iter.
 */

void rclib_db_catalog_set_type(GSequenceIter *iter, RCLibDbCatalogType type)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogData *catalog_data;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    catalog_data = g_sequence_get(iter);
    if(catalog_data==NULL) return;
    catalog_data->type = type;
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_CHANGED],
        0, iter);
}

/**
 * rclib_db_catalog_reorder:
 * @new_order: (array): an array of integers mapping the new position of 
 *   each child to its old position before the re-ordering,
 *   i.e. @new_order<literal>[newpos] = oldpos</literal>.
 *
 * Reorder the catalog to follow the order indicated by @new_order.
 */

void rclib_db_catalog_reorder(gint *new_order)
{
    RCLibDbPrivate *priv;
    gint i;
    GHashTable *new_positions;
    GSequenceIter *ptr;
    gint *order;
    gint length;
    g_return_if_fail(db_instance!=NULL);
    g_return_if_fail(new_order!=NULL);
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    g_return_if_fail(priv!=NULL);
    length = g_sequence_get_length(priv->catalog);
    order = g_new(gint, length);
    for(i=0;i<length;i++) order[new_order[i]] = i;
    new_positions = g_hash_table_new(g_direct_hash, g_direct_equal);
    ptr = g_sequence_get_begin_iter(priv->catalog);
    for(i=0;!g_sequence_iter_is_end(ptr);ptr=g_sequence_iter_next(ptr))
    {
        g_hash_table_insert(new_positions, ptr, GINT_TO_POINTER(order[i++]));
    }
    g_free(order);
    g_sequence_sort_iter(priv->catalog, rclib_db_reorder_func,
        new_positions);
    g_hash_table_destroy(new_positions);
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_REORDERED],
        0, new_order);
}

/**
 * rclib_db_playlist_add_music:
 * @iter: the catalog iter
 * @insert_iter: insert the music before this iter
 * @uri: the URI of the music
 *
 * Add music to the music library by given catalog iter.
 */

void rclib_db_playlist_add_music(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *uri)
{
    RCLibDbPlaylistImportData *import_data;
    RCLibDbPrivate *priv;
    if(uri==NULL || iter==NULL || db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbPlaylistImportData, 1);
    import_data->iter = iter;
    import_data->insert_iter = insert_iter;
    import_data->uri = g_strdup(uri);
    g_async_queue_push(priv->import_queue, import_data);
}

/**
 * rclib_db_playlist_add_music_and_play:
 * @iter: the catalog iter
 * @insert_iter: insert the music before this iter
 * @uri: the URI of the music
 *
 * Add music to the music library by given catalog iter, and then play it
 * if the add operation succeeds.
 */


void rclib_db_playlist_add_music_and_play(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *uri)
{
    RCLibDbPlaylistImportData *import_data;
    RCLibDbPrivate *priv;
    if(uri==NULL || iter==NULL || db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbPlaylistImportData, 1);
    import_data->iter = iter;
    import_data->insert_iter = insert_iter;
    import_data->uri = g_strdup(uri);
    import_data->play_flag = TRUE;
    g_async_queue_push(priv->import_queue, import_data);
} 

/**
 * rclib_db_playlist_delete:
 * @iter: the iter to the playlist data
 *
 * Delete the playlist data pointed to by #iter.
 */

void rclib_db_playlist_delete(GSequenceIter *iter)
{
    RCLibDbPrivate *priv;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_DELETE],
        0, iter);
    g_sequence_remove(iter);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_update_metadata:
 * @iter: the iter to the playlist
 * @data: the new metadata
 *
 * Update the metadata in the playlist pointed to by #iter.
 * Title, artist, album, file type, track number, year information
 * will be updated.
 */

void rclib_db_playlist_update_metadata(GSequenceIter *iter,
    const RCLibDbPlaylistData *data)
{
    RCLibDbPlaylistData *playlist_data;
    RCLibDbPrivate *priv;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    g_free(playlist_data->title);
    g_free(playlist_data->artist);
    g_free(playlist_data->album);
    g_free(playlist_data->ftype);
    playlist_data->title = g_strdup(data->title);
    playlist_data->artist = g_strdup(data->artist);
    playlist_data->album = g_strdup(data->album);
    playlist_data->ftype = g_strdup(data->ftype);
    playlist_data->tracknum = data->tracknum;
    playlist_data->year = data->year;
    playlist_data->type = data->type;
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_CHANGED],
        0, iter);
}

/**
 * rclib_db_playlist_update_length:
 * @iter: the iter to the playlist
 * @length: the new length (duration)
 *
 * Update the length (duration) information in the playlist pointed to
 * by #iter.
 */

void rclib_db_playlist_update_length(GSequenceIter *iter,
    gint64 length)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    playlist_data->length = length;
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_CHANGED],
        0, iter);
}

/**
 * rclib_db_playlist_set_type:
 * @iter: the iter to the playlist
 * @type: the new type to set
 *
 * Set the type information in the playlist pointed to by #iter.
 */

void rclib_db_playlist_set_type(GSequenceIter *iter,
    RCLibDbPlaylistType type)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    playlist_data->type = type;
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_CHANGED],
        0, iter);
}

/**
 * rclib_db_playlist_set_rating:
 * @iter: the iter to the playlist
 * @rating: the new rating to set
 *
 * Set the rating in the playlist pointed to by #iter.
 */

void rclib_db_playlist_set_rating(GSequenceIter *iter, gint rating)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    playlist_data->rating = rating;
    priv->dirty_flag = TRUE;
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_CHANGED],
        0, iter);
}

/**
 * rclib_db_playlist_set_lyric_bind:
 * @iter: the iter to the playlist
 * @filename: the path of the lyric file
 *
 * Bind the lyric file to the music in the playlist pointed to by #iter.
 */

void rclib_db_playlist_set_lyric_bind(GSequenceIter *iter,
    const gchar *filename)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    g_free(playlist_data->lyricfile);
    playlist_data->lyricfile = g_strdup(filename);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_set_lyric_secondary_bind:
 * @iter: the iter to the playlist
 * @filename: the path of the lyric file
 *
 * Bind the secondary lyric file to the music in the playlist pointed
 * to by #iter.
 */

void rclib_db_playlist_set_lyric_secondary_bind(GSequenceIter *iter,
    const gchar *filename)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    g_free(playlist_data->lyricsecfile);
    playlist_data->lyricsecfile = g_strdup(filename);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_set_album_bind:
 * @iter: the iter to the playlist
 * @filename: the path of the album image file
 *
 * Bind the album image file to the music in the playlist pointed
 * to by #iter.
 */

void rclib_db_playlist_set_album_bind(GSequenceIter *iter,
    const gchar *filename)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    g_free(playlist_data->albumfile);
    playlist_data->albumfile = g_strdup(filename);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_get_lyric_bind:
 * @iter: the iter to the playlist
 *
 * Get the lyric file bound to the music in the playlist pointed
 * to by #iter.
 *
 * Returns: The lyric file path.
 */

const gchar *rclib_db_playlist_get_lyric_bind(GSequenceIter *iter)
{
    RCLibDbPlaylistData *playlist_data;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return NULL;
    return playlist_data->lyricfile;
}

/**
 * rclib_db_playlist_get_lyric_secondary_bind:
 * @iter: the iter to the playlist
 *
 * Get the secondary lyric file bound to the music in the playlist pointed
 * to by #iter.
 *
 * Returns: The lyric file path.
 */

const gchar *rclib_db_playlist_get_lyric_secondary_bind(GSequenceIter *iter)
{
    RCLibDbPlaylistData *playlist_data;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return NULL;
    return playlist_data->lyricsecfile;
}

/**
 * rclib_db_playlist_get_album_bind:
 * @iter: the iter to the playlist
 *
 * Get the album image file bound to the music in the playlist pointed
 * to by #iter.
 *
 * Returns: The album image file path.
 */

const gchar *rclib_db_playlist_get_album_bind(GSequenceIter *iter)
{
    RCLibDbPlaylistData *playlist_data;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return NULL;
    return playlist_data->albumfile;
}

/**
 * rclib_db_playlist_reorder:
 * @iter: the iter pointed to catalog
 * @new_order: (array): an array of integers mapping the new position of 
 *   each child to its old position before the re-ordering,
 *   i.e. @new_order<literal>[newpos] = oldpos</literal>.
 *
 * Reorder the playlist to follow the order indicated by @new_order.
 */

void rclib_db_playlist_reorder(GSequenceIter *iter, gint *new_order)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogData *catalog_data;
    GHashTable *new_positions;
    GSequenceIter *ptr;
    gint i;
    gint *order;
    gint length;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(new_order!=NULL);
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    catalog_data = g_sequence_get(iter);
    g_return_if_fail(catalog_data!=NULL);
    length = g_sequence_get_length(catalog_data->playlist);
    order = g_new(gint, length);
    for(i=0;i<length;i++) order[new_order[i]] = i;
    new_positions = g_hash_table_new(g_direct_hash, g_direct_equal);
    ptr = g_sequence_get_begin_iter(catalog_data->playlist);
    for(i=0;!g_sequence_iter_is_end(ptr);ptr=g_sequence_iter_next(ptr))
    {
        g_hash_table_insert(new_positions, ptr, GINT_TO_POINTER(order[i++]));
    }
    g_free(order);
    g_sequence_sort_iter(catalog_data->playlist, rclib_db_reorder_func,
        new_positions);
    g_hash_table_destroy (new_positions);
    g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_REORDERED],
        0, iter, new_order);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_move_to_another_catalog:
 * @iters: an iter array
 * @num: the size of the iter array
 * @catalog_iter: the iter for the catalog
 *
 * Move the playlist data pointed to by #iters to another catalog
 * pointed to by #catalog_iter.
 */

void rclib_db_playlist_move_to_another_catalog(GSequenceIter **iters,
    guint num, GSequenceIter *catalog_iter)
{
    gint i;
    RCLibDbPrivate *priv;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *old_data, *new_data;
    GSequenceIter *new_iter;
    GSequenceIter *reference;
    if(iters==NULL || catalog_iter==NULL || num<1) return;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL) return;
    catalog_data = g_sequence_get(catalog_iter);
    if(catalog_data==NULL) return;
    reference = rclib_core_get_db_reference();
    for(i=0;i<num;i++)
    {
        old_data = g_sequence_get(iters[i]);
        if(old_data==NULL) continue;
        new_data = rclib_db_playlist_data_ref(old_data);
        rclib_db_playlist_delete(iters[i]);
        new_data->catalog = catalog_iter;
        new_iter = g_sequence_append(catalog_data->playlist, new_data);
        if(iters[i]==reference)
            rclib_core_update_db_reference(new_iter);
        g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_ADDED],
            0, new_iter);
    }
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_add_m3u_file:
 * @iter: the catalog iter
 * @insert_iter: insert the music before this iter
 * @filename: the path of the playlist file
 * 
 * Load a m3u playlist file, and add all music inside to
 * the catalog pointed to by #iter.
 */

void rclib_db_playlist_add_m3u_file(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *filename)
{
    GFile *file;
    GFileInputStream *input_stream;
    GDataInputStream *data_stream;
    gsize line_len;
    gchar *line = NULL;
    gchar *uri = NULL;
    gchar *temp_name = NULL;
    gchar *path = NULL;
    if(filename==NULL || iter==NULL) return;
    file = g_file_new_for_path(filename);
    if(file==NULL) return;
    if(!g_file_query_exists(file, NULL))
    {
        g_object_unref(file);
        return;
    }
    input_stream = g_file_read(file, NULL, NULL);
    g_object_unref(file);
    if(input_stream==NULL) return;
    data_stream = g_data_input_stream_new(G_INPUT_STREAM(input_stream));
    g_object_unref(input_stream);
    if(data_stream==NULL) return;
    path = g_path_get_dirname(filename);
    g_data_input_stream_set_newline_type(data_stream,
        G_DATA_STREAM_NEWLINE_TYPE_ANY);
    while((line=g_data_input_stream_read_line(data_stream, &line_len,
        NULL, NULL))!=NULL)
    {
        if(!g_str_has_prefix(line, "#") && *line!='\n' && *line!='\0')
        {
            if(strncmp(line, "file://", 7)!=0
                && strncmp(line, "http://", 7)!=0)
            {
                if(g_path_is_absolute(line))
                    uri = g_filename_to_uri(line, NULL, NULL);
                else
                {
                    temp_name = g_build_filename(path, line, NULL);
                    uri = g_filename_to_uri(temp_name, NULL, NULL);
                    g_free(temp_name);
                }
            }
            else
                uri = g_strdup(line);
            if(uri!=NULL)
            {
                rclib_db_playlist_add_music(iter, insert_iter,
                    uri);
                g_free(uri);
            }
        }
        g_free(line);
    }
    g_object_unref(data_stream);
    g_free(path);
}

static void rclib_db_playlist_add_directory_recursive(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *fdir, guint depth)
{
    gchar *full_path = NULL;
    GDir *dir;
    const gchar *filename = NULL;
    gchar *uri = NULL;
    if(depth==0) return;
    dir = g_dir_open(fdir, 0, NULL);
    if(dir==NULL) return;
    while((filename = g_dir_read_name(dir))!=NULL)
    {
        full_path = g_build_filename(fdir, filename, NULL);
        if(g_file_test(full_path, G_FILE_TEST_IS_DIR))
            rclib_db_playlist_add_directory_recursive(iter, insert_iter,
                full_path, depth-1);
        if(!g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
        {
            g_free(full_path);
            continue;
        }
        if(rclib_util_is_supported_media(full_path))
        {
            uri = g_filename_to_uri(full_path, NULL, NULL);
            rclib_db_playlist_add_music(iter, insert_iter, uri);
            g_free(uri);
        }
        g_free(full_path);
    }
    g_dir_close(dir);
}

/**
 * rclib_db_playlist_add_directory:
 * @iter: the catalog iter
 * @insert_iter: insert the music before this iter
 * @dir: the directory path
 *
 * Add all music in the directory to the catalog pointed to by #iter.
 */

void rclib_db_playlist_add_directory(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *dir)
{
    if(iter==NULL || dir==NULL) return;
    rclib_db_playlist_add_directory_recursive(iter, insert_iter, dir,
        db_import_depth);
}

/**
 * rclib_db_playlist_export_m3u_file:
 * @iter: the catalog iter
 * @sfilename: the new playlist file path
 *
 * Export the catalog pointed to by #iter to a new playlist file.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_db_playlist_export_m3u_file(GSequenceIter *iter,
    const gchar *sfilename)
{
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *playlist_iter;
    gchar *title = NULL;
    gchar *filename;
    FILE *fp;
    if(iter==NULL || sfilename==NULL) return FALSE;
    catalog_data = g_sequence_get(iter);
    if(catalog_data==NULL) return FALSE;
    if(g_regex_match_simple("(.M3U)$", sfilename, G_REGEX_CASELESS, 0))
        filename = g_strdup(sfilename);
    else
        filename = g_strdup_printf("%s.M3U", sfilename);
    fp = g_fopen(filename, "wb");
    g_free(filename);
    if(fp==NULL) return FALSE;
    g_fprintf(fp, "#EXTM3U\n");
    for(playlist_iter = g_sequence_get_begin_iter(catalog_data->playlist);
        !g_sequence_iter_is_end(playlist_iter);
        playlist_iter = g_sequence_iter_next(playlist_iter))
    {
        playlist_data = g_sequence_get(playlist_iter);
        if(playlist_data==NULL || playlist_data->uri==NULL) continue;
        if(playlist_data->title!=NULL)
            title = g_strdup(playlist_data->title);
        else
            title = rclib_tag_get_name_from_uri(playlist_data->uri);
        if(title==NULL) title = g_strdup(_("Unknown Title"));
        if(playlist_data->artist==NULL)
        {
            g_fprintf(fp, "#EXTINF:%"G_GINT64_FORMAT",%s\n%s\n",
                playlist_data->length / GST_SECOND, title,
                playlist_data->uri);
        }
        else
        {
            g_fprintf(fp, "#EXTINF:%"G_GINT64_FORMAT",%s - %s\n%s\n",
                playlist_data->length / GST_SECOND, playlist_data->artist,
                title, playlist_data->uri);
        }
        g_free(title);
    }
    fclose(fp);
    return TRUE;
}

/**
 * rclib_db_playlist_export_all_m3u_files:
 * @dir: the directory to save playlist files
 *
 * Export all playlists in the catalog to playlist files.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_db_playlist_export_all_m3u_files(const gchar *dir)
{
    gboolean flag = FALSE;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPrivate *priv;
    GSequenceIter *catalog_iter;
    gchar *filename;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    for(catalog_iter = g_sequence_get_begin_iter(priv->catalog);
        !g_sequence_iter_is_end(catalog_iter);
        catalog_iter = g_sequence_iter_next(catalog_iter))
    {
        catalog_data = g_sequence_get(catalog_iter);
        if(catalog_data==NULL || catalog_data->name==NULL) continue;
        filename = g_strdup_printf("%s%c%s.M3U", dir,
            G_DIR_SEPARATOR, catalog_data->name);
        if(rclib_db_playlist_export_m3u_file(catalog_iter, filename))
            flag = TRUE;
        g_free(filename);
    }
    return flag;
}

/**
 * rclib_db_playlist_refresh:
 * @iter: the catalog iter
 *
 * Refresh the metadata of the music in the playlist pointed to
 * by the given catalog #iter.
 */

void rclib_db_playlist_refresh(GSequenceIter *iter)
{
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbPlaylistRefreshData *refresh_data;
    GSequenceIter *iter_foreach;
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return;
    if(iter==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL || priv->refresh_queue==NULL) return;
    catalog_data = g_sequence_get(iter);
    if(catalog_data==NULL) return;
    if(catalog_data->playlist==NULL) return;
    for(iter_foreach = g_sequence_get_begin_iter(catalog_data->playlist);
        !g_sequence_iter_is_end(iter_foreach);
        iter_foreach = g_sequence_iter_next(iter_foreach))
    {
        playlist_data = g_sequence_get(iter_foreach);
        if(playlist_data==NULL || playlist_data->uri==NULL) continue;
        refresh_data = g_new0(RCLibDbPlaylistRefreshData, 1);
        refresh_data->catalog_iter = iter;
        refresh_data->playlist_iter = iter_foreach;
        refresh_data->uri = g_strdup(playlist_data->uri);
        g_async_queue_push(priv->refresh_queue, refresh_data);
    }
}

/**
 * rclib_db_playlist_import_cancel:
 *
 * Cancel all remaining import jobs in the queue.
 */

void rclib_db_playlist_import_cancel()
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistImportData *import_data;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL || priv->import_queue==NULL) return;
    while(g_async_queue_length(priv->import_queue)>=0)
    {
        import_data = g_async_queue_try_pop(priv->import_queue);
        if(import_data!=NULL)
            rclib_db_playlist_import_data_free(import_data);
    }
}

/**
 * rclib_db_playlist_refresh_cancel:
 *
 * Cancel all remaining refresh jobs in the queue.
 */

void rclib_db_playlist_refresh_cancel()
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistRefreshData *refresh_data;
    if(db_instance==NULL) return;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL || priv->refresh_queue==NULL) return;
    while(g_async_queue_length(priv->refresh_queue)>=0)
    {
        refresh_data = g_async_queue_try_pop(priv->refresh_queue);
        if(refresh_data!=NULL)
            rclib_db_playlist_refresh_data_free(refresh_data);
    }
}

/**
 * rclib_db_playlist_import_queue_get_length:
 *
 * Get the number of remaining jobs in the import queue.
 *
 * Returns: The number of remaining jobs.
 */

gint rclib_db_playlist_import_queue_get_length()
{
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return -1;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL || priv->import_queue==NULL) return -1;
    return g_async_queue_length(priv->refresh_queue);
}

/**
 * rclib_db_playlist_refresh_queue_get_length:
 *
 * Get the number of remaining jobs in the refresh queue.
 *
 * Returns: The number of remaining jobs.
 */

gint rclib_db_playlist_refresh_queue_get_length()
{
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return -1;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL || priv->refresh_queue==NULL) return -1;
    return g_async_queue_length(priv->refresh_queue);
}

/**
 * rclib_db_sync:
 *
 * Save the database (if modified) to disk.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_db_sync()
{
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB_GET_PRIVATE(RCLIB_DB(db_instance));
    if(priv==NULL || priv->catalog==NULL || priv->filename)
        return FALSE;
    if(!priv->dirty_flag) return TRUE;
    return rclib_db_save_library_db(priv->catalog, priv->filename,
        &(priv->dirty_flag));
}

