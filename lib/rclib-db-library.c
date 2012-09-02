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
    g_slice_free(RCLibDbLibraryData, data);
}

gboolean _rclib_db_instance_init_library(RCLibDb *db, RCLibDbPrivate *priv)
{
    if(db==NULL || priv==NULL) return FALSE;
    priv->library_query = g_sequence_new((GDestroyNotify)
        rclib_db_library_data_unref);
    priv->library_table = g_hash_table_new_full(g_str_hash,
        g_str_equal, g_free, (GDestroyNotify)rclib_db_library_data_unref);
    return TRUE;
}

void _rclib_db_instance_finalize_library(RCLibDbPrivate *priv)
{
    if(priv==NULL) return;
    if(priv->library_query!=NULL)
        g_sequence_free(priv->library_query);
    if(priv->library_table!=NULL)
        g_hash_table_destroy(priv->library_table);
    priv->library_query = NULL;
    priv->library_table = NULL;
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

