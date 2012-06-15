/*
 * RhythmCat UI Player Module
 * The main UI for the player.
 *
 * rc-ui-player.c
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

#include "rc-ui-player.h"
#include "rc-ui-window.h"
#include "rc-ui-listview.h"
#include "rc-ui-listmodel.h"
#include "rc-ui-menu.h"
#include "rc-ui-dialog.h"
#include "rc-ui-slabel.h"
#include "rc-ui-spectrum.h"
#include "rc-ui-img-icon.xpm"
#include "rc-ui-unset-star-inline.h"
#include "rc-ui-no-star-inline.h"
#include "rc-ui-set-star-inline.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-player
 * @Short_description: The main UI of the player
 * @Title: Main UI
 * @Include: rc-ui-player.h
 *
 * This module provides the main UI of the player, it shows the main
 * player window and the widgets inside for the player.
 */

#define RC_UI_PLAYER_COVER_IMAGE_SIZE 160

#define RC_UI_PLAYER_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RC_UI_TYPE_PLAYER, RCUiPlayerPrivate)

typedef struct RCUiPlayerPrivate
{
    GtkApplication *app;
    GtkWidget *main_window;
    GtkUIManager *ui_manager;
    GtkStatusIcon *tray_icon;
    GdkPixbuf *icon_pixbuf;
}RCUiPlayerPrivate;

enum
{
    SIGNAL_READY,
    SIGNAL_LAST
};

static GObject *ui_player_instance = NULL;
static gpointer rc_ui_player_parent_class = NULL;
static gint ui_player_signals[SIGNAL_LAST] = {0};

static void rc_ui_player_tray_icon_popup(GtkStatusIcon *icon, guint button,
    guint activate_time, gpointer data)  
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return;
    gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(priv->ui_manager,
        "/TrayPopupMenu")), NULL, NULL, gtk_status_icon_position_menu,
        icon, 3, activate_time);
}

static void rc_ui_player_tray_icon_activated(GtkStatusIcon *icon,
    gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    gboolean visible;
    if(priv==NULL) return;
    if(!rclib_settings_get_boolean("MainUI", "MinimizeToTray", NULL))
    {
        gtk_window_present(GTK_WINDOW(priv->main_window));
        return;
    }
    g_object_get(priv->main_window, "visible", &visible, NULL);
    if(visible)
    {
        gtk_widget_hide(priv->main_window);
    }
    else
    {
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(priv->main_window),
            FALSE);
        gtk_window_deiconify(GTK_WINDOW(priv->main_window));
        gtk_window_present(GTK_WINDOW(priv->main_window));
    }
        
}

static void rc_ui_player_finalize(GObject *object)
{
    RCUiPlayerPrivate *priv = RC_UI_PLAYER_GET_PRIVATE(RC_UI_PLAYER(object));
    GList *window_list = NULL;
    if(priv->main_window!=NULL)
        gtk_widget_destroy(priv->main_window);
    if(priv->ui_manager!=NULL)
        g_object_unref(priv->ui_manager);
    if(priv->tray_icon!=NULL)
        g_object_unref(priv->tray_icon);
    if(priv->icon_pixbuf!=NULL)
        g_object_unref(priv->icon_pixbuf);
    if(priv->app!=NULL)
    {
        window_list = gtk_application_get_windows(priv->app);
        g_list_foreach(window_list, (GFunc)gtk_widget_destroy, NULL);
        g_object_unref(priv->app);
    }
    else
        gtk_main_quit();
    G_OBJECT_CLASS(rc_ui_player_parent_class)->finalize(object);
}

static GObject *rc_ui_player_constructor(GType type,
    guint n_construct_params, GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(ui_player_instance!=NULL) return ui_player_instance;
    retval = G_OBJECT_CLASS(rc_ui_player_parent_class)->constructor(
        type, n_construct_params, construct_params);
    ui_player_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&ui_player_instance);
    return retval;
}

static void rc_ui_player_class_init(RCUiPlayerClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_player_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_player_finalize;
    object_class->constructor = rc_ui_player_constructor;
    g_type_class_add_private(klass, sizeof(RCUiPlayerPrivate));
    
    /**
     * RCUiPlayer::ready:
     * @player: the #RCUiPlayer that received the signal
     *
     * The ::ready signal is emitted when the main UI is ready.
     */
    ui_player_signals[SIGNAL_READY] = g_signal_new("ready",
        RC_UI_TYPE_PLAYER, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCUiPlayerClass, ready), NULL, NULL,
        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE, NULL);
}

