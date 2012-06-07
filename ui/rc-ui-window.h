/*
 * RhythmCat UI Main Window Header Declaration
 *
 * rc-ui-window.h
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

#ifndef HAVE_RC_UI_WINDOW_H
#define HAVE_RC_UI_WINDOW_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

#define RC_UI_TYPE_MAIN_WINDOW (rc_ui_main_window_get_type())
#define RC_UI_MAIN_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_MAIN_WINDOW, RCUiMainWindow))
#define RC_UI_MAIN_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RC_UI_TYPE_MAIN_WINDOW, RCUiMainWindowClass))
#define RC_UI_IS_MAIN_WINDOW(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RC_UI_TYPE_MAIN_WINDOW))
#define RC_UI_IS_MAIN_WINDOW_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RC_UI_TYPE_MAIN_WINDOW))
#define RC_UI_MAIN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RC_UI_TYPE_MAIN_WINDOW, RCUiMainWindowClass))

typedef struct _RCUiMainWindow RCUiMainWindow;
typedef struct _RCUiMainWindowClass RCUiMainWindowClass;

/**
 * RCUiMainWindow:
 *
 * The data structure used for #RCUiMainWindow class.
 */

struct _RCUiMainWindow {
    /*< private >*/
    GtkWindow parent;
};

/**
 * RCUiMainWindowClass:
 *
 * #RCUiMainWindowClass class.
 */

struct _RCUiMainWindowClass {
    /*< private >*/
    GtkWindowClass parent_class;
    void (*keep_above_changed)(RCUiMainWindow *window, gboolean state);
};

/*< private >*/
GType rc_ui_main_window_get_type();

/*< public >*/
GtkWidget *rc_ui_main_window_get_widget();
void rc_ui_main_window_show();
GdkPixbuf *rc_ui_main_window_get_default_cover_image();
void rc_ui_main_window_present_main_window();
void rc_ui_main_window_set_keep_above_state(gboolean state);
void rc_ui_main_window_cover_image_set_visible(gboolean visible);
gboolean rc_ui_main_window_cover_image_get_visible();
void rc_ui_main_window_lyric_labels_set_visible(gboolean visible);
gboolean rc_ui_main_window_lyric_labels_get_visible();
void rc_ui_main_window_spectrum_set_visible(gboolean visible);
gboolean rc_ui_main_window_spectrum_get_visible();
void rc_ui_main_window_playlist_scrolled_window_set_horizontal_policy(
    gboolean state);

G_END_DECLS

#endif

