/*
 * GNOME Multimedia Key Support
 * Get multimedia key event and control the player.
 *
 * mpris.c
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
#include <gst/gst.h>
#include <rclib.h>
#include <rc-ui-window.h>

#define DBUS_MPRIS_BUS_NAME_PREFIX "org.mpris.MediaPlayer2"
#define DBUS_MPRIS_OBJECT_NAME "/org/mpris/MediaPlayer2"

#define DBUS_MPRIS_ROOT_INTERFACE "org.mpris.MediaPlayer2"
#define DBUS_MPRIS_PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"
#define DBUS_MPRIS_TRACKLIST_INTERFACE "org.mpris.MediaPlayer2.TrackList"
#define DBUS_MPRIS_PLAYLISTS_INTERFACE "org.mpris.MediaPlayer2.Playlists"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif

#define G_LOG_DOMAIN "MPRIS-Support"
#define MPRISSUPPORT_ID "rc2-native-mpris2-controller"

typedef struct RCPluginMPRISPrivate
{
    GDBusConnection *connection;
    guint mpris_name_own_id;
    guint mpris_root_id;
    guint mpris_player_id;
    GHashTable *player_property_changes;
    guint property_emit_id;
    
    gulong state_changed_id;
    gulong uri_changed_id;
    gulong metadata_found_id;
    gulong volume_changed_id;
    gulong repeat_mode_changed_id;
    gulong random_mode_changed_id;
}RCPluginMPRISPrivate;

static RCPluginMPRISPrivate mpris_priv = {0};
static const gchar *mpris_identity = "RhythmCat2";
static const gchar * const mpris_supported_uri_schemes[] =
    {"file", "http", NULL};
static const gchar * const mpris_supported_mime_types[] =
    {"application/ogg", "audio/x-vorbis+ogg", "audio/x-flac", NULL};
static const gchar *mpris_introspection_xml =
    "<node>"
    "  <interface name='org.mpris.MediaPlayer2'>"
    "    <method name='Raise'/>"
    "    <method name='Quit'/>"
    "    <property name='CanQuit' type='b' access='read'/>"
    "    <property name='CanRaise' type='b' access='read'/>"
    "    <property name='HasTrackList' type='b' access='read'/>"
    "    <property name='Identity' type='s' access='read'/>"
    "    <property name='DesktopEntry' type='s' access='read'/>"
    "    <property name='SupportedUriSchemes' type='as' access='read'/>"
    "    <property name='SupportedMimeTypes' type='as' access='read'/>"
    "  </interface>"
    "  <interface name='org.mpris.MediaPlayer2.Player'>"
    "    <method name='Next'/>"
    "    <method name='Previous'/>"
    "    <method name='Pause'/>"
    "    <method name='PlayPause'/>"
    "    <method name='Stop'/>"
    "    <method name='Play'/>"
    "    <method name='Seek'>"
    "      <arg direction='in' name='Offset' type='x'/>"
    "    </method>"
    "    <method name='SetPosition'>"
    "      <arg direction='in' name='TrackId' type='o'/>"
    "      <arg direction='in' name='Position' type='x'/>"
    "    </method>"
    "    <method name='OpenUri'>"
    "      <arg direction='in' name='Uri' type='s'/>"
    "    </method>"
    "    <signal name='Seeked'>"
    "      <arg name='Position' type='x'/>"
    "    </signal>"
    "    <property name='PlaybackStatus' type='s' access='read'/>"
    "    <property name='LoopStatus' type='s' access='readwrite'/>"
    "    <property name='Rate' type='d' access='readwrite'/>"
    "    <property name='Shuffle' type='b' access='readwrite'/>"
    "    <property name='Metadata' type='a{sv}' access='read'/>"
    "    <property name='Volume' type='d' access='readwrite'/>"
    "    <property name='Position' type='x' access='read'/>"
    "    <property name='MinimumRate' type='d' access='read'/>"
    "    <property name='MaximumRate' type='d' access='read'/>"
    "    <property name='CanGoNext' type='b' access='read'/>"
    "    <property name='CanGoPrevious' type='b' access='read'/>"
    "    <property name='CanPlay' type='b' access='read'/>"
    "    <property name='CanPause' type='b' access='read'/>"
    "    <property name='CanSeek' type='b' access='read'/>"
    "    <property name='CanControl' type='b' access='read'/>"
    "  </interface>"
    "</node>";

static void rc_plugin_mpris_emit_property_changes(RCPluginMPRISPrivate *priv,
    GHashTable *changes, const gchar *interface)
{
    GError *error = NULL;
    GVariantBuilder *properties;
    GVariantBuilder *invalidated;
    GVariant *parameters;
    gpointer propname, propvalue;
    GHashTableIter iter;

    /* build property changes */
    properties = g_variant_builder_new(G_VARIANT_TYPE ("a{sv}"));
    invalidated = g_variant_builder_new(G_VARIANT_TYPE ("as"));
    g_hash_table_iter_init(&iter, changes);
    while(g_hash_table_iter_next(&iter, &propname, &propvalue))
    {
        if(propvalue!=NULL)
            g_variant_builder_add(properties, "{sv}", propname, propvalue);
        else
            g_variant_builder_add(invalidated, "s", propname);
    }
    parameters = g_variant_new("(sa{sv}as)", interface, properties,
        invalidated);
    g_variant_builder_unref(properties);
    g_variant_builder_unref(invalidated);
    g_dbus_connection_emit_signal(priv->connection, NULL,
        DBUS_MPRIS_OBJECT_NAME, "org.freedesktop.DBus.Properties",
        "PropertiesChanged", parameters, &error);
    if(error!=NULL)
    {
        g_warning("Unable to send MPRIS2 property changes for %s: %s",
            interface, error->message);
        g_clear_error(&error);
    }
}

