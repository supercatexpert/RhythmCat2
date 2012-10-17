/*
 * RhythmCat Library Music Database Module (Playlist Part.)
 * Manage the catalogs & playlists in the music database.
 *
 * rclib-db-playlist.c
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
#include "rclib-tag.h"
#include "rclib-cue.h"
#include "rclib-core.h"
#include "rclib-util.h"

struct _RCLibDbCatalogSequence
{
    gint dummy;
};

struct _RCLibDbPlaylistSequence
{
    gint dummy;
};

struct _RCLibDbCatalogIter
{
    gint dummy;
};

struct _RCLibDbPlaylistIter
{
    gint dummy;
};

static gint db_import_depth = 5;

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

gboolean _rclib_db_playlist_import_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbPlaylistImportIdleData *idle_data;
    RCLibTagMetadata *mmd;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    idle_data = (RCLibDbPlaylistImportIdleData *)data;
    if(idle_data->mmd==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(instance==NULL) return FALSE;
    mmd = idle_data->mmd;
    playlist_data = rclib_db_playlist_data_new();
    playlist_data->catalog = idle_data->catalog_iter;
    playlist_data->type = idle_data->type;
    playlist_data->uri = g_strdup(mmd->uri);
    playlist_data->title = g_strdup(mmd->title);
    playlist_data->artist = g_strdup(mmd->artist);
    playlist_data->album = g_strdup(mmd->album);
    playlist_data->ftype = g_strdup(mmd->ftype);
    playlist_data->genre = g_strdup(mmd->genre);
    playlist_data->length = mmd->length;
    playlist_data->tracknum = mmd->tracknum;
    playlist_data->year = mmd->year;
    playlist_data->rating = 3.0;
    _rclib_db_playlist_append_data_internal(idle_data->catalog_iter,
        idle_data->playlist_insert_iter, playlist_data);
    priv->dirty_flag = TRUE;
    rclib_db_playlist_import_idle_data_free(idle_data);
    return FALSE;
}

gboolean _rclib_db_playlist_refresh_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistRefreshIdleData *idle_data;
    RCLibTagMetadata *mmd;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    idle_data = (RCLibDbPlaylistRefreshIdleData *)data;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(instance==NULL) return FALSE;
    if(!rclib_db_catalog_is_valid_iter(idle_data->catalog_iter))
    {
        return FALSE;
    }
    if(!rclib_db_playlist_is_valid_iter(idle_data->playlist_iter))
    {
        return FALSE;
    }
    mmd = idle_data->mmd;
    rclib_db_playlist_data_iter_set(idle_data->playlist_iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE, idle_data->type,        
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    if(mmd!=NULL)
    {
        rclib_db_playlist_data_iter_set(idle_data->playlist_iter,
            RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE, mmd->title,
            RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST, mmd->artist,
            RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM, mmd->album,
            RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE, mmd->ftype,
            RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE, mmd->genre,
            RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH, mmd->length,
            RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM, mmd->tracknum,
            RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR, mmd->year,
            RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    }
    priv->dirty_flag = TRUE;
    rclib_db_playlist_refresh_idle_data_free(idle_data);
    return FALSE;
}

gboolean _rclib_db_instance_init_playlist(RCLibDb *db, RCLibDbPrivate *priv)
{
    if(db==NULL || priv==NULL) return FALSE;
    
    g_rw_lock_init(&(priv->catalog_rw_lock));
    g_rw_lock_init(&(priv->playlist_rw_lock));
    
    /* GHashTable<RCLibDbCatalogIter *, RCLibDbCatalogIter *> */
    priv->catalog_iter_table = g_hash_table_new_full(g_direct_hash,
        g_direct_equal, NULL, NULL);
        
    /* GHashTable<RCLibDbPlaylistIter *, RCLibDbPlaylistIter *> */
    priv->playlist_iter_table = g_hash_table_new_full(g_direct_hash,
        g_direct_equal, NULL, NULL);
        
    /* RCLibDbCatalogSequence<RCLibDbCatalogData *> */
    priv->catalog = (RCLibDbCatalogSequence *)g_sequence_new((GDestroyNotify)
        rclib_db_catalog_data_unref);
        
    /* RCLibDbPlaylistSequence<RCLibDbPlaylistData *> */
    
    return TRUE;
}

void _rclib_db_instance_finalize_playlist(RCLibDbPrivate *priv)
{
    if(priv==NULL) return;
    g_rw_lock_writer_lock(&(priv->catalog_rw_lock));
    g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
    if(priv->catalog!=NULL)
    {
        g_sequence_free((GSequence *)priv->catalog);
    }
    if(priv->catalog_iter_table!=NULL)
        g_hash_table_destroy(priv->catalog_iter_table);
    if(priv->playlist_iter_table!=NULL)
        g_hash_table_destroy(priv->playlist_iter_table);
    priv->catalog = NULL;
    priv->catalog_iter_table = NULL;
    priv->playlist_iter_table = NULL;
    g_rw_lock_writer_unlock(&(priv->catalog_rw_lock));
    g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
    g_rw_lock_clear(&(priv->catalog_rw_lock));
    g_rw_lock_clear(&(priv->playlist_rw_lock));
}

/**
 * rclib_db_catalog_data_new:
 * 
 * Create a new empty #RCLibDbCatalogData structure,
 * and set the reference count to 1. MT safe.
 *
 * Returns: The new empty allocated #RCLibDbCatalogData structure.
 */

RCLibDbCatalogData *rclib_db_catalog_data_new()
{
    RCLibDbCatalogData *data = g_slice_new0(RCLibDbCatalogData);
    g_rw_lock_init(&(data->lock));
    data->ref_count = 1;
    return data;
}

/**
 * rclib_db_catalog_data_ref:
 * @data: the #RCLibDbCatalogData structure
 *
 * Increase the reference of #RCLibDbCatalogData by 1. MT safe.
 *
 * Returns: (transfer none): The #RCLibDbCatalogData structure.
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
 * If the reference down to zero, the structure will be freed. MT safe.
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
 * Please use #rclib_db_catalog_data_unref() in case of multi-threading.
 */

void rclib_db_catalog_data_free(RCLibDbCatalogData *data)
{
    if(data==NULL) return;
    g_rw_lock_writer_lock(&(data->lock));
    g_free(data->name);
    if(data->playlist!=NULL)
        g_sequence_free((GSequence *)data->playlist);
    g_rw_lock_writer_unlock(&(data->lock));
    g_rw_lock_clear(&(data->lock));
    g_slice_free(RCLibDbCatalogData, data);
}

static inline gboolean rclib_db_catalog_data_set_valist(
    RCLibDbCatalogData *data, RCLibDbCatalogDataType type1,
    va_list var_args)
{
    gboolean send_signal = FALSE;
    RCLibDbCatalogDataType type;
    RCLibDbPlaylistSequence *sequence;
    RCLibDbCatalogIter *iter;
    RCLibDbCatalogType new_type;
    gpointer ptr;
    const gchar *str;    
    type = type1;
    g_rw_lock_writer_lock(&(data->lock));
    while(type!=RCLIB_DB_CATALOG_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST:
            {
                sequence = va_arg(var_args, RCLibDbPlaylistSequence *);
                data->playlist = sequence;
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_SELF_ITER:
            {
                iter = va_arg(var_args, RCLibDbCatalogIter *);
                data->self_iter = iter;
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_NAME:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(data->name, str)==0) break;
                if(data->name!=NULL) g_free(data->name);
                data->name = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_TYPE:
            {
                new_type = va_arg(var_args, RCLibDbCatalogType);
                if(new_type==data->type) break;
                data->type = new_type;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_STORE:
            {
                ptr = va_arg(var_args, gpointer);
                data->store = ptr;
                break;
            }
            default:
            {
                g_warning("rclib_db_catalog_data_set: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbCatalogDataType);
    }
    g_rw_lock_writer_unlock(&(data->lock));
    return send_signal;
}

static inline void rclib_db_catalog_data_get_valist(
    RCLibDbCatalogData *data, RCLibDbCatalogDataType type1,
    va_list var_args)
{
    RCLibDbCatalogDataType type;
    RCLibDbPlaylistSequence **sequence;
    RCLibDbCatalogIter **iter;
    RCLibDbCatalogType *catalog_type;
    gpointer *ptr;
    gchar **str;    
    type = type1;
    g_rw_lock_reader_lock(&(data->lock));
    while(type!=RCLIB_DB_CATALOG_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST:
            {
                sequence = va_arg(var_args, RCLibDbPlaylistSequence **);
                *sequence = data->playlist;
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_SELF_ITER:
            {
                iter = va_arg(var_args, RCLibDbCatalogIter **);
                *iter = data->self_iter;
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_NAME:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->name);
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_TYPE:
            {
                catalog_type = va_arg(var_args, RCLibDbCatalogType *);
                *catalog_type = data->type;
                break;
            }
            case RCLIB_DB_CATALOG_DATA_TYPE_STORE:
            {
                ptr = va_arg(var_args, gpointer *);
                *ptr = data->store;
                break;
            }
            default:
            {
                g_warning("rclib_db_catalog_data_get: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbCatalogDataType);
    }
    g_rw_lock_reader_unlock(&(data->lock));
}

static inline gboolean rclib_db_playlist_data_set_valist(
    RCLibDbPlaylistData *data, RCLibDbPlaylistDataType type1,
    va_list var_args)
{
    gboolean send_signal = FALSE;
    RCLibDbPlaylistDataType type;
    RCLibDbCatalogIter *catalog_iter;
    RCLibDbPlaylistIter *playlist_iter;
    RCLibDbPlaylistType new_type;
    const gchar *str;   
    gint64 length;
    gint vint;
    gdouble rating;
    type = type1;
    g_rw_lock_writer_lock(&(data->lock));
    while(type!=RCLIB_DB_PLAYLIST_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG:
            {
                catalog_iter = va_arg(var_args, RCLibDbCatalogIter *);
                data->catalog = catalog_iter;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_SELF_ITER:
            {
                playlist_iter = va_arg(var_args, RCLibDbPlaylistIter *);
                data->self_iter = playlist_iter;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE:
            {
                new_type = va_arg(var_args, RCLibDbPlaylistType);
                if(new_type==data->type) break;
                data->type = new_type;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_URI:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->uri)==0) break;
                if(data->uri!=NULL) g_free(data->uri);
                data->uri = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->title)==0) break;
                if(data->title!=NULL) g_free(data->title);
                data->title = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->artist)==0) break;
                if(data->artist!=NULL) g_free(data->artist);
                data->artist = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->album)==0) break;
                if(data->album!=NULL) g_free(data->album);
                data->album = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->ftype)==0) break;
                if(data->ftype!=NULL) g_free(data->ftype);
                data->ftype = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH:
            {
                length = va_arg(var_args, gint64);
                if(data->length==length) break;
                data->length = length;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM:
            {
                vint = va_arg(var_args, gint);
                if(vint==data->tracknum) break;
                data->tracknum = vint;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR:
            {
                vint = va_arg(var_args, gint);
                if(vint==data->year) break;
                data->year = vint;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_RATING:
            {
                rating = va_arg(var_args, gdouble);
                if(rating==data->rating) break;
                data->rating = rating;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->lyricfile)==0) break;
                if(data->lyricfile!=NULL) g_free(data->lyricfile);
                str = va_arg(var_args, const gchar *);
                data->lyricfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->lyricsecfile)==0) break;
                if(data->lyricsecfile!=NULL) g_free(data->lyricsecfile);
                data->lyricsecfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUMFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->albumfile)==0) break;
                if(data->albumfile!=NULL) g_free(data->albumfile);
                data->albumfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->genre)==0) break;
                if(data->genre!=NULL) g_free(data->genre);
                data->genre = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            default:
            {
                g_warning("rclib_db_playlist_data_set: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbPlaylistDataType);
    }
    g_rw_lock_writer_unlock(&(data->lock));
    return send_signal;
}

