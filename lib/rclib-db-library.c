/*
 * RhythmCat Library Music Database Module (Library Part.)
 * Manage the library part of the music database.
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

static inline gboolean rclib_db_library_keyword_table_add_entry(
    GHashTable *library_keyword_table, RCLibDbLibraryData *library_data,
    const gchar *keyword)
{
    gboolean present = TRUE;
    GHashTable *keyword_table;
    keyword_table = g_hash_table_lookup(library_keyword_table, keyword);
    if(keyword_table!=NULL)
    {
        present = (g_hash_table_lookup(keyword_table, library_data)!=NULL);
        g_hash_table_insert(keyword_table, library_data, GINT_TO_POINTER(1));
    }
    else
    {
        present = FALSE;
        keyword_table = g_hash_table_new(g_direct_hash, g_direct_equal);
        g_hash_table_insert(keyword_table, library_data, GINT_TO_POINTER(1));
        g_hash_table_insert(library_keyword_table, g_strdup(keyword),
            keyword_table);
    }
    return present;
}

static inline gboolean rclib_db_library_keyword_table_remove_entry(
    GHashTable *library_keyword_table, RCLibDbLibraryData *library_data,
    const gchar *keyword)
{
    GHashTable *keyword_table;
    gboolean ret;
    keyword_table = g_hash_table_lookup(library_keyword_table, keyword);
    if(keyword_table!=NULL)
    {
        ret = g_hash_table_remove(keyword_table, library_data);
        if(g_hash_table_size(keyword_table)==0)
        {
            g_hash_table_remove(library_keyword_table, keyword);
        }
    }
    else
    {
        ret = FALSE;
    }
    return ret;
}

static inline gboolean rclib_db_library_keyword_table_contains_entry(
    GHashTable *library_keyword_table, RCLibDbLibraryData *library_data,
    const gchar *keyword)
{
    GHashTable *keyword_table;
    gboolean ret;
    keyword_table = g_hash_table_lookup(library_keyword_table, keyword);
    if(keyword_table!=NULL)
    {
        ret = (g_hash_table_lookup(keyword_table, library_data)!=NULL);
    }
    else
    {
        ret = FALSE;
    }
    return ret;
}

static inline void rclib_db_library_import_idle_data_free(
    RCLibDbLibraryImportIdleData *data)
{
    if(data==NULL) return;
    if(data->mmd!=NULL) rclib_tag_free(data->mmd);
    g_free(data);
}

static inline void rclib_db_library_refresh_idle_data_free(
    RCLibDbLibraryRefreshIdleData *data)
{
    if(data==NULL) return;
    if(data->mmd!=NULL) rclib_tag_free(data->mmd);
    g_free(data);
}

gboolean _rclib_db_library_import_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbLibraryData *library_data;
    RCLibTagMetadata *mmd;
    RCLibDbLibraryImportIdleData *idle_data;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    idle_data = (RCLibDbLibraryImportIdleData *)data;
    if(idle_data->mmd==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(instance==NULL) return FALSE;
    if(idle_data->mmd->uri==NULL)
    {
        rclib_db_library_import_idle_data_free(idle_data);
        return FALSE;
    }
    mmd = idle_data->mmd;
    library_data = rclib_db_library_data_new();
    library_data = rclib_db_library_data_new();
    library_data->type = idle_data->type;
    library_data->uri = g_strdup(mmd->uri);
    library_data->title = g_strdup(mmd->title);
    library_data->artist = g_strdup(mmd->artist);
    library_data->album = g_strdup(mmd->album);
    library_data->ftype = g_strdup(mmd->ftype);
    library_data->genre = g_strdup(mmd->genre);
    library_data->length = mmd->length;
    library_data->tracknum = mmd->tracknum;
    library_data->year = mmd->year;
    library_data->rating = 3.0;
    g_hash_table_replace(priv->library_table, g_strdup(library_data->uri),
        library_data);
    priv->dirty_flag = TRUE;
    
    /* Send a signal that the operation succeeded. */
    g_signal_emit_by_name(instance, "library-added",
        library_data->uri);
        
    rclib_db_library_import_idle_data_free(idle_data);
    return FALSE;
}