static gboolean rc_plugin_mpris_emit_properties_idle(gpointer data)
{
    RCPluginMPRISPrivate *priv = (RCPluginMPRISPrivate *)data;
    if(data==NULL) return FALSE;
    if(priv->player_property_changes!=NULL)
    {
        rc_plugin_mpris_emit_property_changes(priv,
            priv->player_property_changes, DBUS_MPRIS_PLAYER_INTERFACE);
        g_hash_table_destroy(priv->player_property_changes);
        priv->player_property_changes = NULL;
    }
    priv->property_emit_id = 0;
    return FALSE;
}

static void rc_plugin_mpris_player_add_property_change(
    RCPluginMPRISPrivate *priv, const char *property, GVariant *value)
{
    if(priv->player_property_changes==NULL)
    {
        priv->player_property_changes = g_hash_table_new_full(g_str_hash,
            g_str_equal, g_free, (GDestroyNotify)g_variant_unref);
    }
    g_hash_table_insert(priv->player_property_changes, g_strdup(property),
        g_variant_ref_sink(value));
    if(priv->property_emit_id == 0)
    {
        priv->property_emit_id = g_idle_add(
            rc_plugin_mpris_emit_properties_idle, priv);
    }
}

static GVariant *rc_plugin_mpris_get_metadata(const gchar *uri,
    GSequenceIter *reference, const RCLibCoreMetadata *metadata)
{
    GVariantBuilder *builder;
    GVariant *variant;
    gint tracknum = 0;
    gchar *track_id = NULL;
    const gchar *title, *artist, *album;
    gint64 length;
    const gchar *strv[] = {NULL, NULL};
    RCLibDbPlaylistData *playlist_data;
    if(reference!=NULL)
    {
        playlist_data = g_sequence_get(reference);
        title = playlist_data->title;
        artist = playlist_data->artist;
        album = playlist_data->album;
        length = playlist_data->length / GST_USECOND;
        tracknum = playlist_data->tracknum;
    }
    else if(metadata!=NULL)
    {
        title = metadata->title;
        artist = metadata->artist;
        album = metadata->album;
        length = metadata->duration / GST_USECOND;
        tracknum = metadata->track;
    }
    else return NULL;
    builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    if(reference!=NULL)
        track_id = g_strdup_printf("%p", reference);
    if(track_id!=NULL)
    {
        g_variant_builder_add(builder, "{sv}", "mpris:trackid",
            g_variant_new("s", track_id));
        g_free(track_id);
    }
    g_variant_builder_add(builder, "{sv}", "mpris:length",
        g_variant_new_int64(length));
    if(title!=NULL)
        g_variant_builder_add(builder, "{sv}", "xesam:title",
            g_variant_new_string(title));
    if(artist!=NULL)
    {
        strv[0] = artist;
        g_variant_builder_add(builder, "{sv}", "xesam:artist",
            g_variant_new_strv(strv, -1));
    }
    if(album!=NULL)
        g_variant_builder_add(builder, "{sv}", "xesam:album",
            g_variant_new_string(album));
    if(uri!=NULL)
        g_variant_builder_add(builder, "{sv}", "xesam:url",
            g_variant_new_string(uri));
    g_variant_builder_add(builder, "{sv}", "xesam:trackNumber",
        g_variant_new_int32(tracknum));
    variant = g_variant_builder_end(builder);
    g_variant_builder_unref(builder);
    return variant;
}