static inline void rclib_db_playlist_data_get_valist(
    RCLibDbPlaylistData *data, RCLibDbPlaylistDataType type1,
    va_list var_args)
{
    RCLibDbPlaylistDataType type;
    RCLibDbCatalogIter **catalog_iter;
    RCLibDbPlaylistIter **playlist_iter;
    RCLibDbPlaylistType *playlist_type;
    gchar **str;   
    gint64 *length;
    gint *vint;
    gfloat *rating;
    type = type1;
    g_rw_lock_reader_lock(&(data->lock));
    while(type!=RCLIB_DB_PLAYLIST_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG:
            {
                catalog_iter = va_arg(var_args, RCLibDbCatalogIter **);
                *catalog_iter = data->catalog;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_SELF_ITER:
            {
                playlist_iter = va_arg(var_args, RCLibDbPlaylistIter **);
                *playlist_iter = data->self_iter;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE:
            {
                playlist_type = va_arg(var_args, RCLibDbPlaylistType *);
                *playlist_type = data->type;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_URI:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->uri);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->title);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->artist);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->album);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->ftype);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH:
            {
                length = va_arg(var_args, gint64 *);
                *length = data->length;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM:
            {
                vint = va_arg(var_args, gint *);
                *vint = data->tracknum;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR:
            {
                vint = va_arg(var_args, gint *);
                *vint = data->year;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_RATING:
            {
                rating = va_arg(var_args, gfloat *);
                *rating = data->rating;
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->lyricfile);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->lyricsecfile);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUMFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->albumfile);
                break;
            }
            case RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->genre);
                break;
            }            
            default:
            {
                g_warning("rclib_db_playlist_data_get: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbPlaylistDataType);
    }
    g_rw_lock_reader_unlock(&(data->lock));
}

/**
 * rclib_db_catalog_data_set: (skip)
 * @data: the #RCLibDbCatalogData data
 * @type1: the first property in catalog data to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_CATALOG_DATA_TYPE_NONE
 *
 * Sets properties on a #RCLibDbCatalogData. Must be called in main thread.
 */

void rclib_db_catalog_data_set(RCLibDbCatalogData *data,
    RCLibDbCatalogDataType type1, ...)
{
    RCLibDbPrivate *priv = NULL;
    GObject *instance = NULL;
    va_list var_args;
    gboolean send_signal = FALSE;
    if(data==NULL) return;
    instance = rclib_db_get_instance();
    if(instance!=NULL)
        priv = RCLIB_DB(instance)->priv;
    va_start(var_args, type1);
    send_signal = rclib_db_catalog_data_set_valist(data, type1, var_args);
    va_end(var_args);
    if(send_signal)
    {
        if(data->self_iter!=NULL)
        {
            if(priv!=NULL)
                priv->dirty_flag = TRUE;
            g_signal_emit_by_name(instance, "catalog-changed",
                data->self_iter);
        }
    }
}

/**
 * rclib_db_catalog_data_get: (skip)
 * @data: the #RCLibDbCatalogData data
 * @type1: the first property in catalog data to get
 * @...: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %RCLIB_DB_CATALOG_DATA_TYPE_NONE
 *
 * Gets properties of a #RCLibDbCatalogData. The property contents will
 * be copied (except the playlist and the store pointer). MT safe.
 */

void rclib_db_catalog_data_get(RCLibDbCatalogData *data,
    RCLibDbCatalogDataType type1, ...)
{
    va_list var_args;
    if(data==NULL) return;
    va_start(var_args, type1);
    rclib_db_catalog_data_get_valist(data, type1, var_args);
    va_end(var_args);
}

/**
 * rclib_db_catalog_data_iter_set: (skip)
 * @iter: the #RCLibDbCatalogIter iter
 * @type1: the first property in catalog data to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_CATALOG_DATA_TYPE_NONE
 *
 * Sets properties on the data in a #RCLibDbCatalogIter. Must be called
 * in main thread.
 */

void rclib_db_catalog_data_iter_set(RCLibDbCatalogIter *iter,
    RCLibDbCatalogDataType type1, ...)
{
    RCLibDbPrivate *priv = NULL;
    GObject *instance = NULL;
    RCLibDbCatalogData *data;
    va_list var_args;
    gboolean send_signal = FALSE;
    if(iter==NULL) return;
    if(!rclib_db_catalog_is_valid_iter(iter)) return;
    instance = rclib_db_get_instance();
    if(instance!=NULL)
        priv = RCLIB_DB(instance)->priv;
    data = rclib_db_catalog_iter_get_data(iter);
    if(data==NULL)
    {
        return;
    }
    va_start(var_args, type1);
    send_signal = rclib_db_catalog_data_set_valist(data, type1, var_args);
    va_end(var_args);
    rclib_db_catalog_data_unref(data);
    if(send_signal)
    {
        if(priv!=NULL)
            priv->dirty_flag = TRUE;
        g_signal_emit_by_name(instance, "catalog-changed", iter);
    }
}

/**
 * rclib_db_catalog_data_iter_get: (skip)
 * @iter: the #RCLibDbCatalogIter iter
 * @type1: the first property in catalog data to get
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_CATALOG_DATA_TYPE_NONE
 *
 * Gets properties of the data in a #RCLibDbCatalogIter. The property
 * contents will be copied, except the playlist and store pointer.
 * MT safe.
 */

void rclib_db_catalog_data_iter_get(RCLibDbCatalogIter *iter,
    RCLibDbCatalogDataType type1, ...)
{
    RCLibDbCatalogData *data;
    va_list var_args;
    if(iter==NULL) return;
    if(!rclib_db_catalog_is_valid_iter(iter)) return;
    data = rclib_db_catalog_iter_get_data(iter);
    if(data==NULL)
        return;
    va_start(var_args, type1);
    rclib_db_catalog_data_get_valist(data, type1, var_args);
    va_end(var_args);
    rclib_db_catalog_data_unref(data);
}

/**
 * rclib_db_playlist_data_new:
 * 
 * Create a new empty #RCLibDbPlaylistData structure,
 * and set the reference count to 1. MT safe.
 *
 * Returns: The new empty allocated #RCLibDbPlaylistData structure.
 */

RCLibDbPlaylistData *rclib_db_playlist_data_new()
{
    RCLibDbPlaylistData *data = g_slice_new0(RCLibDbPlaylistData);
    g_rw_lock_init(&(data->lock));
    data->ref_count = 1;
    return data;
}

/**
 * rclib_db_playlist_data_ref:
 * @data: the #RCLibDbPlaylistData structure
 *
 * Increase the reference of #RCLibDbPlaylistData by 1. MT safe.
 *
 * Returns: (transfer none): The #RCLibDbPlaylistData structure. 
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
 * If the reference down to zero, the structure will be freed. MT safe.
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
 * Please use #rclib_db_playlist_data_unref() in case of multi-threading.
 */

void rclib_db_playlist_data_free(RCLibDbPlaylistData *data)
{
    if(data==NULL) return;
    g_rw_lock_writer_lock(&(data->lock));
    g_free(data->uri);
    data->uri = NULL;
    g_free(data->title);
    data->title = NULL;
    g_free(data->artist);
    data->artist = NULL;
    g_free(data->album);
    data->album = NULL;
    g_free(data->ftype);
    data->ftype = NULL;
    g_free(data->lyricfile);
    data->lyricfile = NULL;
    g_free(data->lyricsecfile);
    data->lyricsecfile = NULL;
    g_free(data->albumfile);
    data->albumfile = NULL;
    g_free(data->genre);
    data->genre = NULL;
    g_rw_lock_writer_unlock(&(data->lock));
    g_rw_lock_clear(&(data->lock));
    g_slice_free(RCLibDbPlaylistData, data);
}

/**
 * rclib_db_playlist_data_set: (skip)
 * @data: the #RCLibDbPlaylistData data
 * @type1: the first property in playlist data to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_PLAYLIST_DATA_TYPE_NONE
 *
 * Sets properties on a #RCLibDbPlaylistData. Must be called in main thread.
 */

void rclib_db_playlist_data_set(RCLibDbPlaylistData *data,
    RCLibDbPlaylistDataType type1, ...)
{
    RCLibDbPrivate *priv = NULL;
    GObject *instance = NULL;
    va_list var_args;
    gboolean send_signal = FALSE;
    if(data==NULL) return;
    va_start(var_args, type1);
    send_signal = rclib_db_playlist_data_set_valist(data, type1, var_args);
    va_end(var_args);
    if(send_signal)
    {
        instance = rclib_db_get_instance();
        if(instance!=NULL)
            priv = RCLIB_DB(instance)->priv;
        if(data->self_iter!=NULL)
        {
            if(priv!=NULL)
                priv->dirty_flag = TRUE;
            g_signal_emit_by_name(instance, "playlist-changed",
                data->self_iter);
        }
    }
}

/**
 * rclib_db_playlist_data_get: (skip)
 * @data: the #RCLibDbPlaylistData data
 * @type1: the first property in playlist data to get
 * @...: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %RCLIB_DB_PLAYLIST_DATA_TYPE_NONE
 *
 * Gets properties of a #RCLibDbPlaylistData. The property contents will
 * be copied, except the catalog and iters. MT safe.
 */

void rclib_db_playlist_data_get(RCLibDbPlaylistData *data,
    RCLibDbPlaylistDataType type1, ...)
{
    va_list var_args;
    if(data==NULL) return;
    va_start(var_args, type1);
    rclib_db_playlist_data_get_valist(data, type1, var_args);
    va_end(var_args);
}

/**
 * rclib_db_playlist_data_iter_set: (skip)
 * @iter: the #RCLibDbPlaylistIter iter
 * @type1: the first property in playlist data to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_PLAYLIST_DATA_TYPE_NONE
 *
 * Sets properties on the data in a #RCLibDbPlaylistIter. Must be called
 * in main thread.
 */

