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

gboolean _rclib_db_playlist_import_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbCatalogData *catalog_data;
    GSequence *playlist;
    GSequenceIter *playlist_iter;
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
    if(!rclib_db_is_iter_valid(priv->catalog, idle_data->catalog_iter))
        return FALSE;
    catalog_data = g_sequence_get(idle_data->catalog_iter);
    if(catalog_data==NULL) return FALSE;
    playlist = catalog_data->playlist;
    if(playlist==NULL) return FALSE;
    mmd = idle_data->mmd;
    playlist_data = rclib_db_playlist_data_new();
    playlist_data->catalog = idle_data->catalog_iter;
    playlist_data->type = idle_data->type;
    playlist_data->uri = g_strdup(mmd->uri);
    playlist_data->title = g_strdup(mmd->title);
    playlist_data->artist = g_strdup(mmd->artist);
    playlist_data->album = g_strdup(mmd->album);
    playlist_data->ftype = g_strdup(mmd->ftype);
    playlist_data->length = mmd->length;
    playlist_data->tracknum = mmd->tracknum;
    playlist_data->year = mmd->year;
    playlist_data->rating = 3.0;
    if(!rclib_db_is_iter_valid(playlist, idle_data->playlist_insert_iter))
        idle_data->playlist_insert_iter = NULL;
    if(idle_data->playlist_insert_iter!=NULL)
    {
        playlist_iter = g_sequence_insert_before(
            idle_data->playlist_insert_iter, playlist_data);
    }
    else
        playlist_iter = g_sequence_append(playlist, playlist_data);
    playlist_data->self_iter = playlist_iter;
    g_hash_table_replace(priv->playlist_iter_table, playlist_iter,
        playlist_iter);
    priv->dirty_flag = TRUE;
    g_signal_emit_by_name(instance, "playlist-added", playlist_iter);
    rclib_db_playlist_import_idle_data_free(idle_data);
    return FALSE;
}

gboolean _rclib_db_playlist_refresh_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    RCLibDbCatalogData *catalog_data;
    GSequence *playlist;
    RCLibDbPlaylistRefreshIdleData *idle_data;
    RCLibTagMetadata *mmd;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    idle_data = (RCLibDbPlaylistRefreshIdleData *)data;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(instance==NULL) return FALSE;
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
    g_signal_emit_by_name(instance, "playlist-changed",
        idle_data->playlist_iter);
    rclib_db_playlist_refresh_idle_data_free(idle_data);
    return FALSE;
}

gboolean _rclib_db_instance_init_playlist(RCLibDb *db, RCLibDbPrivate *priv)
{
    if(db==NULL || priv==NULL) return FALSE;
    priv->catalog_iter_table = g_hash_table_new_full(g_direct_hash,
        g_direct_equal, NULL, NULL);
    priv->playlist_iter_table = g_hash_table_new_full(g_direct_hash,
        g_direct_equal, NULL, NULL);
    priv->catalog = g_sequence_new((GDestroyNotify)
        rclib_db_catalog_data_unref);
    return TRUE;
}

void _rclib_db_instance_finalize_playlist(RCLibDbPrivate *priv)
{
    if(priv==NULL) return;
    if(priv->catalog!=NULL)
        g_sequence_free(priv->catalog);
    if(priv->catalog_iter_table!=NULL)
        g_hash_table_destroy(priv->catalog_iter_table);
    if(priv->playlist_iter_table!=NULL)
        g_hash_table_destroy(priv->playlist_iter_table);
    priv->catalog = NULL;
    priv->catalog_iter_table = NULL;
    priv->playlist_iter_table = NULL;
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
    RCLibDbCatalogData *data = g_slice_new0(RCLibDbCatalogData);
    data->ref_count = 1;
    return data;
}

/**
 * rclib_db_catalog_data_ref:
 * @data: the #RCLibDbCatalogData structure
 *
 * Increase the reference of #RCLibDbCatalogData by 1.
 *
 * Returns: (transfer full): The #RCLibDbCatalogData structure.
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
    g_slice_free(RCLibDbCatalogData, data);
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
    RCLibDbPlaylistData *data = g_slice_new0(RCLibDbPlaylistData);
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
    g_slice_free(RCLibDbPlaylistData, data);
}

/**
 * rclib_db_get_catalog:
 *
 * Get the catalog.
 *
 * Returns: (transfer none): (skip): The catalog, NULL if the catalog does
 *     not exist.
 */

GSequence *rclib_db_get_catalog()
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
 * Check whether the iter is valid in the catalog sequence.
 *
 * Returns: Whether the iter is valid.
 */

gboolean rclib_db_catalog_is_valid_iter(GSequenceIter *catalog_iter)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    return g_hash_table_contains(priv->catalog_iter_table, catalog_iter);
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
 * Returns: (transfer none): (skip): The iter to the new playlist in
 *     the catalog.
 */