static void rc_plugin_mpris_root_method_call_cb(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer data)
{
    if(g_strcmp0(object_path, DBUS_MPRIS_OBJECT_NAME)!= 0 ||
        g_strcmp0(interface_name, DBUS_MPRIS_ROOT_INTERFACE)!=0)
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
            G_DBUS_ERROR_NOT_SUPPORTED, "Method %s.%s is not supported",
            interface_name, method_name);
        return;
    }   
    if(g_strcmp0(method_name, "Raise")==0)
    {
        rc_ui_main_window_present_main_window();
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if(g_strcmp0(method_name, "Quit")==0)
    {
        /* Quit is not allowed yet. */
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

static GVariant *rc_plugin_mpris_get_root_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer data)
{
    if(g_strcmp0(object_path, DBUS_MPRIS_OBJECT_NAME)!=0 ||
        g_strcmp0(interface_name, DBUS_MPRIS_ROOT_INTERFACE)!=0)
    {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
            "Property %s.%s is not supported", interface_name,
            property_name);
        return NULL;
    }
    if(g_strcmp0(property_name, "CanQuit")==0)
    {
        return g_variant_new_boolean(FALSE);
    }
    else if(g_strcmp0(property_name, "CanRaise")==0)
    {
        return g_variant_new_boolean(TRUE);
    }
    else if(g_strcmp0(property_name, "HasTrackList")==0)
    {
        return g_variant_new_boolean(FALSE);
    }
    else if(g_strcmp0(property_name, "Identity")==0)
    {
        return g_variant_new_string(mpris_identity);
    }
    else if(g_strcmp0(property_name, "SupportedUriSchemes")==0)
    {
        return g_variant_new_strv(mpris_supported_uri_schemes, -1);
    }
    else if(g_strcmp0(property_name, "SupportedMimeTypes")==0)
    {
        return g_variant_new_strv(mpris_supported_mime_types, -1);
    }
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
        "Property %s.%s is not supported", interface_name, property_name);
        return NULL;
    return NULL;
}