void rclib_db_playlist_data_iter_set(RCLibDbPlaylistIter *iter,
    RCLibDbPlaylistDataType type1, ...)
{
    RCLibDbPrivate *priv = NULL;
    GObject *instance = NULL;
    RCLibDbPlaylistData *data;
    va_list var_args;
    gboolean send_signal = FALSE;
    if(iter==NULL) return;
    data = rclib_db_playlist_iter_get_data(iter);
    if(data==NULL) return;
    va_start(var_args, type1);
    send_signal = rclib_db_playlist_data_set_valist(data, type1, var_args);
    va_end(var_args);
    rclib_db_playlist_data_unref(data);
    if(send_signal)
    {
        instance = rclib_db_get_instance();
        if(instance!=NULL)
            priv = RCLIB_DB(instance)->priv;
        if(priv!=NULL)
            priv->dirty_flag = TRUE;
        g_signal_emit_by_name(instance, "playlist-changed", iter);
    }
}

/**
 * rclib_db_playlist_data_iter_get: (skip)
 * @iter: the #RCLibDbPlaylistIter iter
 * @type1: the first property in playlist data to get
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_PLAYLIST_DATA_TYPE_NONE
 *
 * Gets properties of the data in a #RCLibDbPlaylistIter. The property
 * contents will be copied, except the catalog and iters. MT safe.
 */

void rclib_db_playlist_data_iter_get(RCLibDbPlaylistIter *iter,
    RCLibDbPlaylistDataType type1, ...)
{
    RCLibDbPlaylistData *data;
    va_list var_args;
    if(iter==NULL) return;
    data = rclib_db_playlist_iter_get_data(iter);
    if(data==NULL) return;
    va_start(var_args, type1);
    rclib_db_playlist_data_get_valist(data, type1, var_args);
    va_end(var_args);
    rclib_db_playlist_data_unref(data);
}

/**
 * rclib_db_catalog_get_length:
 *
 * Get the element length of the playlist catalog. MT safe.
 *
 * Returns: The length of catalog, -1 if any error occurs.
 */

gint rclib_db_catalog_get_length()
{
    RCLibDbPrivate *priv;
    gint length = 0;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return -1;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        length = g_sequence_get_length((GSequence *)priv->catalog);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return length;
}

/**
 * rclib_db_catalog_get_begin_iter:
 *
 * Get the begin iterator for the playlist catalog. MT safe.
 *
 * Returns: (transfer none): (skip): The begin iterator.
 */

RCLibDbCatalogIter *rclib_db_catalog_get_begin_iter()
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_get_begin_iter(
            (GSequence *)priv->catalog);
        if(g_sequence_iter_is_end((GSequenceIter *)catalog_iter))
            catalog_iter = NULL;
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return catalog_iter;
}

/**
 * rclib_db_catalog_get_last_iter:
 *
 * Get the last iterator for the playlist catalog. MT safe.
 *
 * Returns: (transfer none): (skip): The last iterator, #NULL if does
 *     not exist.
 */

RCLibDbCatalogIter *rclib_db_catalog_get_last_iter()
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_get_end_iter(
            (GSequence *)priv->catalog);
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_iter_prev(
            (GSequenceIter *)catalog_iter);
        if(g_sequence_iter_is_end((GSequenceIter *)catalog_iter))
            catalog_iter = NULL;
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return catalog_iter;
}

/**
 * rclib_db_catalog_get_end_iter:
 *
 * Get the end iterator for the playlist catalog. MT safe.
 *
 * Returns: (transfer none): (skip): The end iterator.
 */

RCLibDbCatalogIter *rclib_db_catalog_get_end_iter()
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_get_end_iter(
            (GSequence *)priv->catalog);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return catalog_iter;
}

/**
 * rclib_db_catalog_get_iter_at_pos:
 * @pos: the position in the playlist catalog, or -1 for the end.
 *
 * Return the iterator at position @pos. If @pos is negative or larger
 * than the number of items in the playlist catalog, the end iterator
 * is returned. MT safe.
 *
 * Returns: (transfer none): (skip): The #RCLibDbCatalogIter at
 *     position @pos.
 */

RCLibDbCatalogIter *rclib_db_catalog_get_iter_at_pos(gint pos)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_get_iter_at_pos(
            (GSequence *)priv->catalog, pos);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return catalog_iter;
}

/**
 * rclib_db_catalog_iter_get_data:
 * @iter: the iter pointed to the catalog data
 *
 * Get the catalog item data which the iter pointed to. MT safe.
 *
 * Returns: (transfer full): The #RCLibDbCatalogData.
 */

RCLibDbCatalogData *rclib_db_catalog_iter_get_data(
    RCLibDbCatalogIter *iter)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbCatalogData *catalog_data = NULL;
    if(iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->catalog_iter_table==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, iter)) break;
        catalog_data = (RCLibDbCatalogData *)g_sequence_get(
            (GSequenceIter *)iter);
        if(catalog_data==NULL) break;
        catalog_data = rclib_db_catalog_data_ref(catalog_data);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return catalog_data;
}

/**
 * rclib_db_catalog_iter_is_begin:
 * @iter: a #RCLibDbCatalogIter
 *
 * Return whether @iter is the begin iterator. MT safe.
 * Notice that this function will NOT check whether the iter is valid.
 *
 * Returns: whether @iter is the begin iterator
 */
 
gboolean rclib_db_catalog_iter_is_begin(RCLibDbCatalogIter *iter)
{
    RCLibDbPrivate *priv;
    gboolean result = FALSE;
    GObject *instance;
    if(iter==NULL) return TRUE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        result = g_sequence_iter_is_begin((GSequenceIter *)iter);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));        
    return result;
}

/**
 * rclib_db_catalog_iter_is_end:
 * @iter: a #RCLibDbCatalogIter
 *
 * Return whether @iter is the end iterator. MT safe.
 * Notice that this function will NOT check whether the iter is valid.
 *
 * Returns: whether @iter is the end iterator
 */

gboolean rclib_db_catalog_iter_is_end(RCLibDbCatalogIter *iter)
{
    RCLibDbPrivate *priv;
    gboolean result = FALSE;
    GObject *instance;
    if(iter==NULL) return TRUE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        result = g_sequence_iter_is_end((GSequenceIter *)iter);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));        
    return result;
}

/**
 * rclib_db_catalog_iter_next:
 * @iter: a #RCLibDbCatalogIter
 *
 * Return an iterator pointing to the next position after @iter. If
 * @iter is not valid, or @iter is the last iterator in the catalog,
 * #NULL is returned. MT safe.
 *
 * Returns: (transfer none): (skip): a #RCLibDbCatalogIter pointing to
 *     the next position after @iter.
 */

RCLibDbCatalogIter *rclib_db_catalog_iter_next(RCLibDbCatalogIter *iter)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    if(iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, iter))
            break;
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_iter_next(
            (GSequenceIter *)iter);
        if(g_sequence_iter_is_end((GSequenceIter *)catalog_iter))
            catalog_iter = NULL;
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));        
    return catalog_iter;
}

/**
 * rclib_db_catalog_iter_prev:
 * @iter: a #RCLibDbCatalogIter
 *
 * Return an iterator pointing to the previous position before @iter. If
 * @iter is not valid, or @iter is the first iterator in the catalog,
 * #NULL is returned. MT safe.
 *
 * Returns: (transfer none): (skip): a #RCLibDbCatalogIter pointing to the
 *     previous position before @iter.
 */
 
RCLibDbCatalogIter *rclib_db_catalog_iter_prev(RCLibDbCatalogIter *iter)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    if(iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, iter))
            break;
        if(g_sequence_iter_is_begin((GSequenceIter *)catalog_iter))
            break;
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_iter_prev(
            (GSequenceIter *)iter);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));        
    return catalog_iter;
}

/**
 * rclib_db_catalog_iter_get_position:
 * @iter: a #RCLibDbCatalogIter
 *
 * Returns the position of @iter. MT safe.
 *
 * Returns: the position of @iter, -1 if the @iter is not valid.
 */

gint rclib_db_catalog_iter_get_position(RCLibDbCatalogIter *iter)
{
    RCLibDbPrivate *priv;
    gint pos = 0;
    GObject *instance;
    if(iter==NULL) return -1;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->catalog_iter_table==NULL) return -1;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, iter))
        {
            pos = -1;
            break;
        }
        pos = g_sequence_iter_get_position((GSequenceIter *)iter);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));        
    return pos;
}

/**
 * rclib_db_catalog_iter_range_get_midpoint:
 * @begin: a #RCLibDbCatalogIter
 * @end: a #RCLibDbCatalogIter
 *
 * Find an iterator somewhere in the range (@begin, @end). This
 * iterator will be close to the middle of the range, but is not
 * guaranteed to be <emphasis>exactly</emphasis> in the middle.
 *
 * The @begin and @end iterators must both point to the same
 * #RCLibDbCatalogSequence and @begin must come before or be equal
 * to @end in the sequence. MT safe.
 *
 * Returns: (transfer none): (skip): A #RCLibDbCatalogIter pointing
 *     somewhere in the (@begin, @end) range.
 */

RCLibDbCatalogIter *rclib_db_catalog_iter_range_get_midpoint(
    RCLibDbCatalogIter *begin, RCLibDbCatalogIter *end)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    if(begin==NULL || end==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, begin))
            break;
        if(!g_hash_table_contains(priv->catalog_iter_table, end))
            break;
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_range_get_midpoint(
            (GSequenceIter *)begin, (GSequenceIter *)end);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return catalog_iter;
}

/**
 * rclib_db_catalog_iter_compare:
 * @a: a #RCLibDbCatalogIter
 * @b: a #RCLibDbCatalogIter
 *
 * Return a negative number if @a comes before @b, 0 if they are equal,
 * and a positive number if @a comes after @b. MT safe.
 *
 * The @a and @b iterators must point into the same #RCLibDbCatalogSequence.
 *
 * Returns: A negative number if @a comes before @b, 0 if they are
 *     equal, and a positive number if @a comes after @b.
 */

gint rclib_db_catalog_iter_compare(RCLibDbCatalogIter *a,
    RCLibDbCatalogIter *b)
{
    RCLibDbPrivate *priv;
    gint result = 0;
    GObject *instance;
    if(a==NULL || b==NULL) return 0;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return -1;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, a))
            break;
        if(!g_hash_table_contains(priv->catalog_iter_table, b))
            break;
        result = g_sequence_iter_compare((GSequenceIter *)a,
            (GSequenceIter *)b);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));        
    return result;
}

/**
 * rclib_db_playlist_get_length:
 * @catalog_iter: the #RCLibDbCatalogIter which stores the playlist
 *
 * Get the playlist length stored in the #RCLibDbCatalogIter. MT safe.
 *
 * Returns: The playlist length, -1 if the playlist does not exist.
 */

gint rclib_db_playlist_get_length(RCLibDbCatalogIter *catalog_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    gint length = 0;
    if(catalog_iter==NULL) return -1;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return -1;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, catalog_iter))
            break;
        rclib_db_catalog_data_iter_get(catalog_iter,
            RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        if(playlist==NULL)
        {
            length = -1;
            break;
        }
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        length = g_sequence_get_length((GSequence *)playlist);
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return length;
}

