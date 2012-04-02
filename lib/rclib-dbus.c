/*
 * RhythmCat Library D-Bus Support Module
 * Provide D-Bus support for the player.
 *
 * rclib-dbus.c
 * This file is part of RhythmCat Library (LibRhythmCat)
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

#include "rclib-dbus.h"
#include "rclib-common.h"
#include "rclib-core.h"
#include "rclib-db.h"
#include "rclib-player.h"
#include "rclib-marshal.h"

#define DBUS_MPRIS_BUS_NAME_PREFIX "org.mpris.MediaPlayer2"
#define DBUS_MPRIS_OBJECT_NAME "/org/mpris/MediaPlayer2"

#define DBUS_MPRIS_ROOT_INTERFACE "org.mpris.MediaPlayer2"
#define DBUS_MPRIS_PLAYER_INTERFACE "org.mpris.MediaPlayer2.Player"
#define DBUS_MPRIS_TRACKLIST_INTERFACE "org.mpris.MediaPlayer2.TrackList"
#define DBUS_MPRIS_PLAYLISTS_INTERFACE "org.mpris.MediaPlayer2.Playlists"

#define DBUS_MEDIAKEY_NAME "org.gnome.SettingsDaemon"
#define DBUS_MEDIAKEY_OBJECT_PATH "/org/gnome/SettingsDaemon/MediaKeys"
#define DBUS_MEDIAKEY_INTERFACE "org.gnome.SettingsDaemon.MediaKeys"

#define DBUS_MEDIAKEY_FBACK_NAME "org.gnome.SettingsDaemon"
#define DBUS_MEDIAKEY_FBACK_OBJECT_PATH "/org/gnome/SettingsDaemon"
#define DBUS_MEDIAKEY_FBACK_INTERFACE "org.gnome.SettingsDaemon"

#define RCLIB_DBUS_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RCLIB_DBUS_TYPE, RCLibDBusPrivate)

typedef struct RCLibDBusPrivate
{
    GDBusConnection *connection;
    GDBusProxy *mediakey_proxy;
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
    
    /* root properties */
    GVariant *can_quit;
    GVariant *can_raise;
    GVariant *has_track_list;
    GVariant *identity;
    GVariant *supported_uri_schemes;
    GVariant *supported_mime_types;
}RCLibDBusPrivate;

enum
{
    PROP_O,
    PROP_CAN_QUIT,
    PROP_CAN_RAISE,
    PROP_HAS_TRACK_LIST,
    PROP_IDENTITY,
    PROP_SUPPORTED_URI_SCHEMES,
    PROP_SUPPORTED_MIME_TYPES,
};

enum
{
    SIGNAL_QUIT,
    SIGNAL_RAISE,
    SIGNAL_SEEKED,
    SIGNAL_LAST
};

static GObject *dbus_instance = NULL;
static gpointer rclib_dbus_parent_class = NULL;
static gint dbus_signals[SIGNAL_LAST] = {0};
static gchar *dbus_app_name = NULL;
static gboolean dbus_mpris_enabled = TRUE;
static gboolean dbus_mediakey_enabled = TRUE;
static const gchar *dbus_mpris2_introspection_xml =
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

static void rclib_dbus_emit_property_changes(RCLibDBusPrivate *priv,
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

static gboolean rclib_dbus_emit_properties_idle(gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    if(data==NULL) return FALSE;
    if(priv->player_property_changes!=NULL)
    {
        rclib_dbus_emit_property_changes(priv, priv->player_property_changes,
            DBUS_MPRIS_PLAYER_INTERFACE);
        g_hash_table_destroy(priv->player_property_changes);
        priv->player_property_changes = NULL;
    }
    priv->property_emit_id = 0;
    return FALSE;
}

static void rclib_dbus_player_add_property_change(RCLibDBusPrivate *priv,
    const char *property, GVariant *value)
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
        priv->property_emit_id = g_idle_add(rclib_dbus_emit_properties_idle,
            priv);
    }
}

