/*
 * RhythmCat UI Library Models Header Declaration
 *
 * rc-ui-library-model.h
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

#ifndef HAVE_RC_UI_LIBRARY_VIEW_H
#define HAVE_RC_UI_LIBRARY_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

#define RC_UI_TYPE_LIBRARY_LIST_STORE (rc_ui_library_list_store_get_type())
#define RC_UI_LIBRARY_LIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_LIBRARY_LIST_STORE, RCUiLibraryListStore))
#define RC_UI_LIBRARY_LIST_STORE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_TYPE_LIBRARY_LIST_STORE, \
    RCUiLibraryListStoreClass))
#define RC_UI_IS_LIBRARY_LIST_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_LIBRARY_LIST_STORE))
#define RC_UI_IS_LIBRARY_LIST_STORE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_LIBRARY_LIST_STORE))
#define RC_UI_LIBRARY_LIST_STORE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_LIBRARY_LIST_STORE, \
    RCUiLibraryListStore))
    
#define RC_UI_TYPE_LIBRARY_PROP_STORE (rc_ui_library_prop_store_get_type())
#define RC_UI_LIBRARY_PROP_STORE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_LIBRARY_PROP_STORE, RCUiLibraryPropStore))
#define RC_UI_LIBRARY_PROP_STORE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_TYPE_LIBRARY_PROP_STORE, \
    RCUiLibraryPropStoreClass))
#define RC_UI_IS_LIBRARY_PROP_STORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_LIBRARY_PROP_STORE))
#define RC_UI_IS_LIBRARY_PROP_STORE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_LIBRARY_PROP_STORE))
#define RC_UI_LIBRARY_PROP_STORE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_LIBRARY_PROP_STORE, \
    RCUiLibraryPropStore))
    
/**
 * RCUiLibraryListColumns:
 * @RC_UI_LIBRARY_LIST_COLUMN_TYPE: the type column
 * @RC_UI_LIBRARY_LIST_COLUMN_STATE: the state column
 * @RC_UI_LIBRARY_LIST_COLUMN_FTITLE: the title column
 * @RC_UI_LIBRARY_LIST_COLUMN_TITLE: the real title column
 * @RC_UI_LIBRARY_LIST_COLUMN_ARTIST: the artist column
 * @RC_UI_LIBRARY_LIST_COLUMN_ALBUM: the album column
 * @RC_UI_LIBRARY_LIST_COLUMN_FTYPE: the file type (format) column
 * @RC_UI_LIBRARY_LIST_COLUMN_GENRE: the genre column
 * @RC_UI_LIBRARY_LIST_COLUMN_LENGTH: the time length column
 * @RC_UI_LIBRARY_LIST_COLUMN_TRACK: the track number column
 * @RC_UI_LIBRARY_LIST_COLUMN_RATING: the rating column
 * @RC_UI_LIBRARY_LIST_COLUMN_YEAR: the year column
 * @RC_UI_LIBRARY_LIST_COLUMN_PLAYING_FLAG: the playing flag column
 * @RC_UI_LIBRARY_LIST_COLUMN_LAST: the last column, do not use it
 *
 * The enum type for the columns in the library list store.
 */

typedef enum {
    RC_UI_LIBRARY_LIST_COLUMN_TYPE,
    RC_UI_LIBRARY_LIST_COLUMN_STATE,
    RC_UI_LIBRARY_LIST_COLUMN_FTITLE,
    RC_UI_LIBRARY_LIST_COLUMN_TITLE,
    RC_UI_LIBRARY_LIST_COLUMN_ARTIST,
    RC_UI_LIBRARY_LIST_COLUMN_ALBUM,
    RC_UI_LIBRARY_LIST_COLUMN_FTYPE,
    RC_UI_LIBRARY_LIST_COLUMN_GENRE,
    RC_UI_LIBRARY_LIST_COLUMN_LENGTH,
    RC_UI_LIBRARY_LIST_COLUMN_TRACK,
    RC_UI_LIBRARY_LIST_COLUMN_RATING,
    RC_UI_LIBRARY_LIST_COLUMN_YEAR,
    RC_UI_LIBRARY_LIST_COLUMN_PLAYING_FLAG,
    RC_UI_LIBRARY_LIST_COLUMN_LAST
}RCUiLibraryListColumns;

/**
 * RCUiLibraryPropColumns:
 * @RC_UI_LIBRARY_PROP_COLUMN_NAME: the property text
 * @RC_UI_LIBRARY_PROP_COLUMN_COUNT: the number of the property used
 * @RC_UI_LIBRARY_PROP_COLUMN_FLAG: the flag of the property
 * @RC_UI_LIBRARY_PROP_COLUMN_LAST: the last column, do not use it
 * 
 * The enum type for the columns in the library property store.
 */

typedef enum {
    RC_UI_LIBRARY_PROP_COLUMN_NAME,
    RC_UI_LIBRARY_PROP_COLUMN_COUNT,
    RC_UI_LIBRARY_PROP_COLUMN_FLAG,
    RC_UI_LIBRARY_PROP_COLUMN_LAST
}RCUiLibraryPropColumns;

typedef struct _RCUiLibraryListStore RCUiLibraryListStore;
typedef struct _RCUiLibraryPropStore RCUiLibraryPropStore;
typedef struct _RCUiLibraryListStoreClass RCUiLibraryListStoreClass;
typedef struct _RCUiLibraryPropStoreClass RCUiLibraryPropStoreClass;
typedef struct _RCUiLibraryListStorePrivate RCUiLibraryListStorePrivate;
typedef struct _RCUiLibraryPropStorePrivate RCUiLibraryPropStorePrivate;

/**
 * RCUiLibraryListStore:
 *
 * The data structure used for #RCUiLibraryListStore class.
 */

struct _RCUiLibraryListStore {
    /*< private >*/
    GObject parent;
    RCUiLibraryListStorePrivate *priv;
};

/**
 * RCUiLibraryPropStore:
 *
 * The data structure used for #RCUiLibraryPropStore class.
 */

struct _RCUiLibraryPropStore {
    /*< private >*/
    GObject parent;
    RCUiLibraryPropStorePrivate *priv;
};

/**
 * RCUiLibraryListStoreClass:
 *
 * #RCUiLibraryListStoreClass class.
 */

struct _RCUiLibraryListStoreClass {
    /*< private >*/
    GObjectClass parent_class;
};

/**
 * RCUiLibraryPropStoreClass:
 *
 * #RCUiLibraryPropStoreClass class.
 */

struct _RCUiLibraryPropStoreClass {
    /*< private >*/
    GObjectClass parent_class;
};

/*< private >*/
GType rc_ui_library_list_store_get_type();
GType rc_ui_library_prop_store_get_type();

/*< public >*/
GtkTreeModel *rc_ui_library_list_store_new(
    RCLibDbLibraryQueryResult *query_result);
GtkTreeModel *rc_ui_library_prop_store_new(
    RCLibDbLibraryQueryResult *base, RCLibDbQueryDataType prop_type);

G_END_DECLS

#endif
