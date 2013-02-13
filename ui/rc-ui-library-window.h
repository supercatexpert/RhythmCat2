/*
 * RhythmCat UI Library Window Header Declaration
 *
 * rc-ui-library.h
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

#ifndef HAVE_RC_UI_LIBRARY_WINDOW_H
#define HAVE_RC_UI_LIBRARY_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

#define RC_UI_TYPE_LIBRARY_WINDOW (rc_ui_library_window_get_type())
#define RC_UI_LIBRARY_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_LIBRARY_WINDOW, RCUiLibraryWindow))
#define RC_UI_LIBRARY_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RC_UI_TYPE_LIBRARY_WINDOW, RCUiLibraryWindowClass))
#define RC_UI_IS_LIBRARY_WINDOW(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RC_UI_TYPE_LIBRARY_WINDOW))
#define RC_UI_IS_LIBRARY_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RC_UI_TYPE_LIBRARY_WINDOW))
#define RC_UI_LIBRARY_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RC_UI_TYPE_LIBRARY_WINDOW, RCUiLibraryWindowClass))

typedef struct _RCUiLibraryWindow RCUiLibraryWindow;
typedef struct _RCUiLibraryWindowClass RCUiLibraryWindowClass;
typedef struct _RCUiLibraryWindowPrivate RCUiLibraryWindowPrivate;

/**
 * RCUiLibraryWindowSearchType:
 * @RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_ALL: search all fields
 * @RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_TITLE: search in titles
 * @RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_ARTIST: search in artists
 * @RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_ALBUM: search in albums
 * 
 * The enum type for search entry in the library window.
 */

typedef enum {
    RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_ALL = 0,
    RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_TITLE = 1,
    RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_ARTIST = 2,
    RC_UI_LIBRARY_WINDOW_SEARCH_TYPE_ALBUM = 3
}RCUiLibraryWindowSearchType;

/**
 * RCUiLibraryWindow:
 *
 * The data structure used for #RCUiLibraryWindow class.
 */

struct _RCUiLibraryWindow {
    /*< private >*/
    GtkWindow parent;
    RCUiLibraryWindowPrivate *priv;
};

/**
 * RCUiLibraryWindowClass:
 *
 * #RCUiLibraryWindowClass class.
 */

struct _RCUiLibraryWindowClass {
    /*< private >*/
    GtkWindowClass parent_class;
};

/*< private >*/
GType rc_ui_library_window_get_type();

/*< public >*/
GtkWidget *rc_ui_library_window_get_widget();
void rc_ui_library_window_show();
void rc_ui_library_window_hide();
void rc_ui_library_window_present_window();
GtkWidget *rc_ui_library_window_get_library_list_widget();
void rc_ui_library_window_set_search_type(RCUiLibraryWindowSearchType type);

G_END_DECLS

#endif