static GVariant *rclib_dbus_mpris_get_metadata(const gchar *uri,
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

static void rclib_dbus_mpris_root_method_call_cb(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters,
    GDBusMethodInvocation *invocation, gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    if(data==NULL) return;
    if(!dbus_mpris_enabled) return;
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
        if(priv->can_raise!=NULL && g_variant_get_boolean(priv->can_raise))
        {
            g_signal_emit(dbus_instance, dbus_signals[SIGNAL_RAISE], 0);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
    else if(g_strcmp0(method_name, "Quit")==0)
    {
        if(priv->can_quit!=NULL && g_variant_get_boolean(priv->can_quit))
        {
            g_signal_emit(dbus_instance, dbus_signals[SIGNAL_QUIT], 0);
        }
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

static GVariant *rclib_dbus_mpris_get_root_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    if(data==NULL) return NULL;
    if(!dbus_mpris_enabled) return NULL;
    if(g_strcmp0(object_path, DBUS_MPRIS_OBJECT_NAME)!=0 ||
        g_strcmp0(interface_name, DBUS_MPRIS_ROOT_INTERFACE)!=0)
    {
        g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
            "Property %s.%s is not supported", interface_name,
            property_name);
        return NULL;
    }
    if(priv->can_quit!=NULL &&
        g_strcmp0(property_name, "CanQuit")==0)
    {
        return g_variant_ref(priv->can_quit);
    }
    else if(priv->can_raise!=NULL &&
        g_strcmp0(property_name, "CanRaise")==0)
    {
        return g_variant_ref(priv->can_raise);
    }
    else if(priv->has_track_list!=NULL &&
        g_strcmp0(property_name, "HasTrackList")==0)
    {
        return g_variant_ref(priv->has_track_list);
    }
    else if(priv->identity!=NULL &&
        g_strcmp0(property_name, "Identity")==0)
    {
        return g_variant_ref(priv->identity);
    }
    else if(priv->supported_uri_schemes!=NULL &&
        g_strcmp0(property_name, "SupportedUriSchemes")==0)
    {
        return g_variant_ref(priv->supported_uri_schemes);
    }
    else if(priv->supported_mime_types!=NULL &&
        g_strcmp0(property_name, "SupportedMimeTypes")==0)
    {
        return g_variant_ref(priv->supported_mime_types);
    }
    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_NOT_SUPPORTED,
        "Property %s.%s is not supported", interface_name, property_name);
        return NULL;
    return NULL;
}

static void rclib_dbus_mpris_player_method_call_cb(
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
    if(!dbus_mpris_enabled) return;
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
        rclib_core_set_uri(uri, NULL, NULL);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

static GVariant *rclib_dbus_mpris_get_player_property(
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
    if(!dbus_mpris_enabled) return NULL;
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
        variant = rclib_dbus_mpris_get_metadata(uri, reference, metadata);
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

static gboolean rclib_dbus_mpris_set_player_property(
    GDBusConnection *connection, const gchar *sender,
    const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GVariant *value, GError **error,
    gpointer data)
{
    const gchar *string;
    gdouble volume;
    if(data==NULL) return FALSE;
    if(!dbus_mpris_enabled) return FALSE;
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

static const GDBusInterfaceVTable rclib_dbus_mpris_root_vtable =
{
    .method_call = rclib_dbus_mpris_root_method_call_cb,
    .get_property = rclib_dbus_mpris_get_root_property,
    .set_property = NULL
};

static const GDBusInterfaceVTable rclib_dbus_mpris_player_vtable =
{
    .method_call = rclib_dbus_mpris_player_method_call_cb,
    .get_property = rclib_dbus_mpris_get_player_property,
    .set_property = rclib_dbus_mpris_set_player_property,
};

static void rclib_dbus_mpris_name_acquired_cb(GDBusConnection *connection,
    const gchar *name, gpointer data)
{
    g_debug("Successfully acquired D-Bus name %s for MPRIS2 support.",
        name);
}

static void rclib_dbus_mpris_name_lost_cb(GDBusConnection *connection,
    const gchar *name, gpointer data)
{
    g_debug("Lost D-Bus name %s for MPRIS2 support.", name);
}

static void rclib_dbus_state_changed_cb(RCLibCore *core, GstState state,
    gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    const gchar *playback_status = "Stopped";
    gboolean can_pause = FALSE;
    if(data==NULL) return;
    if(!dbus_mpris_enabled) return;
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
    rclib_dbus_player_add_property_change(priv, "PlaybackStatus",
        g_variant_new_string(playback_status));
    rclib_dbus_player_add_property_change(priv, "CanPause",
        g_variant_new_boolean(can_pause));
}

static void rclib_dbus_uri_changed_cb(RCLibCore *core, const gchar *uri,
    gpointer data)
{
    GSequenceIter *reference;
    GVariant *metadata_variant;
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    if(data==NULL) return;
    if(!dbus_mpris_enabled) return;
    reference = rclib_core_get_db_reference();
    if(reference!=NULL)
    {
        metadata_variant = rclib_dbus_mpris_get_metadata(uri, reference,
            NULL);
        if(metadata_variant!=NULL)
        {
            rclib_dbus_player_add_property_change(priv, "Metadata",
                metadata_variant);
        }
    }
}

static void rclib_dbus_metadata_found_cb(RCLibCore *core,
    const RCLibCoreMetadata *metadata, const gchar *uri, gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    GVariant *metadata_variant;
    if(data==NULL) return;
    if(!dbus_mpris_enabled) return;
    metadata_variant = rclib_dbus_mpris_get_metadata(uri, NULL, metadata);
    if(metadata_variant!=NULL)
    {
        rclib_dbus_player_add_property_change(priv, "Metadata",
            metadata_variant);
    }
}

static void rclib_dbus_volume_changed_cb(RCLibCore *core, gdouble volume,
    gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    if(data==NULL) return;
    if(!dbus_mpris_enabled) return;
    rclib_dbus_player_add_property_change(priv, "Volume",
        g_variant_new_double(volume));
}

static void rclib_dbus_repeat_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRepeatMode mode, gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    const gchar *mode_str = "None";
    if(data==NULL) return;
    if(!dbus_mpris_enabled) return;
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
    rclib_dbus_player_add_property_change(priv, "LoopStatus",
        g_variant_new_string(mode_str));
}

static void rclib_dbus_random_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRandomMode mode, gpointer data)
{
    RCLibDBusPrivate *priv = (RCLibDBusPrivate *)data;
    gboolean flag = FALSE;
    if(data==NULL) return;
    if(!dbus_mpris_enabled) return;
    if(mode>RCLIB_PLAYER_RANDOM_NONE)
        flag = TRUE;
    rclib_dbus_player_add_property_change(priv, "Shuffle",
        g_variant_new_boolean(flag)); 
}

static void rclib_dbus_mediakey_proxy_signal_cb(GDBusProxy *proxy,
    gchar *sender_name, gchar *signal_name, GVariant *parameters,
    gpointer data)
{
    gchar *application, *key;
    GstState state;
    gint64 pos, len;
    RCLibPlayerRepeatMode repeat_mode;
    RCLibPlayerRandomMode random_mode;
    if(!dbus_mediakey_enabled) return;
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

static void rclib_dbus_finalize(GObject *object)
{
    RCLibDBusPrivate *priv = RCLIB_DBUS_GET_PRIVATE(RCLIB_DBUS(object));
    if(priv->can_quit!=NULL)
        g_variant_unref(priv->can_quit);
    if(priv->can_raise!=NULL)
        g_variant_unref(priv->can_raise);
    if(priv->has_track_list!=NULL)
        g_variant_unref(priv->has_track_list);
    if(priv->identity!=NULL)
        g_variant_unref(priv->identity);
    if(priv->supported_uri_schemes!=NULL)
        g_variant_unref(priv->supported_uri_schemes);
    if(priv->supported_mime_types!=NULL)
        g_variant_unref(priv->supported_mime_types);
    if(priv->mpris_root_id!=0)
    {
        g_dbus_connection_unregister_object(priv->connection,
            priv->mpris_root_id);
    }
    if(priv->mpris_name_own_id>0)
        g_bus_unown_name(priv->mpris_name_own_id);
    if(priv->mediakey_proxy!=NULL)
        g_object_unref(priv->mediakey_proxy);
    if(priv->connection!=NULL)
        g_object_unref(priv->connection);
    G_OBJECT_CLASS(rclib_dbus_parent_class)->finalize(object);
}

static GObject *rclib_dbus_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(dbus_instance!=NULL) return dbus_instance;
    retval = G_OBJECT_CLASS(rclib_dbus_parent_class)->constructor
        (type, n_construct_params, construct_params);
    dbus_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&dbus_instance);
    return retval;
}

static void rclib_dbus_set_can_quit(RCLibDBus *dbus, gboolean value)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return;
    if(priv->can_quit!=NULL) g_variant_unref(priv->can_quit);
    priv->can_quit = g_variant_new_boolean(value);
}