static void rc_plugin_mpris_player_method_call_cb(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer data)
{
    GstState state;
    gboolean flag;
    gint64 offset;
    gpointer track_id;
    gchar *uri;
    if(data==NULL) return;
    if(g_strcmp0(object_path, DBUS_MPRIS_OBJECT_NAME)!= 0 ||
        g_strcmp0(interface_name, DBUS_MPRIS_PLAYER_INTERFACE)!=0)
    {
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
            G_DBUS_ERROR_NOT_SUPPORTED, "Method %s.%s is not supported",
            interface_name, method_name);
        return;
    }
    if(g_strcmp0(method_name, "Next")==0)
    {
        rclib_player_play_next(FALSE, FALSE, FALSE);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if(g_strcmp0(method_name, "Previous")==0)
    {
        rclib_player_play_prev(FALSE, FALSE, FALSE);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if(g_strcmp0(method_name, "Pause") == 0)
    {
        if(rclib_core_pause())
            g_dbus_method_invocation_return_value(invocation, NULL);
        else
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot pause");
    }
    else if(g_strcmp0(method_name, "PlayPause")==0)
    {
        rclib_core_get_state(&state, NULL, 0);
        if(state==GST_STATE_PLAYING)
            flag = rclib_core_pause();
        else
            flag = rclib_core_play();
        if(flag)
            g_dbus_method_invocation_return_value(invocation, NULL);
        else
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot play/pause");
    }
    else if(g_strcmp0(method_name, "Stop")==0)
    {
        rclib_core_stop();
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if(g_strcmp0(method_name, "Play")==0)
    {
        if(rclib_core_play())
            g_dbus_method_invocation_return_value(invocation, NULL);
        else
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot play");
    }
    else if(g_strcmp0(method_name, "Seek")==0)
    {
        rclib_core_get_state(&state, NULL, 0);
        if(state!=GST_STATE_PLAYING && state!=GST_STATE_PAUSED)
        {
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot seek");
            return;
        }       
        g_variant_get(parameters, "(x)", &offset);
        offset *= GST_USECOND;
        if(rclib_core_set_position(offset))
            g_dbus_method_invocation_return_value(invocation, NULL);
        else
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot seek");
    }
    else if(g_strcmp0(method_name, "SetPosition")==0)
    {
        rclib_core_get_state(&state, NULL, 0);
        if(state!=GST_STATE_PLAYING && state!=GST_STATE_PAUSED)
        {
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot set position");
            return;
        }
        g_variant_get(parameters, "(&ox)", &track_id, &offset);
        if(track_id!=rclib_core_get_db_reference())
        {
            g_dbus_method_invocation_return_value(invocation, NULL);
            return;
        }
        offset *= GST_USECOND;
        if(rclib_core_set_position(offset))
            g_dbus_method_invocation_return_value(invocation, NULL);
        else
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot set position");
    }
    else if(g_strcmp0(method_name, "OpenUri")==0)
    {
        g_variant_get(parameters, "(&s)", &uri);
        if(uri==NULL)
        {
            g_dbus_method_invocation_return_error_literal(invocation,
                G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "Cannot open URI");
            return;
        }
        rclib_core_set_uri(uri);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

static GVariant *rc_plugin_mpris_get_player_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer data)
{
    GstState state;
    gint64 pos;
    gdouble volume = 1.0;
    GSequenceIter *reference;
    gchar *uri;
    const RCLibCoreMetadata *metadata;
    GVariant *variant;
    if(data==NULL) return NULL;
    if(g_strcmp0(object_path, DBUS_MPRIS_OBJECT_NAME)!=0 ||
        g_strcmp0(interface_name, DBUS_MPRIS_PLAYER_INTERFACE)!=0)
    {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
            "Property %s.%s is not supported", interface_name,
            property_name);
        return NULL;
    }
    if(g_strcmp0(property_name, "PlaybackStatus")==0)
    {
        rclib_core_get_state(&state, NULL, 0);
        switch(state)
        {
            case GST_STATE_PLAYING:
                return g_variant_new_string("Playing");
                break;
            case GST_STATE_PAUSED:
                return g_variant_new_string("Paused");
                break;
            default:
                return g_variant_new_string("Stopped");
                break;
        }
    }
    else if(g_strcmp0(property_name, "LoopStatus")==0)
    {
        switch(rclib_player_get_repeat_mode())
        {
            case RCLIB_PLAYER_REPEAT_SINGLE:
                return g_variant_new_string("Track");
                break;
            case RCLIB_PLAYER_REPEAT_LIST:
                return g_variant_new_string("Playlist");
                break;
            default:
                return g_variant_new_string("None");
                break;
        }
    }
    else if(g_strcmp0(property_name, "Rate")==0)
    {
        return g_variant_new_double(1.0);
    }
    else if(g_strcmp0(property_name, "Shuffle")==0)
    {
        if(rclib_player_get_random_mode()>0)
            return g_variant_new_boolean(TRUE);
        else
            return g_variant_new_boolean(FALSE);
    }
    else if(g_strcmp0(property_name, "Metadata")==0)
    {
        uri = rclib_core_get_uri();
        reference = rclib_core_get_db_reference();
        metadata = rclib_core_get_metadata();
        variant = rc_plugin_mpris_get_metadata(uri, reference, metadata);
        g_free(uri);
        return variant;
    }
    else if(g_strcmp0(property_name, "Volume")==0)
    {
        rclib_core_get_volume(&volume);
        return g_variant_new_double(volume);
    }
    else if(g_strcmp0(property_name, "Position")==0)
    {
        pos = rclib_core_query_position() / GST_USECOND;
        return g_variant_new_int64(pos);
    }
    else if(g_strcmp0(property_name, "MinimumRate")==0)
    {
        return g_variant_new_double(1.0);
    }
    else if(g_strcmp0(property_name, "MaximumRate")==0)
    {
        return g_variant_new_double(1.0);
    }
    else if(g_strcmp0(property_name, "CanGoNext")==0)
    {
        return g_variant_new_boolean(TRUE);
    }
    else if(g_strcmp0(property_name, "CanGoPrevious")==0)
    {
        return g_variant_new_boolean(TRUE);
    }
    else if(g_strcmp0(property_name, "CanPlay")==0)
    {
        return g_variant_new_boolean(TRUE);
    }
    else if(g_strcmp0(property_name, "CanPause")==0)
    {
        rclib_core_get_state(&state, NULL, 0);
        if(state==GST_STATE_PLAYING)
            return g_variant_new_boolean(TRUE);
        else
            return g_variant_new_boolean(FALSE);
    }
    else if(g_strcmp0(property_name, "CanSeek")==0)
    {
        rclib_core_get_state(&state, NULL, 0);
        if(state==GST_STATE_PLAYING || state==GST_STATE_PAUSED)
            return g_variant_new_boolean(TRUE);
        else
            return g_variant_new_boolean(FALSE);
    }
    else if(g_strcmp0(property_name, "CanControl")==0)
    {
        return g_variant_new_boolean(TRUE);
    }
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
        "Property %s.%s is not supported", interface_name, property_name);
        return NULL;
    return NULL;
}

static gboolean rc_plugin_mpris_set_player_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GVariant *value, GError **error,
    gpointer data)
{
    const gchar *string;
    gdouble volume;
    if(data==NULL) return FALSE;
    if(g_strcmp0(object_path, DBUS_MPRIS_OBJECT_NAME)!=0 ||
        g_strcmp0(interface_name, DBUS_MPRIS_PLAYER_INTERFACE)!=0)
    {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
            "Property %s.%s is not supported", interface_name,
            property_name);
        return FALSE;
    }
    if(g_strcmp0(property_name, "LoopStatus")==0)
    {
        string = g_variant_get_string(value, NULL);
        if(g_strcmp0(string, "Playlist")==0)
            rclib_player_set_repeat_mode(RCLIB_PLAYER_REPEAT_LIST);
        else if(g_strcmp0(string, "Track")==0)
            rclib_player_set_repeat_mode(RCLIB_PLAYER_REPEAT_SINGLE);
        else
            rclib_player_set_repeat_mode(RCLIB_PLAYER_REPEAT_NONE);
        return TRUE;
    }
    else if(g_strcmp0(property_name, "Rate")==0)
    {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
            "Rate cannot be set");
        return FALSE;
    }
    else if(g_strcmp0(property_name, "Shuffle")==0)
    {
        if(g_variant_get_boolean(value))
            rclib_player_set_random_mode(RCLIB_PLAYER_RANDOM_SINGLE);
        else
            rclib_player_set_random_mode(RCLIB_PLAYER_RANDOM_NONE);
        return TRUE;
    }
    else if(g_strcmp0(property_name, "Volume")==0)
    {
        volume = g_variant_get_double(value);
        if(volume<0.0) volume = 0.0;
        else if(volume>1.0) volume = 1.0;
        rclib_core_set_volume(volume);
        return TRUE;
    }
    g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
        "Property %s.%s is not supported", interface_name, property_name);
    return FALSE;
}

