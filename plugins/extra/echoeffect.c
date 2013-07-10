/*
 * Echo Effect Plug-in
 * Add echo effect in the music playback.
 *
 * echoeffect.c
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
#include <rc-ui-menu.h>
#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>

#define RC_PLUGIN_TYPE_AUDIO_ECHO (rc_plugin_audio_echo_get_type())
#define RC_PLUGIN_AUDIO_ECHO(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_PLUGIN_TYPE_AUDIO_ECHO, RCPluginAudioEcho))
#define RC_PLUGIN_IS_AUDIO_ECHO(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_PLUGIN_TYPE_AUDIO_ECHO))
#define RC_PLUGIN_AUDIO_ECHO_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
    RC_PLUGIN_TYPE_AUDIO_ECHO, RCPluginAudioEchoClass))
#define RC_PLUGIN_IS_AUDIO_ECHO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE(\
    (klass), RC_PLUGIN_TYPE_AUDIO_ECHO))
#define RC_PLUGIN_AUDIO_ECHO_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS(\
    (obj), RC_PLUGIN_TYPE_AUDIO_ECHO, RCPluginAudioEchoClass))

#define ECHOEFF_ID "rc2-native-echo-effect"

typedef struct _RCPluginAudioEcho RCPluginAudioEcho;
typedef struct _RCPluginAudioEchoClass RCPluginAudioEchoClass;
typedef void (*RCPluginAudioEchoProcessFunc) (RCPluginAudioEcho *, guint8 *,
    guint);

typedef struct RCPluginEchoEffectPrivate
{
    GstElement *echo_element;
    GtkToggleAction *action;
    guint menu_id;
    GtkWidget *echo_window;
    GtkWidget *echo_delay_scale;
    GtkWidget *echo_fb_scale;
    GtkWidget *echo_intensity_scale;
    guint64 delay;
    gfloat feedback;
    gfloat intensity;
    gulong shutdown_id;
    GKeyFile *keyfile;
}RCPluginEchoEffectPrivate;

struct _RCPluginAudioEcho
{
    GstAudioFilter audiofilter;
    
    guint64 delay;
    guint64 max_delay;
    gfloat intensity;
    gfloat feedback;

    /* < private > */
    RCPluginAudioEchoProcessFunc process;
    guint delay_frames;
    guint8 *buffer;
    guint buffer_pos;
    guint buffer_size;
    guint buffer_size_frames;

    GMutex lock;
};

struct _RCPluginAudioEchoClass
{
    GstAudioFilterClass parent;
};

enum
{
    PROP_0,
    PROP_DELAY,
    PROP_MAX_DELAY,
    PROP_INTENSITY,
    PROP_FEEDBACK
};

#define rc_plugin_audio_echo_parent_class parent_class

static RCPluginEchoEffectPrivate echoeff_priv = {0};

#define ALLOWED_CAPS \
    "audio/x-raw,"                                                 \
    " format=(string) {"GST_AUDIO_NE(F32)","GST_AUDIO_NE(F64)"}, " \
    " rate=(int)[1,MAX],"                                          \
    " channels=(int)[1,MAX],"                                      \
    " layout=(string) interleaved"

GType rc_plugin_audio_echo_get_type();

G_DEFINE_TYPE(RCPluginAudioEcho, rc_plugin_audio_echo, GST_TYPE_AUDIO_FILTER);

static void rc_plugin_audio_echo_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec);
static void rc_plugin_audio_echo_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec);
static void rc_plugin_audio_echo_finalize(GObject *object);
static gboolean rc_plugin_audio_echo_setup(GstAudioFilter *self,
    const GstAudioInfo *info);
static gboolean rc_plugin_audio_echo_stop(GstBaseTransform *base);
static GstFlowReturn rc_plugin_audio_echo_transform_ip(GstBaseTransform *base,
    GstBuffer *buf);
static void rc_plugin_audio_echo_transform_float(RCPluginAudioEcho *self,
    gfloat *data, guint num_samples);
static void rc_plugin_audio_echo_transform_double(RCPluginAudioEcho *self,
    gdouble *data, guint num_samples);

