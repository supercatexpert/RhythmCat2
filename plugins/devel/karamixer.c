/*
 * Karaoke Mixer
 * Record the voice and mix it with the music.
 *
 * karamixer.c
 * This file is part of RhythmCat
 *
 * Copyright (C) 2010 - SuperCat, license: GPL v3
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
#include <gtk/gtk.h>
#include <rclib.h>
#include <rc-ui-menu.h>

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif

#define G_LOG_DOMAIN "KaraokeMixer"
#define KARAMIXER_ID "rc2-native-karaoke-mixer"

typedef struct RCPluginKaraMixerPrivate
{
    GtkWidget *window;
    GtkWidget *music_entry;
    GtkWidget *voice_entry;
    GtkWidget *mix_entry;
    GtkWidget *record_button;
    GtkWidget *record_image;
    GtkWidget *playback_button;
    GtkWidget *playback_image;
    GtkWidget *mix_button;
    GtkWidget *mix_image;
    GtkWidget *volumebutton;
    GtkWidget *output_checkbutton;
    GtkWidget *result_label;
    GtkToggleAction *action;
    GstElement *record_pipeline;
    GstElement *record_filesink;
    GstElement *record_volume;
    GstElement *mixer_pipeline;
    GstElement *mixer_filesink;
    GstElement *mixer_filesrc1;
    GstElement *mixer_filesrc2;
    GstElement *echo_pipeline;
    GstElement *echo_volume;
    GstElement *playback_pipeline;
    GstElement *playback_filesrc1;
    GstElement *playback_filesrc2;
    guint menu_id;
}RCPluginKaraMixerPrivate;

static RCPluginKaraMixerPrivate karamixer_priv = {0};

static void rc_plugin_karamixer_record_start(const gchar *file)
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    gchar *uri = NULL;
    gchar *music_file;
    if(file==NULL) return;
    if(priv->record_pipeline==NULL) return;
    uri = rclib_core_get_uri();
    if(uri==NULL) return;
    music_file = g_filename_from_uri(uri, NULL, NULL);
    g_free(uri);
    gtk_entry_set_text(GTK_ENTRY(priv->music_entry), music_file);
    g_free(music_file);
    g_object_set(priv->record_filesink, "location", file, NULL);
    rclib_core_stop();
    if(gst_element_set_state(priv->record_pipeline, GST_STATE_PLAYING))
    {
        rclib_core_play();
    }
}

static void rc_plugin_karamixer_record_stop()
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    if(priv->record_pipeline==NULL) return;
    gst_element_set_state(priv->record_pipeline, GST_STATE_NULL);
    gtk_image_set_from_stock(GTK_IMAGE(priv->record_image),
        GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_label_set_text(GTK_LABEL(priv->result_label), _("Ready"));
    gtk_widget_set_sensitive(priv->record_button, TRUE);
    gtk_widget_set_sensitive(priv->playback_button, TRUE);
    gtk_widget_set_sensitive(priv->mix_button, TRUE);
}

static gboolean rc_plugin_karamixer_record_bus_handler(GstBus *bus,
    GstMessage *msg, gpointer data)
{
    GstState state, pending;
    GError *error = NULL;
    gchar *error_msg;
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    if(data==NULL || priv->record_pipeline==NULL) return TRUE;
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            rc_plugin_karamixer_record_stop();
            break;
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &error, NULL);
            rc_plugin_karamixer_record_stop();
            error_msg = g_strdup_printf(_("Cannot record: %s"),
                error->message);
            g_error_free(error);
            gtk_label_set_text(GTK_LABEL(priv->result_label), error_msg);
            g_free(error_msg);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if(priv!=NULL &&
                GST_MESSAGE_SRC(msg)==GST_OBJECT(priv->record_pipeline))
            {
                gst_message_parse_state_changed(msg, NULL, &state, &pending);
                if(pending==GST_STATE_VOID_PENDING)
                {
                    if(state==GST_STATE_PLAYING)
                    {
                        gtk_image_set_from_stock(GTK_IMAGE(
                            priv->record_image), GTK_STOCK_MEDIA_STOP,
                            GTK_ICON_SIZE_SMALL_TOOLBAR);
                        gtk_label_set_text(GTK_LABEL(priv->result_label),
                            _("Recording..."));
                        gtk_widget_set_sensitive(priv->playback_button,
                            FALSE);
                        gtk_widget_set_sensitive(priv->mix_button, FALSE);
                    }
                    else
                    {
                        gtk_image_set_from_stock(GTK_IMAGE(
                            priv->record_image), GTK_STOCK_MEDIA_RECORD,
                            GTK_ICON_SIZE_SMALL_TOOLBAR);
                        gtk_label_set_text(GTK_LABEL(priv->result_label),
                            _("Ready"));
                        gtk_widget_set_sensitive(priv->record_button, TRUE);
                        gtk_widget_set_sensitive(priv->playback_button, TRUE);
                        gtk_widget_set_sensitive(priv->mix_button, TRUE);
                    }
                }
            }
        default:
            break;
    }
    return TRUE;
}

static gboolean rc_plugin_karamixer_record_pipeline_init(
    RCPluginKaraMixerPrivate *priv)
{
    GstElement *voice_input = NULL;
    GstElement *audio_convert = NULL;
    GstElement *audio_resample = NULL;
    GstElement *filesink = NULL;
    GstElement *volume = NULL;
    GstElement *encoder = NULL;
    GstElement *oggmux = NULL;
    GstElement *pipeline = NULL;
    GstBus *bus;
    gboolean flag = FALSE;
    if(priv==NULL || priv->record_pipeline!=NULL) return FALSE;
    G_STMT_START
    {
        flag = FALSE;
        voice_input = gst_element_factory_make("autoaudiosrc", NULL);
        if(voice_input==NULL) break;
        filesink = gst_element_factory_make("filesink", NULL);
        if(filesink==NULL) break;
        audio_convert = gst_element_factory_make("audioconvert", NULL);
        if(audio_convert==NULL) break;
        audio_resample = gst_element_factory_make("audioresample", NULL);
        if(audio_resample==NULL) break;
        volume = gst_element_factory_make("volume", NULL);
        if(volume==NULL) break;
        encoder = gst_element_factory_make("vorbisenc", NULL);
        if(encoder==NULL) break;
        oggmux = gst_element_factory_make("oggmux", NULL);
        if(oggmux==NULL) break;
        pipeline = gst_pipeline_new("recorder-pipeline");
        if(pipeline==NULL) break;
        flag = TRUE;
    }
    G_STMT_END;
    if(!flag)
    {
        if(voice_input!=NULL) gst_object_unref(voice_input);
        if(filesink!=NULL) gst_object_unref(filesink);
        if(audio_convert!=NULL) gst_object_unref(audio_convert);
        if(audio_resample!=NULL) gst_object_unref(audio_resample);
        if(volume!=NULL) gst_object_unref(volume);
        if(encoder!=NULL) gst_object_unref(encoder);
        if(oggmux!=NULL) gst_object_unref(oggmux);
        if(pipeline!=NULL) gst_object_unref(pipeline);
        return FALSE;
    }
    gst_bin_add_many(GST_BIN(pipeline), voice_input, audio_convert,
        audio_resample, volume, encoder, oggmux, filesink, NULL);
    if(!gst_element_link_many(voice_input, audio_convert, audio_resample,
        volume, encoder, oggmux, filesink, NULL))
    {
        gst_object_unref(pipeline);
        return FALSE;
    }
    g_object_set(volume, "volume", 1.0, NULL);
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus,
        (GstBusFunc)rc_plugin_karamixer_record_bus_handler, priv);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    priv->record_pipeline = pipeline;
    priv->record_volume = volume;
    priv->record_filesink = filesink;
    return TRUE;
}

static void rc_plugin_karamixer_mixer_start(const gchar *srcfile1,
    const gchar *srcfile2, const gchar *mixfile)
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    if(srcfile1==NULL || srcfile2==NULL || mixfile==NULL) return;
    if(priv->mixer_pipeline==NULL) return;
    g_object_set(priv->mixer_filesink, "location", mixfile, NULL);
    g_object_set(priv->mixer_filesrc1, "location", srcfile1, NULL);
    g_object_set(priv->mixer_filesrc2, "location", srcfile2, NULL);
    gst_element_set_state(priv->mixer_pipeline, GST_STATE_PLAYING);
}

static void rc_plugin_karamixer_mixer_stop()
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    if(priv->mixer_pipeline==NULL) return;
    gst_element_set_state(priv->mixer_pipeline, GST_STATE_NULL);
    gtk_image_set_from_stock(GTK_IMAGE(priv->mix_image),
        GTK_STOCK_CONVERT, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_label_set_text(GTK_LABEL(priv->result_label), _("Ready"));
    gtk_widget_set_sensitive(priv->record_button, TRUE);
    gtk_widget_set_sensitive(priv->playback_button, TRUE);
    gtk_widget_set_sensitive(priv->mix_button, TRUE);
}

static gboolean rc_plugin_karamixer_mixer_bus_handler(GstBus *bus,
    GstMessage *msg, gpointer data)
{
    GstState state, pending;
    GError *error = NULL;
    gchar *error_msg;
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    if(data==NULL || priv->mixer_pipeline==NULL) return TRUE;
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            rc_plugin_karamixer_mixer_stop();
            break;
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &error, NULL);
            rc_plugin_karamixer_mixer_stop();
            error_msg = g_strdup_printf(_("Cannot mix: %s"),
                error->message);
            g_error_free(error);
            gtk_label_set_text(GTK_LABEL(priv->result_label), error_msg);
            g_free(error_msg);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if(priv!=NULL &&
                GST_MESSAGE_SRC(msg)==GST_OBJECT(priv->mixer_pipeline))
            {
                gst_message_parse_state_changed(msg, NULL, &state, &pending);
                if(pending==GST_STATE_VOID_PENDING)
                {
                    if(state==GST_STATE_PLAYING)
                    {
                        gtk_image_set_from_stock(GTK_IMAGE(
                            priv->mix_image), GTK_STOCK_CANCEL,
                            GTK_ICON_SIZE_SMALL_TOOLBAR);
                        gtk_label_set_text(GTK_LABEL(priv->result_label),
                            _("Mixing..."));
                        gtk_widget_set_sensitive(priv->record_button, FALSE);
                        gtk_widget_set_sensitive(priv->playback_button,
                            FALSE);
                    }
                    else
                    {
                        gtk_image_set_from_stock(GTK_IMAGE(
                            priv->mix_image), GTK_STOCK_CONVERT,
                            GTK_ICON_SIZE_SMALL_TOOLBAR);
                        gtk_label_set_text(GTK_LABEL(priv->result_label),
                            _("Ready"));
                        gtk_widget_set_sensitive(priv->record_button, TRUE);
                        gtk_widget_set_sensitive(priv->playback_button, TRUE);
                        gtk_widget_set_sensitive(priv->mix_button, TRUE);
                    }
                }
            }
        default:
            break;
    }
    return TRUE;
}

static void rc_plugin_karamixer_mixer_pad_added_cb(GstElement *demux,
    GstPad *pad, gboolean islast, GstElement *convert)
{
    GstPad *conn_pad;
    conn_pad = gst_element_get_compatible_pad(convert, pad, NULL);
    gst_pad_link(pad, conn_pad);
    gst_object_unref(conn_pad);
}

static gboolean rc_plugin_karamixer_mixer_pipeline_init(
    RCPluginKaraMixerPrivate *priv)
{
    GstElement *filesrc1 = NULL, *filesrc2 = NULL;
    GstElement *decodebin1 = NULL, *decodebin2 = NULL;
    GstElement *audioconvert1 = NULL, *audioconvert2 = NULL;
    GstElement *audioresample1 = NULL, *audioresample2 = NULL;
    GstElement *adder = NULL;
    GstElement *encoder = NULL;
    GstElement *oggmux = NULL;
    GstElement *filesink = NULL;
    GstElement *pipeline = NULL;
    GstPad *src_pad, *sink_pad;
    gboolean flag = FALSE;
    if(priv==NULL || priv->mixer_pipeline!=NULL) return FALSE;
    GstBus *bus;
    G_STMT_START
    {
        flag = FALSE;
        filesrc1 = gst_element_factory_make("filesrc", NULL);
        if(filesrc1==NULL) break;
        filesrc2 = gst_element_factory_make("filesrc", NULL);
        if(filesrc2==NULL) break;
        decodebin1 = gst_element_factory_make("decodebin2", NULL);
        if(decodebin1==NULL) break;
        decodebin2 = gst_element_factory_make("decodebin2", NULL);
        if(decodebin2==NULL) break;
        audioconvert1 = gst_element_factory_make("audioconvert", NULL);
        if(audioconvert1==NULL) break;
        audioconvert2 = gst_element_factory_make("audioconvert", NULL);
        if(audioconvert2==NULL) break;
        audioresample1 = gst_element_factory_make("audioresample", NULL);
        if(audioresample1==NULL) break;
        audioresample2 = gst_element_factory_make("audioresample", NULL);
        if(audioresample2==NULL) break;
        adder = gst_element_factory_make("adder", NULL);
        if(adder==NULL) break;
        filesink = gst_element_factory_make("filesink", NULL);
        if(filesink==NULL) break;
        encoder = gst_element_factory_make("vorbisenc", NULL);
        if(encoder==NULL) break;
        oggmux = gst_element_factory_make("oggmux", NULL);
        if(oggmux==NULL) break;
        pipeline = gst_pipeline_new("mixer-pipeline");
        if(pipeline==NULL) break;
        flag = TRUE;
    }
    G_STMT_END;
    if(!flag)
    {
        if(filesrc1!=NULL) gst_object_unref(filesrc1);
        if(filesrc2!=NULL) gst_object_unref(filesrc2);
        if(decodebin1!=NULL) gst_object_unref(decodebin1);
        if(decodebin2!=NULL) gst_object_unref(decodebin2);
        if(audioconvert1!=NULL) gst_object_unref(audioconvert1);
        if(audioconvert2!=NULL) gst_object_unref(audioconvert2);
        if(audioresample1!=NULL) gst_object_unref(audioresample1);
        if(audioresample2!=NULL) gst_object_unref(audioresample2);
        if(adder!=NULL) gst_object_unref(adder);
        if(filesink!=NULL) gst_object_unref(filesink);
        if(encoder!=NULL) gst_object_unref(encoder);
        if(oggmux!=NULL) gst_object_unref(oggmux);
        if(pipeline!=NULL) gst_object_unref(pipeline);
        return FALSE;
    }
    gst_bin_add_many(GST_BIN(pipeline), filesrc1, filesrc2, decodebin1,
        decodebin2, audioconvert1, audioconvert2, audioresample1,
        audioresample2, adder, encoder, oggmux, filesink, NULL);
    if(!gst_element_link(filesrc1, decodebin1) ||
        !gst_element_link(filesrc2, decodebin2) ||
        !gst_element_link(audioconvert1, audioresample1) ||
        !gst_element_link(audioconvert2, audioresample2) ||
        !gst_element_link_many(adder, encoder, oggmux, filesink, NULL))
    {
        gst_object_unref(pipeline);
        return FALSE;
    }
    sink_pad = gst_element_get_request_pad(adder, "sink%d");
    src_pad = gst_element_get_static_pad(audioresample1, "src");
    gst_pad_link(src_pad, sink_pad);
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
    sink_pad = gst_element_get_request_pad(adder, "sink%d");
    src_pad = gst_element_get_static_pad(audioresample2, "src");
    gst_pad_link(src_pad, sink_pad);
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
    g_signal_connect(decodebin1, "new-decoded-pad",
        G_CALLBACK(rc_plugin_karamixer_mixer_pad_added_cb),
        audioconvert1);
    g_signal_connect(decodebin2, "new-decoded-pad",
        G_CALLBACK(rc_plugin_karamixer_mixer_pad_added_cb),
        audioconvert2);
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus,
        (GstBusFunc)rc_plugin_karamixer_mixer_bus_handler, priv);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    priv->mixer_pipeline = pipeline;
    priv->mixer_filesink = filesink;
    priv->mixer_filesrc1 = filesrc1;
    priv->mixer_filesrc2 = filesrc2;
    return TRUE;
}

static gboolean rc_plugin_karamixer_echo_pipeline_init(
    RCPluginKaraMixerPrivate *priv)
{
    GstElement *voice_input = NULL;
    GstElement *audio_convert = NULL;
    GstElement *audio_resample = NULL;
    GstElement *volume = NULL;
    GstElement *audio_sink = NULL;
    GstElement *pipeline = NULL;
    gboolean flag = FALSE;
    if(priv==NULL || priv->echo_pipeline!=NULL) return FALSE;
    G_STMT_START
    {
        flag = FALSE;
        voice_input = gst_element_factory_make("autoaudiosrc", NULL);
        if(voice_input==NULL) break;
        audio_convert = gst_element_factory_make("audioconvert", NULL);
        if(audio_convert==NULL) break;
        audio_resample = gst_element_factory_make("audioresample", NULL);
        if(audio_resample==NULL) break;
        volume = gst_element_factory_make("volume", NULL);
        if(volume==NULL) break;
        audio_sink = gst_element_factory_make("autoaudiosink", NULL);
        if(audio_sink==NULL) break;
        pipeline = gst_pipeline_new("echo-pipeline");
        flag = TRUE;
    }
    G_STMT_END;
    if(!flag)
    {
        if(voice_input!=NULL) gst_object_unref(voice_input);
        if(audio_convert!=NULL) gst_object_unref(audio_convert);
        if(audio_resample!=NULL) gst_object_unref(audio_resample);
        if(volume!=NULL) gst_object_unref(volume);
        if(audio_sink!=NULL) gst_object_unref(audio_sink);
        if(pipeline!=NULL) gst_object_unref(pipeline);
        return FALSE;
    }
    gst_bin_add_many(GST_BIN(pipeline), voice_input, audio_convert,
        audio_resample, volume, audio_sink, NULL);
    if(!gst_element_link_many(voice_input, audio_convert, audio_resample,
        volume, audio_sink, NULL))
    {
        gst_object_unref(pipeline);
        return FALSE;
    }
    g_object_set(volume, "volume", 1.0, "mute", TRUE, NULL);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    priv->echo_pipeline = pipeline;
    priv->echo_volume = volume;
    return TRUE;
}

static void rc_plugin_karamixer_playback_start(const gchar *srcfile1,
    const gchar *srcfile2)
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    if(srcfile1==NULL || srcfile2==NULL) return;
    if(priv->playback_pipeline==NULL) return;
    g_object_set(priv->playback_filesrc1, "location", srcfile1, NULL);
    g_object_set(priv->playback_filesrc2, "location", srcfile2, NULL);
    gst_element_set_state(priv->playback_pipeline, GST_STATE_PLAYING);
}

static void rc_plugin_karamixer_playback_stop()
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    if(priv->playback_pipeline==NULL) return;
    gst_element_set_state(priv->playback_pipeline, GST_STATE_NULL);
    gtk_image_set_from_stock(GTK_IMAGE(priv->playback_image),
        GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_label_set_text(GTK_LABEL(priv->result_label), _("Ready"));
    gtk_widget_set_sensitive(priv->record_button, TRUE);
    gtk_widget_set_sensitive(priv->playback_button, TRUE);
    gtk_widget_set_sensitive(priv->mix_button, TRUE);
}

static gboolean rc_plugin_karamixer_playback_bus_handler(GstBus *bus,
    GstMessage *msg, gpointer data)
{
    GstState state, pending;
    GError *error = NULL;
    gchar *error_msg;
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    if(data==NULL || priv->playback_pipeline==NULL) return TRUE;
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            rc_plugin_karamixer_playback_stop();
            break;
        case GST_MESSAGE_ERROR:
            rc_plugin_karamixer_playback_stop();
            error_msg = g_strdup_printf(_("Cannot playback: %s"),
                error->message);
            g_error_free(error);
            gtk_label_set_text(GTK_LABEL(priv->result_label), error_msg);
            g_free(error_msg);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if(priv!=NULL &&
                GST_MESSAGE_SRC(msg)==GST_OBJECT(priv->playback_pipeline))
            {
                gst_message_parse_state_changed(msg, NULL, &state, &pending);
                if(pending==GST_STATE_VOID_PENDING)
                {
                    if(state==GST_STATE_PLAYING)
                    {
                        gtk_image_set_from_stock(GTK_IMAGE(
                            priv->playback_image), GTK_STOCK_MEDIA_STOP,
                            GTK_ICON_SIZE_SMALL_TOOLBAR);
                        gtk_label_set_text(GTK_LABEL(priv->result_label),
                            _("Playbacking..."));
                        gtk_widget_set_sensitive(priv->record_button, FALSE);
                        gtk_widget_set_sensitive(priv->mix_button, FALSE);
                    }
                    else
                    {
                        gtk_image_set_from_stock(GTK_IMAGE(
                            priv->playback_image), GTK_STOCK_MEDIA_PLAY,
                            GTK_ICON_SIZE_SMALL_TOOLBAR);
                        gtk_label_set_text(GTK_LABEL(priv->result_label),
                            _("Ready"));
                        gtk_widget_set_sensitive(priv->record_button, TRUE);
                        gtk_widget_set_sensitive(priv->playback_button, TRUE);
                        gtk_widget_set_sensitive(priv->mix_button, TRUE);
                    }
                }
            }
        default:
            break;
    }
    return TRUE;
}

static void rc_plugin_karamixer_playback_pad_added_cb(GstElement *demux,
    GstPad *pad, gboolean islast, GstElement *convert)
{
    GstPad *conn_pad;
    conn_pad = gst_element_get_compatible_pad(convert, pad, NULL);
    gst_pad_link(pad, conn_pad);
    gst_object_unref(conn_pad);
}

static gboolean rc_plugin_karamixer_playback_pipeline_init(
    RCPluginKaraMixerPrivate *priv)
{
    GstElement *filesrc1 = NULL, *filesrc2 = NULL;
    GstElement *decodebin1 = NULL, *decodebin2 = NULL;
    GstElement *audioconvert1 = NULL, *audioconvert2 = NULL;
    GstElement *audioresample1 = NULL, *audioresample2 = NULL;
    GstElement *adder = NULL;
    GstElement *audio_sink = NULL;
    GstElement *pipeline = NULL;
    GstPad *src_pad, *sink_pad;
    gboolean flag = FALSE;
    if(priv==NULL || priv->playback_pipeline!=NULL) return FALSE;
    GstBus *bus;
    G_STMT_START
    {
        flag = FALSE;
        filesrc1 = gst_element_factory_make("filesrc", NULL);
        if(filesrc1==NULL) break;
        filesrc2 = gst_element_factory_make("filesrc", NULL);
        if(filesrc2==NULL) break;
        decodebin1 = gst_element_factory_make("decodebin2", NULL);
        if(decodebin1==NULL) break;
        decodebin2 = gst_element_factory_make("decodebin2", NULL);
        if(decodebin2==NULL) break;
        audioconvert1 = gst_element_factory_make("audioconvert", NULL);
        if(audioconvert1==NULL) break;
        audioconvert2 = gst_element_factory_make("audioconvert", NULL);
        if(audioconvert2==NULL) break;
        audioresample1 = gst_element_factory_make("audioresample", NULL);
        if(audioresample1==NULL) break;
        audioresample2 = gst_element_factory_make("audioresample", NULL);
        if(audioresample2==NULL) break;
        adder = gst_element_factory_make("adder", NULL);
        if(adder==NULL) break;
        audio_sink = gst_element_factory_make("autoaudiosink", NULL);
        if(audio_sink==NULL) break;
        pipeline = gst_pipeline_new("playback-pipeline");
        if(pipeline==NULL) break;
        flag = TRUE;
    }
    G_STMT_END;
    if(!flag)
    {
        if(filesrc1!=NULL) gst_object_unref(filesrc1);
        if(filesrc2!=NULL) gst_object_unref(filesrc2);
        if(decodebin1!=NULL) gst_object_unref(decodebin1);
        if(decodebin2!=NULL) gst_object_unref(decodebin2);
        if(audioconvert1!=NULL) gst_object_unref(audioconvert1);
        if(audioconvert2!=NULL) gst_object_unref(audioconvert2);
        if(audioresample1!=NULL) gst_object_unref(audioresample1);
        if(audioresample2!=NULL) gst_object_unref(audioresample2);
        if(adder!=NULL) gst_object_unref(adder);
        if(audio_sink!=NULL) gst_object_unref(audio_sink);
        if(pipeline!=NULL) gst_object_unref(pipeline);
        return FALSE;
    }
    gst_bin_add_many(GST_BIN(pipeline), filesrc1, filesrc2, decodebin1,
        decodebin2, audioconvert1, audioconvert2, audioresample1,
        audioresample2, adder, audio_sink, NULL);
    if(!gst_element_link(filesrc1, decodebin1) ||
        !gst_element_link(filesrc2, decodebin2) ||
        !gst_element_link(audioconvert1, audioresample1) ||
        !gst_element_link(audioconvert2, audioresample2) ||
        !gst_element_link(adder, audio_sink))
    {
        gst_object_unref(pipeline);
        return FALSE;
    }
    sink_pad = gst_element_get_request_pad(adder, "sink%d");
    src_pad = gst_element_get_static_pad(audioresample1, "src");
    gst_pad_link(src_pad, sink_pad);
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
    sink_pad = gst_element_get_request_pad(adder, "sink%d");
    src_pad = gst_element_get_static_pad(audioresample2, "src");
    gst_pad_link(src_pad, sink_pad);
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
    g_signal_connect(decodebin1, "new-decoded-pad",
        G_CALLBACK(rc_plugin_karamixer_playback_pad_added_cb),
        audioconvert1);
    g_signal_connect(decodebin2, "new-decoded-pad",
        G_CALLBACK(rc_plugin_karamixer_playback_pad_added_cb),
        audioconvert2);
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus,
        (GstBusFunc)rc_plugin_karamixer_playback_bus_handler, priv);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    priv->playback_pipeline = pipeline;
    priv->playback_filesrc1 = filesrc1;
    priv->playback_filesrc2 = filesrc2;
    return TRUE;
}

static void rc_plugin_karamixer_record_button_clicked(GtkButton *button,
    gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    GstState state;
    const gchar *file;
    if(data==NULL) return;
    if(!gst_element_get_state(priv->record_pipeline, &state, NULL, 0))
        return;
    file = gtk_entry_get_text(GTK_ENTRY(priv->voice_entry));
    if(state!=GST_STATE_PLAYING)
        rc_plugin_karamixer_record_start(file);
    else
        rc_plugin_karamixer_record_stop();
}

static void rc_plugin_karamixer_playback_button_clicked(GtkButton *button,
    gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    GstState state;
    const gchar *music_file;
    const gchar *voice_file;
    if(data==NULL) return;
    if(!gst_element_get_state(priv->playback_pipeline, &state, NULL, 0))
        return;
    music_file = gtk_entry_get_text(GTK_ENTRY(priv->music_entry));
    voice_file = gtk_entry_get_text(GTK_ENTRY(priv->voice_entry));
    if(music_file==NULL || voice_file==NULL || strlen(music_file)<1 ||
        strlen(voice_file)<1)
    {
        return;
    }
    if(state!=GST_STATE_PLAYING)
        rc_plugin_karamixer_playback_start(voice_file, music_file);
    else
        rc_plugin_karamixer_playback_stop(); 
}

static void rc_plugin_karamixer_mix_button_clicked(GtkButton *button,
    gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    GstState state;
    const gchar *music_file;
    const gchar *voice_file;
    const gchar *mixed_file;
    if(data==NULL) return;
    if(!gst_element_get_state(priv->mixer_pipeline, &state, NULL, 0))
        return;
    music_file = gtk_entry_get_text(GTK_ENTRY(priv->music_entry));
    voice_file = gtk_entry_get_text(GTK_ENTRY(priv->voice_entry));
    mixed_file = gtk_entry_get_text(GTK_ENTRY(priv->mix_entry));
    if(music_file==NULL || voice_file==NULL || mixed_file==NULL ||
        strlen(music_file)<1 || strlen(voice_file)<1 || strlen(mixed_file)<1)
    {
        return;
    }
    if(state!=GST_STATE_PLAYING)
        rc_plugin_karamixer_mixer_start(voice_file, music_file, mixed_file);
    else
        rc_plugin_karamixer_mixer_stop(); 
    
}

static void rc_plugin_karamixer_echo_button_toggled(GtkToggleButton *button,
    gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    gboolean flag;
    if(data==NULL) return;
    flag = gtk_toggle_button_get_active(button);
    g_object_set(priv->echo_volume, "mute", !flag, NULL);
}

static gboolean rc_plugin_karamixer_volume_changed(GtkScaleButton *widget,
    gdouble volume, gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    if(data==NULL) return FALSE;
    g_object_set(priv->echo_volume, "volume", volume, NULL);
    g_object_set(priv->record_volume, "volume", volume, NULL);
    return FALSE;
}

static void rc_plugin_karamixer_view_menu_toggled(GtkToggleAction *toggle,
    gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    gboolean flag;
    if(data==NULL) return;
    flag = gtk_toggle_action_get_active(toggle);
    if(flag)
        gtk_window_present(GTK_WINDOW(priv->window));
    else
        gtk_widget_hide(priv->window);
}

static gboolean rc_plugin_karamixer_window_delete_event_cb(GtkWidget *widget,
    GdkEvent *event, gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    if(data==NULL) return TRUE;
    gtk_toggle_action_set_active(priv->action, FALSE);
    return TRUE;
}

static void rc_plugin_karamixer_window_destroy_cb(GtkWidget *widget,
    gpointer data)
{
    RCPluginKaraMixerPrivate *priv = (RCPluginKaraMixerPrivate *)data;
    if(data==NULL) return;
    gtk_widget_destroyed(priv->window, &(priv->window));
}

static void rc_plugin_karamixer_ui_init(RCPluginKaraMixerPrivate *priv)
{
    GtkWidget *main_grid;
    GtkWidget *file_grid;
    GtkWidget *button_grid;
    GtkWidget *music_file_label;
    GtkWidget *voice_file_label;
    GtkWidget *mix_file_label;
    priv->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    priv->music_entry = gtk_entry_new();
    priv->voice_entry = gtk_entry_new();
    priv->mix_entry = gtk_entry_new();
    priv->record_button = gtk_button_new();
    priv->playback_button = gtk_button_new();
    priv->mix_button = gtk_button_new();
    priv->record_image = gtk_image_new_from_stock(
        GTK_STOCK_MEDIA_RECORD, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->playback_image = gtk_image_new_from_stock(
        GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->mix_image = gtk_image_new_from_stock(
        GTK_STOCK_CONVERT, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->volumebutton = gtk_volume_button_new();    
    priv->output_checkbutton = gtk_check_button_new_with_mnemonic(
        _("Output _Voice to Speaker"));
    priv->result_label = gtk_label_new(_("Ready"));
    music_file_label = gtk_label_new(_("Music File: "));
    voice_file_label = gtk_label_new(_("Voice File: "));
    mix_file_label = gtk_label_new(_("Mix File: "));
    main_grid = gtk_grid_new();
    file_grid = gtk_grid_new();
    button_grid = gtk_grid_new();
    g_object_set(priv->window, "title", _("Karaoke Mixer"),
        "window-position", GTK_WIN_POS_CENTER, NULL);
    g_object_set(priv->result_label, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->music_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->voice_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->mix_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->record_button, "relief", GTK_RELIEF_NONE,
        "can-focus", FALSE, NULL);
    g_object_set(priv->playback_button, "relief", GTK_RELIEF_NONE,
        "can-focus", FALSE, NULL);
    g_object_set(priv->mix_button, "relief", GTK_RELIEF_NONE,
        "can-focus", FALSE, NULL);
    g_object_set(priv->volumebutton, "can-focus", FALSE, "size",
        GTK_ICON_SIZE_SMALL_TOOLBAR, "relief", GTK_RELIEF_NONE,
        "use-symbolic", TRUE, "value", 1.0, "halign", GTK_ALIGN_END,
        "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    g_object_set(file_grid, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(button_grid, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    gtk_widget_set_size_request(priv->window, 400, -1);
    gtk_container_add(GTK_CONTAINER(priv->record_button),
        priv->record_image);
    gtk_container_add(GTK_CONTAINER(priv->playback_button),
        priv->playback_image);
    gtk_container_add(GTK_CONTAINER(priv->mix_button),
        priv->mix_image);
    gtk_grid_attach(GTK_GRID(file_grid), music_file_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(file_grid), priv->music_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(file_grid), voice_file_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(file_grid), priv->voice_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(file_grid), mix_file_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(file_grid), priv->mix_entry, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(button_grid), priv->record_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(button_grid), priv->playback_button, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(button_grid), priv->mix_button, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(button_grid), priv->volumebutton, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), file_grid, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), button_grid, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), priv->output_checkbutton,
        0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), priv->result_label, 0, 3, 1, 1);
    gtk_container_add(GTK_CONTAINER(priv->window), main_grid);
    priv->action = gtk_toggle_action_new("RC2ViewPluginKaraokeMixer",
        _("Karaoke Mixer"), _("Show/hide karaoke mixer window"), NULL);
    priv->menu_id = rc_ui_menu_add_menu_action(GTK_ACTION(priv->action),
        "/RC2MenuBar/ViewMenu/ViewSep2", "RC2ViewPluginKaraokeMixer",
        "RC2ViewPluginKaraokeMixer", TRUE);
    g_signal_connect(priv->action, "toggled",
        G_CALLBACK(rc_plugin_karamixer_view_menu_toggled), priv);
    g_signal_connect(priv->output_checkbutton, "toggled",
        G_CALLBACK(rc_plugin_karamixer_echo_button_toggled), priv);
    g_signal_connect(priv->volumebutton, "value-changed",
        G_CALLBACK(rc_plugin_karamixer_volume_changed), priv);
    g_signal_connect(priv->record_button, "clicked",
        G_CALLBACK(rc_plugin_karamixer_record_button_clicked), priv);        
    g_signal_connect(priv->playback_button, "clicked",
        G_CALLBACK(rc_plugin_karamixer_playback_button_clicked), priv);
    g_signal_connect(priv->mix_button, "clicked",
        G_CALLBACK(rc_plugin_karamixer_mix_button_clicked), priv);
    g_signal_connect(priv->window, "destroy",
        G_CALLBACK(rc_plugin_karamixer_window_destroy_cb), priv);
    g_signal_connect(priv->window, "delete-event",
        G_CALLBACK(rc_plugin_karamixer_window_delete_event_cb), priv);
    gtk_widget_show_all(priv->window);
    gtk_widget_hide(priv->window);
}

static gboolean rc_plugin_karamixer_load(RCLibPluginData *plugin)
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    const gchar *home_dir;
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gchar *filepath;
    if(!rc_plugin_karamixer_record_pipeline_init(priv))
    {
        g_warning("Cannot start record pipeline!");
        return FALSE;
    }
    if(!rc_plugin_karamixer_mixer_pipeline_init(priv))
    {
        g_warning("Cannot start mixer pipeline!");
        return FALSE;
    }
    if(!rc_plugin_karamixer_playback_pipeline_init(priv))
    {
        g_warning("Cannot start playback pipeline!");
        return FALSE;
    }
    if(!rc_plugin_karamixer_echo_pipeline_init(priv))
    {
        g_warning("Cannot start echo pipeline!");
    }
    rc_plugin_karamixer_ui_init(priv);
    filepath = g_build_filename(home_dir, "RC-Voice.OGG", NULL);
    gtk_entry_set_text(GTK_ENTRY(priv->voice_entry), filepath);
    g_free(filepath);
    filepath = g_build_filename(home_dir, "RC-Mixed.OGG", NULL);
    gtk_entry_set_text(GTK_ENTRY(priv->mix_entry), filepath);
    g_free(filepath);
    return TRUE;
}

static gboolean rc_plugin_karamixer_unload(RCLibPluginData *plugin)
{
    RCPluginKaraMixerPrivate *priv = &karamixer_priv;
    if(priv->menu_id>0)
    {
        rc_ui_menu_remove_menu_action(GTK_ACTION(priv->action),
            priv->menu_id);
        g_object_unref(priv->action);
    }
    if(priv->window!=NULL)
    {
        gtk_widget_destroy(priv->window);
        priv->window = NULL;
    }
    if(priv->record_pipeline!=NULL)
    {
        gst_element_set_state(priv->record_pipeline, GST_STATE_NULL);
        gst_object_unref(priv->record_pipeline);
        priv->record_pipeline = NULL;
    }
    if(priv->mixer_pipeline!=NULL)
    {
        gst_element_set_state(priv->mixer_pipeline, GST_STATE_NULL);
        gst_object_unref(priv->mixer_pipeline);
        priv->mixer_pipeline = NULL;
    }
    if(priv->echo_pipeline!=NULL)
    {
        gst_element_set_state(priv->echo_pipeline, GST_STATE_NULL);
        gst_object_unref(priv->echo_pipeline);
        priv->echo_pipeline = NULL;
    }
    if(priv->playback_pipeline!=NULL)
    {
        gst_element_set_state(priv->playback_pipeline, GST_STATE_NULL);
        gst_object_unref(priv->playback_pipeline);
        priv->playback_pipeline = NULL;
    }
    return TRUE;
}

static gboolean rc_plugin_karamixer_init(RCLibPluginData *plugin)
{

    return TRUE;
}

static void rc_plugin_karamixer_destroy(RCLibPluginData *plugin)
{

}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = KARAMIXER_ID,
    .name = N_("Karaoke Mixer"),
    .version = "0.5",
    .description = N_("Record your voice and mix it with any "
        "music you like."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_karamixer_load,
    .unload = rc_plugin_karamixer_unload,
    .configure = NULL,
    .destroy = rc_plugin_karamixer_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_karamixer_init, rcplugin_info);


