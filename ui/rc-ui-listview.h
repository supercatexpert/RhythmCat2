/*
 * RhythmCat UI List View Header Declaration
 *
 * rc-ui-listview.h
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

#ifndef HAVE_RC_UI_LISTVIEW_H
#define HAVE_RC_UI_LISTVIEW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RC_UI_TYPE_CATALOG_VIEW (rc_ui_catalog_view_get_type())
#define RC_UI_CATALOG_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_CATALOG_VIEW, RCUiCatalogView))
#define RC_UI_CATALOG_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
    RC_UI_TYPE_CATALOG_VIEW, RCUiCatalogViewClass))
#define RC_UI_IS_CATALOG_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_CATALOG_VIEW))
#define RC_UI_IS_CATALOG_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_CATALOG_VIEW))
#define RC_UI_CATALOG_VIEW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_CATALOG_VIEW, \
    RCUiCatalogView))

#define RC_UI_TYPE_PLAYLIST_VIEW (rc_ui_playlist_view_get_type())
#define RC_UI_PLAYLIST_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_PLAYLIST_VIEW, RCUiPlaylistView))
#define RC_UI_PLAYLIST_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
    RC_UI_TYPE_PLAYLIST_VIEW, RCUiPlaylistViewClass))
#define RC_UI_IS_PLAYLIST_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_PLAYLIST_VIEW))
#define RC_UI_IS_PLAYLIST_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_PLAYLIST_VIEW))
#define RC_UI_PLAYLIST_VIEW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_PLAYLIST_VIEW, \
    RCUiPlaylistView))

/**
 * RCUiListViewPlaylistColumnFlags:
 * @RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST: the artist column
 * @RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM: the album column
 * @RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK: the track number column
 * @RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR: the year column
 * @RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE: the file type (format) column
 *
 * The enum type for control the visibility of some columns in the playlist.
 */

typedef enum {
    RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST = 1<<0,
    RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM = 1<<1,
    RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK = 1<<2,
    RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR = 1<<3,
    RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE = 1<<4,
    RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING = 1<<5
}RCUiListViewPlaylistColumnFlags;

typedef struct _RCUiCatalogView RCUiCatalogView;
typedef struct _RCUiPlaylistView RCUiPlaylistView;
typedef struct _RCUiCatalogViewClass RCUiCatalogViewClass;
typedef struct _RCUiPlaylistViewClass RCUiPlaylistViewClass;

/**
 * RCUiCatalogView:
 *
 * The data structure used for #RCUiCatalogView class.
 */

struct _RCUiCatalogView {
    /*< private >*/
    GtkTreeView parent;
};

/**
 * RCUiPlaylistView:
 *
 * The data structure used for #RCUiPlaylistView class.
 */

struct _RCUiPlaylistView {
    /*< private >*/
    GtkTreeView parent;
};

/**
 * RCUiCatalogViewClass:
 *
 * #RCUiCatalogViewClass class.
 */

struct _RCUiCatalogViewClass {
    /*< private >*/
    GtkTreeViewClass parent_class;
};

/**
 * RCUiPlaylistViewClass:
 *
 * #RCUiPlaylistViewClass class.
 */

struct _RCUiPlaylistViewClass {
    /*< private >*/
    GtkTreeViewClass parent_class;
};

/*< private >*/
GType rc_ui_catalog_view_get_type();
GType rc_ui_playlist_view_get_type();

/*< public >*/
GtkWidget *rc_ui_listview_get_catalog_widget();
GtkWidget *rc_ui_listview_get_playlist_widget();
void rc_ui_listview_catalog_set_pango_attributes(const PangoAttrList *list);
void rc_ui_listview_playlist_set_pango_attributes(const PangoAttrList *list);
void rc_ui_listview_catalog_select(GtkTreeIter *iter);
void rc_ui_listview_playlist_select(GtkTreeIter *iter);
gboolean rc_ui_listview_catalog_get_cursor(GtkTreeIter *iter);
gboolean rc_ui_listview_playlist_get_cursor(GtkTreeIter *iter);
GtkTreeModel *rc_ui_listview_catalog_get_model();
GtkTreeModel *rc_ui_listview_playlist_get_model();
void rc_ui_listview_catalog_new_playlist();
void rc_ui_listview_catalog_rename_playlist();
void rc_ui_listview_catalog_delete_items();
void rc_ui_listview_playlist_select_all();
void rc_ui_listview_playlist_delete_items();
void rc_ui_listview_playlist_refresh();
void rc_ui_listview_playlist_set_column_display_mode(gboolean mode);
gboolean rc_ui_listview_playlist_get_column_display_mode();
void rc_ui_listview_playlist_set_title_format(const gchar *format);
void rc_ui_listview_playlist_set_enabled_columns(guint column_flags,
    guint enable_flags);
guint rc_ui_listview_playlist_get_enabled_columns();
void rc_ui_listview_refresh();

G_END_DECLS

#endif