GSequenceIter *rclib_db_catalog_add(const gchar *name,
    GSequenceIter *iter, gint type)
{
    RCLibDbCatalogData *catalog_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    GSequence *playlist;
    GSequenceIter *catalog_iter;
    instance = rclib_db_get_instance();
    if(instance==NULL || name==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
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
    catalog_data->self_iter = catalog_iter;
    g_hash_table_replace(priv->catalog_iter_table, catalog_iter,
        catalog_iter);
    priv->dirty_flag = TRUE;
    g_signal_emit_by_name(instance, "catalog-added", catalog_iter);
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
    RCLibDbCatalogData *catalog_data;
    GObject *instance;
    GSequenceIter *iter_foreach;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    catalog_data = g_sequence_get(iter);
    catalog_data->self_iter = NULL;
    g_signal_emit_by_name(instance, "catalog-delete", iter);
    for(iter_foreach=g_sequence_get_begin_iter(catalog_data->playlist);
        !g_sequence_iter_is_end(iter_foreach);
        iter_foreach=g_sequence_iter_next(iter_foreach))
    {
        g_hash_table_remove(priv->playlist_iter_table, iter_foreach);
    }
    g_sequence_remove(iter);
    g_hash_table_remove(priv->catalog_iter_table, iter);
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
    GObject *instance;
    RCLibDbCatalogData *catalog_data;
    if(iter==NULL || name==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    catalog_data = g_sequence_get(iter);
    if(catalog_data==NULL) return;
    g_free(catalog_data->name);
    catalog_data->name = g_strdup(name);
    priv->dirty_flag = TRUE;
    g_signal_emit_by_name(instance, "catalog-changed", iter);
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
    GObject *instance;
    RCLibDbCatalogData *catalog_data;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    catalog_data = g_sequence_get(iter);
    if(catalog_data==NULL) return;
    catalog_data->type = type;
    priv->dirty_flag = TRUE;
    g_signal_emit_by_name(instance, "catalog-changed", iter);
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
 */

void rclib_db_catalog_reorder(gint *new_order)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    gint i;
    GHashTable *new_positions;
    GSequenceIter *ptr;
    gint *order;
    gint length;
    instance = rclib_db_get_instance();
    g_return_if_fail(instance!=NULL);
    g_return_if_fail(new_order!=NULL);
    priv = RCLIB_DB(instance)->priv;
    g_return_if_fail(priv!=NULL);
    length = g_sequence_get_length(priv->catalog);
    order = g_new(gint, length);
    for(i=0;i<length;i++) order[new_order[i]] = i;
    new_positions = g_hash_table_new(g_direct_hash, g_direct_equal);
    ptr = g_sequence_get_begin_iter(priv->catalog);
    if(!g_sequence_iter_is_end(ptr))
    {
        for(i=0;!g_sequence_iter_is_end(ptr);ptr=g_sequence_iter_next(ptr))
        {
            g_hash_table_insert(new_positions, ptr,
                GINT_TO_POINTER(order[i++]));
        }
    }
    g_free(order);
    g_sequence_sort_iter(priv->catalog, rclib_db_reorder_func,
        new_positions);
    g_hash_table_destroy(new_positions);
    priv->dirty_flag = TRUE;
    g_signal_emit_by_name(instance, "catalog-reordered", new_order);
}

/**
 * rclib_db_playlist_is_valid_iter:
 * @playlist_iter: the iter to check
 *
 * Check whether the iter is valid in the playlist sequence.
 *
 * Returns: Whether the iter is valid.
 */

gboolean rclib_db_playlist_is_valid_iter(GSequenceIter *playlist_iter)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    return g_hash_table_contains(priv->playlist_iter_table, playlist_iter);
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
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || iter==NULL || instance==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
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
 * Add music to the music library by given catalog iter, and then play it
 * if the add operation succeeds.
 */

void rclib_db_playlist_add_music_and_play(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *uri)
{
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || iter==NULL || instance==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
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
 */

void rclib_db_playlist_delete(GSequenceIter *iter)
{
    RCLibDbPrivate *priv;
    RCLibDbPlaylistData *playlist_data;
    GObject *instance;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    playlist_data->self_iter = NULL;
    g_signal_emit_by_name(instance, "playlist-delete", iter);
    g_sequence_remove(iter);
    g_hash_table_remove(priv->playlist_iter_table, iter);
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
    GObject *instance;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
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
    g_signal_emit_by_name(instance, "playlist-changed", iter);
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
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    if(playlist_data->length!=length)
        priv->dirty_flag = TRUE;
    playlist_data->length = length;
    g_signal_emit_by_name(instance, "playlist-changed", iter);
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
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    if(playlist_data->type!=type)
        priv->dirty_flag = TRUE;
    playlist_data->type = type;
    g_signal_emit_by_name(instance, "playlist-changed", iter);
}

/**
 * rclib_db_playlist_set_rating:
 * @iter: the iter to the playlist
 * @rating: the new rating to set
 *
 * Set the rating in the playlist pointed to by #iter.
 */

void rclib_db_playlist_set_rating(GSequenceIter *iter, gfloat rating)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    if(rating<0.0) rating = 0.0;
    if(rating>5.0) rating = 5.0;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    if(playlist_data->rating!=rating)
        priv->dirty_flag = TRUE;
    playlist_data->rating = rating;
    g_signal_emit_by_name(instance, "playlist-changed", iter);
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
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
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
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
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
    GObject *instance;
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    g_free(playlist_data->albumfile);
    playlist_data->albumfile = g_strdup(filename);
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_playlist_get_rating:
 * @iter: the iter to the playlist
 * @rating: (out) (allow-none): the rating to return
 *
 * Get the lyric file bound to the music in the playlist pointed
 * to by #iter.
 *
 * Returns: The lyric file path.
 */

gboolean rclib_db_playlist_get_rating(GSequenceIter *iter, gfloat *rating)
{
    RCLibDbPlaylistData *playlist_data;
    if(g_sequence_iter_is_end(iter)) return FALSE;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return FALSE;
    if(rating!=NULL) *rating = playlist_data->rating;
    return TRUE;
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
    if(g_sequence_iter_is_end(iter)) return NULL;
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
    if(g_sequence_iter_is_end(iter)) return NULL;
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
    if(g_sequence_iter_is_end(iter)) return NULL;
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
    GObject *instance;
    RCLibDbCatalogData *catalog_data;
    GHashTable *new_positions;
    GSequenceIter *ptr;
    gint i;
    gint *order;
    gint length;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(new_order!=NULL);
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
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
 * pointed to by #catalog_iter.
 */

void rclib_db_playlist_move_to_another_catalog(GSequenceIter **iters,
    guint num, GSequenceIter *catalog_iter)
{
    gint i;
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *old_data, *new_data;
    GSequenceIter *new_iter;
    GSequenceIter *reference;
    if(iters==NULL || catalog_iter==NULL || num<1) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
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
        new_data->self_iter = new_iter;
        g_hash_table_replace(priv->playlist_iter_table, new_iter, new_iter);
        g_signal_emit_by_name(instance, "playlist-added", new_iter);
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
    GObject *instance;
    GSequenceIter *catalog_iter;
    gchar *filename;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
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
    RCLibDbRefreshData *refresh_data;
    GSequenceIter *iter_foreach;
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    if(iter==NULL) return;
    priv = RCLIB_DB(instance)->priv;
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
        refresh_data = g_new0(RCLibDbRefreshData, 1);
        refresh_data->catalog_iter = iter;
        refresh_data->playlist_iter = iter_foreach;
        refresh_data->uri = g_strdup(playlist_data->uri);
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
    GSequenceIter *catalog_iter = NULL;
    GSequenceIter *playlist_iter = NULL;
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
            playlist_iter = g_sequence_append(catalog_data->playlist,
                playlist_data);
            playlist_data->self_iter = playlist_iter;
            g_hash_table_replace(priv->playlist_iter_table, playlist_iter,
                playlist_iter);
            g_signal_emit_by_name(instance, "playlist-added", playlist_iter);
        }
        else if(playlist_data!=NULL && strncmp(line, "TI=", 3)==0)
            playlist_data->title = g_strdup(line+3);
        else if(playlist_data!=NULL && strncmp(line, "AR=", 3)==0)
            playlist_data->artist = g_strdup(line+3);
        else if(playlist_data!=NULL && strncmp(line, "AL=", 3)==0)
            playlist_data->album = g_strdup(line+3);

        /* time length */
        else if(playlist_data!=NULL && strncmp(line, "TL=", 3)==0) 
        {
            timeinfo = g_ascii_strtoll(line+3, NULL, 10) * 10;
            timeinfo *= GST_MSECOND;
            playlist_data->length = timeinfo;
        }
        else if(strncmp(line, "TN=", 3)==0)  /* track number */
        {
            sscanf(line+3, "%d", &trackno);
            playlist_data->tracknum = trackno;
        }
        else if(strncmp(line, "LF=", 3)==0)
            playlist_data->lyricfile = g_strdup(line+3);
        else if(strncmp(line, "AF=", 3)==0)
            playlist_data->albumfile = g_strdup(line+3);
        else if(strncmp(line, "LI=", 3)==0)
        {
            catalog_data = rclib_db_catalog_data_new();
            catalog_data->name = g_strdup(line+3);
            catalog_data->type = RCLIB_DB_CATALOG_TYPE_PLAYLIST;
            catalog_data->playlist = g_sequence_new((GDestroyNotify)
                rclib_db_playlist_data_unref);
            catalog_iter = g_sequence_append(priv->catalog, catalog_data);
            catalog_data->self_iter = catalog_iter;
            g_hash_table_replace(priv->catalog_iter_table, catalog_iter,
                catalog_iter);
            g_signal_emit_by_name(instance, "catalog-added", catalog_iter);
        }
        g_free(line);
    }
    g_object_unref(data_stream);
    return TRUE;
}

