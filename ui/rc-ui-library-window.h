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
void rc_ui_library_window_present_main_window();

G_END_DECLS

#endif
