/*
 * GNOME Multimedia Key Support
 * Get multimedia key event and control the player.
 *
 * mediakey.c
 * This file is part of RhythmCat
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

#include <glib.h>
#include <gio/gio.h>
#include <rclib.h>

#define DBUS_MEDIAKEY_NAME "org.gnome.SettingsDaemon"
#define DBUS_MEDIAKEY_OBJECT_PATH "/org/gnome/SettingsDaemon/MediaKeys"
#define DBUS_MEDIAKEY_INTERFACE "org.gnome.SettingsDaemon.MediaKeys"

#define DBUS_MEDIAKEY_FBACK_NAME "org.gnome.SettingsDaemon"
#define DBUS_MEDIAKEY_FBACK_OBJECT_PATH "/org/gnome/SettingsDaemon"
#define DBUS_MEDIAKEY_FBACK_INTERFACE "org.gnome.SettingsDaemon"

typedef struct RCPluginMediaKeyPrivate
{
    GDBusProxy *proxy;
}RCPluginMediaKeyPrivate;

static RCPluginMediaKeyPrivate mediakey_priv = {0};

static void rc_plugin_mediakey_proxy_signal_cb(GDBusProxy *proxy,
    gchar *sender_name, gchar *signal_name, GVariant *parameters,
    gpointer data)
{
    gchar *application, *key;
    GstState state;
    gint64 pos, len;
    RCLibPlayerRepeatMode repeat_mode;
    RCLibPlayerRandomMode random_mode;
    g_variant_get(parameters, "(ss)", &application, &key);
    if(g_strcmp0(application, "RhythmCat2")==0)
    {
        if(g_strcmp0("Play", key) == 0)
        {
            rclib_core_get_state(&state, NULL, 0);    
            if(state==GST_STATE_PLAYING)
                rclib_core_pause();
            else
                rclib_core_play();
        }
        else if(g_strcmp0("Previous", key)==0)
            rclib_player_play_prev(FALSE, TRUE, FALSE);
        else if(g_strcmp0("Next", key)==0)
            rclib_player_play_next(FALSE, TRUE, FALSE);
        else if(g_strcmp0("Stop", key)==0)
            rclib_core_stop();
        else if(g_strcmp0("Repeat", key)==0)
        {
            repeat_mode = rclib_player_get_repeat_mode();
            rclib_player_set_repeat_mode(!repeat_mode);
        }
        else if(g_strcmp0("Shuffle", key)==0)
        {
            random_mode = rclib_player_get_random_mode();
            rclib_player_set_random_mode(!random_mode);
        }
        else if(g_strcmp0("Rewind", key)==0)
        {
            pos = rclib_core_query_position();
            if(pos>5 * GST_SECOND) pos -= 5 * GST_SECOND;
            else pos = 0;
            rclib_core_set_position(pos);
        }
        else if(g_strcmp0("FastForward", key)==0)
        {
            pos = rclib_core_query_position();
            len = rclib_core_query_duration();
            pos += 5 * GST_SECOND;
            if(pos>len) pos = len;
            rclib_core_set_position(pos);
        }
    }
    g_free(application);
    g_free(key);
}

static gboolean rc_plugin_mediakey_load(RCLibPluginData *plugin)
{
    RCPluginMediaKeyPrivate *priv = &mediakey_priv;
    GDBusConnection *connection;
    GError *error = NULL;
    GVariant *variant;
    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if(error!=NULL)
    {
        g_warning("MediaKey: Cannot connect to D-Bus: %s",
            error->message);
        g_error_free(error);
        if(connection!=NULL) g_object_unref(connection);
        return FALSE;
    }
    priv->proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
        NULL, DBUS_MEDIAKEY_NAME, DBUS_MEDIAKEY_OBJECT_PATH,
        DBUS_MEDIAKEY_INTERFACE, NULL, NULL);
    if(priv->proxy==NULL)
    {
        priv->proxy = g_dbus_proxy_new_sync(connection,
            G_DBUS_PROXY_FLAGS_NONE, NULL, DBUS_MEDIAKEY_FBACK_NAME,
            DBUS_MEDIAKEY_FBACK_OBJECT_PATH, DBUS_MEDIAKEY_FBACK_INTERFACE,
            NULL, &error);
        if(error!=NULL)
        {
            g_warning("MediaKey: Unable to connect MediaKey proxy: %s",
                error->message);
            g_error_free(error);
            error = NULL;
            priv->proxy = NULL; 
        }
    }     
    g_object_unref(connection);
    if(priv->proxy==NULL) return FALSE;
    variant = g_dbus_proxy_call_sync(priv->proxy,
        "GrabMediaPlayerKeys", g_variant_new("(su)", "RhythmCat2", 0),
        G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if(variant!=NULL)
        g_variant_unref(variant);
    else
    {
        g_warning("MediaKey: Unable to call method GrabMediaPlayerKeys: %s",
            error->message);
        g_error_free(error);
        error = NULL;
        g_object_unref(priv->proxy);
        priv->proxy = NULL;
        return FALSE;
    }
    g_signal_connect(priv->proxy, "g-signal",
        G_CALLBACK(rc_plugin_mediakey_proxy_signal_cb), priv);
    return TRUE;
}


static gboolean rc_plugin_mediakey_unload(RCLibPluginData *plugin)
{
    RCPluginMediaKeyPrivate *priv = &mediakey_priv;
    if(priv->proxy!=NULL) g_object_unref(priv->proxy);
    priv->proxy = NULL;
    return TRUE;
}

static gboolean rc_plugin_mediakey_init(RCLibPluginData *plugin)
{

    return TRUE;
}

static void rc_plugin_mediakey_destroy(RCLibPluginData *plugin)
{

}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = "rc2-native-mediakey-controller",
    .name = N_("Multimedia Key Support"),
    .version = "0.5",
    .description = N_("Control the player with multimedia key in GNOME."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_mediakey_load,
    .unload = rc_plugin_mediakey_unload,
    .configure = NULL,
    .destroy = rc_plugin_mediakey_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_mediakey_init, rcplugin_info);


