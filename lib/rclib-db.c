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
#include "rclib-db-priv.h"
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
 * data can be accessed by #RCLibDbCatalogIter and #RCLibDbPlaylistIter.
 * The database can import music or playlist file asynchronously, or update
 * the metadata in playlists asynchronously.
 */

typedef struct RCLibDbXMLParserData
{
    gboolean db_flag;
    RCLibDbCatalogSequence *catalog;
    RCLibDbPlaylistSequence *playlist;
    RCLibDbCatalogIter *catalog_iter;
    GHashTable *catalog_iter_table;
    GHashTable *playlist_iter_table;
    GHashTable *library_table;
}RCLibDbXMLParserData;

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
    SIGNAL_LIBRARY_ADDED,
    SIGNAL_LIBRARY_CHANGED,
    SIGNAL_LIBRARY_DELETE,
    SIGNAL_LAST
};

static GObject *db_instance = NULL;
static gpointer rclib_db_parent_class = NULL;
static gint db_signals[SIGNAL_LAST] = {0};
static const gint db_autosave_timeout = 120;

static gboolean rclib_db_import_update_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    gint length;
    length = GPOINTER_TO_INT(data);
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL) return FALSE;
    if(length<=0)
        priv->import_work_flag = FALSE;
    g_signal_emit(db_instance, db_signals[SIGNAL_IMPORT_UPDATED], 0,
        length);
    return FALSE;
}

static gboolean rclib_db_refresh_update_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    gint length;
    length = GPOINTER_TO_INT(data);
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL) return FALSE;
    if(length<=0)
        priv->refresh_work_flag = FALSE;
    g_signal_emit(db_instance, db_signals[SIGNAL_REFRESH_UPDATED], 0,
        length);
    return FALSE;
}

static void rclib_db_import_data_free(RCLibDbImportData *data)
{
    if(data==NULL) return;
    g_free(data->uri);
    g_free(data);
}