static gboolean rclib_dbus_get_can_quit(RCLibDBus *dbus)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return FALSE;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return FALSE;
    if(priv->can_quit==NULL) return FALSE;
    return g_variant_get_boolean(priv->can_quit);
}

static void rclib_dbus_set_can_raise(RCLibDBus *dbus, gboolean value)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return;
    if(priv->can_raise!=NULL) g_variant_unref(priv->can_raise);
    priv->can_raise = g_variant_new_boolean(value);
}

static gboolean rclib_dbus_get_can_raise(RCLibDBus *dbus)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return FALSE;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return FALSE;
    if(priv->can_raise==NULL) return FALSE;
    return g_variant_get_boolean(priv->can_raise);
}

static void rclib_dbus_set_has_track_list(RCLibDBus *dbus, gboolean value)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return;
    if(priv->has_track_list!=NULL) g_variant_unref(priv->has_track_list);
    priv->has_track_list = g_variant_new_boolean(value);
}

static gboolean rclib_dbus_get_has_track_list(RCLibDBus *dbus)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return FALSE;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return FALSE;
    if(priv->has_track_list==NULL) return FALSE;
    return g_variant_get_boolean(priv->has_track_list);
}

static void rclib_dbus_set_identity(RCLibDBus *dbus, const gchar *value)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return;
    if(priv->identity!=NULL) g_variant_unref(priv->identity);
    priv->identity = g_variant_new_string(value);
}