static void rc_plugin_audio_echo_class_init(RCPluginAudioEchoClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *)klass;
    GstElementClass *gstelement_class = (GstElementClass *)klass;
    GstBaseTransformClass *basetransform_class =
        (GstBaseTransformClass *)klass;
    GstAudioFilterClass *audioself_class = (GstAudioFilterClass *)klass;
    GstCaps *caps;

    gobject_class->set_property = rc_plugin_audio_echo_set_property;
    gobject_class->get_property = rc_plugin_audio_echo_get_property;
    gobject_class->finalize = rc_plugin_audio_echo_finalize;

    g_object_class_install_property(gobject_class, PROP_DELAY,
        g_param_spec_uint64 ("delay", "Delay",
        "Delay of the echo in nanoseconds", 1, G_MAXUINT64, 1,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_CONTROLLABLE));
    g_object_class_install_property (gobject_class, PROP_MAX_DELAY,
        g_param_spec_uint64 ("max-delay", "Maximum Delay",
        "Maximum delay of the echo in nanoseconds"
        " (can't be changed in PLAYING or PAUSED state)", 1, G_MAXUINT64, 1,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY));
    g_object_class_install_property (gobject_class, PROP_INTENSITY,
        g_param_spec_float ("intensity", "Intensity", "Intensity of the echo",
        0.0, 1.0, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
        GST_PARAM_CONTROLLABLE));
    g_object_class_install_property (gobject_class, PROP_FEEDBACK,
        g_param_spec_float ("feedback", "Feedback", "Amount of feedback", 0.0,
        1.0, 0.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
        GST_PARAM_CONTROLLABLE));
        
    gst_element_class_set_static_metadata(gstelement_class, "RC Audio echo",
      "Filter/Effect/Audio", "Modified from the source codes of audioecho "
      "element",
      "Origin Author: Sebastian Dröge <sebastian.droege@collabora.co.uk>");

    caps = gst_caps_from_string (ALLOWED_CAPS);
    gst_audio_filter_class_add_pad_templates(GST_AUDIO_FILTER_CLASS (klass),
        caps);
    gst_caps_unref(caps);

    audioself_class->setup = GST_DEBUG_FUNCPTR(rc_plugin_audio_echo_setup);
    basetransform_class->transform_ip =
        GST_DEBUG_FUNCPTR(rc_plugin_audio_echo_transform_ip);
    basetransform_class->stop = GST_DEBUG_FUNCPTR(rc_plugin_audio_echo_stop);
}

static void rc_plugin_audio_echo_init(RCPluginAudioEcho *self)
{
    self->delay = 1;
    self->max_delay = 1;
    self->intensity = 0.0;
    self->feedback = 0.0;
    g_mutex_init (&self->lock);
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), TRUE);
}

