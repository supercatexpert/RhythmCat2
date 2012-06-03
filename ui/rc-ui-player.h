/*
 * RhythmCat UI Player Header Declaration
 *
 * rc-ui-player.h
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

#ifndef HAVE_RC_UI_PLAYER_H
#define HAVE_RC_UI_PLAYER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

#define RC_UI_PLAYER_TYPE (rc_ui_player_get_type())
#define RC_UI_PLAYER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_PLAYER_TYPE, RCUiPlayer))
#define RC_UI_PLAYER_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RC_UI_PLAYER_TYPE, RCUiPlayerClass))
#define RC_UI_IS_PLAYER(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RC_UI_PLAYER_TYPE))
#define RC_UI_IS_PLAYER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RC_UI_PLAYER_TYPE))
#define RC_UI_PLAYER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RC_UI_PLAYER_TYPE, RCUiPlayerClass))

typedef struct _RCUiPlayer RCUiPlayer;
typedef struct _RCUiPlayerClass RCUiPlayerClass;

/**
 * RCUiPlayer:
 *
 * The data structure used for #RCUiPlayer class.
 */

struct _RCUiPlayer {
    /*< private >*/
    GObject parent;
};

/**
 * RCUiPlayerClass:
 *
 * #RCUiPlayerClass class.
 */

struct _RCUiPlayerClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*ready)(RCUiPlayer *player);
};

/*< private >*/
GType rc_ui_player_get_type();

/*< public >*/
void rc_ui_player_init(GtkApplication *app);
void rc_ui_player_exit();
GObject *rc_ui_player_get_instance();
gulong rc_ui_player_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rc_ui_player_signal_disconnect(gulong handler_id);
GtkWidget *rc_ui_player_get_main_window();
GdkPixbuf *rc_ui_player_get_icon_image();
GtkStatusIcon *rc_ui_player_get_tray_icon();

G_END_DECLS

#endif