/**
 * rclib_db_playlist_iter_get_length:
 * @playlist_iter: the #RCLibDbPlaylistIter which the playlist belongs to
 *
 * Get the playlist length that the #RCLibDbPlaylistIter belongs to. MT safe.
 *
 * Returns: The playlist length, -1 if the playlist iter is not valid.
 */

gint rclib_db_playlist_iter_get_length(RCLibDbPlaylistIter *playlist_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    gint length = 0;
    if(playlist_iter==NULL) return -1;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return -1;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, playlist_iter))
        {
            length = -1;
            break;
        }
        playlist = (RCLibDbPlaylistSequence *)g_sequence_iter_get_sequence(
            (GSequenceIter *)playlist_iter);
        length = g_sequence_get_length((GSequence *)playlist);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return length;
}

/**
 * rclib_db_playlist_get_begin_iter:
 * @catalog_iter: the #RCLibDbCatalogIter which stores the playlist
 *
 * Get the begin iterator for the playlist stored in #RCLibDbCatalogIter.
 * MT safe.
 *
 * Returns: (transfer none): (skip): The begin iterator.
 */

RCLibDbPlaylistIter *rclib_db_playlist_get_begin_iter(
    RCLibDbCatalogIter *catalog_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    if(catalog_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, catalog_iter))
            break;
        rclib_db_catalog_data_iter_get(catalog_iter,
            RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        if(playlist==NULL)
            break;
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_get_begin_iter(
            (GSequence *)playlist);
        if(g_sequence_iter_is_end((GSequenceIter *)playlist_iter))
            playlist_iter = NULL;
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return playlist_iter;
}

/**
 * rclib_db_playlist_get_last_iter:
 * @catalog_iter: the #RCLibDbCatalogIter which stores the playlist
 *
 * Get the last iterator for the playlist stored in #RCLibDbCatalogIter.
 * MT safe.
 *
 * Returns: (transfer none): (skip): The last iterator.
 */

RCLibDbPlaylistIter *rclib_db_playlist_get_last_iter(
    RCLibDbCatalogIter *catalog_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    if(catalog_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, catalog_iter))
            break;
        rclib_db_catalog_data_iter_get(catalog_iter,
            RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        if(playlist==NULL)
            break;
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_get_end_iter(
            (GSequence *)playlist);
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_iter_prev(
            (GSequenceIter *)playlist_iter);
        if(g_sequence_iter_is_end((GSequenceIter *)playlist_iter))
            playlist_iter = NULL;
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return playlist_iter;
}

/**
 * rclib_db_playlist_get_end_iter:
 * @catalog_iter: the #RCLibDbCatalogIter which stores the playlist
 *
 * Get the end iterator for the playlist stored in #RCLibDbCatalogIter.
 * MT safe.
 *
 * Returns: (transfer none): (skip): The end iterator.
 */

RCLibDbPlaylistIter *rclib_db_playlist_get_end_iter(
    RCLibDbCatalogIter *catalog_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    if(catalog_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, catalog_iter))
            break;
        rclib_db_catalog_data_iter_get(catalog_iter,
            RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        if(playlist==NULL)
            break;
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_get_end_iter(
            (GSequence *)playlist);
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return playlist_iter;
}

/**
 * rclib_db_playlist_iter_get_begin_iter:
 * @playlist_iter: the #RCLibDbPlaylistIter which the playlist belongs to
 *
 * Get the begin iterator of the playlist that the #RCLibDbPlaylistIter
 * belongs to. MT safe.
 *
 * Returns: (transfer none): (skip): The begin iterator.
 */

RCLibDbPlaylistIter *rclib_db_playlist_iter_get_begin_iter(
    RCLibDbPlaylistIter *playlist_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *result_iter = NULL;
    if(playlist_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, playlist_iter))
            break;
        playlist = (RCLibDbPlaylistSequence *)g_sequence_iter_get_sequence(
            (GSequenceIter *)playlist_iter);
        result_iter = (RCLibDbPlaylistIter *)g_sequence_get_begin_iter(
            (GSequence *)playlist);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return result_iter; 
}

/**
 * rclib_db_playlist_iter_get_last_iter:
 * @playlist_iter: the #RCLibDbPlaylistIter which the playlist belongs to
 *
 * Get the last iterator of the playlist that the #RCLibDbPlaylistIter
 * belongs to. MT safe.
 *
 * Returns: (transfer none): (skip): The last iterator.
 */

RCLibDbPlaylistIter *rclib_db_playlist_iter_get_last_iter(
    RCLibDbPlaylistIter *playlist_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *result_iter = NULL;
    if(playlist_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, playlist_iter))
            break;
        playlist = (RCLibDbPlaylistSequence *)g_sequence_iter_get_sequence(
            (GSequenceIter *)playlist_iter);
        result_iter = (RCLibDbPlaylistIter *)g_sequence_get_end_iter(
            (GSequence *)playlist);
        result_iter = (RCLibDbPlaylistIter *)g_sequence_iter_prev(
            (GSequenceIter *)result_iter);
        if(g_sequence_iter_is_end((GSequenceIter *)result_iter))
            result_iter = NULL;
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return result_iter;
}

/**
 * rclib_db_playlist_iter_get_end_iter:
 * @playlist_iter: the #RCLibDbPlaylistIter which the playlist belongs to
 *
 * Get the end iterator of the playlist that the #RCLibDbPlaylistIter
 * belongs to. MT safe.
 *
 * Returns: (transfer none): (skip): The end iterator.
 */

RCLibDbPlaylistIter *rclib_db_playlist_iter_get_end_iter(
    RCLibDbPlaylistIter *playlist_iter)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *result_iter = NULL;
    if(playlist_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, playlist_iter))
            break;
        playlist = (RCLibDbPlaylistSequence *)g_sequence_iter_get_sequence(
            (GSequenceIter *)playlist_iter);
        result_iter = (RCLibDbPlaylistIter *)g_sequence_get_end_iter(
            (GSequence *)playlist);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return result_iter;
}

/**
 * rclib_db_playlist_get_iter_at_pos:
 * @catalog_iter: the #RCLibDbCatalogIter which stores the playlist
 * @pos: the position in the playlist, or -1 for the end.
 *
 * Return the iterator at position @pos. If @pos is negative or larger
 * than the number of items in the playlist, the end iterator is returned.
 * MT safe.
 *
 * Returns: (transfer none): (skip): The #RCLibDbPlaylistIter at
 *     position @pos.
 */

RCLibDbPlaylistIter *rclib_db_playlist_get_iter_at_pos(
    RCLibDbCatalogIter *catalog_iter, gint pos)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    if(catalog_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, catalog_iter))
            break;
        rclib_db_catalog_data_iter_get(catalog_iter,
            RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        if(playlist==NULL)
            break;
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_get_iter_at_pos(
            (GSequence *)playlist, pos);;
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return playlist_iter;
}

/**
 * rclib_db_playlist_iter_get_iter_at_pos:
 * @playlist_iter: the #RCLibDbPlaylistIter which the playlist belongs to
 * @pos: the position in the playlist, or -1 for the end.
 *
 * Return the iterator at position @pos. If @pos is negative or larger
 * than the number of items in the playlist, the end iterator is returned.
 * MT safe.
 *
 * Returns: (transfer none): (skip): The #RCLibDbPlaylistIter at
 *     position @pos.
 */

RCLibDbPlaylistIter *rclib_db_playlist_iter_get_iter_at_pos(
    RCLibDbPlaylistIter *playlist_iter, gint pos)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *result_iter = NULL;
    if(playlist_iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, playlist_iter))
            break;
        playlist = (RCLibDbPlaylistSequence *)g_sequence_iter_get_sequence(
            (GSequenceIter *)playlist_iter);
        result_iter = (RCLibDbPlaylistIter *)g_sequence_get_iter_at_pos(
            (GSequence *)playlist, pos);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return result_iter;
}

/**
 * rclib_db_playlist_iter_get_data:
 * @iter: the iter pointed to the playlist data
 *
 * Get the playlist item data which the iter pointed to. MT safe.
 *
 * Returns: (transfer full): The #RCLibDbPlaylistData.
 */

RCLibDbPlaylistData *rclib_db_playlist_iter_get_data(
    RCLibDbPlaylistIter *iter)
{
    RCLibDbPlaylistData *data = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    if(iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->playlist_iter_table==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, iter))
            break;
        data = (RCLibDbPlaylistData *)g_sequence_get((GSequenceIter *)iter);
        if(data==NULL) break;
        data = rclib_db_playlist_data_ref(data);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return data;
}

/**
 * rclib_db_playlist_iter_is_begin:
 * @iter: a #RCLibDbPlaylistIter
 *
 * Return whether @iter is the begin iterator. MT safe.
 * Notice that this function will NOT check whether the iter is valid.
 *
 * Returns: whether @iter is the begin iterator.
 */
 
gboolean rclib_db_playlist_iter_is_begin(RCLibDbPlaylistIter *iter)
{
    if(iter==NULL) return FALSE;
    return g_sequence_iter_is_begin((GSequenceIter *)iter);
}

/**
 * rclib_db_playlist_iter_is_end:
 * @iter: a #RCLibDbPlaylistIter
 *
 * Return whether @iter is the end iterator. MT safe.
 * Notice that this function will NOT check whether the iter is valid.
 *
 * Returns: whether @iter is the end iterator.
 */

gboolean rclib_db_playlist_iter_is_end(RCLibDbPlaylistIter *iter)
{
    if(iter==NULL) return TRUE;
    return g_sequence_iter_is_end((GSequenceIter *)iter);
}

/**
 * rclib_db_playlist_iter_next:
 * @iter: a #RCLibDbPlaylistIter
 *
 * Return an iterator pointing to the next position after @iter. If
 * @iter is not valid , or @iter the end iterator, #NULL is returned.
 * MT safe.
 *
 * Returns: (transfer none): (skip): a #RCLibDbPlaylistIter pointing to
 *     the next position after @iter.
 */

RCLibDbPlaylistIter *rclib_db_playlist_iter_next(RCLibDbPlaylistIter *iter)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    GObject *instance;
    if(iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->playlist_iter_table==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, iter))
            break;
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_iter_next(
            (GSequenceIter *)iter);
        if(g_sequence_iter_is_end((GSequenceIter *)playlist_iter))
            playlist_iter = NULL;
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));    
    return playlist_iter;
}

/**
 * rclib_db_playlist_iter_prev:
 * @iter: a #RCLibDbPlaylistIter
 *
 * Return an iterator pointing to the previous position before @iter. If
 * @iter is not valid , or @iter the begin iterator, #NULL is returned.
 * MT safe.
 *
 * Returns: (transfer none): (skip): a #RCLibDbPlaylistIter pointing to the
 *     previous position before @iter.
 */
 
RCLibDbPlaylistIter *rclib_db_playlist_iter_prev(RCLibDbPlaylistIter *iter)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    GObject *instance;
    if(iter==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->playlist_iter_table==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, iter))
            break;
        if(g_sequence_iter_is_begin((GSequenceIter *)iter))
            break;
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_iter_prev(
            (GSequenceIter *)iter);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));    
    return playlist_iter;
}