static void rclib_db_refresh_data_free(RCLibDbRefreshData *data)
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
    if(cue_data->genre!=NULL)
        mmd->genre = g_strdup(cue_data->genre);
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
    RCLibDbLibraryImportIdleData *library_idle_data;
    RCLibTagMetadata *mmd = NULL, *cue_mmd = NULL;
    RCLibDbImportData *import_data;
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
    priv = RCLIB_DB(object)->priv;
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
                memset(&cue_data, 0, sizeof(RCLibCueData));
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
                        if(import_data->type==RCLIB_DB_IMPORT_TYPE_PLAYLIST)
                        {
                            idle_data = g_new0(
                                RCLibDbPlaylistImportIdleData, 1);
                            idle_data->mmd = mmd;
                            idle_data->catalog_iter =
                                import_data->catalog_iter;
                            idle_data->playlist_insert_iter =
                                import_data->playlist_insert_iter;
                            if(i==0)
                                idle_data->play_flag = import_data->play_flag;
                            idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                            g_idle_add(_rclib_db_playlist_import_idle_cb,
                                idle_data);
                        }
                        else if(import_data->type==
                            RCLIB_DB_IMPORT_TYPE_LIBRARY)
                        {
                            library_idle_data = g_new0(
                                RCLibDbLibraryImportIdleData, 1);
                            library_idle_data->mmd = mmd;
                            if(i==0)
                            {
                                library_idle_data->play_flag =
                                    import_data->play_flag;
                            }
                            library_idle_data->type =
                                RCLIB_DB_LIBRARY_TYPE_CUE;
                            g_idle_add(_rclib_db_library_import_idle_cb,
                                library_idle_data);
                        }
                        else
                        {
                            g_warning("Unknown import type!");
                            rclib_tag_free(mmd);
                        }
                    }
                    rclib_tag_free(cue_mmd);
                }
                rclib_cue_free(&cue_data);
                break;
            }
            if(local_flag && rclib_cue_get_track_num(import_data->uri,
                &cue_uri, &track))
            {
                memset(&cue_data, 0, sizeof(RCLibCueData));
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
                            if(import_data->type==
                                RCLIB_DB_IMPORT_TYPE_PLAYLIST)
                            {
                                idle_data =
                                    g_new0(RCLibDbPlaylistImportIdleData, 1);
                                idle_data->catalog_iter =
                                    import_data->catalog_iter;
                                idle_data->playlist_insert_iter =
                                    import_data->playlist_insert_iter;
                                idle_data->mmd = mmd;
                                 idle_data->play_flag = import_data->play_flag;
                                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                                g_idle_add(_rclib_db_playlist_import_idle_cb,
                                    idle_data);
                            }
                            else if(import_data->type==
                                RCLIB_DB_IMPORT_TYPE_LIBRARY)
                            {
                                library_idle_data = g_new0(
                                    RCLibDbLibraryImportIdleData, 1);
                                library_idle_data->mmd = mmd;
                                library_idle_data->play_flag =
                                    import_data->play_flag;
                                library_idle_data->type =
                                    RCLIB_DB_LIBRARY_TYPE_CUE;
                                g_idle_add(_rclib_db_library_import_idle_cb,
                                    library_idle_data);
                            }
                            else
                            {
                                g_warning("Unknown import type!");
                                rclib_tag_free(mmd);
                            }
                        }
                    }
                    rclib_cue_free(&cue_data);
                    g_free(cue_uri);
                    break;
                }
                else /* Maybe a embedded CUE audio file? */
                {
                    g_free(import_data->uri);
                    import_data->uri = g_strdup(cue_uri);
                    g_free(cue_uri);
                }
            }
            mmd = rclib_tag_read_metadata(import_data->uri);
            if(mmd==NULL) break;
            if(mmd->emb_cue!=NULL) /* Embedded CUE check */
            {
                if(rclib_cue_read_data(mmd->emb_cue,
                    RCLIB_CUE_INPUT_EMBEDDED, &cue_data)>0)
                {
                    if(track>0)
                    {
                        cue_mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            track-1, mmd);
                        if(cue_mmd!=NULL)
                        {
                            rclib_tag_free(mmd);
                            if(import_data->type==
                                RCLIB_DB_IMPORT_TYPE_PLAYLIST)
                            {
                                idle_data = g_new0(
                                    RCLibDbPlaylistImportIdleData, 1);
                                idle_data->catalog_iter =
                                    import_data->catalog_iter;
                                idle_data->playlist_insert_iter =
                                    import_data->playlist_insert_iter;
                                idle_data->mmd = cue_mmd;
                                idle_data->play_flag = import_data->play_flag;
                                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                                g_idle_add(_rclib_db_playlist_import_idle_cb,
                                    idle_data);
                            }
                            else if(import_data->type==
                                RCLIB_DB_IMPORT_TYPE_LIBRARY)
                            {
                                library_idle_data = g_new0(
                                    RCLibDbLibraryImportIdleData, 1);
                                library_idle_data->mmd = cue_mmd;
                                library_idle_data->play_flag =
                                    import_data->play_flag;
                                library_idle_data->type =
                                    RCLIB_DB_LIBRARY_TYPE_CUE;
                                g_idle_add(_rclib_db_library_import_idle_cb,
                                    library_idle_data);
                            }
                            else
                            {
                                g_warning("Unknown import type!");
                                rclib_tag_free(cue_mmd);
                            }
                        }
                    }
                    else
                    {
                        for(i=0;i<cue_data.length;i++)
                        {
                            cue_mmd = rclib_db_get_metadata_from_cue(
                                &cue_data, i, mmd);
                            if(cue_mmd==NULL) continue;
                            if(import_data->type==
                                RCLIB_DB_IMPORT_TYPE_PLAYLIST)
                            {
                                idle_data = g_new0(
                                    RCLibDbPlaylistImportIdleData, 1);
                                idle_data->catalog_iter =
                                    import_data->catalog_iter;
                                idle_data->playlist_insert_iter =
                                    import_data->playlist_insert_iter;
                                idle_data->mmd = cue_mmd;
                                if(i==0)
                                {
                                   idle_data->play_flag =
                                       import_data->play_flag;
                                }
                                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                                g_idle_add(_rclib_db_playlist_import_idle_cb,
                                    idle_data);
                            }
                            else if(import_data->type==
                                RCLIB_DB_IMPORT_TYPE_LIBRARY)
                            {
                                library_idle_data = g_new0(
                                    RCLibDbLibraryImportIdleData, 1);
                                library_idle_data->mmd = cue_mmd;
                                if(i==0)
                                {
                                    library_idle_data->play_flag =
                                        import_data->play_flag;
                                }
                                library_idle_data->type =
                                    RCLIB_DB_LIBRARY_TYPE_CUE;
                                g_idle_add(_rclib_db_library_import_idle_cb,
                                    library_idle_data);
                            }
                            else
                            {
                                g_warning("Invalid import type!");
                                rclib_tag_free(cue_mmd);
                            }
                        }
                        rclib_tag_free(mmd);
                    }
                    break;
                }
            }
            if(import_data->type==RCLIB_DB_IMPORT_TYPE_PLAYLIST)
            {
                idle_data = g_new0(RCLibDbPlaylistImportIdleData, 1);
                idle_data->catalog_iter = import_data->catalog_iter;
                idle_data->playlist_insert_iter =
                    import_data->playlist_insert_iter;
                idle_data->mmd = mmd;
                idle_data->play_flag = import_data->play_flag;
                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
                g_idle_add(_rclib_db_playlist_import_idle_cb, idle_data);
            }
            else if(import_data->type==RCLIB_DB_IMPORT_TYPE_LIBRARY)
            {
                library_idle_data = g_new0(RCLibDbLibraryImportIdleData, 1);
                library_idle_data->mmd = mmd;
                library_idle_data->play_flag = import_data->play_flag;
                library_idle_data->type = RCLIB_DB_LIBRARY_TYPE_MUSIC;
                g_idle_add(_rclib_db_library_import_idle_cb,
                    library_idle_data);
            }
            else
            {
                g_warning("Invalid import type!");
                rclib_tag_free(mmd);
            }
        }
        G_STMT_END;
        rclib_db_import_data_free(import_data);
        length = g_async_queue_length(priv->import_queue);
        g_idle_add(rclib_db_import_update_idle_cb, GINT_TO_POINTER(length));
    }
    g_thread_exit(NULL);
    return NULL;
}