static const gchar *rclib_dbus_get_identity(RCLibDBus *dbus)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return NULL;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return NULL;
    if(priv->identity==NULL) return NULL;
    return g_variant_get_string(priv->identity, NULL);
}

static void rclib_dbus_set_supported_uri_schemes(RCLibDBus *dbus,
    const gchar * const *value)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return;
    if(priv->supported_uri_schemes!=NULL)
        g_variant_unref(priv->supported_uri_schemes);
    priv->supported_uri_schemes = g_variant_new_strv(value, -1);
}

static const gchar **rclib_dbus_get_supported_uri_schemes(RCLibDBus *dbus)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return NULL;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return NULL;
    if(priv->supported_uri_schemes==NULL) return NULL;
    return g_variant_get_strv(priv->supported_uri_schemes, NULL);
}

static void rclib_dbus_set_supported_mime_types(RCLibDBus *dbus,
    const gchar * const *value)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return;
    if(priv->supported_mime_types!=NULL)
        g_variant_unref(priv->supported_mime_types);
    priv->supported_mime_types = g_variant_new_strv(value, -1);
}

static const gchar **rclib_dbus_get_supported_mime_types(RCLibDBus *dbus)
{
    RCLibDBusPrivate *priv;
    if(dbus==NULL) return NULL;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return NULL;
    if(priv->supported_mime_types==NULL) return NULL;
    return g_variant_get_strv(priv->supported_mime_types, NULL);
}

static void rclib_dbus_player_seeked(RCLibDBus *dbus, gint64 offset,
    gpointer data)
{
    RCLibDBusPrivate *priv;
    GError *error = NULL;
    if(dbus==NULL) return;
    priv = RCLIB_DBUS_GET_PRIVATE(dbus);
    if(priv==NULL) return;
    g_dbus_connection_emit_signal(priv->connection, NULL,
        DBUS_MPRIS_OBJECT_NAME, DBUS_MPRIS_PLAYER_INTERFACE,
        "Seeked", g_variant_new("(x)", offset), &error);
    if(error!=NULL)
    {
        g_warning("Unable to set MPRIS Seeked signal: %s", error->message);
        g_error_free(error);
    }
}