static const GDBusInterfaceVTable rc_plugin_mpris_root_vtable =
{
    .method_call = rc_plugin_mpris_root_method_call_cb,
    .get_property = rc_plugin_mpris_get_root_property,
    .set_property = NULL
};

static const GDBusInterfaceVTable rc_plugin_mpris_player_vtable =
{
    .method_call = rc_plugin_mpris_player_method_call_cb,
    .get_property = rc_plugin_mpris_get_player_property,
    .set_property = rc_plugin_mpris_set_player_property,
};

static void rc_plugin_mpris_name_acquired_cb(GDBusConnection *connection,
    const gchar *name, gpointer data)
{
    g_debug("Successfully acquired D-Bus name %s for MPRIS2 support.",
        name);
}

static void rc_plugin_mpris_name_lost_cb(GDBusConnection *connection,
    const gchar *name, gpointer data)
{
    g_debug("Lost D-Bus name %s for MPRIS2 support.", name);
}

static void rc_plugin_mpris_state_changed_cb(RCLibCore *core, GstState state,
    gpointer data)
{
    RCPluginMPRISPrivate *priv = (RCPluginMPRISPrivate *)data;
    const gchar *playback_status = "Stopped";
    gboolean can_pause = FALSE;
    if(data==NULL) return;
    switch(state)
    {
        case GST_STATE_PLAYING:
            playback_status = "Playing";
            can_pause = TRUE;
            break;
        case GST_STATE_PAUSED:
            playback_status = "Paused";
            break;
        default:
            break;
    }
    rc_plugin_mpris_player_add_property_change(priv, "PlaybackStatus",
        g_variant_new_string(playback_status));
    rc_plugin_mpris_player_add_property_change(priv, "CanPause",
        g_variant_new_boolean(can_pause));
}