static gpointer rclib_db_playlist_refresh_thread_cb(gpointer data)
{
    RCLibDbPlaylistRefreshIdleData *idle_data;
    RCLibDbLibraryRefreshIdleData *library_idle_data;
    RCLibDbRefreshData *refresh_data;
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
    priv = RCLIB_DB(object)->priv;
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
                    memset(&cue_data, 0, sizeof(RCLibCueData));
                    if(rclib_cue_read_data(cue_uri, RCLIB_CUE_INPUT_URI,
                        &cue_data)>0)
                    {
                        mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            track-1, NULL);
                        if(mmd!=NULL) g_free(mmd->uri);
                        if(refresh_data->type==
                            RCLIB_DB_REFRESH_TYPE_PLAYLIST)
                        {
                            if(mmd!=NULL)
                            {
                                mmd->uri = g_strdup(refresh_data->uri);
                                idle_data =
                                    g_new0(RCLibDbPlaylistRefreshIdleData, 1);
                                idle_data->catalog_iter =
                                    refresh_data->catalog_iter;
                                idle_data->playlist_iter =
                                    refresh_data->playlist_iter;
                                idle_data->mmd = mmd;
                                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                                g_idle_add(_rclib_db_playlist_refresh_idle_cb,
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
                                idle_data->type =
                                    RCLIB_DB_PLAYLIST_TYPE_MISSING;
                                g_idle_add(_rclib_db_playlist_refresh_idle_cb,
                                    idle_data);
                            }
                        }
                        else if(refresh_data->type==
                            RCLIB_DB_REFRESH_TYPE_LIBRARY)
                        {
                            if(mmd!=NULL)
                            {
                                library_idle_data = g_new0(
                                    RCLibDbLibraryRefreshIdleData, 1);
                                library_idle_data->mmd = mmd;
                                library_idle_data->type =
                                    RCLIB_DB_PLAYLIST_TYPE_CUE;
                                g_idle_add(_rclib_db_library_refresh_idle_cb,
                                    library_idle_data);
                            }
                            else
                            {
                                library_idle_data = g_new0(
                                    RCLibDbLibraryRefreshIdleData, 1);
                                library_idle_data->mmd = NULL;
                                library_idle_data->type =
                                    RCLIB_DB_PLAYLIST_TYPE_MISSING;
                                g_idle_add(_rclib_db_library_refresh_idle_cb,
                                    library_idle_data);
                            }
                        }
                        else
                        {
                            g_warning("Unknown refresh type!");
                            if(mmd!=NULL) rclib_tag_free(mmd);
                        }
                    }
                    rclib_cue_free(&cue_data);
                    g_free(cue_uri);
                    break;
                }
                else /* Maybe a embedded CUE audio file? */
                {
                    g_free(refresh_data->uri);
                    refresh_data->uri = g_strdup(cue_uri);
                    g_free(cue_uri);
                }
            }
            mmd = rclib_tag_read_metadata(refresh_data->uri);
            if(mmd!=NULL && mmd->emb_cue!=NULL) /* Embedded CUE check */
            {
                if(rclib_cue_read_data(mmd->emb_cue,
                    RCLIB_CUE_INPUT_EMBEDDED, &cue_data)>0)
                {
                    if(track>0)
                    {
                        cue_mmd = rclib_db_get_metadata_from_cue(&cue_data,
                            track-1, mmd);
                        rclib_tag_free(mmd);
                        if(refresh_data->type==
                            RCLIB_DB_REFRESH_TYPE_PLAYLIST)
                        {
                            if(cue_mmd!=NULL)
                            {
                                idle_data =
                                    g_new0(RCLibDbPlaylistRefreshIdleData, 1);
                                idle_data->catalog_iter =
                                    refresh_data->catalog_iter;
                                idle_data->playlist_iter =
                                    refresh_data->playlist_iter;
                                idle_data->mmd = cue_mmd;
                                idle_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
                                g_idle_add(_rclib_db_playlist_refresh_idle_cb,
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
                                idle_data->type =
                                    RCLIB_DB_PLAYLIST_TYPE_MISSING;
                                g_idle_add(_rclib_db_playlist_refresh_idle_cb,
                                    idle_data);
                            }
                        }
                        else if(refresh_data->type==
                                RCLIB_DB_REFRESH_TYPE_LIBRARY)
                        {
                            if(cue_mmd!=NULL)
                            {
                                library_idle_data = g_new0(
                                    RCLibDbLibraryRefreshIdleData, 1);
                                library_idle_data->mmd = cue_mmd;
                                library_idle_data->type =
                                    RCLIB_DB_PLAYLIST_TYPE_CUE;
                                g_idle_add(_rclib_db_library_refresh_idle_cb,
                                    library_idle_data);
                            }
                            else
                            {
                                library_idle_data = g_new0(
                                    RCLibDbLibraryRefreshIdleData, 1);
                                library_idle_data->mmd = NULL;
                                library_idle_data->type =
                                    RCLIB_DB_PLAYLIST_TYPE_MISSING;
                                g_idle_add(_rclib_db_library_refresh_idle_cb,
                                    library_idle_data);
                            }
                        }
                        else
                        {
                            g_warning("Unknown refresh type!");
                            if(cue_mmd!=NULL) rclib_tag_free(cue_mmd);
                        }
                    }
                    break;
                }
            }
            if(refresh_data->type==RCLIB_DB_REFRESH_TYPE_PLAYLIST)
            {
                idle_data = g_new0(RCLibDbPlaylistRefreshIdleData, 1);
                idle_data->catalog_iter = refresh_data->catalog_iter;
                idle_data->playlist_iter = refresh_data->playlist_iter;
                idle_data->mmd = mmd;
                if(mmd!=NULL)
                    idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
                else
                    idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MISSING;
                g_idle_add(_rclib_db_playlist_refresh_idle_cb, idle_data);
            }
            else if(refresh_data->type==RCLIB_DB_REFRESH_TYPE_LIBRARY)
            {
                library_idle_data = g_new0(RCLibDbLibraryRefreshIdleData, 1);
                library_idle_data->mmd = mmd;
                if(mmd!=NULL)
                    library_idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
                else
                    library_idle_data->type = RCLIB_DB_PLAYLIST_TYPE_MISSING;
                g_idle_add(_rclib_db_library_refresh_idle_cb,
                    library_idle_data);
            }
            else
            {
                g_warning("Unknown refresh type!");
                if(mmd!=NULL) rclib_tag_free(mmd);
            }
        }
        G_STMT_END;
        rclib_db_refresh_data_free(refresh_data);
        length = g_async_queue_length(priv->refresh_queue);
        g_idle_add(rclib_db_refresh_update_idle_cb, GINT_TO_POINTER(length));
    }
    g_thread_exit(NULL);
    return NULL;
}