static void rc_plugin_audio_echo_finalize(GObject *object)
{
    RCPluginAudioEcho *self = RC_PLUGIN_AUDIO_ECHO(object);
    g_free (self->buffer);
    self->buffer = NULL;
    g_mutex_clear(&self->lock);
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void rc_plugin_audio_echo_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
    RCPluginAudioEcho *self = RC_PLUGIN_AUDIO_ECHO(object);
    switch(prop_id)
    {
        case PROP_DELAY:
        {
            guint64 max_delay, delay;
            guint rate, bpf, new_size;
            g_mutex_lock (&self->lock);
            delay = g_value_get_uint64(value);
            max_delay = self->max_delay;
            rate = GST_AUDIO_FILTER_RATE(self);
            if(delay > max_delay && GST_STATE(self) > GST_STATE_READY)
            {
                GST_WARNING_OBJECT (self, "New delay (%" GST_TIME_FORMAT ") "
                    "is larger than maximum delay (%" GST_TIME_FORMAT ")",
                    GST_TIME_ARGS (delay), GST_TIME_ARGS (max_delay));
                self->delay = max_delay;
            }
            else
            {
                self->delay = delay;
                self->max_delay = MAX(delay, max_delay);
                if(self->max_delay>max_delay)
                {
                    bpf = GST_AUDIO_FILTER_BPF(self);
                    self->buffer_size_frames = MAX(gst_util_uint64_scale(
                        self->max_delay, rate, GST_SECOND), 1);
                    new_size = self->buffer_size_frames * bpf;
                    self->buffer = g_realloc(self->buffer, new_size);
                    if(new_size > self->buffer_size)
                    {
                        memset(self->buffer + self->buffer_size, 0,
                            new_size - self->buffer_size);
                    }
                    self->buffer_size = new_size;
                }
            }
            self->delay_frames = MAX(gst_util_uint64_scale(self->delay, rate,
                GST_SECOND), 1);
            g_mutex_unlock(&self->lock);
            break;
        }
        case PROP_MAX_DELAY:
        {
            guint64 max_delay, delay;
            g_mutex_lock(&self->lock);
            max_delay = g_value_get_uint64 (value);
            delay = self->delay;
            if(GST_STATE (self) > GST_STATE_READY)
            {
                GST_ERROR_OBJECT (self, "Can't change maximum delay in"
                    " PLAYING or PAUSED state");
            }
            else
            {
                self->delay = delay;
                self->max_delay = max_delay;
            }
            g_mutex_unlock (&self->lock);
            break;
        }
        case PROP_INTENSITY:
        {
            g_mutex_lock (&self->lock);
            self->intensity = g_value_get_float(value);
            g_mutex_unlock (&self->lock);
            break;
        }
        case PROP_FEEDBACK:
        {
            g_mutex_lock(&self->lock);
            self->feedback = g_value_get_float(value);
            g_mutex_unlock (&self->lock);
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void rc_plugin_audio_echo_get_property(GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
    RCPluginAudioEcho *self = RC_PLUGIN_AUDIO_ECHO(object);
    switch(prop_id)
    {
        case PROP_DELAY:
            g_mutex_lock (&self->lock);
            g_value_set_uint64 (value, self->delay);
            g_mutex_unlock (&self->lock);
            break;
        case PROP_MAX_DELAY:
            g_mutex_lock (&self->lock);
            g_value_set_uint64 (value, self->max_delay);
            g_mutex_unlock (&self->lock);
            break;
        case PROP_INTENSITY:
            g_mutex_lock (&self->lock);
            g_value_set_float (value, self->intensity);
            g_mutex_unlock (&self->lock);
            break;
        case PROP_FEEDBACK:
            g_mutex_lock (&self->lock);
            g_value_set_float (value, self->feedback);
            g_mutex_unlock (&self->lock);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static gboolean rc_plugin_audio_echo_setup(GstAudioFilter *base,
    const GstAudioInfo *info)
{
    RCPluginAudioEcho *self = RC_PLUGIN_AUDIO_ECHO (base);
    gboolean ret = TRUE;

    switch(GST_AUDIO_INFO_FORMAT(info))
    {
        case GST_AUDIO_FORMAT_F32:
            self->process = (RCPluginAudioEchoProcessFunc)
                rc_plugin_audio_echo_transform_float;
            break;
        case GST_AUDIO_FORMAT_F64:
            self->process = (RCPluginAudioEchoProcessFunc)
                rc_plugin_audio_echo_transform_double;
            break;
        default:
            ret = FALSE;
            break;
    }
    g_free (self->buffer);
    self->buffer = NULL;
    self->buffer_pos = 0;
    self->buffer_size = 0;
    self->buffer_size_frames = 0;
    return ret;
}

static gboolean rc_plugin_audio_echo_stop(GstBaseTransform *base)
{
    RCPluginAudioEcho *self = RC_PLUGIN_AUDIO_ECHO(base);
    g_free(self->buffer);
    self->buffer = NULL;
    self->buffer_pos = 0;
    self->buffer_size = 0;
    self->buffer_size_frames = 0;
    return TRUE;
}

#define TRANSFORM_FUNC(name, type) \
static void rc_plugin_audio_echo_transform_##name(RCPluginAudioEcho *self, \
    type *data, guint num_samples) \
{ \
    type *buffer = (type *)self->buffer; \
    guint channels = GST_AUDIO_FILTER_CHANNELS (self); \
    guint rate = GST_AUDIO_FILTER_RATE (self); \
    guint i, j; \
    guint echo_index = self->buffer_size_frames - self->delay_frames; \
    gdouble echo_off = ((((gdouble) self->delay) * rate) / GST_SECOND) - \
        self->delay_frames; \
    \
    if(echo_off < 0.0) echo_off = 0.0; \
    \
    num_samples /= channels; \
    \
    for (i = 0; i < num_samples; i++) \
    { \
        guint echo0_index = ((echo_index + self->buffer_pos) % \
            self->buffer_size_frames) * channels; \
        guint echo1_index = ((echo_index + self->buffer_pos +1) % \
            self->buffer_size_frames) * channels; \
        guint rbout_index = (self->buffer_pos % self->buffer_size_frames) * \
            channels; \
        for(j = 0; j < channels; j++) \
        { \
            gdouble in = data[i*channels + j]; \
            gdouble echo0 = buffer[echo0_index + j]; \
            gdouble echo1 = buffer[echo1_index + j]; \
            gdouble echo = echo0 + (echo1-echo0)*echo_off; \
            type out = in + self->intensity * echo; \
            \
            data[i*channels + j] = out; \
            \
            buffer[rbout_index + j] = in + self->feedback * echo; \
        } \
        self->buffer_pos = (self->buffer_pos + 1) % self->buffer_size_frames; \
    } \
}

TRANSFORM_FUNC(float, gfloat);
TRANSFORM_FUNC(double, gdouble);

static GstFlowReturn rc_plugin_audio_echo_transform_ip(GstBaseTransform *base,
    GstBuffer * buf)
{
    RCPluginAudioEcho *self = RC_PLUGIN_AUDIO_ECHO (base);
    guint num_samples;
    GstClockTime timestamp, stream_time;
    GstMapInfo map;

    g_mutex_lock(&self->lock);
    timestamp = GST_BUFFER_TIMESTAMP (buf);
    stream_time = gst_segment_to_stream_time(&base->segment, GST_FORMAT_TIME,
        timestamp);

    GST_DEBUG_OBJECT(self, "sync to %" GST_TIME_FORMAT,
        GST_TIME_ARGS(timestamp));

    if(GST_CLOCK_TIME_IS_VALID(stream_time))
        gst_object_sync_values(GST_OBJECT(self), stream_time);

    if(self->buffer==NULL)
    {
        guint bpf, rate;
        bpf = GST_AUDIO_FILTER_BPF (self);
        rate = GST_AUDIO_FILTER_RATE (self);
        self->delay_frames =
            MAX (gst_util_uint64_scale (self->delay, rate, GST_SECOND), 1);
        self->buffer_size_frames =
            MAX (gst_util_uint64_scale (self->max_delay, rate, GST_SECOND), 1);
        self->buffer_size = self->buffer_size_frames * bpf;
        self->buffer = g_try_malloc0(self->buffer_size);
        self->buffer_pos = 0;

        if(self->buffer==NULL)
        {
            g_mutex_unlock(&self->lock);
            GST_ERROR_OBJECT(self, "Failed to allocate %u bytes",
                self->buffer_size);
            return GST_FLOW_ERROR;
        }
    }
    gst_buffer_map (buf, &map, GST_MAP_READWRITE);
    num_samples = map.size / GST_AUDIO_FILTER_BPS (self);

    self->process(self, map.data, num_samples);

    gst_buffer_unmap (buf, &map);
    g_mutex_unlock (&self->lock);

    return GST_FLOW_OK;
}

static void rc_plugin_echoeff_load_conf(RCPluginEchoEffectPrivate *priv)
{
    gint ivalue;
    gdouble dvalue;
    if(priv==NULL || priv->keyfile==NULL) return;
    ivalue = g_key_file_get_integer(priv->keyfile, ECHOEFF_ID, "Delay",
        NULL);
    if(ivalue>=0 && ivalue<=1000)
        priv->delay = ivalue * GST_MSECOND;
    if(priv->delay==0) priv->delay = 1;
    dvalue = g_key_file_get_double(priv->keyfile, ECHOEFF_ID, "Feedback",
        NULL);
    if(dvalue>=0.0 && dvalue<=1)
        priv->feedback = dvalue;
    dvalue = g_key_file_get_double(priv->keyfile, ECHOEFF_ID, "Intensity",
        NULL);
    if(dvalue>=0.0 && dvalue<=1)
        priv->intensity = dvalue;
}

static void rc_plugin_echoeff_save_conf(RCPluginEchoEffectPrivate *priv)
{
    if(priv==NULL || priv->keyfile==NULL) return;
    g_key_file_set_integer(priv->keyfile, ECHOEFF_ID, "Delay",
        (gint)(priv->delay / GST_MSECOND));
    g_key_file_set_double(priv->keyfile, ECHOEFF_ID, "Feedback",
        (gdouble)priv->feedback);
    g_key_file_set_double(priv->keyfile, ECHOEFF_ID, "Intensity",
        (gdouble)priv->intensity);    
}

static void rc_plugin_echoeff_delay_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gdouble value = 0.0;
    gint64 delay;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    delay = value * GST_MSECOND;
    if(delay==0) delay = 1;
    g_object_set(priv->echo_element, "delay", delay, NULL);
    priv->delay = delay;
}

static void rc_plugin_echoeff_fb_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->echo_element, "feedback", (gfloat)value, NULL);
    priv->feedback = value;
}

static void rc_plugin_echoeff_intensity_scale_changed_cb(
    GtkRange *range, gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->echo_element, "intensity", (gfloat)value, NULL);
    priv->intensity = value;
}