static void rclib_dbus_set_property(GObject *object,
    guint prop_id, const GValue *value, GParamSpec *pspec)
{
    RCLibDBus *dbus = RCLIB_DBUS(object);
    switch(prop_id)
    {
        /* root properties */
        case PROP_CAN_QUIT:
            rclib_dbus_set_can_quit(dbus, g_value_get_boolean(value));
            break;
        case PROP_CAN_RAISE:
            rclib_dbus_set_can_raise(dbus, g_value_get_boolean(value));
            break;
        case PROP_HAS_TRACK_LIST:
            rclib_dbus_set_has_track_list(dbus, g_value_get_boolean(value));
            break;
        case PROP_IDENTITY:
            rclib_dbus_set_identity(dbus, g_value_get_string(value));
            break;
        case PROP_SUPPORTED_URI_SCHEMES:
            rclib_dbus_set_supported_uri_schemes(dbus,
                (const gchar * const *)g_value_get_pointer(value));
            break;
        case PROP_SUPPORTED_MIME_TYPES:
            rclib_dbus_set_supported_mime_types(dbus,
                (const gchar * const *)g_value_get_pointer(value));
            break;
  
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void rclib_dbus_get_property(GObject *object,
    guint prop_id, GValue *value, GParamSpec *pspec)
{
    RCLibDBus *dbus = RCLIB_DBUS(object);
    switch(prop_id)
    {
        /* root properties */
        case PROP_CAN_QUIT:
            g_value_set_boolean(value, rclib_dbus_get_can_quit(dbus));
            break;
        case PROP_CAN_RAISE:
            g_value_set_boolean(value, rclib_dbus_get_can_raise(dbus));
            break;
        case PROP_HAS_TRACK_LIST:
            g_value_set_boolean(value, rclib_dbus_get_has_track_list(dbus));
            break;
        case PROP_IDENTITY:
            g_value_set_string(value, rclib_dbus_get_identity(dbus));
            break;
        case PROP_SUPPORTED_URI_SCHEMES:
            g_value_set_pointer(value,
                rclib_dbus_get_supported_uri_schemes(dbus));
            break;
        case PROP_SUPPORTED_MIME_TYPES:
            g_value_set_pointer(value,
                rclib_dbus_get_supported_mime_types(dbus));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void rclib_dbus_class_init(RCLibDBusClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_dbus_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_dbus_finalize;
    object_class->constructor = rclib_dbus_constructor;
    object_class->set_property = rclib_dbus_set_property;
    object_class->get_property = rclib_dbus_get_property;
    g_type_class_add_private(klass, sizeof(RCLibDBusPrivate));

    /* properies */

    /**
     * RCLibDBus:can-quit:
     *
     * Whether the player can be quited by D-Bus controller.
     */
    g_object_class_install_property(object_class, PROP_CAN_QUIT,
        g_param_spec_boolean("can-quit", "Can Quit",
        "Whether the player can be quited", FALSE, G_PARAM_READWRITE));

    /**
     * RCLibDBus:can-raise:
     *
     * Whether the player can be raised by D-Bus controller.
     */
    g_object_class_install_property(object_class, PROP_CAN_RAISE,
        g_param_spec_boolean("can-raise", "Can Raise",
        "Whether the player can be raised", FALSE, G_PARAM_READWRITE));

    /**
     * RCLibDBus:has-track-list:
     *
     * Whether the player has track lists.
     */
    g_object_class_install_property(object_class, PROP_HAS_TRACK_LIST,
        g_param_spec_boolean("has-track-list", "Has track list",
        "Whether the player has track lists", FALSE, G_PARAM_READWRITE));

    /**
     * RCLibDBus:identity:
     *
     * A friendly name to identify the media player to users.
     */
    g_object_class_install_property(object_class, PROP_IDENTITY,
        g_param_spec_string("identity", "Identity",
        "A friendly name to identify the media player to users",
        NULL, G_PARAM_READWRITE));

    /**
     * RCLibDBus:supported-uri-schemes:
     *
     * The URI schemes supported by the media player.
     */
    g_object_class_install_property(object_class, PROP_SUPPORTED_URI_SCHEMES,
        g_param_spec_pointer("supported-uri-schemes", "Supported URI schemes",
        "The URI schemes supported by the media player", G_PARAM_READWRITE));

    /**
     * RCLibDBus:supported-mime-types:
     *
     * The mime-types supported by the media player.
     */
    g_object_class_install_property(object_class, PROP_SUPPORTED_MIME_TYPES,
        g_param_spec_pointer("supported-mime-types", "Supported mime-types",
        "The mime-types supported by the media player", G_PARAM_READWRITE));

    /* signals */
    
    /**
     * RCLibDBus::quit:
     * @dbus: the #RCLibDBus that received the signal
     *
     * The ::quit signal is emitted the D-Bus controller requires the
     * player to quit (if property "can-quit" is TRUE).
     */
    dbus_signals[SIGNAL_QUIT] = g_signal_new("quit",
        RCLIB_DBUS_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDBusClass,
        quit), NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0, G_TYPE_NONE, NULL);
        
    /**
     * RCLibDBus::raise:
     * @dbus: the #RCLibDBus that received the signal
     *
     * The ::raise signal is emitted the D-Bus controller requires the
     * player to raise (if property "can-raise" is TRUE).
     */
    dbus_signals[SIGNAL_RAISE] = g_signal_new("raise",
        RCLIB_DBUS_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDBusClass,
        raise), NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0, G_TYPE_NONE, NULL);
        
    /**
     * RCLibDBus::seeked:
     * @dbus: the #RCLibDBus that received the signal
     * @position: the new position, in microsecond
     *
     * The ::seeked is emitted by the application that tells the D-Bus
     * controller track position has changed in a way that is inconsistant
     * with the current playing state.
     */
    dbus_signals[SIGNAL_SEEKED] = g_signal_new("seeked",
        RCLIB_DBUS_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDBusClass,
        seeked), NULL, NULL, rclib_marshal_VOID__INT64,
        G_TYPE_NONE, 1, G_TYPE_INT64, NULL);
}


static void rclib_dbus_instance_init(RCLibDBus *db)
{
    RCLibDBusPrivate *priv = RCLIB_DBUS_GET_PRIVATE(db);
    GDBusNodeInfo *mpris_node_info;
    GDBusInterfaceInfo *ifaceinfo;
    gchar *mpris_full_name;
    GError *error = NULL;
    GVariant *variant;
    priv->connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Unable to connect to D-Bus session bus: %s",
            error->message);
        g_error_free(error);
        priv->connection = NULL;
        return;
    }
    mpris_node_info = g_dbus_node_info_new_for_xml(
        dbus_mpris2_introspection_xml, &error);
    if(error!=NULL)
    {
        g_warning("Unable to read MPRIS2 interface specificiation: %s",
            error->message);
        g_error_free(error);
        error = NULL;
        mpris_node_info = NULL;
    }
    if(mpris_node_info!=NULL)
    {
        /* Register MPRIS root interface. */
        ifaceinfo = g_dbus_node_info_lookup_interface(mpris_node_info,
            DBUS_MPRIS_ROOT_INTERFACE);
        priv->mpris_root_id = g_dbus_connection_register_object(
            priv->connection, DBUS_MPRIS_OBJECT_NAME, ifaceinfo,
            &rclib_dbus_mpris_root_vtable, priv, NULL, &error);
        if(error!=NULL)
        {
            g_warning("Unable to register MPRIS2 root interface: %s",
                error->message);
            g_error_free(error);
            error = NULL;
            priv->mpris_root_id = 0;
        }
        
        /* Register MPRIS player interface. */
        ifaceinfo = g_dbus_node_info_lookup_interface(mpris_node_info,
            DBUS_MPRIS_PLAYER_INTERFACE);
        priv->mpris_player_id = g_dbus_connection_register_object(
            priv->connection, DBUS_MPRIS_OBJECT_NAME, ifaceinfo,
            &rclib_dbus_mpris_player_vtable, priv, NULL, &error);
        if(error!=NULL)
        {
            g_warning("Unable to register MPRIS player interface: %s",
                error->message);
            g_error_free(error);
            error = NULL;
            priv->mpris_player_id = 0;
        }
        
        g_dbus_node_info_unref(mpris_node_info);
    }
    mpris_full_name = g_strdup_printf("%s.%s", DBUS_MPRIS_BUS_NAME_PREFIX,
        dbus_app_name);
    priv->mpris_name_own_id = g_bus_own_name(G_BUS_TYPE_SESSION,
        mpris_full_name, G_BUS_NAME_OWNER_FLAGS_NONE, NULL,
        rclib_dbus_mpris_name_acquired_cb, rclib_dbus_mpris_name_lost_cb,
        NULL, NULL);
    g_free(mpris_full_name);
    
    /* Connect to MediaKey Proxy. */
    priv->mediakey_proxy = g_dbus_proxy_new_sync(priv->connection,
        G_DBUS_PROXY_FLAGS_NONE, NULL, DBUS_MEDIAKEY_NAME,
        DBUS_MEDIAKEY_OBJECT_PATH, DBUS_MEDIAKEY_INTERFACE, NULL, NULL);
    if(priv->mediakey_proxy==NULL)
    {
        priv->mediakey_proxy = g_dbus_proxy_new_sync(priv->connection,
            G_DBUS_PROXY_FLAGS_NONE, NULL, DBUS_MEDIAKEY_FBACK_NAME,
            DBUS_MEDIAKEY_FBACK_OBJECT_PATH, DBUS_MEDIAKEY_FBACK_INTERFACE,
            NULL, &error);
        if(error!=NULL)
        {
            g_warning("Unable to connect MediaKey proxy: %s",
                error->message);
            g_error_free(error);
            error = NULL;
            priv->mediakey_proxy = NULL; 
        }
    }    
    if(priv->mediakey_proxy!=NULL)
    {
        variant = g_dbus_proxy_call_sync(priv->mediakey_proxy,
            "GrabMediaPlayerKeys", g_variant_new("(su)", "RhythmCat2", 0),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
        if(variant!=NULL) g_variant_unref(variant);
        else
        {
            g_warning("Unable to call method GrabMediaPlayerKeys: %s",
                error->message);
            g_error_free(error);
            error = NULL;
            g_object_unref(priv->mediakey_proxy);
            priv->mediakey_proxy = NULL; 
        }
    }
    if(priv->mediakey_proxy!=NULL)
    {
        g_signal_connect(priv->mediakey_proxy, "g-signal",
            G_CALLBACK(rclib_dbus_mediakey_proxy_signal_cb), NULL);
        g_debug("Connected to GNOME Multimedia Key support interface.");
    }

    rclib_dbus_signal_connect("seeked",
        G_CALLBACK(rclib_dbus_player_seeked), NULL);
    priv->state_changed_id = rclib_core_signal_connect("state-changed",
        G_CALLBACK(rclib_dbus_state_changed_cb), priv);
    priv->uri_changed_id = rclib_core_signal_connect("uri-changed",
        G_CALLBACK(rclib_dbus_uri_changed_cb), priv);
    priv->metadata_found_id = rclib_core_signal_connect("tag-found",
        G_CALLBACK(rclib_dbus_metadata_found_cb), priv);
    priv->volume_changed_id = rclib_core_signal_connect("volume-changed",
        G_CALLBACK(rclib_dbus_volume_changed_cb), priv);
    priv->repeat_mode_changed_id = rclib_player_signal_connect(
        "repeat-mode-changed", G_CALLBACK(rclib_dbus_repeat_mode_changed_cb),
        NULL);
    priv->random_mode_changed_id = rclib_player_signal_connect(
        "random-mode-changed", G_CALLBACK(rclib_dbus_random_mode_changed_cb),
        NULL);
}