/**
 * rclib_db_playlist_iter_get_position:
 * @iter: a #RCLibDbPlaylistIter
 *
 * Return the position of @iter. MT safe.
 *
 * Returns: the position of @iter, -1 if the @iter is not valid.
 */

gint rclib_db_playlist_iter_get_position(RCLibDbPlaylistIter *iter)
{
    RCLibDbPrivate *priv;
    gint pos = 0;
    GObject *instance;
    if(iter==NULL) return -1;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->playlist_iter_table==NULL) return -1;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, iter))
        {
            pos = -1;
            break;
        }
        pos = g_sequence_iter_get_position((GSequenceIter *)iter);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));        
    return pos;
}

/**
 * rclib_db_playlist_iter_range_get_midpoint:
 * @begin: a #RCLibDbPlaylistIter
 * @end: a #RCLibDbPlaylistIter
 *
 * Find an iterator somewhere in the range (@begin, @end). This
 * iterator will be close to the middle of the range, but is not
 * guaranteed to be <emphasis>exactly</emphasis> in the middle. MT safe.
 *
 * The @begin and @end iterators must both point to the same
 * #RCLibDbPlaylistSequence and @begin must come before or be equal
 * to @end in the sequence.
 *
 * Returns: (transfer none): (skip): A #RCLibDbPlaylistIter pointing
 *     somewhere in the (@begin, @end) range.
 */

RCLibDbPlaylistIter *rclib_db_playlist_iter_range_get_midpoint(
    RCLibDbPlaylistIter *begin, RCLibDbPlaylistIter *end)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    GObject *instance;
    if(begin==NULL || end==NULL) return NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, begin))
            break;
        if(!g_hash_table_contains(priv->playlist_iter_table, end))
            break;
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_range_get_midpoint(
            (GSequenceIter *)begin, (GSequenceIter *)end);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return playlist_iter;
}

/**
 * rclib_db_playlist_iter_compare:
 * @a: a #RCLibDbPlaylistIter
 * @b: a #RCLibDbPlaylistIter
 *
 * Return a negative number if @a comes before @b, 0 if they are equal,
 * and a positive number if @a comes after @b. MT safe.
 *
 * The @a and @b iterators must point into the same #RCLibDbPlaylistSequence.
 *
 * Returns: A negative number if @a comes before @b, 0 if they are
 *     equal, and a positive number if @a comes after @b.
 */

gint rclib_db_playlist_iter_compare(RCLibDbPlaylistIter *a,
    RCLibDbPlaylistIter *b)
{
    RCLibDbPrivate *priv;
    gint result = 0;
    GObject *instance;
    if(a==NULL || b==NULL) return 0;
    instance = rclib_db_get_instance();
    if(instance==NULL) return -1;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return -1;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, a))
            break;
        if(!g_hash_table_contains(priv->playlist_iter_table, b))
            break;
        result = g_sequence_iter_compare((GSequenceIter *)a,
            (GSequenceIter *)b);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));        
    return result;
}

/**
 * rclib_db_catalog_foreach:
 * @func: (scope call): the function to call for each item in @seq
 * @user_data: user data passed to @func
 *
 * Calls @func for each item in the playlist catalog passing
 * @user_data to the function. MT safe.
 */

void rclib_db_catalog_foreach(GFunc func, gpointer user_data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    if(func==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->catalog_iter_table==NULL)
        return;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    g_sequence_foreach((GSequence *)priv->catalog, func, user_data);
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
} 

/**
 * rclib_db_catalog_iter_foreach_range:
 * @begin: a #RCLibDbCatalogIter
 * @end: a #RCLibDbCatalogIter
 * @func: (scope call): a #GFunc
 * @user_data: user data passed to @func
 *
 * Calls @func for each item in the range (@begin, @end) passing
 * @user_data to the function. MT safe.
 */

void rclib_db_catalog_iter_foreach_range(RCLibDbCatalogIter *begin,
    RCLibDbCatalogIter *end, GFunc func, gpointer user_data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    if(func==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->catalog_iter_table==NULL) return;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, begin)) break;
        if(!g_hash_table_contains(priv->catalog_iter_table, end)) break;
        g_sequence_foreach_range((GSequenceIter *)begin, (GSequenceIter *)end,
            func, user_data);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
}

/**
 * rclib_db_playlist_foreach:
 * @catalog_iter: the #RCLibDbCatalogIter which stores the playlist
 * @func: (scope call): the function to call for each item in @seq
 * @user_data: user data passed to @func
 *
 * Calls @func for each item in the playlist passing
 * @user_data to the function. MT safe.
 */

void rclib_db_playlist_foreach(RCLibDbCatalogIter *catalog_iter,
    GFunc func, gpointer user_data)
{
    RCLibDbPlaylistSequence *playlist = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    if(catalog_iter==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, catalog_iter))
            break;
        rclib_db_catalog_data_iter_get(catalog_iter,
            RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        if(playlist==NULL)
            break;
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        g_sequence_foreach((GSequence *)playlist, func, user_data);;
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
}

/**
 * rclib_db_playlist_iter_foreach_range:
 * @begin: a #RCLibDbPlaylistIter
 * @end: a #RCLibDbPlaylistIter
 * @func: (scope call): a #GFunc
 * @user_data: user data passed to @func
 *
 * Calls @func for each item in the range (@begin, @end) passing
 * @user_data to the function. MT safe.
 */

void rclib_db_playlist_iter_foreach_range(RCLibDbPlaylistIter *begin,
    RCLibDbPlaylistIter *end, GFunc func, gpointer user_data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    if(func==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->playlist_iter_table==NULL) return;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, begin)) break;
        if(!g_hash_table_contains(priv->playlist_iter_table, end)) break;
        g_sequence_foreach_range((GSequenceIter *)begin, (GSequenceIter *)end,
            func, user_data);
    }
    G_STMT_END;
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
}

RCLibDbCatalogIter *_rclib_db_catalog_append_data_internal(
    RCLibDbCatalogIter *insert_iter, RCLibDbCatalogData *catalog_data)
{
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    if(priv->catalog==NULL) return NULL;
    catalog_data->playlist = (RCLibDbPlaylistSequence *)
        g_sequence_new((GDestroyNotify)rclib_db_playlist_data_unref);
    g_rw_lock_writer_lock(&(priv->catalog_rw_lock));
    if(insert_iter!=NULL &&
        !g_hash_table_contains(priv->catalog_iter_table, insert_iter))
    {
        insert_iter = NULL;
    }
    if(insert_iter==NULL)
    {
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_append((GSequence *)
            priv->catalog, catalog_data);
    }
    else
    {
        catalog_iter = (RCLibDbCatalogIter *)g_sequence_insert_before(
            (GSequenceIter *)insert_iter, catalog_data);
    }
    catalog_data->self_iter = catalog_iter;
    if(priv->catalog_iter_table!=NULL)
    {
        g_hash_table_replace(priv->catalog_iter_table, catalog_iter,
            catalog_iter);
    }
    g_rw_lock_writer_unlock(&(priv->catalog_rw_lock));
    g_signal_emit_by_name(instance, "catalog-added", catalog_iter);
    return catalog_iter;
}

/**
 * rclib_db_get_catalog:
 *
 * Get the catalog.
 *
 * Returns: (transfer none): (skip): The catalog, NULL if the catalog does
 *     not exist.
 */

RCLibDbCatalogSequence *rclib_db_get_catalog()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    return priv->catalog;
}

/**
 * rclib_db_catalog_is_valid_iter:
 * @catalog_iter: the iter to check
 *
 * Check whether the iter is valid in the catalog sequence. MT safe.
 *
 * Returns: Whether the iter is valid.
 */

gboolean rclib_db_catalog_is_valid_iter(RCLibDbCatalogIter *catalog_iter)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    gboolean result = FALSE;
    if(catalog_iter==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(priv->catalog_iter_table==NULL) return FALSE;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    result = g_hash_table_contains(priv->catalog_iter_table, catalog_iter);
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    return result;
}

/**
 * rclib_db_catalog_add:
 * @name: the name for the new playlist
 * @iter: insert position (before this #iter)
 * @type: the type of the new playlist
 *
 * Add a new playlist to the catalog before the #iter, if the #iter
 * is #NULL, it will be added to the end. Must be called in main thread.
 *
 * Returns: (transfer none): (skip): The iter to the new playlist in
 *     the catalog.
 */

RCLibDbCatalogIter *rclib_db_catalog_add(const gchar *name,
    RCLibDbCatalogIter *iter, gint type)
{
    RCLibDbCatalogData *catalog_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    RCLibDbCatalogIter *catalog_iter;
    instance = rclib_db_get_instance();
    if(instance==NULL || name==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    catalog_data = rclib_db_catalog_data_new();
    catalog_data->name = g_strdup(name);
    catalog_data->type = type;
    catalog_iter = _rclib_db_catalog_append_data_internal(iter, catalog_data);
    priv->dirty_flag = TRUE;
    return catalog_iter;
}

/**
 * rclib_db_catalog_delete:
 * @iter: the iter to the catalog
 *
 * Delete the catalog pointed to by #iter. Must be called in main thread.
 */

void rclib_db_catalog_delete(RCLibDbCatalogIter *iter)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistIter *iter_foreach;
    if(iter==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->catalog_iter_table==NULL) return;
    g_signal_emit_by_name(instance, "catalog-delete", iter);
    g_rw_lock_writer_lock(&(priv->catalog_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->catalog_iter_table, iter))
            break;
        rclib_db_catalog_data_iter_set(iter,
            RCLIB_DB_CATALOG_DATA_TYPE_SELF_ITER, NULL,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        for(iter_foreach=rclib_db_playlist_get_begin_iter(iter);
            iter_foreach!=NULL;
            iter_foreach=rclib_db_playlist_iter_next(iter_foreach))
        {
            g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
            g_hash_table_remove(priv->playlist_iter_table, iter_foreach);
            g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
        }
        g_sequence_remove((GSequenceIter *)iter);
        g_hash_table_remove(priv->catalog_iter_table, iter);
    }
    G_STMT_END;
    g_rw_lock_writer_unlock(&(priv->catalog_rw_lock));
    priv->dirty_flag = TRUE;
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
 * rclib_db_catalog_reorder:
 * @new_order: (array): an array of integers mapping the new position of 
 *   each child to its old position before the re-ordering,
 *   i.e. @new_order<literal>[newpos] = oldpos</literal>.
 *
 * Reorder the catalog to follow the order indicated by @new_order.
 * Must be called in main thread.
 */

void rclib_db_catalog_reorder(gint *new_order)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    gint i;
    GHashTable *new_positions;
    RCLibDbCatalogIter *ptr;
    gint *order;
    gint length;
    instance = rclib_db_get_instance();
    g_return_if_fail(instance!=NULL);
    g_return_if_fail(new_order!=NULL);
    priv = RCLIB_DB(instance)->priv;
    g_return_if_fail(priv!=NULL);
    length = rclib_db_catalog_get_length();
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    order = g_new(gint, length);
    for(i=0;i<length;i++) order[new_order[i]] = i;
    new_positions = g_hash_table_new(g_direct_hash, g_direct_equal);
    ptr = rclib_db_catalog_get_begin_iter();
    for(i=0;ptr!=NULL;ptr=rclib_db_catalog_iter_next(ptr))
    {
        g_hash_table_insert(new_positions, ptr,
            GINT_TO_POINTER(order[i++]));
    }
    g_free(order);
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    g_rw_lock_writer_lock(&(priv->catalog_rw_lock));
    g_sequence_sort_iter((GSequence *)priv->catalog, rclib_db_reorder_func,
        new_positions);
    g_hash_table_destroy(new_positions);
    g_rw_lock_writer_unlock(&(priv->catalog_rw_lock));
    priv->dirty_flag = TRUE;
    g_signal_emit_by_name(instance, "catalog-reordered", new_order);
}