gboolean _rclib_db_library_refresh_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbLibraryData *library_data;
    RCLibTagMetadata *mmd;
    RCLibDbLibraryRefreshIdleData *idle_data;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    idle_data = (RCLibDbLibraryRefreshIdleData *)data;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(instance==NULL) return FALSE;
    if(idle_data->mmd->uri==NULL)
    {
        rclib_db_library_refresh_idle_data_free(idle_data);
        return FALSE;
    }
    mmd = idle_data->mmd;
    library_data = g_hash_table_lookup(priv->library_table, mmd->uri);
    if(library_data==NULL)
    {
        rclib_db_library_refresh_idle_data_free(idle_data);
        return FALSE;
    }
    library_data->type = idle_data->type;
    if(mmd!=NULL)
    {
        g_free(library_data->title);
        g_free(library_data->artist);
        g_free(library_data->album);
        g_free(library_data->ftype);
        library_data->title = g_strdup(mmd->title);
        library_data->artist = g_strdup(mmd->artist);
        library_data->album = g_strdup(mmd->album);
        library_data->ftype = g_strdup(mmd->ftype);
        library_data->genre = g_strdup(mmd->genre);
        library_data->length = mmd->length;
        library_data->tracknum = mmd->tracknum;
        library_data->year = mmd->year;
    }
    priv->dirty_flag = TRUE;
    
    /* Send a signal that the operation succeeded. */
    g_signal_emit_by_name(instance, "library-changed",
        library_data->uri);
    
    rclib_db_library_refresh_idle_data_free(idle_data);
    return FALSE;
}

/**
 * rclib_db_library_data_new:
 * 
 * Create a new empty #RCLibDbLibraryData structure,
 * and set the reference count to 1.
 *
 * Returns: The new empty allocated #RCLibDbLibraryData structure.
 */

RCLibDbLibraryData *rclib_db_library_data_new()
{
    RCLibDbLibraryData *data = g_slice_new0(RCLibDbLibraryData);
    data->ref_count = 1;
    return data;
}

/**
 * rclib_db_library_data_ref:
 * @data: the #RCLibDbLibraryData structure
 *
 * Increase the reference of #RCLibDbLibraryData by 1.
 *
 * Returns: (transfer none): The #RCLibDbLibraryData structure.
 */

RCLibDbLibraryData *rclib_db_library_data_ref(RCLibDbLibraryData *data)
{
    if(data==NULL) return NULL;
    g_atomic_int_add(&(data->ref_count), 1);
    return data;
}

/**
 * rclib_db_library_data_unref:
 * @data: the #RCLibDbLibraryData structure
 *
 * Decrease the reference of #RCLibDbLibraryData by 1.
 * If the reference down to zero, the structure will be freed.
 *
 * Returns: The #RCLibDbLibraryData structure.
 */

void rclib_db_library_data_unref(RCLibDbLibraryData *data)
{
    if(data==NULL) return;
    if(g_atomic_int_dec_and_test(&(data->ref_count)))
        rclib_db_library_data_free(data);
}

/**
 * rclib_db_library_data_free:
 * @data: the data to free
 *
 * Free the #RCLibDbLibraryData structure.
 */

void rclib_db_library_data_free(RCLibDbLibraryData *data)
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
    g_free(data->genre);
    g_slice_free(RCLibDbLibraryData, data);
}

