/*
 * RhythmCat Library Music Database Private Header Declaration
 *
 * rclib-db-priv.h
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

#ifndef HAVE_RC_LIB_DB_PRIVATE_H
#define HAVE_RC_LIB_DB_PRIVATE_H

#include "rclib-db.h"
#include "rclib-tag.h"

#define RCLIB_DB_ERROR rclib_db_error_quark()

typedef enum
{
    RCLIB_DB_IMPORT_TYPE_PLAYLIST = 0,
    RCLIB_DB_IMPORT_TYPE_LIBRARY = 1
}RCLibDbImportType;

typedef enum
{
    RCLIB_DB_REFRESH_TYPE_PLAYLIST = 0,
    RCLIB_DB_REFRESH_TYPE_LIBRARY = 1
}RCLibDbRefreshType;

typedef struct _RCLibDbCatalogSequence RCLibDbCatalogSequence;
typedef struct _RCLibDbPlaylistSequence RCLibDbPlaylistSequence;

typedef struct RCLibDbImportData
{
    RCLibDbImportType type;
    gchar *uri;
    RCLibDbCatalogIter *catalog_iter;
    RCLibDbPlaylistIter *playlist_insert_iter;
    gboolean play_flag;
}RCLibDbImportData;

typedef struct RCLibDbRefreshData
{
    RCLibDbRefreshType type;
    gchar *uri;
    RCLibDbCatalogIter *catalog_iter;
    RCLibDbPlaylistIter *playlist_iter;
}RCLibDbRefreshData;

typedef struct RCLibDbPlaylistImportIdleData
{
    RCLibTagMetadata *mmd;
    RCLibDbCatalogIter *catalog_iter;
    RCLibDbPlaylistIter *playlist_insert_iter;
    RCLibDbPlaylistType type;
    gboolean play_flag;
}RCLibDbPlaylistImportIdleData;

typedef struct RCLibDbPlaylistRefreshIdleData
{
    RCLibTagMetadata *mmd;
    RCLibDbCatalogIter *catalog_iter;
    RCLibDbPlaylistIter *playlist_iter;
    RCLibDbPlaylistType type;
}RCLibDbPlaylistRefreshIdleData;

typedef struct RCLibDbLibraryImportIdleData
{
    RCLibTagMetadata *mmd;
    RCLibDbLibraryType type;
    gboolean play_flag;
}RCLibDbLibraryImportIdleData;

typedef struct RCLibDbLibraryRefreshIdleData
{
    RCLibTagMetadata *mmd;
    RCLibDbLibraryType type;
}RCLibDbLibraryRefreshIdleData;

struct _RCLibDbPrivate
{
    gchar *filename;
    RCLibDbCatalogSequence *catalog;
    GHashTable *catalog_iter_table;
    GHashTable *playlist_iter_table;
    GHashTable *playlist_sequence_table;
    GRWLock catalog_rw_lock;
    GRWLock playlist_rw_lock;
    GSequence *library_query;
    GHashTable *library_table;
    GRWLock library_rw_lock;
    GHashTable *library_keyword_table;
    GObject *library_query_base;
    GObject *library_query_genre;
    GObject *library_query_artist;
    GObject *library_query_album;
    GThread *import_thread;
    GThread *refresh_thread;
    GThread *autosave_thread;
    GAsyncQueue *import_queue;
    GAsyncQueue *refresh_queue;
    gboolean import_work_flag;
    gboolean refresh_work_flag;
    gboolean dirty_flag;
    gboolean work_flag;
    gulong autosave_timeout;
    GMutex autosave_mutex;
    GCond autosave_cond;
    GString *autosave_xml_data;
};

struct _RCLibDbLibraryQueryResultPrivate
{
    RCLibDbLibraryQueryResult *base_query_result;
    RCLibDbQuery *query;
    GAsyncQueue *query_queue;
    GSequence *query_sequence;
    GHashTable *query_iter_table;
    GHashTable *query_uri_table;
    GHashTable *prop_table;
    RCLibDbLibraryDataType sort_column;
    gboolean sort_direction;
    GThread *query_thread;
    GCancellable *cancellable;
    gboolean list_need_sort;
    gulong library_added_id;
    gulong library_changed_id;
    gulong library_deleted_id;
    gulong base_added_id;
    gulong base_changed_id;
    gulong base_delete_id;
};

struct _RCLibDbCatalogData
{
    /*< private >*/
    gint ref_count;
    GRWLock lock;
    
    /*< public >*/
    RCLibDbPlaylistSequence *playlist;
    RCLibDbCatalogIter *self_iter;
    gchar *name;
    RCLibDbCatalogType type;
    gpointer store;
};

struct _RCLibDbPlaylistData
{
    /*< private >*/
    gint ref_count;
    GRWLock lock;

    /*< public >*/
    RCLibDbCatalogIter *catalog;
    RCLibDbPlaylistIter *self_iter;
    RCLibDbPlaylistType type;
    gchar *uri;
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *ftype;
    gchar *genre;
    gint64 length;
    gint tracknum;
    gint year;
    gfloat rating;
    gchar *lyricfile;
    gchar *lyricsecfile;
    gchar *albumfile;
};

struct _RCLibDbLibraryData
{
    /*< private >*/
    gint ref_count;
    GRWLock lock;

    /*< public >*/
    RCLibDbLibraryType type;
    gchar *uri;
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *ftype;
    gchar *genre;
    gint64 length;
    gint tracknum;
    gint year;
    gfloat rating;
    gchar *lyricfile;
    gchar *lyricsecfile;
    gchar *albumfile;
};

typedef struct _RCLibDbQueryData {
    /*< private >*/
	guint type;
	guint propid;
	GValue *val;
	RCLibDbQuery *subquery;
}RCLibDbQueryData;

/*< private >*/
gboolean _rclib_db_instance_init_playlist(RCLibDb *db, RCLibDbPrivate *priv);
gboolean _rclib_db_instance_init_library(RCLibDb *db, RCLibDbPrivate *priv);
void _rclib_db_instance_finalize_playlist(RCLibDbPrivate *priv);
void _rclib_db_instance_finalize_library(RCLibDbPrivate *priv);
gboolean _rclib_db_playlist_import_idle_cb(gpointer data);
gboolean _rclib_db_playlist_refresh_idle_cb(gpointer data);
RCLibDbCatalogIter *_rclib_db_catalog_append_data_internal(
    RCLibDbCatalogIter *insert_iter, RCLibDbCatalogData *catalog_data);
RCLibDbPlaylistIter *_rclib_db_playlist_append_data_internal(
    RCLibDbCatalogIter *catalog_iter, RCLibDbPlaylistIter *insert_iter,
    RCLibDbPlaylistData *playlist_data);
gboolean _rclib_db_library_import_idle_cb(gpointer data);
gboolean _rclib_db_library_refresh_idle_cb(gpointer data);
void _rclib_db_library_append_data_internal(const gchar *uri,
    RCLibDbLibraryData *library_data);

#endif