RCLibDbPlaylistIter *_rclib_db_playlist_append_data_internal(
    RCLibDbCatalogIter *catalog_iter, RCLibDbPlaylistIter *insert_iter,
    RCLibDbPlaylistData *playlist_data)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist = NULL;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    if(priv->catalog_iter_table==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    if(!g_hash_table_contains(priv->catalog_iter_table, catalog_iter))
    {
        g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
        return NULL;
    }
    rclib_db_catalog_data_iter_get(catalog_iter,
        RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
        RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    if(playlist==NULL)
    {
        g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
        return NULL;
    }
    g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
    if(insert_iter!=NULL &&
        !g_hash_table_contains(priv->playlist_iter_table, insert_iter))
    {
        insert_iter = NULL;
    }
    if(insert_iter==NULL)
    {
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_append(
            (GSequence *)playlist, playlist_data);
    }
    else
    {
        playlist_iter = (RCLibDbPlaylistIter *)g_sequence_insert_before(
            (GSequenceIter *)insert_iter, playlist_data);
    }
    playlist_data->self_iter = playlist_iter;
    if(priv->playlist_iter_table!=NULL)
    {
        g_hash_table_replace(priv->playlist_iter_table, playlist_iter,
            playlist_iter);
    }
    g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    g_signal_emit_by_name(instance, "playlist-added", playlist_iter);
    return playlist_iter;
}

/**
 * rclib_db_playlist_is_valid_iter:
 * @playlist_iter: the iter to check
 *
 * Check whether the iter is valid in the playlist sequence. MT safe.
 *
 * Returns: Whether the iter is valid.
 */

gboolean rclib_db_playlist_is_valid_iter(RCLibDbPlaylistIter *playlist_iter)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    gboolean result;
    if(playlist_iter==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(priv->playlist_iter_table==NULL) return FALSE;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    result = g_hash_table_contains(priv->playlist_iter_table, playlist_iter);
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    return result;
}

/**
 * rclib_db_playlist_add_music:
 * @iter: the catalog iter
 * @insert_iter: insert the music before this iter
 * @uri: the URI of the music
 *
 * Add music to the playlist by given catalog iter. MT safe.
 */

void rclib_db_playlist_add_music(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *uri)
{
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || iter==NULL || instance==NULL) return;
    if(rclib_db_catalog_iter_is_end(iter)) return;
    priv = RCLIB_DB(instance)->priv;
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbImportData, 1);
    import_data->type = RCLIB_DB_IMPORT_TYPE_PLAYLIST;
    import_data->catalog_iter = iter;
    import_data->playlist_insert_iter = insert_iter;
    import_data->uri = g_strdup(uri);
    g_async_queue_push(priv->import_queue, import_data);
}

/**
 * rclib_db_playlist_add_music_and_play:
 * @iter: the catalog iter
 * @insert_iter: insert the music before this iter
 * @uri: the URI of the music
 *
 * Add music to the playlist by given catalog iter, and then play it
 * if the add operation succeeds. MT safe.
 */

void rclib_db_playlist_add_music_and_play(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *uri)
{
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || iter==NULL || instance==NULL) return;
    if(rclib_db_catalog_iter_is_end(iter)) return;
    priv = RCLIB_DB(instance)->priv;
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbImportData, 1);
    import_data->type = RCLIB_DB_IMPORT_TYPE_PLAYLIST;
    import_data->catalog_iter = iter;
    import_data->playlist_insert_iter = insert_iter;
    import_data->uri = g_strdup(uri);
    import_data->play_flag = TRUE;
    g_async_queue_push(priv->import_queue, import_data);
} 

/**
 * rclib_db_playlist_delete:
 * @iter: the iter to the playlist data
 *
 * Delete the playlist data pointed to by #iter.
 * Must be called in main thread.
 */

void rclib_db_playlist_delete(RCLibDbPlaylistIter *iter)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    if(iter==NULL) return;
    if(rclib_db_playlist_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->playlist_iter_table==NULL) return;
    rclib_db_playlist_data_iter_set(iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_SELF_ITER, NULL,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    g_signal_emit_by_name(instance, "playlist-delete", iter);
    g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->playlist_iter_table, iter)) break;
        g_sequence_remove((GSequenceIter *)iter);
        g_hash_table_remove(priv->playlist_iter_table, iter);
    }
    G_STMT_END;
    g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_update_metadata:
 * @iter: the iter to the playlist
 * @data: the new metadata
 *
 * Update the metadata in the playlist pointed to by #iter.
 * Title, artist, album, file type, track number, year information
 * will be updated. Must be called in main thread.
 */