static gboolean rc_plugin_echoeff_window_delete_event_cb(
    GtkWidget *widget, GdkEvent *event, gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    if(data==NULL) return TRUE;
    gtk_toggle_action_set_active(priv->action, FALSE);
    return TRUE;
}

static inline void rc_plugin_echoeff_window_init(
    RCPluginEchoEffectPrivate *priv)
{
    GtkWidget *echo_delay_label;
    GtkWidget *echo_fb_label, *echo_intensity_label;
    GtkWidget *echo_main_grid;
    if(priv->echo_window!=NULL) return;
    priv->echo_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    echo_delay_label = gtk_label_new(_("Delay"));
    echo_fb_label = gtk_label_new(_("Feedback"));
    echo_intensity_label = gtk_label_new(_("Intensity"));
    priv->echo_delay_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1000.0, 1);
    priv->echo_fb_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    priv->echo_intensity_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    g_object_set(priv->echo_window, "title", _("Echo Effect"),
        "window-position", GTK_WIN_POS_CENTER, "has-resize-grip", FALSE,
        "type-hint", GDK_WINDOW_TYPE_HINT_DIALOG, NULL);
    gtk_range_set_value(GTK_RANGE(priv->echo_delay_scale),
        priv->delay / GST_MSECOND);
    gtk_range_set_value(GTK_RANGE(priv->echo_fb_scale),
        (gdouble)priv->feedback); 
    gtk_range_set_value(GTK_RANGE(priv->echo_intensity_scale),
        (gdouble)priv->intensity);
    gtk_widget_set_hexpand(priv->echo_delay_scale, TRUE);
    gtk_widget_set_hexpand(priv->echo_fb_scale, TRUE);
    gtk_widget_set_hexpand(priv->echo_intensity_scale, TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_delay_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_fb_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_intensity_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_delay_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_fb_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_intensity_scale),
        GTK_POS_RIGHT);
    echo_main_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(echo_main_grid), 3);
    gtk_grid_set_row_spacing(GTK_GRID(echo_main_grid), 4);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_delay_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_delay_scale, 1, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_fb_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_fb_scale, 1, 1,
        1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_intensity_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_intensity_scale, 1, 2,
        1, 1);
    gtk_container_add(GTK_CONTAINER(priv->echo_window), echo_main_grid);
    gtk_widget_set_size_request(priv->echo_window, 300, 120);
    gtk_widget_show_all(echo_main_grid);
    g_signal_connect(priv->echo_window, "delete-event",
        G_CALLBACK(rc_plugin_echoeff_window_delete_event_cb), priv);
    g_signal_connect(G_OBJECT(priv->echo_delay_scale), "value-changed",
        G_CALLBACK(rc_plugin_echoeff_delay_scale_changed_cb), priv);
    g_signal_connect(G_OBJECT(priv->echo_fb_scale), "value-changed",
        G_CALLBACK(rc_plugin_echoeff_fb_scale_changed_cb), priv);
    g_signal_connect(G_OBJECT(priv->echo_intensity_scale), "value-changed",
        G_CALLBACK(rc_plugin_echoeff_intensity_scale_changed_cb), priv);
}

