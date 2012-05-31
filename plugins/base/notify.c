/*
 * Notify Popups
 * Show the music information in popups when the player starts playing.
 *
 * notify.c
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
#include <gtk/gtk.h>
#include <rclib.h>
#include <rc-ui-player.h>

#define NOTIFY_DBUS_NAME           "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_INTERFACE "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_OBJECT    "/org/freedesktop/Notifications"

typedef struct RCPluginNotifyPrivate
{
    GDBusProxy *proxy;
    guint32 id;
    guint xid;
    gulong uri_changed_id;
    gulong tag_found_id;
}RCPluginNotifyPrivate;

static RCPluginNotifyPrivate notify_priv = {0};
static const gchar *notify_app_name = "RhythmCat";

static gboolean rc_plugin_notify_show(RCPluginNotifyPrivate *priv,
    const gchar *title, const gchar *body, GdkPixbuf *pixbuf,
    gint timeout)
{
    GError *error = NULL;
    gint width, height, rowstride, bits_per_sample, channels;
    guchar *image;
    gboolean has_alpha;
    gsize image_len;
    GVariant *image_variant = NULL;
    GVariantBuilder actions_builder, hints_builder;
    GVariant *result;
    if(priv==NULL) return FALSE;
    if(pixbuf!=NULL)
    {
        g_object_get(G_OBJECT(pixbuf), "width", &width, "height", &height, 
            "rowstride", &rowstride, "n-channels", &channels, 
            "bits-per-sample", &bits_per_sample, "pixels", &image, 
            "has-alpha", &has_alpha, NULL);
        image_len = (height-1) * rowstride + width *
            ((channels * bits_per_sample + 7)/8);    
        image_variant = g_variant_new("(iiibii@ay)", width, height,
            rowstride, has_alpha, bits_per_sample, channels,
            g_variant_new_from_data(G_VARIANT_TYPE("ay"), image, image_len,
            TRUE, (GDestroyNotify)g_object_unref, g_object_ref(pixbuf)));            
    }
    g_variant_builder_init(&actions_builder, G_VARIANT_TYPE("as"));
    g_variant_builder_init(&hints_builder, G_VARIANT_TYPE ("a{sv}"));
    if(image_variant!=NULL)
    {
        g_variant_builder_add(&hints_builder, "{sv}", "image-data",
            image_variant);
    }
    if(priv->xid>0)
    {
        g_variant_builder_add(&hints_builder, "{sv}", "window-xid",
            g_variant_new("(u)", priv->xid));
    }
    result = g_dbus_proxy_call_sync(priv->proxy, "Notify",
        g_variant_new("(susssasa{sv}i)", notify_app_name, priv->id, "",
        title ? title : "", body ? body : "", &actions_builder,
        &hints_builder, timeout), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
    if(error!=NULL)
    {
        g_warning("NotifyPopups: Cannot send D-Bus message: %s",
            error->message);
        g_error_free(error);
        if(result!=NULL) g_variant_unref(result);
        return FALSE;
    }
    if(result==NULL) return FALSE;
    if(!g_variant_is_of_type(result, G_VARIANT_TYPE("(u)")))
    {
        g_variant_unref(result);
        g_warning("NotifyPopups: Unexpected reply type!");
        return FALSE;
    }
    g_variant_get(result, "(u)", &(priv->id));
    g_variant_unref(result);
    return TRUE;
}

static void rc_plugin_notify_metadata_changed_cb(RCLibCore *core,
    const RCLibCoreMetadata *metadata, const gchar *uri, gpointer data)
{
    RCPluginNotifyPrivate *priv = (RCPluginNotifyPrivate *)data;
    GdkPixbufLoader *loader;
    gchar *filename;
    gchar *rtitle = NULL;
    gchar *body;
    GdkPixbuf *pixbuf = NULL;
    const gchar *rartist, *ralbum;
    GError *error = NULL;
    if(data==NULL) return;
    if(metadata->title!=NULL && strlen(metadata->title)>0)
        rtitle = g_strdup(metadata->title);
    else
    {
        filename = g_filename_from_uri(uri, NULL, NULL);
        if(filename!=NULL)
            rtitle = rclib_tag_get_name_from_fpath(filename);
        g_free(filename);
    }
    if(rtitle==NULL) rtitle = g_strdup(_("Unknown Title"));
    if(metadata->artist && strlen(metadata->artist)>0)
        rartist = metadata->artist;
    else
        rartist = _("Unknown Artist");
    if(metadata->album && strlen(metadata->album)>0)
        ralbum = metadata->album;
    else
        ralbum = _("Unknown Album");
    body = g_strdup_printf("%s - %s", rartist, ralbum);
    if(metadata->image!=NULL)
    {
        loader = gdk_pixbuf_loader_new();
        if(gdk_pixbuf_loader_write(loader, metadata->image->data,
            metadata->image->size, &error))
        {
            pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
            if(pixbuf!=NULL) g_object_ref(pixbuf);
        }
        else
        {
            g_warning("NotifyPopups: Cannot load cover image "
                "from GstBuffer: %s", error->message);
            g_error_free(error);
        }
        g_object_unref(loader);
    }
    rc_plugin_notify_show(priv, rtitle, body, pixbuf, 5000);
    g_free(rtitle);
    g_free(body);
    if(pixbuf!=NULL) g_object_unref(pixbuf);
}

static void rc_plugin_notify_uri_changed_cb(RCLibCore *core,
    const gchar *uri, gpointer data)
{
    RCPluginNotifyPrivate *priv = (RCPluginNotifyPrivate *)data;
    gchar *filename;
    gchar *rtitle;
    if(data==NULL) return;
    filename = g_filename_from_uri(uri, NULL, NULL);
    if(filename==NULL) return;
    rtitle = rclib_tag_get_name_from_fpath(filename);
    g_free(filename);
    rc_plugin_notify_show(priv, rtitle, "", NULL, 5000);
    g_free(rtitle);
}

static gboolean rc_plugin_notify_load(RCLibPluginData *plugin)
{
    RCPluginNotifyPrivate *priv = &notify_priv;
    GDBusConnection *connection;
    GError *error = NULL;
    GtkStatusIcon *status_icon;
    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if(error!=NULL)
    {
        g_warning("NotifyPopups: Cannot connect to D-Bus: %s",
            error->message);
        g_error_free(error);
        if(connection!=NULL) g_object_unref(connection);
        return FALSE;
    }
    priv->proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE,
        NULL, NOTIFY_DBUS_NAME, NOTIFY_DBUS_CORE_OBJECT,
        NOTIFY_DBUS_CORE_INTERFACE, NULL, &error);
    g_object_unref(connection);
    if(error!=NULL)
    {
        g_warning("NotifyPopups: Cannot connect to D-Bus proxy: %s",
            error->message);
        g_error_free(error);
        if(priv->proxy!=NULL) g_object_unref(priv->proxy);
        priv->proxy = NULL;
        return FALSE;
    }
    status_icon = rc_ui_player_get_tray_icon();
    if(status_icon!=NULL)
        priv->xid = gtk_status_icon_get_x11_window_id(status_icon);
    else
        priv->xid = 0;
    rc_plugin_notify_show(priv, _("Welcome to RhythmCat"),
        _("Welcome to RhythmCat 2.0, the music player with plug-in support!"),
        NULL, 5000);
    priv->tag_found_id = rclib_core_signal_connect("tag-found",
        G_CALLBACK(rc_plugin_notify_metadata_changed_cb), priv);
    priv->uri_changed_id = rclib_core_signal_connect("uri-changed",
        G_CALLBACK(rc_plugin_notify_uri_changed_cb), priv);
    return TRUE;
}


static gboolean rc_plugin_notify_unload(RCLibPluginData *plugin)
{
    RCPluginNotifyPrivate *priv = &notify_priv;
    if(priv->proxy!=NULL) g_object_unref(priv->proxy);
    priv->proxy = NULL;
    if(priv->tag_found_id>0)
        rclib_core_signal_disconnect(priv->tag_found_id);
    priv->tag_found_id = 0;
    if(priv->uri_changed_id>0)
        rclib_core_signal_disconnect(priv->uri_changed_id);
    priv->uri_changed_id = 0;
    return TRUE;
}

static gboolean rc_plugin_notify_init(RCLibPluginData *plugin)
{

    return TRUE;
}

static void rc_plugin_notify_destroy(RCLibPluginData *plugin)
{

}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = "rc2-native-notify-popup",
    .name = N_("Notify Popups"),
    .version = "0.5",
    .description = N_("Show the music information in popups when "
        "the player starts playing."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_notify_load,
    .unload = rc_plugin_notify_unload,
    .configure = NULL,
    .destroy = rc_plugin_notify_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_notify_init, rcplugin_info);