static void rclib_db_xml_parser_start_element_cb(GMarkupParseContext *context,
    const gchar *element_name, const gchar **attribute_names,
    const gchar **attribute_values, gpointer data, GError **error)
{
    RCLibDbXMLParserData *parser_data = (RCLibDbXMLParserData *)data;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbLibraryData *library_data;
    RCLibDbCatalogIter *catalog_iter;
    RCLibDbPlaylistIter *playlist_iter;
    guint i;
    if(data==NULL) return;
    if(g_strcmp0(element_name, "rclibdb")==0)
    {
        parser_data->db_flag = TRUE;
        return;
    }
    if(!parser_data->db_flag) return;
    if(parser_data->playlist!=NULL && g_strcmp0(element_name, "item")==0)
    {
        playlist_data = rclib_db_playlist_data_new();
        playlist_data->catalog = parser_data->catalog_iter;
        for(i=0;attribute_names[i]!=NULL;i++)
        {
            if(playlist_data->uri==NULL &&
                g_strcmp0(attribute_names[i], "uri")==0)
            {
                playlist_data->uri = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "type")==0)
            {
                sscanf(attribute_values[i], "%u", &(playlist_data->type));
            }
            else if(playlist_data->title==NULL &&
                g_strcmp0(attribute_names[i], "title")==0)
            {
                playlist_data->title = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->artist==NULL &&
                g_strcmp0(attribute_names[i], "artist")==0)
            {
                playlist_data->artist = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->album==NULL &&
                g_strcmp0(attribute_names[i], "album")==0)
            {
                playlist_data->album = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->ftype==NULL &&
                g_strcmp0(attribute_names[i], "filetype")==0)
            {
                playlist_data->ftype = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "length")==0)
            {
                playlist_data->length = g_ascii_strtoll(attribute_values[i],
                    NULL, 10);
            }
            else if(g_strcmp0(attribute_names[i], "tracknum")==0)
            {
                sscanf(attribute_values[i], "%d",
                    &(playlist_data->tracknum));
            }
            else if(g_strcmp0(attribute_names[i], "year")==0)
            {
                sscanf(attribute_values[i], "%d",
                    &(playlist_data->year));
            }
            else if(g_strcmp0(attribute_names[i], "rating")==0)
            {
                sscanf(attribute_values[i], "%f",
                    &(playlist_data->rating));
            }
            else if(playlist_data->lyricfile==NULL &&
                g_strcmp0(attribute_names[i], "lyricfile")==0)
            {
                playlist_data->lyricfile = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->albumfile==NULL &&
                g_strcmp0(attribute_names[i], "albumfile")==0)
            {
                playlist_data->albumfile = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->lyricsecfile==NULL &&
                g_strcmp0(attribute_names[i], "lyricsecondfile")==0)
            {
                playlist_data->lyricsecfile = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->genre==NULL &&
                g_strcmp0(attribute_names[i], "genre")==0)
            {
                playlist_data->genre = g_strdup(attribute_values[i]);
            }
        }
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_append(
            (GSequence *)parser_data->playlist, playlist_data);
        playlist_data->self_iter = playlist_iter;
        g_hash_table_replace(parser_data->playlist_iter_table, playlist_iter,
            playlist_iter);
    }
    else if(parser_data->catalog!=NULL &&
        g_strcmp0(element_name, "playlist")==0)
    {
        catalog_data = rclib_db_catalog_data_new();
        for(i=0;attribute_names[i]!=NULL;i++)
        {
            if(catalog_data->name==NULL &&
                g_strcmp0(attribute_names[i], "name")==0)
            {
                catalog_data->name = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "type")==0)
            {
                sscanf(attribute_values[i], "%u", &(catalog_data->type));
            }
        }
        catalog_data->playlist = (RCLibDbPlaylistSequence *)
            g_sequence_new((GDestroyNotify)rclib_db_playlist_data_unref);
        parser_data->playlist = catalog_data->playlist;
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_append(
            (GSequence *)parser_data->catalog, catalog_data);
        catalog_data->self_iter = catalog_iter;
        parser_data->catalog_iter = catalog_iter;
        g_hash_table_replace(parser_data->catalog_iter_table, catalog_iter,
            catalog_iter);
    }
    if(parser_data->library_table!=NULL && g_strcmp0(element_name,
        "libitem")==0)
    {
        library_data = rclib_db_library_data_new();
        for(i=0;attribute_names[i]!=NULL;i++)
        {
            if(library_data->uri==NULL &&
                g_strcmp0(attribute_names[i], "uri")==0)
            {
                library_data->uri = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "type")==0)
            {
                sscanf(attribute_values[i], "%u", &(library_data->type));
            }
            else if(library_data->title==NULL &&
                g_strcmp0(attribute_names[i], "title")==0)
            {
                library_data->title = g_strdup(attribute_values[i]);
            }
            else if(library_data->artist==NULL &&
                g_strcmp0(attribute_names[i], "artist")==0)
            {
                library_data->artist = g_strdup(attribute_values[i]);
            }
            else if(library_data->album==NULL &&
                g_strcmp0(attribute_names[i], "album")==0)
            {
                library_data->album = g_strdup(attribute_values[i]);
            }
            else if(library_data->ftype==NULL &&
                g_strcmp0(attribute_names[i], "filetype")==0)
            {
                library_data->ftype = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "length")==0)
            {
                library_data->length = g_ascii_strtoll(attribute_values[i],
                    NULL, 10);
            }
            else if(g_strcmp0(attribute_names[i], "tracknum")==0)
            {
                sscanf(attribute_values[i], "%d",
                    &(library_data->tracknum));
            }
            else if(g_strcmp0(attribute_names[i], "year")==0)
            {
                sscanf(attribute_values[i], "%d",
                    &(library_data->year));
            }
            else if(g_strcmp0(attribute_names[i], "rating")==0)
            {
                sscanf(attribute_values[i], "%f",
                    &(library_data->rating));
            }
            else if(library_data->lyricfile==NULL &&
                g_strcmp0(attribute_names[i], "lyricfile")==0)
            {
                library_data->lyricfile = g_strdup(attribute_values[i]);
            }
            else if(library_data->albumfile==NULL &&
                g_strcmp0(attribute_names[i], "albumfile")==0)
            {
                library_data->albumfile = g_strdup(attribute_values[i]);
            }
            else if(library_data->lyricsecfile==NULL &&
                g_strcmp0(attribute_names[i], "lyricsecondfile")==0)
            {
                library_data->lyricsecfile = g_strdup(attribute_values[i]);
            }
            else if(library_data->genre==NULL &&
                g_strcmp0(attribute_names[i], "genre")==0)
            {
                library_data->genre = g_strdup(attribute_values[i]);
            }
        }
        g_hash_table_replace(parser_data->library_table,
            g_strdup(library_data->uri), library_data);
    }
}

