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
#include <gdk/gdk.h>
#include <rclib.h>

#ifdef GDK_WINDOWING_X11
    #include <X11/Xlib.h>
    #include <X11/XF86keysym.h>
    #include <gdk/gdkx.h>
#endif

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

static gboolean rc_plugin_mediakey_gnome_initialize(
    RCPluginMediaKeyPrivate *priv)
{
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

#ifdef GDK_WINDOWING_X11

static void rc_plugin_mediakey_grab_mmkey(gint key_code, GdkWindow *root)
{
    Display *display;
    gdk_error_trap_push();

    display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    XGrabKey(display, key_code, 0, GDK_WINDOW_XID(root), True, GrabModeAsync,
        GrabModeAsync);
    XGrabKey(display, key_code, Mod2Mask, GDK_WINDOW_XID (root), True,
        GrabModeAsync, GrabModeAsync);
    XGrabKey(display, key_code, Mod5Mask, GDK_WINDOW_XID(root), True,
        GrabModeAsync, GrabModeAsync);
    XGrabKey(display, key_code, LockMask, GDK_WINDOW_XID (root), True,
        GrabModeAsync, GrabModeAsync);
    XGrabKey(display, key_code, Mod2Mask | Mod5Mask, GDK_WINDOW_XID(root),
        True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, key_code, Mod2Mask | LockMask, GDK_WINDOW_XID(root),
        True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, key_code, Mod5Mask | LockMask, GDK_WINDOW_XID(root),
        True, GrabModeAsync, GrabModeAsync);
    XGrabKey(display, key_code, Mod2Mask | Mod5Mask | LockMask,
        GDK_WINDOW_XID(root), True, GrabModeAsync, GrabModeAsync);

    gdk_flush ();

    if(gdk_error_trap_pop ())
    {
        g_warning("Cannot grabbing key!");
    }
}

static void rc_plugin_mediakey_ungrab_mmkey(gint key_code, GdkWindow *root)
{
    Display *display;
    gdk_error_trap_push();

    display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    XUngrabKey (display, key_code, 0, GDK_WINDOW_XID(root));
    XUngrabKey (display, key_code, Mod2Mask, GDK_WINDOW_XID(root));
    XUngrabKey (display, key_code, Mod5Mask, GDK_WINDOW_XID(root));
    XUngrabKey (display, key_code, LockMask, GDK_WINDOW_XID(root));
    XUngrabKey (display, key_code, Mod2Mask | Mod5Mask, GDK_WINDOW_XID(root));
    XUngrabKey (display, key_code, Mod2Mask | LockMask, GDK_WINDOW_XID(root));
    XUngrabKey (display, key_code, Mod5Mask | LockMask, GDK_WINDOW_XID(root));
    XUngrabKey (display, key_code, Mod2Mask | Mod5Mask | LockMask,
        GDK_WINDOW_XID (root));

    gdk_flush ();

    if(gdk_error_trap_pop ())
    {
        g_warning("Cannot ungrab key!");
    }
}

static GdkFilterReturn rc_plugin_mediakey_filter_mmkeys(GdkXEvent *xevent,
    GdkEvent *event, gpointer data)
{
    GstState state;
    XEvent *xev;
    XKeyEvent *key;
    Display *display;
    xev = (XEvent *) xevent;
    if(xev->type != KeyPress)
    {
        return GDK_FILTER_CONTINUE;
    }

    key = (XKeyEvent *) xevent;

    display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

    if(XKeysymToKeycode(display, XF86XK_AudioPlay)==key->keycode)
    {
        rclib_core_get_state(&state, NULL, 0);    
        if(state==GST_STATE_PLAYING)
            rclib_core_pause();
        else
            rclib_core_play();
        return GDK_FILTER_REMOVE;
    }
    else if(XKeysymToKeycode (display, XF86XK_AudioPause)==key->keycode)
    {
        rclib_core_pause();
        return GDK_FILTER_REMOVE;
    }
    else if(XKeysymToKeycode(display, XF86XK_AudioStop)==key->keycode)
    {
        rclib_core_stop();
        return GDK_FILTER_REMOVE;
    }
    else if(XKeysymToKeycode(display, XF86XK_AudioPrev)==key->keycode)
    {
        rclib_player_play_prev(FALSE, TRUE, FALSE);
        return GDK_FILTER_REMOVE;
    }
    else if(XKeysymToKeycode(display, XF86XK_AudioNext)==key->keycode)
    {
        rclib_player_play_next(FALSE, TRUE, FALSE);
        return GDK_FILTER_REMOVE;
    }
    else
    {
        return GDK_FILTER_CONTINUE;
    }
}

static void rc_plugin_mediakey_mmkeys_grab(gboolean grab)
{
    gint keycodes[] = {0, 0, 0, 0, 0};
    GdkDisplay *display;
    GdkScreen *screen;
    GdkWindow *root;
    guint i, j;

    display = gdk_display_get_default();
    keycodes[0] = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(display),
        XF86XK_AudioPlay);
    keycodes[1] = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(display),
        XF86XK_AudioStop);
    keycodes[2] = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(display),
        XF86XK_AudioPrev);
    keycodes[3] = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(display),
        XF86XK_AudioNext);
    keycodes[4] = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(display),
        XF86XK_AudioPause);

    for(i=0;i<gdk_display_get_n_screens(display); i++)
    {
        screen = gdk_display_get_screen (display, i);

        if(screen!=NULL)
        {
            root = gdk_screen_get_root_window (screen);

            for(j=0;j<G_N_ELEMENTS(keycodes);j++)
            {
                if(keycodes[j]!=0)
                {
                    if(grab)
                        rc_plugin_mediakey_grab_mmkey(keycodes[j], root);
                    else
                        rc_plugin_mediakey_ungrab_mmkey(keycodes[j], root);
                }

            }

            if(grab)
            {
                gdk_window_add_filter(root, rc_plugin_mediakey_filter_mmkeys,
                    NULL);
            }
            else
            {
                gdk_window_remove_filter(root,
                    rc_plugin_mediakey_filter_mmkeys, NULL);
            }
        }
    }
}

#endif


static gboolean rc_plugin_mediakey_load(RCLibPluginData *plugin)
{
    RCPluginMediaKeyPrivate *priv = &mediakey_priv;

    if(rc_plugin_mediakey_gnome_initialize(priv)) return TRUE;

    #ifdef GDK_WINDOWING_X11
        rc_plugin_mediakey_mmkeys_grab(TRUE);
        g_message("Use X11 multimedia key grab as fallback support.");
        return TRUE;
    #endif
   
    return FALSE;
}


static gboolean rc_plugin_mediakey_unload(RCLibPluginData *plugin)
{
    RCPluginMediaKeyPrivate *priv = &mediakey_priv;
    if(priv->proxy!=NULL) g_object_unref(priv->proxy);
    priv->proxy = NULL;

    #ifdef GDK_WINDOWING_X11
        rc_plugin_mediakey_mmkeys_grab(FALSE);
        return TRUE;
    #endif

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