gboolean _rclib_db_instance_init_library(RCLibDb *db, RCLibDbPrivate *priv)
{
    if(db==NULL || priv==NULL) return FALSE;
    
    /* GSequence<RCLibDbLibraryData *> */
    priv->library_query = g_sequence_new((GDestroyNotify)
        rclib_db_library_data_unref);
        
    /* GHashTable<gchar *, RCLibDbLibraryData *> */
    priv->library_table = g_hash_table_new_full(g_str_hash,
        g_str_equal, g_free, (GDestroyNotify)rclib_db_library_data_unref);
        
    /* GHashTable<gchar *, GHashTable<RCLibDbLibraryData *, 1>> */
    priv->library_keyword_table = g_hash_table_new_full(g_str_hash,
        g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    return TRUE;
}

void _rclib_db_instance_finalize_library(RCLibDbPrivate *priv)
{
    if(priv==NULL) return;
    if(priv->library_query!=NULL)
        g_sequence_free(priv->library_query);
    if(priv->library_keyword_table!=NULL)
        g_hash_table_destroy(priv->library_keyword_table);
    if(priv->library_table!=NULL)
        g_hash_table_destroy(priv->library_table);
    priv->library_query = NULL;
    priv->library_table = NULL;
}

static inline gboolean rclib_db_library_data_set_valist(
    RCLibDbLibraryData *data, RCLibDbLibraryDataType type1,
    va_list var_args)
{
    gboolean send_signal = FALSE;
    RCLibDbLibraryDataType type;
    RCLibDbLibraryType new_type;
    const gchar *str;   
    gint64 length;
    gint vint;
    gdouble rating;
    type = type1;
    while(type!=RCLIB_DB_LIBRARY_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_LIBRARY_DATA_TYPE_TYPE:
            {
                new_type = va_arg(var_args, RCLibDbLibraryType);
                if(new_type==data->type) break;
                data->type = new_type;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_URI:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->uri)==0) break;
                if(data->uri!=NULL) g_free(data->uri);
                data->uri = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TITLE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->title)==0) break;
                if(data->title!=NULL) g_free(data->title);
                data->title = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->artist)==0) break;
                if(data->artist!=NULL) g_free(data->artist);
                data->artist = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->album)==0) break;
                if(data->album!=NULL) g_free(data->album);
                data->album = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->ftype)==0) break;
                if(data->ftype!=NULL) g_free(data->ftype);
                data->ftype = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH:
            {
                length = va_arg(var_args, gint64);
                if(data->length==length) break;
                data->length = length;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM:
            {
                vint = va_arg(var_args, gint);
                if(vint==data->tracknum) break;
                data->tracknum = vint;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_YEAR:
            {
                vint = va_arg(var_args, gint);
                if(vint==data->year) break;
                data->year = vint;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_RATING:
            {
                rating = va_arg(var_args, gdouble);
                if(rating==data->rating) break;
                data->rating = rating;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->lyricfile)==0) break;
                if(data->lyricfile!=NULL) g_free(data->lyricfile);
                str = va_arg(var_args, const gchar *);
                data->lyricfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICSECFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->lyricsecfile)==0) break;
                if(data->lyricsecfile!=NULL) g_free(data->lyricsecfile);
                data->lyricsecfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUMFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->albumfile)==0) break;
                if(data->albumfile!=NULL) g_free(data->albumfile);
                data->albumfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_GENRE:
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
                g_warning("rclib_db_library_data_set: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbLibraryDataType);
    }
    return send_signal;
}

