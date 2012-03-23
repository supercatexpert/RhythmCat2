/*
 * RhythmCat UI Settings Module
 * The setting manager for the player.
 *
 * rc-ui-settings.c
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

#include "rc-ui-settings.h"
#include "rc-common.h"

typedef struct RCUiSettingsPrivate
{
    GtkWidget *settings_window;
}RCUiSettingsPrivate;

static RCUiSettingsPrivate settings_priv = {0};

static void rc_ui_settings_window_destroy_cb(GtkWidget *widget, gpointer data)
{
    RCUiSettingsPrivate *priv = &settings_priv;
    gtk_widget_destroyed(priv->settings_window, &(priv->settings_window));
}

static inline void rc_ui_settings_window_init()
{
    RCUiSettingsPrivate *priv = &settings_priv;
    priv->settings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    g_object_set(priv->settings_window, "title", _("Player Preferences"),
        "window-position", GTK_WIN_POS_CENTER, "has-resize-grip", FALSE,
        "default-width", 350, "default-height", 300, "icon-name",
        GTK_STOCK_PREFERENCES, "type-hint", GDK_WINDOW_TYPE_HINT_DIALOG,
        NULL);   

    g_signal_connect(G_OBJECT(priv->settings_window), "destroy",
        G_CALLBACK(rc_ui_settings_window_destroy_cb), NULL);
    gtk_widget_show_all(priv->settings_window);
}


/**
 * rc_ui_plugin_window_show:
 *
 * Show a player configuration window.
 */

void rc_ui_settings_window_show()
{
    RCUiSettingsPrivate *priv = &settings_priv;
    if(priv->settings_window==NULL)
        rc_ui_settings_window_init();
    else
        gtk_window_present(GTK_WINDOW(priv->settings_window));
}

/**
 * rc_ui_settings_window_destroy:
 *
 * Destroy the player configuration window.
 */

void rc_ui_settings_window_destroy()
{
    RCUiSettingsPrivate *priv = &settings_priv;
    if(priv->settings_window!=NULL)
        gtk_widget_destroy(priv->settings_window);
}