static void rc_plugin_echoeff_view_menu_toggled(GtkToggleAction *toggle,
    gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gboolean flag;
    if(data==NULL) return;
    flag = gtk_toggle_action_get_active(toggle);
    if(flag)
        gtk_window_present(GTK_WINDOW(priv->echo_window));
    else
        gtk_widget_hide(priv->echo_window);
}

static void rc_plugin_echoeff_shutdown_cb(RCLibPlugin *plugin, gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    if(data==NULL) return;
    rc_plugin_echoeff_save_conf(priv);
}

static gboolean rc_plugin_echoeff_load(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
    gboolean flag;
    //priv->echo_element = gst_element_factory_make("audioecho", NULL);
    priv->echo_element = g_object_new(RC_PLUGIN_TYPE_AUDIO_ECHO, NULL);
    flag = rclib_core_effect_plugin_add(priv->echo_element);
    if(!flag)
    {
        gst_object_unref(priv->echo_element);
        priv->echo_element = NULL;
    }
    g_object_set(priv->echo_element, "max-delay", 1 * GST_SECOND,
        "delay", priv->delay, "feedback", priv->feedback, "intensity",
        priv->intensity, NULL);
    rc_plugin_echoeff_window_init(priv);
    priv->action = gtk_toggle_action_new("RC2ViewPluginEchoEffect",
        _("Echo Effect"), _("Show/hide echo audio effect "
        "configuation window"), NULL);
    priv->menu_id = rc_ui_menu_add_menu_action(GTK_ACTION(priv->action),
        "/RC2MenuBar/ViewMenu/ViewSep2", "RC2ViewPluginEchoEffect",
        "RC2ViewPluginEchoEffect", TRUE);
    g_signal_connect(priv->action, "toggled",
        G_CALLBACK(rc_plugin_echoeff_view_menu_toggled), priv);
    return flag;
}