GType rclib_dbus_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo dbus_info = {
        .class_size = sizeof(RCLibDBusClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_dbus_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibDBus),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_dbus_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibDBus"), &dbus_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_dbus_init:
 * @app_name: the name of the application for register in the D-Bus
 * @first_property_name: the name of the first property
 * @...: the value of the first property, followed optionally by more
 *  name/value pairs, followed by %NULL
 * Initialize the D-Bus support module.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rclib_dbus_init(const gchar *app_name,
    const gchar *first_property_name, ...)
{
    va_list var_args;
    g_message("Loading D-Bus support....");
    if(dbus_instance!=NULL)
    {
        g_warning("D-Bus Support is already enabled!");
        return FALSE;
    }
    if(app_name==NULL) return FALSE;
    dbus_app_name = g_strdup(app_name);
    if(first_property_name==NULL)
        dbus_instance = g_object_newv(RCLIB_DBUS_TYPE, 0, NULL);
    else
    {
        va_start(var_args, first_property_name);
        dbus_instance = g_object_new_valist(RCLIB_DBUS_TYPE,
            first_property_name, var_args);
        va_end(var_args);
    }
    g_message("D-Bus support loaded.");
    return TRUE;
}

/**
 * rclib_db_exit:
 *
 * Unload D-Bus support module.
 */

void rclib_dbus_exit()
{
    if(dbus_instance!=NULL) g_object_unref(dbus_instance);
    dbus_instance = NULL;
    g_message("D-Bus support exited.");
}

/**
 * rclib_dbus_get_instance:
 *
 * Get the running #RCLibDBus instance.
 *
 * Returns: The running instance.
 */

GObject *rclib_dbus_get_instance()
{
    return dbus_instance;
}

/**
 * rclib_dbus_signal_connect:
 * @name: the name of the signal
 * @callback: the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibDBus object.
 *
 * Returns: The handler ID.
 */

gulong rclib_dbus_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(dbus_instance==NULL) return 0;
    return g_signal_connect(dbus_instance, name, callback, data);
}

/**
 * rclib_dbus_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running dbus instance so it will
 * not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_dbus_signal_disconnect(gulong handler_id)
{
    if(dbus_instance==NULL) return;
    g_signal_handler_disconnect(dbus_instance, handler_id);
}