static void rc_plugin_mpris_uri_changed_cb(RCLibCore *core,
    const gchar *uri, gpointer data)
{
    GSequenceIter *reference;
    GVariant *metadata_variant;
    RCPluginMPRISPrivate *priv = (RCPluginMPRISPrivate *)data;
    if(data==NULL) return;
    reference = rclib_core_get_db_reference();
    if(reference!=NULL)
    {
        metadata_variant = rc_plugin_mpris_get_metadata(uri, reference,
            NULL);
        if(metadata_variant!=NULL)
        {
            rc_plugin_mpris_player_add_property_change(priv, "Metadata",
                metadata_variant);
        }
    }
}

static void rc_plugin_mpris_metadata_found_cb(RCLibCore *core,
    const RCLibCoreMetadata *metadata, const gchar *uri, gpointer data)
{
    RCPluginMPRISPrivate *priv = (RCPluginMPRISPrivate *)data;
    GVariant *metadata_variant;
    if(data==NULL) return;
    metadata_variant = rc_plugin_mpris_get_metadata(uri, NULL, metadata);
    if(metadata_variant!=NULL)
    {
        rc_plugin_mpris_player_add_property_change(priv, "Metadata",
            metadata_variant);
    }
}

static void rc_plugin_mpris_volume_changed_cb(RCLibCore *core,
    gdouble volume, gpointer data)
{
    RCPluginMPRISPrivate *priv = (RCPluginMPRISPrivate *)data;
    if(data==NULL) return;
    rc_plugin_mpris_player_add_property_change(priv, "Volume",
        g_variant_new_double(volume));
}

static void rc_plugin_mpris_repeat_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRepeatMode mode, gpointer data)
{
    RCPluginMPRISPrivate *priv = (RCPluginMPRISPrivate *)data;
    const gchar *mode_str = "None";
    if(data==NULL) return;
    switch(mode)
    {
        case RCLIB_PLAYER_REPEAT_SINGLE:
            mode_str = "Track";
            break;
        case RCLIB_PLAYER_REPEAT_LIST:
            mode_str = "Playlist";
            break;
        default:
            break;
    }
    rc_plugin_mpris_player_add_property_change(priv, "LoopStatus",
        g_variant_new_string(mode_str));
}

static void rc_plugin_mpris_random_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRandomMode mode, gpointer data)
{
    RCPluginMPRISPrivate *priv = (RCPluginMPRISPrivate *)data;
    gboolean flag = FALSE;
    if(data==NULL) return;
    if(mode>RCLIB_PLAYER_RANDOM_NONE)
        flag = TRUE;
    rc_plugin_mpris_player_add_property_change(priv, "Shuffle",
        g_variant_new_boolean(flag)); 
}