static gboolean rc_plugin_echoeff_unload(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
    if(priv->menu_id>0)
    {
        rc_ui_menu_remove_menu_action(GTK_ACTION(priv->action),
            priv->menu_id);
        g_object_unref(priv->action);
    }
    if(priv->echo_window!=NULL)
    {
        gtk_widget_destroy(priv->echo_window);
        priv->echo_window = NULL;
    }
    if(priv->echo_element!=NULL)
    {
        rclib_core_effect_plugin_remove(priv->echo_element);
        priv->echo_element = NULL;
    }
    return TRUE;
}

static gboolean rc_plugin_echoeff_init(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
    priv->delay = 1;
    priv->keyfile = rclib_plugin_get_keyfile();
    rc_plugin_echoeff_load_conf(priv);
    priv->shutdown_id = rclib_plugin_signal_connect("shutdown",
        G_CALLBACK(rc_plugin_echoeff_shutdown_cb), priv);
    return TRUE;
}

static void rc_plugin_echoeff_destroy(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
    if(priv->shutdown_id>0)
    {
        rclib_plugin_signal_disconnect(priv->shutdown_id);
        priv->shutdown_id = 0;
    }
}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = ECHOEFF_ID,
    .name = N_("Echo Audio Effect"),
    .version = "0.1",
    .description = N_("Add echo audio effect in the music playback."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_echoeff_load,
    .unload = rc_plugin_echoeff_unload,
    .configure = NULL,
    .destroy = rc_plugin_echoeff_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_echoeff_init, rcplugin_info);