static inline void rclib_db_library_data_get_valist(
    const RCLibDbLibraryData *data, RCLibDbLibraryDataType type1,
    va_list var_args)
{
    RCLibDbLibraryDataType type;
    RCLibDbLibraryType *library_type;
    gchar **str;   
    gint64 *length;
    gint *vint;
    gfloat *rating;
    type = type1;
    while(type!=RCLIB_DB_LIBRARY_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_LIBRARY_DATA_TYPE_TYPE:
            {
                library_type = va_arg(var_args, RCLibDbLibraryType *);
                *library_type = data->type;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_URI:
            {
                str = va_arg(var_args, gchar **);
                *str = data->uri;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TITLE:
            {
                str = va_arg(var_args, gchar **);
                *str = data->title;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST:
            {
                str = va_arg(var_args, gchar **);
                *str = data->artist;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM:
            {
                str = va_arg(var_args, gchar **);
                *str = data->album;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE:
            {
                str = va_arg(var_args, gchar **);
                *str = data->ftype;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH:
            {
                length = va_arg(var_args, gint64 *);
                *length = data->length;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM:
            {
                vint = va_arg(var_args, gint *);
                *vint = data->tracknum;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_YEAR:
            {
                vint = va_arg(var_args, gint *);
                *vint = data->year;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_RATING:
            {
                rating = va_arg(var_args, gfloat *);
                *rating = data->rating;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = data->lyricfile;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICSECFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = data->lyricsecfile;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUMFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = data->albumfile;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_GENRE:
            {
                str = va_arg(var_args, gchar **);
                *str = data->genre;
                break;
            }
            default:
            {
                g_warning("rclib_db_library_data_get: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbLibraryDataType);
    }
}

/**
 * rclib_db_library_data_set: (skip)
 * @data: the #RCLibDbLibraryDataType data
 * @type1: the first property in playlist data to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_LIBRARY_DATA_TYPE_NONE
 *
 * Sets properties on a #RCLibDbPlaylistData.
 */

void rclib_db_library_data_set(RCLibDbLibraryData *data,
    RCLibDbLibraryDataType type1, ...)
{
    RCLibDbPrivate *priv = NULL;
    GObject *instance = NULL;
    va_list var_args;
    gboolean send_signal = FALSE;
    if(data==NULL) return;
    va_start(var_args, type1);
    send_signal = rclib_db_library_data_set_valist(data, type1, var_args);
    va_end(var_args);
    if(send_signal)
    {
        instance = rclib_db_get_instance();
        if(instance!=NULL)
            priv = RCLIB_DB(instance)->priv;
        if(data->uri!=NULL)
        {
            if(priv!=NULL)
                priv->dirty_flag = TRUE;
            g_signal_emit_by_name(instance, "library-changed", data->uri);
        }
    }
}

/**
 * rclib_db_library_data_get: (skip)
 * @data: the #RCLibDbLibraryData data
 * @type1: the first property in library data to get
 * @...: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %RCLIB_DB_LIBRARY_DATA_TYPE_NONE
 *
 * Gets properties of a #RCLibDbLibraryData. The property contents will not
 * be copied, just get their address instead, if the contents are stored
 * inside a pointer.
 */

void rclib_db_library_data_get(const RCLibDbLibraryData *data,
    RCLibDbLibraryDataType type1, ...)
{
    va_list var_args;
    if(data==NULL) return;
    va_start(var_args, type1);
    rclib_db_library_data_get_valist(data, type1, var_args);
    va_end(var_args);
}

/**
 * rclib_db_get_library_table:
 *
 * Get the library table.
 *
 * Returns: (transfer none): (skip): The library table, NULL if the
 *     table does not exist.
 */

GHashTable *rclib_db_get_library_table()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    return priv->library_table;
}

/**
 * rclib_db_library_has_uri:
 * @uri: the URI to find
 *
 * Check whether the give URI exists in the library.
 *
 * Returns: Whether the URI exists in the library.
 */

gboolean rclib_db_library_has_uri(const gchar *uri)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    return g_hash_table_contains(priv->library_table, uri);
}

/**
 * rclib_db_library_add_music:
 * @uri: the URI of the music to add
 *
 * Add music to the music library.
 */

void rclib_db_library_add_music(const gchar *uri)
{
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbImportData, 1);
    import_data->type = RCLIB_DB_IMPORT_TYPE_LIBRARY;
    import_data->uri = g_strdup(uri);
    g_async_queue_push(priv->import_queue, import_data);
}

/**
 * rclib_db_library_add_music_and_play:
 * @uri: the URI of the music to add
 *
 * Add music to the music library, and then play it if the add
 *     operation succeeds.
 */

void rclib_db_library_add_music_and_play(const gchar *uri)
{
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbImportData, 1);
    import_data->type = RCLIB_DB_IMPORT_TYPE_LIBRARY;
    import_data->uri = g_strdup(uri);
    import_data->play_flag = TRUE;
    g_async_queue_push(priv->import_queue, import_data);
}

/**
 * rclib_db_library_delete:
 * @uri: the URI of the music item to delete
 *
 * Delete the the music item by the given URI.
 */

void rclib_db_library_delete(const gchar *uri)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    if(uri==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    if(!g_hash_table_contains(priv->library_table, uri)) return;
    g_signal_emit_by_name(instance, "library-delete", uri);
    g_hash_table_remove(priv->library_table, uri);
    priv->dirty_flag = TRUE;
}