static gboolean rc_plugin_mpris_load(RCLibPluginData *plugin)
{
    RCPluginMPRISPrivate *priv = &mpris_priv;
    GDBusNodeInfo *mpris_node_info;
    GDBusInterfaceInfo *ifaceinfo;
    gchar *mpris_full_name;
    GError *error = NULL;
    priv->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Unable to connect to D-Bus session bus: %s",
            error->message);
        g_error_free(error);
        priv->connection = NULL;
        return FALSE;
    }
    mpris_node_info = g_dbus_node_info_new_for_xml(
        mpris_introspection_xml, &error);
    if(error!=NULL)
    {
        g_warning("Unable to read MPRIS2 interface specificiation: %s",
            error->message);
        g_error_free(error);
        error = NULL;
        mpris_node_info = NULL;
        g_object_unref(priv->connection);
        priv->connection = NULL;
        return FALSE;
    } 
    
    /* Register MPRIS root interface. */
    ifaceinfo = g_dbus_node_info_lookup_interface(mpris_node_info,
        DBUS_MPRIS_ROOT_INTERFACE);
    priv->mpris_root_id = g_dbus_connection_register_object(
        priv->connection, DBUS_MPRIS_OBJECT_NAME, ifaceinfo,
        &rc_plugin_mpris_root_vtable, priv, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Unable to register MPRIS2 root interface: %s",
            error->message);
        g_error_free(error);
        error = NULL;
        priv->mpris_root_id = 0;
        g_dbus_node_info_unref(mpris_node_info);
        g_object_unref(priv->connection);
        priv->connection = NULL;
        return FALSE;
    }
        
    /* Register MPRIS player interface. */
    ifaceinfo = g_dbus_node_info_lookup_interface(mpris_node_info,
        DBUS_MPRIS_PLAYER_INTERFACE);
    priv->mpris_player_id = g_dbus_connection_register_object(
        priv->connection, DBUS_MPRIS_OBJECT_NAME, ifaceinfo,
        &rc_plugin_mpris_player_vtable, priv, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Unable to register MPRIS player interface: %s",
            error->message);
        g_error_free(error);
        error = NULL;
        g_dbus_connection_unregister_object(priv->connection,
            priv->mpris_root_id);
        priv->mpris_root_id = 0;
        priv->mpris_player_id = 0;
        g_dbus_node_info_unref(mpris_node_info);
        g_object_unref(priv->connection);
        priv->connection = NULL;
    }
        
    g_dbus_node_info_unref(mpris_node_info);
    
    mpris_full_name = g_strdup_printf("%s.%s", DBUS_MPRIS_BUS_NAME_PREFIX,
        mpris_identity);
    priv->mpris_name_own_id = g_bus_own_name(G_BUS_TYPE_SESSION,
        mpris_full_name, G_BUS_NAME_OWNER_FLAGS_NONE, NULL,
        rc_plugin_mpris_name_acquired_cb, rc_plugin_mpris_name_lost_cb,
        NULL, NULL);
    g_free(mpris_full_name);
    
    priv->state_changed_id = rclib_core_signal_connect("state-changed",
        G_CALLBACK(rc_plugin_mpris_state_changed_cb), priv);
    priv->uri_changed_id = rclib_core_signal_connect("uri-changed",
        G_CALLBACK(rc_plugin_mpris_uri_changed_cb), priv);
    priv->metadata_found_id = rclib_core_signal_connect("tag-found",
        G_CALLBACK(rc_plugin_mpris_metadata_found_cb), priv);
    priv->volume_changed_id = rclib_core_signal_connect("volume-changed",
        G_CALLBACK(rc_plugin_mpris_volume_changed_cb), priv);
    priv->repeat_mode_changed_id = rclib_player_signal_connect(
        "repeat-mode-changed", G_CALLBACK(rc_plugin_mpris_repeat_mode_changed_cb),
        NULL);
    priv->random_mode_changed_id = rclib_player_signal_connect(
        "random-mode-changed", G_CALLBACK(rc_plugin_mpris_random_mode_changed_cb),
        NULL);
    return TRUE;
}


static gboolean rc_plugin_mpris_unload(RCLibPluginData *plugin)
{
    RCPluginMPRISPrivate *priv = &mpris_priv;
    if(priv->state_changed_id>0)
        rclib_core_signal_disconnect(priv->state_changed_id);
    if(priv->uri_changed_id>0)
        rclib_core_signal_disconnect(priv->uri_changed_id);
    if(priv->metadata_found_id>0)
        rclib_core_signal_disconnect(priv->metadata_found_id);
    if(priv->volume_changed_id>0)
        rclib_core_signal_disconnect(priv->volume_changed_id);
    if(priv->repeat_mode_changed_id>0)
        rclib_player_signal_disconnect(priv->repeat_mode_changed_id);
    if(priv->random_mode_changed_id>0)
        rclib_player_signal_disconnect(priv->random_mode_changed_id);
    if(priv->mpris_root_id!=0)
    {
        g_dbus_connection_unregister_object(priv->connection,
            priv->mpris_root_id);
    }
    if(priv->mpris_player_id!=0)
    {
        g_dbus_connection_unregister_object(priv->connection,
            priv->mpris_player_id);
    }
    if(priv->mpris_name_own_id>0)
        g_bus_unown_name(priv->mpris_name_own_id);
    if(priv->connection!=NULL)
        g_object_unref(priv->connection);
    priv->connection = NULL;
    return TRUE;
}

static gboolean rc_plugin_mpris_init(RCLibPluginData *plugin)
{

    return TRUE;
}

static void rc_plugin_mpris_destroy(RCLibPluginData *plugin)
{

}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = MPRISSUPPORT_ID,
    .name = N_("MPRIS2 Support"),
    .version = "0.5",
    .description = N_("Provide MPRIS2 support for the player."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_mpris_load,
    .unload = rc_plugin_mpris_unload,
    .configure = NULL,
    .destroy = rc_plugin_mpris_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_mpris_init, rcplugin_info);