static void rc_ui_player_instance_init(RCUiPlayer *ui)
{
    RCUiPlayerPrivate *priv = RC_UI_PLAYER_GET_PRIVATE(ui);
    GdkPixbuf *pixbuf;
    priv->icon_pixbuf = gdk_pixbuf_new_from_xpm_data(
        (const gchar **)&ui_image_icon);
    gtk_icon_theme_add_builtin_icon("RhythmCatIcon", 128, priv->icon_pixbuf);
    pixbuf = gdk_pixbuf_new_from_inline(-1, rc_ui_unset_star_inline, FALSE,
        NULL);
    gtk_icon_theme_add_builtin_icon("RCUnsetStar", 16, pixbuf);
    g_object_unref(pixbuf);   
    pixbuf = gdk_pixbuf_new_from_inline(-1, rc_ui_no_star_inline, FALSE,
        NULL);
    gtk_icon_theme_add_builtin_icon("RCNoStar", 16, pixbuf);
    g_object_unref(pixbuf);
    pixbuf = gdk_pixbuf_new_from_inline(-1, rc_ui_set_star_inline, FALSE,
        NULL);
    gtk_icon_theme_add_builtin_icon("RCSetStar", 16, pixbuf);
    g_object_unref(pixbuf);
    priv->tray_icon = gtk_status_icon_new_from_pixbuf(priv->icon_pixbuf);
    priv->main_window = rc_ui_main_window_get_widget();
    priv->ui_manager = rc_ui_menu_get_ui_manager();
    g_signal_connect(priv->tray_icon, "activate", 
        G_CALLBACK(rc_ui_player_tray_icon_activated), priv);
    g_signal_connect(priv->tray_icon, "popup-menu",
        G_CALLBACK(rc_ui_player_tray_icon_popup), priv);
    g_signal_emit(ui, ui_player_signals[SIGNAL_READY], 0);
}

GType rc_ui_player_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo ui_info = {
        .class_size = sizeof(RCUiPlayerClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_player_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiPlayer),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_player_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCUiPlayer"), &ui_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_ui_player_init:
 * @app: the #GtkApplication for the program
 *
 * Initialize the main UI of the player.
 */

void rc_ui_player_init(GtkApplication *app)
{
    RCUiPlayerPrivate *priv;
    g_message("Loading main UI....");
    if(ui_player_instance!=NULL)
    {
        g_warning("Main UI is already initialized!");
        return;
    }
    ui_player_instance = g_object_new(RC_UI_TYPE_PLAYER, NULL);
    priv = RC_UI_PLAYER_GET_PRIVATE(ui_player_instance);
    if(priv!=NULL && app!=NULL)
    {
        priv->app = g_object_ref(app);
        gtk_window_set_application(GTK_WINDOW(priv->main_window),
            priv->app);
    }
    g_message("Main UI loaded.");
}

/**
 * rc_ui_player_exit:
 *
 * Unload the running instance of the main UI.
 */

void rc_ui_player_exit()
{
    if(ui_player_instance!=NULL)
        g_object_unref(ui_player_instance);
    else
        return;
    ui_player_instance = NULL;
    g_message("Main UI exited.");
}

/**
 * rc_ui_player_get_instance:
 *
 * Get the running #RCUiPlayer instance.
 *
 * Returns: (transfer none): The running instance.
 */

GObject *rc_ui_player_get_instance()
{
    return ui_player_instance;
}

/**
 * rc_ui_player_signal_connect:
 * @name: the name of the signal
 * @callback: (scope call): the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCUiPlayer object.
 *
 * Returns: The handler ID.
 */

gulong rc_ui_player_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(ui_player_instance==NULL) return 0;
    return g_signal_connect(ui_player_instance, name, callback, data);
}

/**
 * rc_ui_player_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running #RCUiPlayer instance so it
 * will not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rc_ui_player_signal_disconnect(gulong handler_id)
{
    if(ui_player_instance==NULL) return;
    g_signal_handler_disconnect(ui_player_instance, handler_id);
}

/**
 * rc_ui_player_get_main_window:
 *
 * Get the main window of this player.
 *
 * Returns: (transfer none): The main window.
 */

GtkWidget *rc_ui_player_get_main_window()
{
    RCUiPlayerPrivate *priv = NULL;
    if(ui_player_instance==NULL) return NULL;
    priv = RC_UI_PLAYER_GET_PRIVATE(ui_player_instance);
    if(priv==NULL) return NULL;
    return priv->main_window;
}

/**
 * rc_ui_player_get_icon_image:
 *
 * Get the icon image of this player.
 *
 * Returns: (transfer none): The icon image of this player.
 */

GdkPixbuf *rc_ui_player_get_icon_image()
{
    RCUiPlayerPrivate *priv = NULL;
    if(ui_player_instance==NULL) return NULL;
    priv = RC_UI_PLAYER_GET_PRIVATE(ui_player_instance);
    if(priv==NULL) return NULL;
    return priv->icon_pixbuf;
}

/**
 * rc_ui_player_get_tray_icon:
 *
 * Get the tray icon of this player.
 *
 * Returns: (transfer none): The tray icon.
 */

GtkStatusIcon *rc_ui_player_get_tray_icon()
{
    RCUiPlayerPrivate *priv = NULL;
    if(ui_player_instance==NULL) return NULL;
    priv = RC_UI_PLAYER_GET_PRIVATE(ui_player_instance);
    if(priv==NULL) return NULL;
    return priv->tray_icon;
}
    