void rclib_db_playlist_update_metadata(RCLibDbPlaylistIter *iter,
    const RCLibDbPlaylistData *data)
{
    RCLibDbPlaylistData *playlist_data;
    RCLibDbPrivate *priv;
    GObject *instance;
    if(iter==NULL) return;
    if(rclib_db_playlist_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    playlist_data = rclib_db_playlist_iter_get_data(iter);
    if(playlist_data==NULL)
        return;
    g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
    if(data->title!=NULL && g_strcmp0(playlist_data->title,
        data->title)!=0)
    {
        priv->dirty_flag = TRUE;
        g_debug("Playlist data updated, title: %s -> %s",
            playlist_data->title, data->title);
    }
    if(data->artist!=NULL && g_strcmp0(playlist_data->artist,
        data->artist)!=0)
    {
        priv->dirty_flag = TRUE;
        g_debug("Playlist data updated, artist: %s -> %s",
            playlist_data->artist, data->artist);
    }
    if(data->album!=NULL && g_strcmp0(playlist_data->album,
        data->album)!=0)
    {
        priv->dirty_flag = TRUE;
        g_debug("Playlist data updated, album: %s -> %s",
            playlist_data->album, data->album);
    }
    if(playlist_data->tracknum!=data->tracknum)
    {
        priv->dirty_flag = TRUE;
        g_debug("Playlist data updated, track number: %d -> %d",
            playlist_data->tracknum, data->tracknum);
    }
    if(playlist_data->year!=data->year)
    {
        priv->dirty_flag = TRUE;
        g_debug("Playlist data updated, year: %d -> %d",
            playlist_data->year, data->year);
    }
    if(playlist_data->type!=data->type)
    {
        priv->dirty_flag = TRUE;
        g_debug("Playlist data updated, type: %d -> %d",
            playlist_data->type, data->type);
    }
    if(data->genre!=NULL && g_strcmp0(playlist_data->genre,
        data->genre)!=0)
    {
        priv->dirty_flag = TRUE;
        g_debug("Playlist data updated, genre: %s -> %s",
            playlist_data->genre, data->genre);
    }
    g_free(playlist_data->title);
    g_free(playlist_data->artist);
    g_free(playlist_data->album);
    g_free(playlist_data->ftype);
    g_free(playlist_data->genre);
    playlist_data->title = g_strdup(data->title);
    playlist_data->artist = g_strdup(data->artist);
    playlist_data->album = g_strdup(data->album);
    playlist_data->ftype = g_strdup(data->ftype);
    playlist_data->genre = g_strdup(data->genre);
    playlist_data->tracknum = data->tracknum;
    playlist_data->year = data->year;
    playlist_data->type = data->type;
    rclib_db_playlist_data_unref(playlist_data);
    g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
    g_signal_emit_by_name(instance, "playlist-changed", iter);
}

/**
 * rclib_db_playlist_reorder:
 * @iter: the iter pointed to catalog
 * @new_order: (array): an array of integers mapping the new position of 
 *   each child to its old position before the re-ordering,
 *   i.e. @new_order<literal>[newpos] = oldpos</literal>.
 *
 * Reorder the playlist to follow the order indicated by @new_order.
 * Must be called in main thread.
 */

void rclib_db_playlist_reorder(RCLibDbCatalogIter *iter, gint *new_order)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistSequence *playlist = NULL;
    GHashTable *new_positions;
    RCLibDbPlaylistIter *ptr;
    gint i;
    gint *order;
    gint length;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(new_order!=NULL);
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
    length = rclib_db_playlist_get_length(iter);
    if(length<0)
    {
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
        return;
    }
    order = g_new(gint, length);
    for(i=0;i<length;i++) order[new_order[i]] = i;
    new_positions = g_hash_table_new(g_direct_hash, g_direct_equal);
    ptr = rclib_db_playlist_get_begin_iter(iter);
    for(i=0;ptr!=NULL;ptr=rclib_db_playlist_iter_next(ptr))
    {
        g_hash_table_insert(new_positions, ptr, GINT_TO_POINTER(order[i++]));
    }
    g_free(order);
    g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
    g_rw_lock_reader_lock(&(priv->catalog_rw_lock));
    rclib_db_catalog_data_iter_get(iter, RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST,
        &playlist, RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    if(playlist!=NULL)
    {
        g_sequence_sort_iter((GSequence *)playlist, rclib_db_reorder_func,
            new_positions);
    }
    g_hash_table_destroy(new_positions);
    g_rw_lock_reader_unlock(&(priv->catalog_rw_lock));
    g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
    g_signal_emit_by_name(instance, "playlist-reordered", iter, new_order);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_move_to_another_catalog:
 * @iters: an iter array
 * @num: the size of the iter array
 * @catalog_iter: the iter for the catalog
 *
 * Move the playlist data pointed to by #iters to another catalog
 * pointed to by #catalog_iter. Must be called in main thread.
 */

void rclib_db_playlist_move_to_another_catalog(RCLibDbPlaylistIter **iters,
    guint num, RCLibDbCatalogIter *catalog_iter)
{
    gint i;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *old_data, *new_data;
    RCLibDbPlaylistIter *new_iter;
    RCLibDbPlaylistIter *reference;
    if(iters==NULL || catalog_iter==NULL || num<1) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    reference = (RCLibDbPlaylistIter *)rclib_core_get_db_reference();
    catalog_data = rclib_db_catalog_iter_get_data(catalog_iter);
    if(catalog_data==NULL) return;
    for(i=0;i<num;i++)
    {
        g_rw_lock_writer_lock(&(priv->catalog_rw_lock));
        old_data = rclib_db_playlist_iter_get_data(iters[i]);
        if(old_data==NULL)
        {
            g_rw_lock_writer_unlock(&(priv->catalog_rw_lock));
            continue;
        }
        new_data = old_data;
        rclib_db_playlist_delete(iters[i]);
        g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
        new_data->catalog = catalog_iter;
        new_iter = (RCLibDbPlaylistIter *)g_sequence_append(
            (GSequence *)catalog_data->playlist, new_data);
        if(iters[i]==reference)
            rclib_core_update_db_reference(new_iter);
        new_data->self_iter = new_iter;
        g_hash_table_replace(priv->playlist_iter_table, new_iter, new_iter);
        g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
        g_rw_lock_writer_unlock(&(priv->catalog_rw_lock));
        g_signal_emit_by_name(instance, "playlist-added", new_iter);
    }
    rclib_db_catalog_data_unref(catalog_data);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_add_m3u_file:
 * @iter: the catalog iter
 * @insert_iter: insert the music before this iter
 * @filename: the path of the playlist file
 * 
 * Load a m3u playlist file, and add all music inside to
 * the catalog pointed to by #iter. MT safe.
 */

void rclib_db_playlist_add_m3u_file(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *filename)
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

static void rclib_db_playlist_add_directory_recursive(
    RCLibDbCatalogIter *iter, RCLibDbPlaylistIter *insert_iter,
    const gchar *fdir, guint depth)
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
 * MT safe.
 */

void rclib_db_playlist_add_directory(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *dir)
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
 * MT safe.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_db_playlist_export_m3u_file(RCLibDbCatalogIter *iter,
    const gchar *sfilename)
{
    RCLibDbPlaylistIter *playlist_iter;
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *uri = NULL;
    gchar *filename;
    gint64 length;
    FILE *fp;
    if(iter==NULL || sfilename==NULL) return FALSE;
    if(g_regex_match_simple("(.M3U)$", sfilename, G_REGEX_CASELESS, 0))
        filename = g_strdup(sfilename);
    else
        filename = g_strdup_printf("%s.M3U", sfilename);
    fp = g_fopen(filename, "wb");
    g_free(filename);
    if(fp==NULL) return FALSE;
    g_fprintf(fp, "#EXTM3U\n");
    for(playlist_iter = rclib_db_playlist_get_begin_iter(iter);
        playlist_iter!=NULL;
        playlist_iter = rclib_db_playlist_iter_next(playlist_iter))
    {
        uri = NULL;
        title = NULL;
        artist = NULL;
        length = 0;
        rclib_db_playlist_data_iter_get(playlist_iter,
            RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
            RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE, &title,
            RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST, &artist,
            RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH, &length,
            RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
        if(uri==NULL)
        {
            g_free(title);
            g_free(artist);
            continue;
        }
        if(title==NULL)
            title = rclib_tag_get_name_from_uri(uri);
        if(title==NULL) title = g_strdup(_("Unknown Title"));
        if(artist==NULL)
        {
            g_fprintf(fp, "#EXTINF:%"G_GINT64_FORMAT",%s\n%s\n",
                length / GST_SECOND, title, uri);
        }
        else
        {
            g_fprintf(fp, "#EXTINF:%"G_GINT64_FORMAT",%s - %s\n%s\n",
                length / GST_SECOND, artist, title, uri);
        }
        g_free(uri);
        g_free(title);
        g_free(artist);
    }
    fclose(fp);
    return TRUE;
}

/**
 * rclib_db_playlist_export_all_m3u_files:
 * @dir: the directory to save playlist files
 *
 * Export all playlists in the catalog to playlist files. MTsafe.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_db_playlist_export_all_m3u_files(const gchar *dir)
{
    gboolean flag = FALSE;
    RCLibDbCatalogData *catalog_data;
    RCLibDbCatalogIter *catalog_iter;
    gchar *filename;
    for(catalog_iter = rclib_db_catalog_get_begin_iter();
        !rclib_db_catalog_iter_is_end(catalog_iter);
        catalog_iter = rclib_db_catalog_iter_next(catalog_iter))
    {
        catalog_data = rclib_db_catalog_iter_get_data(catalog_iter);
        if(catalog_data==NULL || catalog_data->name==NULL) continue;
        filename = g_strdup_printf("%s%c%s.M3U", dir,
            G_DIR_SEPARATOR, catalog_data->name);
        if(rclib_db_playlist_export_m3u_file(catalog_iter, filename))
            flag = TRUE;
        g_free(filename);
        rclib_db_catalog_data_unref(catalog_data);
    }
    return flag;
}

/**
 * rclib_db_playlist_refresh:
 * @iter: the catalog iter
 *
 * Refresh the metadata of the music in the playlist pointed to
 * by the given catalog #iter. MT safe.
 */

void rclib_db_playlist_refresh(RCLibDbCatalogIter *iter)
{
    RCLibDbRefreshData *refresh_data;
    RCLibDbPlaylistIter *iter_foreach;
    RCLibDbPrivate *priv;
    gchar *uri;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    if(iter==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL || priv->refresh_queue==NULL) return;
    for(iter_foreach = rclib_db_playlist_get_begin_iter(iter);
        iter_foreach!=NULL;
        iter_foreach = rclib_db_playlist_iter_next(iter_foreach))
    {
        uri = NULL;
        rclib_db_playlist_data_iter_get(iter_foreach,
            RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
            RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
        if(uri==NULL) continue;
        refresh_data = g_new0(RCLibDbRefreshData, 1);
        refresh_data->catalog_iter = iter;
        refresh_data->playlist_iter = iter_foreach;
        refresh_data->uri = uri;
        g_async_queue_push(priv->refresh_queue, refresh_data);
    }
}

/**
 * rclib_db_load_legacy:
 *
 * Load legacy playlist file from RhythmCat 1.0.
 *
 * Returns: Whether the operation succeeds.
 */

gboolean rclib_db_load_legacy()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    GFile *file;
    GFileInputStream *input_stream;
    GDataInputStream *data_stream;
    gsize line_len;
    RCLibDbCatalogData *catalog_data = NULL;
    RCLibDbPlaylistData *playlist_data = NULL;
    RCLibDbCatalogIter *catalog_iter = NULL;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    gchar *line = NULL;
    gint64 timeinfo;
    gint trackno;
    gchar *plist_set_file_full_path = NULL;
    const gchar *home_dir = NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    plist_set_file_full_path = g_build_filename(home_dir, ".RhythmCat",
        "playlist.dat", NULL);
    if(plist_set_file_full_path==NULL) return FALSE;
    file = g_file_new_for_path(plist_set_file_full_path);
    if(file==NULL) return FALSE;
    if(!g_file_query_exists(file, NULL))
    {
        g_object_unref(file);
        return FALSE;
    }
    input_stream = g_file_read(file, NULL, NULL);
    g_object_unref(file);
    if(input_stream==NULL) return FALSE;
    data_stream = g_data_input_stream_new(G_INPUT_STREAM(input_stream));
    g_object_unref(input_stream);
    if(data_stream==NULL) return FALSE;
    g_data_input_stream_set_newline_type(data_stream,
        G_DATA_STREAM_NEWLINE_TYPE_ANY);
    while((line=g_data_input_stream_read_line(data_stream, &line_len,
        NULL, NULL))!=NULL)
    {
        if(catalog_data!=NULL && strncmp(line, "UR=", 3)==0)
        {
            if(playlist_iter!=NULL)
            {
                g_signal_emit_by_name(instance, "playlist-changed",
                    playlist_iter);
            }
            playlist_data = rclib_db_playlist_data_new();
            playlist_data->uri = g_strdup(line+3);
            playlist_data->catalog = catalog_iter;
            playlist_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
            g_rw_lock_writer_lock(&(priv->playlist_rw_lock));
            playlist_iter = (RCLibDbPlaylistIter *)g_sequence_append(
                (GSequence *)catalog_data->playlist, playlist_data);
            playlist_data->self_iter = playlist_iter;
            g_hash_table_replace(priv->playlist_iter_table, playlist_iter,
                playlist_iter);
            g_rw_lock_writer_unlock(&(priv->playlist_rw_lock));
            g_signal_emit_by_name(instance, "playlist-added", playlist_iter);
        }
        else if(playlist_data!=NULL && strncmp(line, "TI=", 3)==0)
        {
            g_rw_lock_writer_lock(&(playlist_data->lock));
            playlist_data->title = g_strdup(line+3);
            g_rw_lock_writer_unlock(&(playlist_data->lock));
        }
        else if(playlist_data!=NULL && strncmp(line, "AR=", 3)==0)
        {
            g_rw_lock_writer_lock(&(playlist_data->lock));
            playlist_data->artist = g_strdup(line+3);
            g_rw_lock_writer_unlock(&(playlist_data->lock));
        }
        else if(playlist_data!=NULL && strncmp(line, "AL=", 3)==0)
        {
            g_rw_lock_writer_lock(&(playlist_data->lock));
            playlist_data->album = g_strdup(line+3);
            g_rw_lock_writer_unlock(&(playlist_data->lock));
        }
        /* time length */
        else if(playlist_data!=NULL && strncmp(line, "TL=", 3)==0) 
        {
            timeinfo = g_ascii_strtoll(line+3, NULL, 10) * 10;
            timeinfo *= GST_MSECOND;
            g_rw_lock_writer_lock(&(playlist_data->lock));
            playlist_data->length = timeinfo;
            g_rw_lock_writer_unlock(&(playlist_data->lock));
        }
        else if(strncmp(line, "TN=", 3)==0)  /* track number */
        {
            sscanf(line+3, "%d", &trackno);
            g_rw_lock_writer_lock(&(playlist_data->lock));
            playlist_data->tracknum = trackno;
            g_rw_lock_writer_unlock(&(playlist_data->lock));
        }
        else if(strncmp(line, "LF=", 3)==0)
        {
            g_rw_lock_writer_lock(&(playlist_data->lock));
            playlist_data->lyricfile = g_strdup(line+3);
            g_rw_lock_writer_unlock(&(playlist_data->lock));
        }
        else if(strncmp(line, "AF=", 3)==0)
        {
            g_rw_lock_writer_lock(&(playlist_data->lock));
            playlist_data->albumfile = g_strdup(line+3);
            g_rw_lock_writer_unlock(&(playlist_data->lock));
        }
        else if(strncmp(line, "LI=", 3)==0)
        {
            catalog_data = rclib_db_catalog_data_new();
            catalog_data->name = g_strdup(line+3);
            catalog_data->type = RCLIB_DB_CATALOG_TYPE_PLAYLIST;
            catalog_iter = _rclib_db_catalog_append_data_internal(NULL,
                catalog_data);
        }
        g_free(line);
    }
    g_object_unref(data_stream);
    return TRUE;
}

static inline RCLibDbPlaylistDataType rclib_db_playlist_property_convert(
    RCLibDbQueryDataType query_type)
{
    switch(query_type)
    {
        case RCLIB_DB_QUERY_DATA_TYPE_URI:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_URI;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TITLE:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ARTIST:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ALBUM:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM;
        case RCLIB_DB_QUERY_DATA_TYPE_FTYPE:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_LENGTH:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TRACKNUM:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_RATING:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_RATING;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_YEAR:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_GENRE:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE;
            break;
        default:
            return RCLIB_DB_PLAYLIST_DATA_TYPE_NONE;
            break;
    }
    return RCLIB_DB_PLAYLIST_DATA_TYPE_NONE;
}

/**
 * rclib_db_playlist_data_query:
 * @playlist_data: the playlist data to check
 * @query: the query condition
 *
 * Check whether the playlist data satisfied the query condition.
 * MT safe.
 * 
 * Returns: Whether the playlist data satisfied the query condition.
 */

gboolean rclib_db_playlist_data_query(RCLibDbPlaylistData *playlist_data,
    RCLibDbQuery *query)
{
    RCLibDbQueryData *query_data;
    gboolean result = FALSE;
    guint i;
    RCLibDbPlaylistDataType ptype;
    GType dtype;
    gchar *pstring;
    gint pint;
    gint64 pint64;
    gdouble pdouble;
    gchar *uri;
    if(playlist_data==NULL || query==NULL)
        return FALSE;
    for(i=0;i<query->len;i++)
    {
        query_data = g_ptr_array_index(query, i);
        if(query_data==NULL) continue;
        switch(query_data->type)
        {
            case RCLIB_DB_QUERY_CONDITION_TYPE_SUBQUERY:
            {
                if(query_data->subquery==NULL)
                {
                    result = FALSE;
                    break;
                }
                result = rclib_db_playlist_data_query(playlist_data,
                    query_data->subquery);
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), pstring)==0);
                        g_free(pstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)==pint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint64 = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint64, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)==pint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pdouble = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pdouble, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (ABS(g_value_get_double(query_data->val) -
                            pdouble)<=10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_EQUAL:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), pstring)!=0);
                        g_free(pstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)!=pint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint64 = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint64, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)!=pint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pdouble = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pdouble, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (ABS(g_value_get_double(query_data->val) -
                            pdouble)>10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), pstring)>0);
                        g_free(pstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)>pint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint64 = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint64, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)>pint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pdouble = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pdouble, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            pdouble>10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER_OR_EQUAL:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), pstring)>=0);
                        g_free(pstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)>=pint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint64 = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint64, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)>=pint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pdouble = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pdouble, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            pdouble>=0);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), pstring)<0);
                        g_free(pstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)<pint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint64 = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint64, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)<pint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pdouble = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pdouble, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            pdouble<-10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS_OR_EQUAL:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), pstring)<=0);
                        g_free(pstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)<=pint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pint64 = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pint64, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)<=pint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pdouble = 0;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pdouble, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            pdouble<=0);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LIKE:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }

                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        if(pstring==NULL)
                        {
                            result = FALSE;
                            break;
                        }
                        result = (g_strstr_len(pstring, -1,
                            g_value_get_string(query_data->val))!=NULL);
                        g_free(pstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_LIKE:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        if(pstring==NULL)
                        {
                            result = FALSE;
                            break;
                        }
                        result = (g_strstr_len(pstring, -1,
                            g_value_get_string(query_data->val))==NULL);
                        g_free(pstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_PREFIX:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = g_str_has_prefix(pstring,
                            g_value_get_string(query_data->val));
                        g_free(pstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_SUFFIX:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ptype = rclib_db_playlist_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        pstring = NULL;
                        rclib_db_playlist_data_get(playlist_data, ptype,
                            &pstring, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        if(ptype==RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE &&
                            (pstring==NULL || strlen(pstring)==0))
                        {
                            uri = NULL;
                            if(pstring!=NULL) g_free(pstring);
                            pstring = NULL;
                            rclib_db_playlist_data_get(playlist_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                pstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = g_str_has_suffix(pstring,
                            g_value_get_string(query_data->val));
                        g_free(pstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            default:
                break;
        }
        if(!result) break;
    }
    return result;
}

/**
 * rclib_db_playlist_iter_query:
 * @playlist_iter: the playlist iter to check
 * @query: the query condition
 *
 * Check whether the playlist data stored in the iter satisfied the
 * query condition. MT safe.
 * 
 * Returns: Whether the playlist data stored in the iter satisfied the
 *     query condition.
 */

gboolean rclib_db_playlist_iter_query(RCLibDbPlaylistIter *playlist_iter,
    RCLibDbQuery *query)
{
    RCLibDbPlaylistData *data;
    gboolean result;
    if(playlist_iter==NULL || query==NULL)
        return FALSE;
    data = rclib_db_playlist_iter_get_data(playlist_iter);
    if(data==NULL) return FALSE;
    result = rclib_db_playlist_data_query(data, query);
    rclib_db_playlist_data_unref(data);
    return result;
}

/**
 * rclib_db_playlist_query: 
 * @catalog_iter: the catalog to query, set to #NULL to query in all catalogs
 * @query: the query condition
 *
 * Query data from the playlist library. MT safe.
 * 
 * Returns: (transfer full): The playlist data array which satisfied
 *     the query condition. Free with #g_ptr_array_free() or
 *     #g_ptr_array_unref() after usage.
 */

GPtrArray *rclib_db_playlist_query(RCLibDbCatalogIter *catalog_iter,
    RCLibDbQuery *query)
{
    GPtrArray *query_result = NULL;
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbPlaylistIter *playlist_iter;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    query_result = g_ptr_array_new_with_free_func((GDestroyNotify)
        rclib_db_playlist_data_unref);
    if(catalog_iter!=NULL)
    {
        
        for(playlist_iter=rclib_db_playlist_get_begin_iter(catalog_iter);
            playlist_iter!=NULL;
            playlist_iter=rclib_db_playlist_iter_next(playlist_iter))
        {
            playlist_data = rclib_db_playlist_iter_get_data(playlist_iter);
            if(playlist_data==NULL) continue;
            if(rclib_db_playlist_data_query(playlist_data, query))
                g_ptr_array_add(query_result, playlist_data); 
            else
                rclib_db_playlist_data_unref(playlist_data);
        }
    }
    else
    {
        GHashTableIter iter;
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        g_hash_table_iter_init(&iter, priv->playlist_iter_table);
        while(g_hash_table_iter_next(&iter, (gpointer *)&playlist_iter, NULL))
        {
            if(playlist_iter==NULL) continue;
            playlist_data = rclib_db_playlist_iter_get_data(playlist_iter);
            if(playlist_data==NULL) continue;
            if(rclib_db_playlist_data_query(playlist_data, query))
                g_ptr_array_add(query_result, playlist_data);
            else
                rclib_db_playlist_data_unref(playlist_data);
        }
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    return query_result;
}

/**
 * rclib_db_playlist_query_get_iters: 
 * @catalog_iter: the catalog to query, set to #NULL to query in all catalogs
 * @query: the query condition
 *
 * Query data from the playlist library, and get an array of iters which
 * satisfied the query condition. MT safe.
 * 
 * Returns: (transfer full): The playlist iter array which satisfied
 *     the query condition. Free with #g_ptr_array_free() or
 *     #g_ptr_array_unref() after usage.
 */

GPtrArray *rclib_db_playlist_query_get_iters(RCLibDbCatalogIter *catalog_iter,
    RCLibDbQuery *query)
{
    GPtrArray *query_result = NULL;
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbPlaylistIter *playlist_iter;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    query_result = g_ptr_array_new();
    if(catalog_iter!=NULL)
    {
        
        for(playlist_iter=rclib_db_playlist_get_begin_iter(catalog_iter);
            playlist_iter!=NULL;
            playlist_iter=rclib_db_playlist_iter_next(playlist_iter))
        {
            playlist_data = rclib_db_playlist_iter_get_data(playlist_iter);
            if(playlist_data==NULL) continue;
            if(rclib_db_playlist_data_query(playlist_data, query))
                g_ptr_array_add(query_result, playlist_iter); 
            rclib_db_playlist_data_unref(playlist_data);
        }
    }
    else
    {
        GHashTableIter iter;
        g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
        g_hash_table_iter_init(&iter, priv->playlist_iter_table);
        while(g_hash_table_iter_next(&iter, (gpointer *)&playlist_iter, NULL))
        {
            if(playlist_iter==NULL) continue;
            playlist_data = rclib_db_playlist_iter_get_data(playlist_iter);
            if(playlist_data==NULL) continue;
            if(rclib_db_playlist_data_query(playlist_data, query))
                g_ptr_array_add(query_result, playlist_iter);
            rclib_db_playlist_data_unref(playlist_data);
        }
        g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
    }
    return query_result;
}

/**
 * rclib_db_playlist_get_random_iter:
 * @catalog_iter: the catalog iter pointed to the catalog, #NULL to use all
 *     catalogs
 * @rating_limit: whether to use rating limit
 * @condition: the condition, #TRUE to get a random iter which pointed to
 *     the #RCLibDbPlaylistData whose rating is less or equal to the @rating,
 *     #FLASE to get the a random iter which pointed to the
 *     #RCLibDbPlaylistData whose rating is greater or equal to the @rating
 * @rating: the rating limit value
 * 
 * Get a random playlist iter from in the given catalog or all catalogs.
 * MT safe.
 * 
 * Returns: (transfer none): (skip): A random playlist iter.
 */

RCLibDbPlaylistIter *rclib_db_playlist_get_random_iter(
    RCLibDbCatalogIter *catalog_iter, gboolean rating_limit,
    gboolean condition, gfloat rating)
{
    GPtrArray *query_result = NULL;
    GObject *instance;
    gint playlist_length, pos;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbPlaylistIter *playlist_iter;
    RCLibDbPrivate *priv;
    RCLibDbPlaylistIter *result_iter = NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    if(!rating_limit)
    {
        if(catalog_iter!=NULL)
        {
            g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
            playlist_length = rclib_db_playlist_get_length(catalog_iter);
            if(playlist_length>0)
            {
                pos = g_random_int_range(0, playlist_length);
                result_iter = rclib_db_playlist_get_iter_at_pos(catalog_iter,
                    pos);
            }
            g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
        }
        else
        {
            GHashTableIter iter;
            g_rw_lock_reader_lock(&(priv->playlist_rw_lock));
            g_hash_table_iter_init(&iter, priv->playlist_iter_table);
            playlist_length = g_hash_table_size(priv->playlist_iter_table);
            pos = g_random_int_range(0, playlist_length);
            playlist_iter = NULL;
            while(g_hash_table_iter_next(&iter, (gpointer *)&playlist_iter,
                NULL) && pos>0)
            {
                pos--;
            }
            result_iter = playlist_iter;
            g_rw_lock_reader_unlock(&(priv->playlist_rw_lock));
            
        }
        return result_iter;
    }
    
    
    return result_iter;
}