static void rclib_db_xml_parser_end_element_cb(GMarkupParseContext *context,
    const gchar *element_name, gpointer data, GError **error)
{
    RCLibDbXMLParserData *parser_data = (RCLibDbXMLParserData *)data;
    if(data==NULL) return;
    if(g_strcmp0(element_name, "rclibdb")==0)
    {
        parser_data->db_flag = FALSE;
    }
    else if(g_strcmp0(element_name, "playlist")==0)
    {
        parser_data->playlist = NULL;
        parser_data->catalog_iter = NULL;
    }
    else if(g_strcmp0(element_name, "library")==0)
    {
        parser_data->library_table = NULL;
    }
}

static gboolean rclib_db_load_library_db(RCLibDbCatalogSequence *catalog,
    GHashTable *catalog_iter_table, GHashTable *playlist_iter_table,
    GHashTable *library_table, const gchar *file, gboolean *dirty_flag)
{
    RCLibDbXMLParserData parser_data = {0};
    GMarkupParseContext *parse_context;
    GMarkupParser markup_parser =
    {
        .start_element = rclib_db_xml_parser_start_element_cb, 
        .end_element = rclib_db_xml_parser_end_element_cb,
        .text = NULL,
        .passthrough = NULL,
        .error = NULL
    };
    GError *error = NULL;
    GFile *input_file;
    GZlibDecompressor *decompressor;
    GFileInputStream *file_istream;
    GInputStream *decompress_istream;
    gchar buffer[4096];
    gssize read_size;
    input_file = g_file_new_for_path(file);
    if(input_file==NULL) return FALSE;
    file_istream = g_file_read(input_file, NULL, &error);
    g_object_unref(input_file);
    if(file_istream==NULL)
    {
        g_warning("Cannot open input stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        return FALSE;
    }
    decompressor = g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB);
    decompress_istream = g_converter_input_stream_new(G_INPUT_STREAM(
        file_istream), G_CONVERTER(decompressor));
    g_object_unref(file_istream);
    g_object_unref(decompressor);
    parser_data.catalog = catalog;
    parser_data.catalog_iter_table = catalog_iter_table;
    parser_data.playlist_iter_table = playlist_iter_table;
    parser_data.library_table = library_table;
    parse_context = g_markup_parse_context_new(&markup_parser, 0,
        &parser_data, NULL);
    while((read_size=g_input_stream_read(decompress_istream, buffer, 4096,
        NULL, NULL))>0)
    {
        g_markup_parse_context_parse(parse_context, buffer, read_size,
            NULL);
    }
    g_markup_parse_context_end_parse(parse_context, NULL);
    g_object_unref(decompress_istream);
    g_markup_parse_context_free(parse_context);
    g_message("Playlist loaded.");
    if(dirty_flag!=NULL) *dirty_flag = FALSE;
    return TRUE;
}

