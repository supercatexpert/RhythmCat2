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
    RC_UI_TYPE_CATALOG_STORE, GObject))
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
    RC_UI_TYPE_PLAYLIST_STORE, GObject))
#define RC_UI_IS_PLAYLIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_PLAYLIST_STORE))
#define RC_UI_IS_PLAYLIST_STORE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_PLAYLIST_STORE))
#define RC_UI_PLAYLIST_STORE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_PLAYLIST_STORE, \
    RCUiPlaylistStore))

typedef enum {
    RC_UI_CATALOG_COLUMN_TYPE,
    RC_UI_CATALOG_COLUMN_STATE,
    RC_UI_CATALOG_COLUMN_NAME,
    RC_UI_CATALOG_COLUMN_PLAYING_FLAG,
    RC_UI_CATALOG_COLUMN_LAST
}RCUiCatalogColumns;

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

struct _RCUiCatalogStore {
    /*< private >*/
    GObject parent;
};

struct _RCUiPlaylistStore {
    /*< private >*/
    GObject parent;
};

struct _RCUiCatalogStoreClass {
    /*< private >*/
    GObjectClass parent_class;
};

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

G_END_DECLS

#endif

