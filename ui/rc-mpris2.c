/*
 * RhythmCat MPRIS 2.1 Support Declaration
 * Show dialogs in the player.
 *
 * rc-mpris2.c
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
 
#include "rc-mpris2.h"
#include "rc-ui-player.h"

static gchar *rc_mpris2_supported_uri_schemes[] = {"file", "http", NULL};
static gchar *rc_mpris2_supported_mime_types[] = {"application/ogg",
    "audio/x-vorbis+ogg", "audio/x-flac", NULL};

static void rc_mpris2_dbus_raise_cb(RCLibDBus *dbus, gpointer data)
{
    GtkWidget *window = rc_ui_player_get_main_window();
    if(window!=NULL)
        gtk_window_present(GTK_WINDOW(window));
}

/**
 * rc_mpris2_init:
 *
 * Initialize the MPRIS2 support for the player.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rc_mpris2_init()
{
    rclib_dbus_init("RhythmCat2", "can-quit", FALSE, "can-raise", TRUE,
        "has-track-list", FALSE, "identity", "RhythmCat2",
        "supported-uri-schemes", rc_mpris2_supported_uri_schemes,
        "supported-mime-types", rc_mpris2_supported_mime_types, NULL);    
    rclib_dbus_signal_connect("raise", G_CALLBACK(rc_mpris2_dbus_raise_cb),
        NULL);
    return TRUE;
}

/**
 * rc_mpris2_exit:
 *
 * Unload MPRIS2 support.
 */

void rc_mpris2_exit()
{
    rclib_dbus_exit();
}