static inline GString *rclib_db_build_xml_data(RCLibDbCatalogSequence *catalog,
    GHashTable *library)
{
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbLibraryData *library_data;
    RCLibDbCatalogIter *catalog_iter;
    RCLibDbPlaylistIter *playlist_iter;
    GHashTableIter library_iter;
    GString *data_str;
    gchar *tmp;
    extern guint rclib_major_version;
    extern guint rclib_minor_version;
    extern guint rclib_micro_version;
    data_str = g_string_new("<?xml version=\"1.0\" standalone=\"yes\"?>\n");
    g_string_append_printf(data_str, "<rclibdb version=\"%u.%u.%u\">\n",
        rclib_major_version, rclib_minor_version, rclib_micro_version);
    if(catalog!=NULL)
    {
        for(catalog_iter = rclib_db_catalog_sequence_get_begin_iter(catalog);
            !rclib_db_catalog_iter_is_end(catalog_iter);
            catalog_iter = rclib_db_catalog_iter_next(catalog_iter))
        {
            catalog_data = rclib_db_catalog_iter_get_data(catalog_iter);
            if(catalog_data==NULL) continue;
            tmp = g_markup_printf_escaped("  <playlist name=\"%s\" "
                "type=\"%u\">\n", catalog_data->name, catalog_data->type);
            g_string_append(data_str, tmp);
            g_free(tmp);
            for(playlist_iter=rclib_db_playlist_sequence_get_begin_iter(
                catalog_data->playlist);
                !rclib_db_playlist_iter_is_end(playlist_iter);
                playlist_iter=rclib_db_playlist_iter_next(playlist_iter))
            {
                playlist_data = rclib_db_playlist_iter_get_data(
                    playlist_iter);
                if(playlist_data==NULL) continue;
                g_string_append_printf(data_str, "    <item type=\"%u\" ",
                    playlist_data->type);
                if(playlist_data->uri!=NULL)
                {
                    tmp = g_markup_printf_escaped("uri=\"%s\" ",
                        playlist_data->uri);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                if(playlist_data->title!=NULL)
                {
                    tmp = g_markup_printf_escaped("title=\"%s\" ",
                        playlist_data->title);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                if(playlist_data->artist!=NULL)
                {
                    tmp = g_markup_printf_escaped("artist=\"%s\" ",
                        playlist_data->artist);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                if(playlist_data->album!=NULL)
                {
                    tmp = g_markup_printf_escaped("album=\"%s\" ",
                        playlist_data->album);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                if(playlist_data->ftype!=NULL)
                {
                    tmp = g_markup_printf_escaped("filetype=\"%s\" ",
                        playlist_data->ftype);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                tmp = g_markup_printf_escaped("length=\"%"G_GINT64_FORMAT"\" ",
                    playlist_data->length);
                g_string_append(data_str, tmp);
                g_free(tmp);
                tmp = g_markup_printf_escaped("tracknum=\"%d\" year=\"%d\" "
                    "rating=\"%f\" ", playlist_data->tracknum,
                    playlist_data->year, playlist_data->rating);
                g_string_append(data_str, tmp);
                g_free(tmp);
                if(playlist_data->lyricfile!=NULL)
                {
                    tmp = g_markup_printf_escaped("lyricfile=\"%s\" ",    
                        playlist_data->lyricfile);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                if(playlist_data->albumfile!=NULL)
                {
                    tmp = g_markup_printf_escaped("albumfile=\"%s\" ",
                        playlist_data->albumfile);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                if(playlist_data->lyricsecfile!=NULL)
                {
                    tmp = g_markup_printf_escaped("lyricsecondfile=\"%s\" ",
                        playlist_data->lyricsecfile);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                if(playlist_data->genre!=NULL)
                {
                    tmp = g_markup_printf_escaped("genre=\"%s\" ",
                        playlist_data->genre);
                    g_string_append(data_str, tmp);
                    g_free(tmp);
                }
                g_string_append(data_str, "/>\n");
            }    
            g_string_append(data_str, "  </playlist>\n");
        }
    }
    if(library!=NULL)
    {
        g_string_append(data_str, "  <library>\n");
        g_hash_table_iter_init(&library_iter, library);
        while(g_hash_table_iter_next(&library_iter, NULL, (gpointer *)
            &library_data))
        {
            if(library_data==NULL) continue;
            g_string_append_printf(data_str, "    <libitem type=\"%u\" ",
                library_data->type);
            if(library_data->uri!=NULL)
            {
                tmp = g_markup_printf_escaped("uri=\"%s\" ",
                    library_data->uri);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(library_data->title!=NULL)
            {
                tmp = g_markup_printf_escaped("title=\"%s\" ",
                    library_data->title);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(library_data->artist!=NULL)
            {
                tmp = g_markup_printf_escaped("artist=\"%s\" ",
                    library_data->artist);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(library_data->album!=NULL)
            {
                tmp = g_markup_printf_escaped("album=\"%s\" ",
                    library_data->album);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(library_data->ftype!=NULL)
            {
                tmp = g_markup_printf_escaped("filetype=\"%s\" ",
                    library_data->ftype);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            tmp = g_markup_printf_escaped("length=\"%"G_GINT64_FORMAT"\" ",
                library_data->length);
            g_string_append(data_str, tmp);
            g_free(tmp);
            tmp = g_markup_printf_escaped("tracknum=\"%d\" year=\"%d\" "
                "rating=\"%f\" ", library_data->tracknum,
                library_data->year, library_data->rating);
            g_string_append(data_str, tmp);
            g_free(tmp);
            if(library_data->lyricfile!=NULL)
            {
                tmp = g_markup_printf_escaped("lyricfile=\"%s\" ",    
                    library_data->lyricfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(library_data->albumfile!=NULL)
            {
                tmp = g_markup_printf_escaped("albumfile=\"%s\" ",
                    library_data->albumfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(library_data->lyricsecfile!=NULL)
            {
                tmp = g_markup_printf_escaped("lyricsecondfile=\"%s\" ",
                    library_data->lyricsecfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(library_data->genre!=NULL)
            {
                tmp = g_markup_printf_escaped("genre=\"%s\" ",
                    library_data->genre);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            g_string_append(data_str, "/>\n");
        }
        g_string_append(data_str, "  </library>\n");
    }
    g_string_append(data_str, "</rclibdb>\n");
    return data_str;
}

static gboolean rclib_db_save_library_db(RCLibDbCatalogSequence *catalog, 
    GHashTable *library, const gchar *file, gboolean *dirty_flag)
{
    GString *xml_str;
    GZlibCompressor *compressor;
    GFileOutputStream *file_ostream;
    GOutputStream *compress_ostream;
    GFile *output_file;
    gssize write_size;
    GError *error = NULL;
    gboolean flag = TRUE;
    output_file = g_file_new_for_path(file);
    if(output_file==NULL) return FALSE;
    xml_str = rclib_db_build_xml_data(catalog, library);
    if(xml_str==NULL) return FALSE;
    file_ostream = g_file_replace(output_file, NULL, FALSE,
        G_FILE_CREATE_PRIVATE, NULL, &error);
    g_object_unref(output_file);
    if(file_ostream==NULL)
    {
        g_warning("Cannot open output file stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        return FALSE;
    }
    compressor = g_zlib_compressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 5);
    compress_ostream = g_converter_output_stream_new(G_OUTPUT_STREAM(
        file_ostream), G_CONVERTER(compressor));
    g_object_unref(file_ostream);
    g_object_unref(compressor);
    write_size = g_output_stream_write(compress_ostream, xml_str->str,
        xml_str->len, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Cannot save playlist: %s", error->message);
        g_error_free(error);
        error = NULL;
        flag = FALSE;
    }
    g_output_stream_close(compress_ostream, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Cannot close file stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        flag = FALSE;
    }
    g_object_unref(compress_ostream);
    if(flag)
    {
        g_message("Playlist saved, wrote %ld bytes.", (glong)write_size);
    }
    if(dirty_flag!=NULL) *dirty_flag = FALSE;  
    return flag;
}

static gpointer rclib_db_playlist_autosave_thread_cb(gpointer data)
{
    RCLibDbPrivate *priv = (RCLibDbPrivate *)data;
    gchar *filename;
    GZlibCompressor *compressor;
    GFileOutputStream *file_ostream;
    GOutputStream *compress_ostream;
    GFile *output_file;
    gssize write_size;
    GError *error = NULL;
    gboolean flag = TRUE;
    if(data==NULL)
    {
        g_thread_exit(NULL);
        return NULL;
    }
    while(priv->work_flag)
    {
        g_mutex_lock(&(priv->autosave_mutex));
        g_cond_wait(&(priv->autosave_cond), &(priv->autosave_mutex));
        G_STMT_START
        {
            if(priv->autosave_xml_data==NULL) break;
            filename = g_strdup_printf("%s.autosave", priv->filename);
            output_file = g_file_new_for_path(filename);
            g_free(filename);
            if(output_file==NULL) break;
            file_ostream = g_file_replace(output_file, NULL, FALSE,
                G_FILE_CREATE_PRIVATE, NULL, NULL);
            g_object_unref(output_file);
            if(file_ostream==NULL) break;
            compressor = g_zlib_compressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB,
                5);
            compress_ostream = g_converter_output_stream_new(G_OUTPUT_STREAM(
                file_ostream), G_CONVERTER(compressor));
            g_object_unref(file_ostream);
            g_object_unref(compressor);
            write_size = g_output_stream_write(compress_ostream,
                priv->autosave_xml_data->str, priv->autosave_xml_data->len,
                NULL, &error);
            if(error!=NULL)
            {
                g_error_free(error);
                error = NULL;
                flag = FALSE;
            }
            g_output_stream_close(compress_ostream, NULL, &error);
            if(error!=NULL)
            {
                g_error_free(error);
                error = NULL;
                flag = FALSE;
            }
            g_object_unref(compress_ostream);
            if(flag)
            {
                g_message("Auto save successfully, wrote %ld bytes.",
                    (glong)write_size);
            }
        }
        G_STMT_END;
        if(priv->autosave_xml_data!=NULL)
            g_string_free(priv->autosave_xml_data, TRUE);
        priv->autosave_xml_data = NULL;
        priv->dirty_flag = FALSE;
        g_mutex_unlock(&(priv->autosave_mutex));
    }
    g_thread_exit(NULL);
    return NULL;
}

static gboolean rclib_db_autosave_timeout_cb(gpointer data)
{
    RCLibDbPrivate *priv = (RCLibDbPrivate *)data;
    if(data==NULL) return FALSE;
    if(!priv->dirty_flag) return TRUE;
    if(priv->filename==NULL) return TRUE;
    if(priv->autosave_xml_data!=NULL) return TRUE;
    priv->autosave_xml_data = rclib_db_build_xml_data(priv->catalog,
        priv->library_table);
    g_mutex_lock(&(priv->autosave_mutex));
    g_cond_signal(&(priv->autosave_cond));
    g_mutex_unlock(&(priv->autosave_mutex));
    return TRUE;
}

static void rclib_db_finalize(GObject *object)
{
    RCLibDbImportData *import_data;
    RCLibDbRefreshData *refresh_data;
    gchar *autosave_file;
    RCLibDbPrivate *priv = RCLIB_DB(object)->priv;
    priv->work_flag = FALSE;
    g_mutex_lock(&(priv->autosave_mutex));
    g_cond_signal(&(priv->autosave_cond));
    g_mutex_unlock(&(priv->autosave_mutex));
    g_source_remove(priv->autosave_timeout);
    g_thread_join(priv->autosave_thread);
    g_mutex_clear(&(priv->autosave_mutex));
    g_cond_clear(&(priv->autosave_cond));
    rclib_db_import_cancel();
    rclib_db_refresh_cancel();
    import_data = g_new0(RCLibDbImportData, 1);
    refresh_data = g_new0(RCLibDbRefreshData, 1);
    g_async_queue_push(priv->import_queue, import_data);
    g_async_queue_push(priv->refresh_queue, refresh_data);
    g_thread_join(priv->import_thread);
    g_thread_join(priv->refresh_thread);
    rclib_db_save_library_db(priv->catalog, priv->library_table,
        priv->filename, &(priv->dirty_flag));
    autosave_file = g_strdup_printf("%s.autosave", priv->filename);
    g_remove(autosave_file);
    g_free(autosave_file);
    g_free(priv->filename);
    g_async_queue_unref(priv->import_queue);
    _rclib_db_instance_finalize_playlist(priv);
    _rclib_db_instance_finalize_library(priv);
    priv->import_queue = NULL;
    priv->refresh_queue = NULL;
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
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
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        refresh_updated), NULL, NULL, g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT, NULL);
        
    /**
     * RCLibDb::library-added:
     * @db: the #RCLibDb that received the signal
     * @uri: the new URI added to the music library
     * 
     * The ::library-added signal is emitted when a new music item is added
     * to the music library.
     */
    db_signals[SIGNAL_LIBRARY_ADDED] = g_signal_new("library-added",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        library_added), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
        
    /**
     * RCLibDb::library-changed:
     * @db: the #RCLibDb that received the signal
     * @uri: the URI of the music item that changed in the music library
     * 
     * The ::library-added signal is emitted when the data of the music item
     * in the music library is changed.
     */
    db_signals[SIGNAL_LIBRARY_CHANGED] = g_signal_new("library-changed",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        library_changed), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
        
    /**
     * RCLibDb::library-delete:
     * @db: the #RCLibDb that received the signal
     * @uri: the URI of the music item which is about to be deleted
     * 
     * The ::library-added signal is emitted when the data of the music item
     * in the music library is changed.
     */
    db_signals[SIGNAL_LIBRARY_DELETE] = g_signal_new("library-delete",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        library_delete), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
}

static void rclib_db_instance_init(RCLibDb *db)
{
    RCLibDbPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE(db, RCLIB_TYPE_DB,
        RCLibDbPrivate);
    db->priv = priv;
    priv->work_flag = TRUE;
    _rclib_db_instance_init_playlist(db, priv);
    _rclib_db_instance_init_library(db, priv);
    priv->import_queue = g_async_queue_new_full((GDestroyNotify)
        rclib_db_import_data_free);
    priv->refresh_queue = g_async_queue_new_full((GDestroyNotify)
        rclib_db_refresh_data_free);
    g_mutex_init(&(priv->autosave_mutex));
    g_cond_init(&(priv->autosave_cond));
    priv->import_thread = g_thread_new("RC2-Import-Thread",
        rclib_db_playlist_import_thread_cb, db);
    priv->refresh_thread = g_thread_new("RC2-Refresh-Thread",
        rclib_db_playlist_refresh_thread_cb, db);
    priv->autosave_thread = g_thread_new("RC2-Autosave-Thread",
        rclib_db_playlist_autosave_thread_cb, priv);
    priv->import_work_flag = FALSE;
    priv->refresh_work_flag = FALSE;
    priv->dirty_flag = FALSE;
    priv->autosave_timeout = g_timeout_add_seconds(db_autosave_timeout,
        rclib_db_autosave_timeout_cb, priv);
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

GType rclib_db_catalog_data_get_type(void)
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibDbCatalogData"),
            (GBoxedCopyFunc)rclib_db_catalog_data_ref,
            (GBoxedFreeFunc)rclib_db_catalog_data_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rclib_db_playlist_data_get_type(void)
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibDbPlaylistData"),
            (GBoxedCopyFunc)rclib_db_playlist_data_ref,
            (GBoxedFreeFunc)rclib_db_playlist_data_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rclib_db_library_data_get_type(void)
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibDbLibraryData"),
            (GBoxedCopyFunc)rclib_db_library_data_ref,
            (GBoxedFreeFunc)rclib_db_library_data_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
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
    db_instance = g_object_new(RCLIB_TYPE_DB, NULL);
    priv = RCLIB_DB(db_instance)->priv;
    if(priv->catalog==NULL || priv->import_queue==NULL ||
        priv->import_thread==NULL)
    {
        g_object_unref(db_instance);
        db_instance = NULL;
        g_warning("Failed to load database!");
        return FALSE;
    }
    rclib_db_load_library_db(priv->catalog, priv->catalog_iter_table,
        priv->playlist_iter_table, priv->library_table, file,
        &(priv->dirty_flag));
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
 * Returns: (transfer none): The running instance.
 */

GObject *rclib_db_get_instance()
{
    return db_instance;
}

/**
 * rclib_db_signal_connect:
 * @name: the name of the signal
 * @callback: (scope call): the the #GCallback to connect
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
 * rclib_db_import_cancel:
 *
 * Cancel all remaining import jobs in the queue.
 */

void rclib_db_import_cancel()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbImportData *import_data;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->import_queue==NULL) return;
    while(g_async_queue_length(priv->import_queue)>=0)
    {
        import_data = g_async_queue_try_pop(priv->import_queue);
        if(import_data!=NULL)
            rclib_db_import_data_free(import_data);
    }
}

/**
 * rclib_db_refresh_cancel:
 *
 * Cancel all remaining refresh jobs in the queue.
 */

void rclib_db_refresh_cancel()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbRefreshData *refresh_data;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->refresh_queue==NULL) return;
    while(g_async_queue_length(priv->refresh_queue)>=0)
    {
        refresh_data = g_async_queue_try_pop(priv->refresh_queue);
        if(refresh_data!=NULL)
            rclib_db_refresh_data_free(refresh_data);
    }
}

/**
 * rclib_db_import_queue_get_length:
 *
 * Get the number of remaining jobs in the import queue.
 *
 * Returns: The number of remaining jobs.
 */

gint rclib_db_import_queue_get_length()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->import_queue==NULL) return -1;
    return g_async_queue_length(priv->refresh_queue);
}

/**
 * rclib_db_refresh_queue_get_length:
 *
 * Get the number of remaining jobs in the refresh queue.
 *
 * Returns: The number of remaining jobs.
 */

gint rclib_db_refresh_queue_get_length()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
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
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return FALSE;
    if(!priv->dirty_flag) return TRUE;
    return rclib_db_save_library_db(priv->catalog, priv->library_table,
        priv->filename, &(priv->dirty_flag));
}

/**
 * rclib_db_load_autosaved:
 *
 * Load all playlist data from auto-saved playlist.
 */

gboolean rclib_db_load_autosaved()
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *iter;
    gchar *filename;
    gboolean flag;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return FALSE;
    while(rclib_db_catalog_sequence_get_length(priv->catalog)>0)
    {
        iter = rclib_db_catalog_sequence_get_begin_iter(priv->catalog);
        rclib_db_catalog_delete(iter);
    }
    filename = g_strdup_printf("%s.autosave", priv->filename);
    flag = rclib_db_load_library_db(priv->catalog, priv->catalog_iter_table,
        priv->playlist_iter_table, priv->library_table, filename,
        &(priv->dirty_flag));
    g_free(filename);
    if(flag)
    {
        for(iter=rclib_db_catalog_sequence_get_begin_iter(priv->catalog);
            !rclib_db_catalog_iter_is_end(iter);
            iter=rclib_db_catalog_iter_next(iter))
        {
            g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_ADDED],
                0, iter);
        }
    }
    return flag;
}

/**
 * rclib_db_autosaved_exist:
 *
 * Whether auto-saved playlist file exists.
 *
 * Returns: The existence of auto-saved playlist file.
 */

gboolean rclib_db_autosaved_exist()
{
    RCLibDbPrivate *priv;
    gchar *filename;
    gboolean flag = FALSE;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return FALSE;
    filename = g_strdup_printf("%s.autosave", priv->filename);
    flag = g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
    g_free(filename);
    return flag;
}

/**
 * rclib_db_autosaved_remove:
 *
 * Remove the auto-saved playlist file.
 */

void rclib_db_autosaved_remove()
{
    RCLibDbPrivate *priv;
    gchar *filename;
    if(db_instance==NULL) return;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return;
    filename = g_strdup_printf("%s.autosave", priv->filename);
    g_remove(filename);
    g_free(filename);
}

