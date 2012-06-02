/*
 * RhythmCat UI List Model Header Declaration
 *
 * rc-ui-listmodel.h
 * This file is part of RhythmCat Music Player (GTK+ Version)
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

#ifndef HAVE_RC_UI_LISTMODEL_H
#define HAVE_RC_UI_LISTMODEL_H

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

#define RC_UI_TYPE_CATALOG_STORE (rc_ui_catalog_store_get_type())
#define RC_UI_CATALOG_STORE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_CATALOG_STORE, RCUiCatalogStore))
#define RC_UI_CATALOG_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
    RC_UI_TYPE_CATALOG_STORE, RCUiCatalogStoreClass))
#define RC_UI_IS_CATALOG_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_CATALOG_STORE))
#define RC_UI_IS_CATALOG_STORE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_CATALOG_STORE))
#define RC_UI_CATALOG_STORE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_CATALOG_STORE, \
    RCUiCatalogStore))

#define RC_UI_TYPE_PLAYLIST_STORE (rc_ui_playlist_store_get_type())
#define RC_UI_PLAYLIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_PLAYLIST_STORE, RCUiPlaylistStore))
#define RC_UI_PLAYLIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
    RC_UI_TYPE_PLAYLIST_STORE, RCUiPlaylistStoreClass))
#define RC_UI_IS_PLAYLIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_PLAYLIST_STORE))
#define RC_UI_IS_PLAYLIST_STORE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_PLAYLIST_STORE))
#define RC_UI_PLAYLIST_STORE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_PLAYLIST_STORE, \
    RCUiPlaylistStore))

/**
 * RCUiCatalogColumns:
 * @RC_UI_CATALOG_COLUMN_TYPE: the type column
 * @RC_UI_CATALOG_COLUMN_STATE: the state column
 * @RC_UI_CATALOG_COLUMN_NAME: the name column
 * @RC_UI_CATALOG_COLUMN_PLAYING_FLAG: the playing flag column
 * @RC_UI_CATALOG_COLUMN_LAST: the last column, do not use it
 *
 * The enum type for the columns in the catalog store.
 */

typedef enum {
    RC_UI_CATALOG_COLUMN_TYPE,
    RC_UI_CATALOG_COLUMN_STATE,
    RC_UI_CATALOG_COLUMN_NAME,
    RC_UI_CATALOG_COLUMN_PLAYING_FLAG,
    RC_UI_CATALOG_COLUMN_LAST
}RCUiCatalogColumns;

/**
 * RCUiPlaylistColumns:
 * @RC_UI_PLAYLIST_COLUMN_TYPE: the type column
 * @RC_UI_PLAYLIST_COLUMN_STATE: the state column
 * @RC_UI_PLAYLIST_COLUMN_FTITLE: the title column
 * @RC_UI_PLAYLIST_COLUMN_TITLE: the real title column
 * @RC_UI_PLAYLIST_COLUMN_ARTIST: the artist column
 * @RC_UI_PLAYLIST_COLUMN_ALBUM: the album column
 * @RC_UI_PLAYLIST_COLUMN_FTYPE: the file type (format) column
 * @RC_UI_PLAYLIST_COLUMN_LENGTH: the time length column
 * @RC_UI_PLAYLIST_COLUMN_TRACK: the track number column
 * @RC_UI_PLAYLIST_COLUMN_RATING: the rating column
 * @RC_UI_PLAYLIST_COLUMN_YEAR: the year column
 * @RC_UI_PLAYLIST_COLUMN_PLAYING_FLAG: the playing flag column
 * @RC_UI_PLAYLIST_COLUMN_LAST: the last column, do not use it
 *
 * The enum type for the columns in the playlist store.
 */

typedef enum {
    RC_UI_PLAYLIST_COLUMN_TYPE,
    RC_UI_PLAYLIST_COLUMN_STATE,
    RC_UI_PLAYLIST_COLUMN_FTITLE,
    RC_UI_PLAYLIST_COLUMN_TITLE,
    RC_UI_PLAYLIST_COLUMN_ARTIST,
    RC_UI_PLAYLIST_COLUMN_ALBUM,
    RC_UI_PLAYLIST_COLUMN_FTYPE,
    RC_UI_PLAYLIST_COLUMN_LENGTH,
    RC_UI_PLAYLIST_COLUMN_TRACK,
    RC_UI_PLAYLIST_COLUMN_RATING,
    RC_UI_PLAYLIST_COLUMN_YEAR,
    RC_UI_PLAYLIST_COLUMN_PLAYING_FLAG,
    RC_UI_PLAYLIST_COLUMN_LAST
}RCUiPlaylistColumns;

typedef struct _RCUiCatalogStore RCUiCatalogStore;
typedef struct _RCUiPlaylistStore RCUiPlaylistStore;
typedef struct _RCUiCatalogStoreClass RCUiCatalogStoreClass;
typedef struct _RCUiPlaylistStoreClass RCUiPlaylistStoreClass;

/**
 * RCUiCatalogStore:
 *
 * The data structure used for #RCUiCatalogStore class.
 */

struct _RCUiCatalogStore {
    /*< private >*/
    GObject parent;
};

/**
 * RCUiPlaylistStore:
 *
 * The data structure used for #RCUiPlaylistStore class.
 */

struct _RCUiPlaylistStore {
    /*< private >*/
    GObject parent;
};

/**
 * RCUiCatalogStoreClass:
 *
 * #RCUiCatalogStoreClass class.
 */

struct _RCUiCatalogStoreClass {
    /*< private >*/
    GObjectClass parent_class;
};

/**
 * RCUiPlaylistStoreClass:
 *
 * #RCUiPlaylistStoreClass class.
 */

struct _RCUiPlaylistStoreClass {
    /*< private >*/
    GObjectClass parent_class;
};

/*< private >*/
GType rc_ui_catalog_store_get_type();
GType rc_ui_playlist_store_get_type();

/*< public >*/
gboolean rc_ui_list_model_init();
void rc_ui_list_model_exit();
GtkTreeModel *rc_ui_list_model_get_catalog_store();
GtkTreeModel *rc_ui_list_model_get_playlist_store(GtkTreeIter *iter);
GSequenceIter *rc_ui_list_model_get_catalog_by_model(GtkTreeModel *model);
void rc_ui_list_model_set_playlist_title_format(const gchar *format);
const gchar *rc_ui_list_model_get_playlist_title_format();

G_END_DECLS

#endif

