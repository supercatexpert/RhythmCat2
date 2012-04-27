/*
 * RhythmCat Library Core Module
 * The core of the player to play music.
 *
 * rclib-core.c
 * This file is part of RhythmCat Library (LibRhythmCat)
 *
 * Copyright (C) 2011 - SuperCat, license: GPL v3
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

#include "rclib-core.h"
#include "rclib-common.h"
#include "rclib-marshal.h"
#include "rclib-db.h"
#include "rclib-cue.h"
#include "rclib-tag.h"

/**
 * SECTION: rclib-core
 * @Short_description: The audio player core
 * @Title: RCLibCore
 * @Include: rclib-core.h
 *
 * The #RCLibCore is a class which plays audio files, controls the player,
 * and manages sound effects. The core uses GStreamer as its backend.
 */

#define RCLIB_CORE_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RCLIB_CORE_TYPE, RCLibCorePrivate)
#define RCLIB_CORE_ERROR rclib_core_error_quark()

typedef struct RCLibCoreSpectrumChannel
{
    gfloat *input;
    gfloat *input_tmp;
    GstFFTF32Complex *freqdata;
    gfloat *spect_magnitude;
    GstFFTF32 *fft_ctx;
}RCLibCoreSpectrumChannel;

typedef struct RCLibCorePrivate
{
    GstElement *playbin;
    GstElement *audiosink;
    GstElement *videosink;
    GstElement *effectbin;
    GstElement *bal_plugin; /* balance: audiopanorama */
    GstElement *eq_plugin; /* equalizer: equalizer-10bands */
    GstElement *echo_plugin; /* echo: audioecho */
    GList *extra_plugin_list;
    GError *error;
    RCLibCoreSourceType type;
    GstState last_state;
    gint64 start_time;
    gint64 end_time;
    RCLibCoreMetadata metadata;
    RCLibCoreEQType eq_type;
    GSequenceIter *db_reference;
    GSequenceIter *db_reference_pre;
    gpointer ext_reference;
    gpointer ext_reference_pre;
    guint64 num_frames;
    guint64 num_fft;
    guint64 frames_per_interval;
    guint64 frames_todo;
    guint input_pos;
    guint64 error_per_interval;
    guint64 accumulated_error;
    RCLibCoreSpectrumChannel *channel_data;
    GstClockTime message_ts;
}RCLibCorePrivate;

enum
{
    SIGNAL_STATE_CHANGED,
    SIGNAL_EOS,
    SIGNAL_TAG_FOUND,
    SIGNAL_NEW_DURATION,
    SIGNAL_URI_CHANGED,
    SIGNAL_VOLUME_CHANGED,
    SIGNAL_EQ_CHANGED,
    SIGNAL_BALANCE_CHANGED,
    SIGNAL_ECHO_CHANGED,
    SIGNAL_SPECTRUM_UPDATED,
    SIGNAL_ERROR,
    SIGNAL_LAST
};

struct RCLibCoreEQData
{
    const gchar *name;
    gdouble data[10];
}core_eq_data[RCLIB_CORE_EQ_TYPE_CUSTOM] =
{
    {.name = N_("None"),
     .data = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}},
    {.name = N_("Pop"),
     .data = { 3.0, 1.0, 0.0,-2.0,-4.0,-4.0,-2.0, 0.0, 1.0, 2.0}},
    {.name = N_("Rock"),
     .data = {-2.0, 0.0, 2.0, 4.0,-2.0,-2.0, 0.0, 0.0, 4.0, 4.0}},
    {.name = N_("Metal"),
     .data = {-6.0, 0.0, 0.0, 0.0, 0.0, 0.0, 4.0, 0.0, 4.0, 0.0}},
    {.name = N_("Dance"),
     .data = {-2.0, 3.0, 4.0, 1.0,-2.0,-2.0, 0.0, 0.0, 4.0, 4.0}},
    {.name = N_("Electronic"),
     .data = {-6.0, 1.0, 4.0,-2.0,-2.0,-4.0, 0.0, 0.0, 6.0, 6.0}},
    {.name = N_("Jazz"),
     .data = { 0.0, 0.0, 0.0, 4.0, 4.0, 4.0, 0.0, 2.0, 3.0, 4.0}},
    {.name = N_("Classical"),
     .data = { 0.0, 5.0, 5.0, 4.0, 0.0, 0.0, 0.0, 0.0, 2.0, 2.0}},
    {.name = N_("Blues"),
     .data = {-2.0, 0.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0,-2.0,-4.0}},
    {.name = N_("Vocal"),
     .data = {-4.0, 0.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0,-4.0,-6.0}}
};

static GObject *core_instance = NULL;
static gpointer rclib_core_parent_class = NULL;
static gint core_signals[SIGNAL_LAST] = {0};

static GQuark rclib_core_error_quark()
{
    return g_quark_from_static_string("rclib-core-error-quark");
}

static void rclib_core_metadata_free(RCLibCoreMetadata *metadata)
{
    if(metadata==NULL) return;
    if(metadata->image!=NULL) gst_buffer_unref(metadata->image);
    g_free(metadata->title);
    g_free(metadata->artist);
    g_free(metadata->album);
    g_free(metadata->ftype);
    bzero(metadata, sizeof(RCLibCoreMetadata));
}

static gboolean rclib_core_parse_metadata(const GstTagList *tags,
    GstPad *pad, RCLibCoreMetadata *metadata, guint64 start_time,
    guint64 end_time)
{
    gchar *string = NULL;
    guint value = 0;
    gint intv = 0;
    gboolean update = FALSE;
    GstBuffer *buffer = NULL;
    GDate *date;
    GstCaps *caps = NULL;
    GstStructure *structure;
    gint64 duration = 0;
    GstFormat format = GST_FORMAT_TIME;
    if(gst_tag_list_get_string(tags, GST_TAG_TITLE, &string))
    {
        if(metadata->title==NULL)
        {
            metadata->title = string;
            update = TRUE;
        }
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_ARTIST, &string))
    {
        if(metadata->artist==NULL)
        {
            metadata->artist = string;
            update = TRUE;
        }
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_ALBUM, &string))
    {
        if(metadata->album==NULL)
        {
            metadata->album = string;
            update = TRUE;
        }
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &string))
    {
        if(metadata->ftype==NULL)
        {
            metadata->ftype = string;
            update = TRUE;
        }
        else g_free(string);
    }
    if(gst_tag_list_get_buffer(tags, GST_TAG_IMAGE, &buffer))
    {
        if(metadata->image==NULL)
        {
            metadata->image = buffer;
            update = TRUE;
        }
        else gst_buffer_unref(buffer);
    }
    if(gst_tag_list_get_buffer(tags, GST_TAG_PREVIEW_IMAGE, &buffer))
    {
        if(metadata->image==NULL)
        {
            metadata->image = buffer;
            update = TRUE;
        }
        else gst_buffer_unref(buffer);
    }
    if(gst_tag_list_get_uint(tags, GST_TAG_NOMINAL_BITRATE, &value))
    {
        if(metadata->bitrate!=value)
        {
            metadata->bitrate = value;
            update = TRUE;
        }
    }
    if(gst_tag_list_get_uint(tags, GST_TAG_TRACK_NUMBER, &value))
    {
        if(metadata->track!=value)
        {
            metadata->track = value;
            update = TRUE;
        }
    }
    if(gst_tag_list_get_date(tags, GST_TAG_DATE, &date))
    {
        intv = g_date_get_year(date);
        g_date_free(date);
        if(metadata->year!=intv)
        {
            metadata->year = intv;
            update = TRUE;
        }
    }
    if(pad!=NULL)
    {
        caps = gst_pad_get_negotiated_caps(pad);
        if(caps!=NULL)
        {
            structure = gst_caps_get_structure(caps, 0);
            if(gst_structure_get_int(structure, "rate",
                &intv))
            {
                if(metadata->rate!=intv)
                {
                    metadata->rate = intv;
                    update = TRUE;
                }
            }
            if(gst_structure_get_int(structure, "channels",
                &intv))
            {
                if(metadata->channels!=intv)
                {
                    metadata->channels = intv;
                    update = TRUE;
                }
            }
            gst_caps_unref(caps);
        }
        if(end_time>0 && end_time>start_time)
            duration = end_time - start_time;
        else if(gst_pad_query_duration(pad, &format, &duration))
            if(duration<0) duration = 0;
        if(start_time>0 && duration>start_time)
            duration = duration - start_time;
        if(duration>=0 && metadata->duration!=duration)
        {
            metadata->duration = duration;
            update = TRUE;
        }
    }
    return update;
}

static void rclib_core_spectrum_flush(RCLibCorePrivate *priv)
{
    priv->num_frames = 0;
    priv->num_fft = 0;
    priv->accumulated_error = 0;
}

static void rclib_core_spectrum_alloc_channel_data(RCLibCorePrivate *priv,
    guint bands)
{
    RCLibCoreSpectrumChannel *cd;
    guint nfft = 2 * bands - 2;
    if(priv->channel_data!=NULL) return;
    priv->channel_data = g_new(RCLibCoreSpectrumChannel, 1);
    cd = priv->channel_data;
    cd->fft_ctx = gst_fft_f32_new(nfft, FALSE);
    cd->input = g_new0(gfloat, nfft);
    cd->input_tmp = g_new0(gfloat, nfft);
    cd->freqdata = g_new0(GstFFTF32Complex, bands);
    cd->spect_magnitude = g_new0(gfloat, bands);
}

static void rclib_core_spectrum_free_channel_data(RCLibCorePrivate *priv)
{
    RCLibCoreSpectrumChannel *cd;
    if(priv->channel_data==NULL) return;
    cd = priv->channel_data;
    if(cd->fft_ctx!=NULL)
        gst_fft_f32_free(cd->fft_ctx);
    g_free(cd->input);
    g_free(cd->input_tmp);
    g_free(cd->freqdata);
    g_free(cd->spect_magnitude);
    g_free(priv->channel_data);
    priv->channel_data = NULL;
}

static void rclib_core_spectrum_reset_state(RCLibCorePrivate *priv)
{
    rclib_core_spectrum_free_channel_data(priv);
    rclib_core_spectrum_flush(priv);
}

static void rclib_core_finalize(GObject *object)
{
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(object));
    if(priv->playbin!=NULL)
        gst_element_set_state(priv->playbin, GST_STATE_NULL);
    if(priv->extra_plugin_list!=NULL)
        g_list_free(priv->extra_plugin_list);
    rclib_core_spectrum_reset_state(priv);
    if(priv->playbin!=NULL)
        gst_object_unref(priv->playbin);
    G_OBJECT_CLASS(rclib_core_parent_class)->finalize(object);
}

static GObject *rclib_core_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(core_instance!=NULL) return core_instance;
    retval = G_OBJECT_CLASS(rclib_core_parent_class)->constructor
        (type, n_construct_params, construct_params);
    core_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&core_instance);
    return retval;
}

static void rclib_core_spectrum_input_data_mixed_float(const guint8 *_in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j, ip = 0;
    gfloat v;
    gfloat *in = (gfloat *)_in;
    for(j=0;j<len;j++)
    {
        v = in[ip++];
        for(i=1;i<channels;i++)
            v+=in[ip++];
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static void rclib_core_spectrum_input_data_mixed_double(const guint8 *_in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j, ip = 0;
    gfloat v;
    gdouble *in = (gdouble *)_in;
    for(j=0;j<len;j++)
    {
        v = in[ip++];
        for(i=1;i<channels;i++)
            v+=in[ip++];
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static void rclib_core_spectrum_input_data_mixed_int32(const guint8 *_in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j, ip = 0;
    gint32 *in = (gint32 *)_in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] * 2 + 1;
        for(i=1;i<channels;i++)
            v += in[ip++] * 2 + 1;
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static void rclib_core_spectrum_input_data_mixed_int32_max(const guint8 *_in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j, ip = 0;
    gint32 *in = (gint32 *) _in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] / max_value;
        for(i=1;i<channels;i++)
            v += in[ip++] / max_value;
        out[op] = v / channels;
        op = (op + 1) % nfft;
  }
}

static void rclib_core_spectrum_input_data_mixed_int24(const guint8 *_in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j;
    gfloat v = 0.0;
    gint32 value;
    for(j=0;j<len;j++)
    {
        for(i=0;i<channels;i++)
        {
            #if G_BYTE_ORDER==G_BIG_ENDIAN
                value = GST_READ_UINT24_BE (_in);
            #else
                value = GST_READ_UINT24_LE (_in);
            #endif
           if(value & 0x00800000)
               value |= 0xff000000;
           v += value * 2 + 1;
           _in += 3;
        }
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static void rclib_core_spectrum_input_data_mixed_int24_max(const guint8 * _in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j;
    gfloat v = 0.0;
    gint32 value;
    for(j=0;j<len;j++)
    {
        for(i=0;i< channels;i++)
        {
            #if G_BYTE_ORDER==G_BIG_ENDIAN
                value = GST_READ_UINT24_BE (_in);
            #else
                value = GST_READ_UINT24_LE (_in);
            #endif
            if(value & 0x00800000)
                value |= 0xff000000;
            v += value / max_value;
            _in += 3;
        }
        out[op] = v / channels;
        op = (op + 1) % nfft;
    }
}

static void rclib_core_spectrum_input_data_mixed_int16(const guint8 *_in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j, ip = 0;
    gint16 *in = (gint16 *)_in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] * 2 + 1;
        for(i=1;i<channels;i++)
            v += in[ip++] * 2 + 1;
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static void rclib_core_spectrum_input_data_mixed_int16_max(const guint8 *_in,
    gfloat *out, guint len, guint channels, gfloat max_value, guint op,
    guint nfft)
{
    guint i, j, ip = 0;
    gint16 *in = (gint16 *)_in;
    gfloat v;
    for(j=0;j<len;j++)
    {
        v = in[ip++] / max_value;
        for(i=1;i<channels;i++)
            v += in[ip++] / max_value;
        out[op] = v / channels;
        op = (op + 1)%nfft;
    }
}

static void rclib_core_spectrum_input_data(const gchar *mimetype, gint width,
    gint depth, const guint8 *in, gfloat *out, guint len, guint channels,
    gfloat _max_value, guint op, guint nfft)
{
    void (*input_data)(const guint8 *_in, gfloat *out, guint len,
        guint channels, gfloat max_value, guint op, guint nfft) = NULL;
    gboolean is_float = FALSE;
    guint max_value = (1UL << (depth - 1)) - 1;
    if(g_strcmp0(mimetype, "audio/x-raw-float")==0)
        is_float = TRUE;
    if(is_float)
    {
        if(width==4)
            input_data = rclib_core_spectrum_input_data_mixed_float;
        else if(width==8)
            input_data = rclib_core_spectrum_input_data_mixed_double;
    }
    else
    {
        if(width==4)
        {
            if(max_value)
                input_data = rclib_core_spectrum_input_data_mixed_int32_max;
            else
                input_data = rclib_core_spectrum_input_data_mixed_int32;
        }
        else if(width==3)
        {
            if(max_value)
                input_data = rclib_core_spectrum_input_data_mixed_int24_max;
            else
                input_data = rclib_core_spectrum_input_data_mixed_int24;
        }
        else if(width==2)
        {
            if(max_value)
                input_data = rclib_core_spectrum_input_data_mixed_int16_max;
            else
                input_data = rclib_core_spectrum_input_data_mixed_int16;
        }
    }
    if(input_data!=NULL)
        input_data(in, out, len, channels, _max_value, op, nfft);
}

static void rclib_core_spectrum_run_fft(RCLibCoreSpectrumChannel *cd,
    guint bands, gfloat threshold, guint input_pos)
{
    guint i;
    guint nfft = 2 * bands - 2;
    gdouble val;
    gfloat *input = cd->input;
    gfloat *input_tmp = cd->input_tmp;
    gfloat *spect_magnitude = cd->spect_magnitude;
    GstFFTF32Complex *freqdata = cd->freqdata;
    GstFFTF32 *fft_ctx = cd->fft_ctx;
    for(i=0;i<nfft;i++)
        input_tmp[i] = input[(input_pos + i) % nfft];
    gst_fft_f32_window(fft_ctx, input_tmp, GST_FFT_WINDOW_HAMMING);
    gst_fft_f32_fft(fft_ctx, input_tmp, freqdata);
    for(i=0;i<bands;i++)
    {
        val = freqdata[i].r * freqdata[i].r;
        val += freqdata[i].i * freqdata[i].i;
        val /= nfft * nfft;
        val = 10.0 * log10 (val);
        if (val < threshold)
            val = threshold;
        spect_magnitude[i] += val;
    }
}

static GValue *rclib_core_spectrum_message_add_container(GstStructure *s,
    GType type, const gchar *name)
{
    GValue v = {0, };
    g_value_init (&v, type);
    gst_structure_set_value(s, name, &v);
    g_value_unset(&v);
    return (GValue *)gst_structure_get_value(s, name);
}

static void rclib_core_spectrum_message_add_array(GValue *cv, gfloat *data,
    guint num_values)
{
    GValue v = {0, };
    guint i;
    g_value_init(&v, G_TYPE_FLOAT);
    for(i=0;i<num_values;i++)
    {
        g_value_set_float(&v, data[i]);
        gst_value_array_append_value(cv, &v);
    }
    g_value_unset(&v);
}

static GstMessage *rclib_core_spectrum_message_new(RCLibCorePrivate *priv,
    guint bands, guint rate, gfloat threshold, GstClockTime timestamp,
    GstClockTime duration)
{
    RCLibCoreSpectrumChannel *cd;
    GstStructure *s;
    GValue *mcv = NULL;
    s = gst_structure_new("spectrum", "timestamp", G_TYPE_UINT64,
        timestamp, "duration", G_TYPE_UINT64, duration, "rate",
        G_TYPE_UINT, rate, "bands", G_TYPE_UINT, bands, "threshold",
        G_TYPE_DOUBLE, threshold, NULL);
    cd = priv->channel_data;
    mcv = rclib_core_spectrum_message_add_container(s, GST_TYPE_ARRAY,
        "magnitude");
    rclib_core_spectrum_message_add_array(mcv, cd->spect_magnitude, bands);
    return gst_message_new_element(GST_OBJECT(priv->playbin), s);
}

static void rclib_core_spectrum_reset_message_data(guint bands,
    RCLibCoreSpectrumChannel *cd)
{
    gfloat *spect_magnitude = cd->spect_magnitude;
    memset(spect_magnitude, 0, bands*sizeof(gfloat));
}

static void rclib_core_class_init(RCLibCoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_core_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_core_finalize;
    object_class->constructor = rclib_core_constructor;
    g_type_class_add_private(klass, sizeof(RCLibCorePrivate));
    
    /**
     * RCLibCore::state-changed:
     * @core: the #RCLibCore that received the signal
     * @state: the state of the core
     * 
     * The ::state-changed signal is emitted when the state of the core
     * is changed.
     */
    core_signals[SIGNAL_STATE_CHANGED] = g_signal_new("state-changed",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        state_changed), NULL, NULL, g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT, NULL);

    /**
     * RCLibCore::eos:
     * @core: the #RCLibCore that received the signal
     * 
     * The ::eos signal is emitted when end-of-stream reached in the
     * current pipeline of the core.
     */
    core_signals[SIGNAL_EOS] = g_signal_new("eos", RCLIB_CORE_TYPE,
        G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass, eos),
        NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0, G_TYPE_NONE, NULL);
    
    /**
     * RCLibCore::tag-found:
     * @core: the #RCLibCore that received the signal
     * @metadata: the metadata
     * @uri: the URI
     * 
     * The ::tag-found signal is emitted when new metadata (tag) was
     * found.
     */
    core_signals[SIGNAL_TAG_FOUND] = g_signal_new("tag-found",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        tag_found), NULL, NULL, rclib_marshal_VOID__POINTER_STRING,
        G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_STRING, NULL);

    /**
     * RCLibCore::new-duration:
     * @core: the #RCLibCore that received the signal
     * @duration: the duration (unit: nanosecond)
     * 
     * The ::new-duration signal is emitted when new duration information
     * was found.
     */
    core_signals[SIGNAL_NEW_DURATION] = g_signal_new("new-duration",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        new_duration), NULL, NULL, rclib_marshal_VOID__INT64,
        G_TYPE_NONE, 1, G_TYPE_INT64, NULL);

    /**
     * RCLibCore::uri-changed:
     * @core: the #RCLibCore that received the signal
     * @uri: the URI
     * 
     * The ::uri-changed signal is emitted when new URI was set.
     */
    core_signals[SIGNAL_URI_CHANGED] = g_signal_new("uri-changed",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        uri_changed), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
        
    /**
     * RCLibCore::volume-changed:
     * @core: the #RCLibCore that received the signal
     * @volume: the volume (from 0.0 to 1.0)
     *
     * The ::volume-changed signal is emitted when the volume was changed.
     */
    core_signals[SIGNAL_VOLUME_CHANGED] = g_signal_new("volume-changed",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        volume_changed), NULL, NULL, g_cclosure_marshal_VOID__DOUBLE,
        G_TYPE_NONE, 1, G_TYPE_DOUBLE, NULL);
        
    /**
     * RCLibCore::eq-changed:
     * @core: the #RCLibCore that received the signal
     * @type: the equalizer style type
     * @values: an array of 10 #gdouble elements for equalizer settings
     *
     * The ::eq-changed signal is emitted when new equalizer settings were
     * applied.
     */
    core_signals[SIGNAL_EQ_CHANGED] = g_signal_new("eq-changed",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        eq_changed), NULL, NULL, g_cclosure_marshal_VOID__UINT_POINTER,
        G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER, NULL);

    /**
     * RCLibCore::balance-changed:
     * @core: the #RCLibCore that received the signal
     * @balance: the balance value (from -1.0 (left) to 1.0 (right))
     *
     * The ::balance-changed signal is emitted when new balance was applied.
     */
    core_signals[SIGNAL_BALANCE_CHANGED] = g_signal_new("balance-changed",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        balance_changed), NULL, NULL, g_cclosure_marshal_VOID__FLOAT,
        G_TYPE_NONE, 1, G_TYPE_FLOAT, NULL);
        
    /**
     * RCLibCore::echo-changed:
     * @core: the #RCLibCore that received the signal
     *
     * The ::echo-changed signal is emitted when new echo settings
     * were applied.
     */
    core_signals[SIGNAL_ECHO_CHANGED] = g_signal_new("echo-changed",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        echo_changed), NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0, G_TYPE_NONE, NULL);

    /**
     * RCLibCore::spectrum-updated:
     * @core: the #RCLibCore that received the signal
     * @rate: the sample rate
     * @bands: the band number
     * @threshold: the threshold
     * @magnitudes: the magnitude array
     *
     * The ::spectrum-updated signal is emitted when the spectrum
     * message is received.
     */
    core_signals[SIGNAL_SPECTRUM_UPDATED] = g_signal_new("spectrum-updated",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        spectrum_updated), NULL, NULL,
        rclib_marshal_VOID__UINT_UINT_FLOAT_POINTER, G_TYPE_NONE, 4,
        G_TYPE_UINT, G_TYPE_UINT, G_TYPE_FLOAT, G_TYPE_POINTER,
        NULL);

    /**
     * RCLibCore::error:
     * @core: the #RCLibCore that received the signal
     *
     * The ::error signal is emitted when any error in the core occurs.
     */
    core_signals[SIGNAL_ERROR] = g_signal_new("error",
        RCLIB_CORE_TYPE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        error), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
}

static gboolean rclib_core_effect_add_element_nonblock(GstElement *effectbin,
    GstElement *new_element)
{
    GstPad *binsinkpad;
    GstPad *binsrcpad;
    GstPad *realpad;
    GstPad *prevpad;
    GstElement *bin = NULL;
    GstElement *identity = NULL;
    GstElement *audioconvert = NULL;
    GstElement *audioconvert2 = NULL;
    GstPadLinkReturn link;
    gboolean flag = FALSE;
    G_STMT_START
    {
        audioconvert = gst_element_factory_make("audioconvert", NULL);
        if(audioconvert==NULL) break;
        audioconvert2 = gst_element_factory_make("audioconvert", NULL);
        if(audioconvert2==NULL) break;
        bin = gst_bin_new(NULL);
        if(bin==NULL) break;
        gst_bin_add_many(GST_BIN(bin), audioconvert, new_element,
            audioconvert2, NULL);
        if(!gst_element_link_many(audioconvert, new_element, audioconvert2,
            NULL))
            break;
        flag = TRUE;
    }
    G_STMT_END;
    if(!flag)
    {
        if(bin!=NULL)
            gst_object_unref(bin);
        else
        {
            if(audioconvert!=NULL) gst_object_unref(audioconvert);
            if(audioconvert2!=NULL) gst_object_unref(audioconvert2);
        }
        return FALSE;
    }
    realpad = gst_element_get_static_pad(audioconvert, "sink");
    binsinkpad = gst_ghost_pad_new("sink", realpad);
    gst_element_add_pad(bin, binsinkpad);
    gst_object_unref(realpad);
    realpad = gst_element_get_static_pad(audioconvert2, "src");
    binsrcpad = gst_ghost_pad_new("src", realpad);
    gst_element_add_pad(bin, binsrcpad);
    gst_object_unref(realpad);
    gst_bin_add(GST_BIN(effectbin), bin);
    identity = gst_bin_get_by_name(GST_BIN(effectbin), "effect-identity");
    realpad = gst_element_get_static_pad(identity, "sink");
    prevpad = gst_pad_get_peer(realpad);
    gst_object_unref(identity);
    gst_pad_unlink(prevpad, realpad);
    link = gst_pad_link(prevpad, binsinkpad);
    gst_object_unref(prevpad);
    if(link!=GST_PAD_LINK_OK)
    {
        gst_pad_link(prevpad, realpad);
        gst_object_unref(realpad);
        gst_bin_remove(GST_BIN(effectbin), bin);
        gst_object_unref(bin);
        return FALSE;
    }
    link = gst_pad_link(binsrcpad, realpad);
    gst_object_unref(realpad);
    if(link!=GST_PAD_LINK_OK)
    {
        return FALSE;
    }
    return TRUE;
}

static void rclib_core_effect_remove_element_nonblock(GstElement *effectbin,
    GstElement *element)
{
    GstPad *mypad;
    GstPad *prevpad, *nextpad;
    GstElement *bin;
    bin = GST_ELEMENT(gst_element_get_parent(element));
    if(bin==NULL) return;
    mypad = gst_element_get_static_pad(bin, "sink");
    prevpad = gst_pad_get_peer(mypad);
    gst_pad_unlink(prevpad, mypad);
    gst_object_unref(mypad);
    mypad = gst_element_get_static_pad (bin, "src");
    nextpad = gst_pad_get_peer (mypad);
    gst_pad_unlink (mypad, nextpad);
    gst_object_unref (mypad);
    gst_pad_link (prevpad, nextpad);
    gst_object_unref (prevpad);
    gst_object_unref (nextpad);
    gst_bin_remove(GST_BIN(effectbin), bin);
    gst_object_unref(bin);
}

static gboolean rclib_core_bus_callback(GstBus *bus, GstMessage *msg,
    gpointer data)
{
    gint64 duration = 0;
    gchar *uri;
    GstQuery *query;
    GstFormat format = GST_FORMAT_TIME;
    RCLibCorePrivate *priv = NULL;
    GObject *object = G_OBJECT(data);
    const GstStructure *structure;
    const GValue *magnitudes;
    GstTagList *tags;
    GstState state, pending;
    GError *error = NULL;
    gchar *error_msg;
    GstPad *pad = NULL;
    const gchar *name;
    guint bands = 0;
    guint rate = 0;
    gdouble threshold = -60;
    if(object==NULL) return TRUE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(object));    
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            gst_element_set_state(priv->playbin, GST_STATE_NULL); 
            gst_element_set_state(priv->playbin, GST_STATE_READY);
            g_signal_emit(object, core_signals[SIGNAL_EOS], 0);
            break;
        case GST_MESSAGE_TAG:
            gst_message_parse_tag_full(msg, &pad, &tags);
            query = gst_query_new_uri();
            gst_element_query(priv->playbin, query);
            gst_query_parse_uri(query, &uri);         
            if(rclib_core_parse_metadata(tags, pad, &(priv->metadata),
                priv->start_time, priv->end_time))
            {
                g_signal_emit(object, core_signals[SIGNAL_TAG_FOUND], 0,
                    &(priv->metadata), uri);
            }
            g_free(uri);
            gst_tag_list_free(tags);
            if(pad!=NULL) gst_object_unref(pad);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if(priv!=NULL && GST_MESSAGE_SRC(msg)==GST_OBJECT(priv->playbin))
            {
                gst_message_parse_state_changed(msg, NULL, &state, &pending);
                if(pending==GST_STATE_VOID_PENDING)
                {
                    priv->last_state = state;
                    g_signal_emit(object, core_signals[SIGNAL_STATE_CHANGED],
                        0, state);
                    g_debug("Core state changed to: %s",
                        gst_element_state_get_name(state));
                }
            }
            break;
        case GST_MESSAGE_DURATION:
            gst_message_parse_duration(msg, &format, &duration);
            if(format!=GST_FORMAT_TIME)
            {
                format = GST_FORMAT_TIME;
                break;
            }
            if(priv->end_time>0 && priv->end_time>priv->start_time)
                duration = priv->end_time - priv->start_time;
            else if(priv->start_time>0 && duration>priv->start_time)
                duration = duration - priv->start_time;
            g_signal_emit(object, core_signals[SIGNAL_NEW_DURATION], 0,
                duration);
            break;
        case GST_MESSAGE_BUFFERING:
            break;
        case GST_MESSAGE_NEW_CLOCK:
            g_debug("A new clock was selected.");
            if(priv->start_time>0)
            {
                gst_element_seek_simple(priv->playbin, GST_FORMAT_TIME,
                    GST_SEEK_FLAG_FLUSH,
                    priv->start_time);
            }
            duration = rclib_core_query_duration();
            if(duration>0)
                g_signal_emit(object, core_signals[SIGNAL_NEW_DURATION], 0,
                    duration);
            break;
        case GST_MESSAGE_ELEMENT:
            structure = gst_message_get_structure(msg);
            if(structure==NULL) break;
            name = gst_structure_get_name(structure);
            if(g_strcmp0(name, "spectrum")==0)
            {
                gst_structure_get_uint(structure, "bands", &bands);
                gst_structure_get_uint(structure, "rate", &rate);
                gst_structure_get_double(structure, "threshold", &threshold);
                magnitudes = gst_structure_get_value(structure,
                    "magnitude");
                if(bands==0 || rate==0 || magnitudes==NULL) break;
                g_signal_emit(object, core_signals[SIGNAL_SPECTRUM_UPDATED],
                    0, rate, bands, threshold, magnitudes);
            }
            break;
        case GST_MESSAGE_ERROR:
            error = NULL;
            error_msg = NULL;
            gst_message_parse_error(msg, &error, &error_msg);
            g_warning("%s\nDEBUG: %s", error->message, error_msg);
            g_error_free(error);
            g_signal_emit(object, core_signals[SIGNAL_ERROR], 0, error_msg);
            g_free(error_msg);
            if(!gst_element_post_message(priv->playbin,
                gst_message_new_eos(GST_OBJECT(priv->playbin))))
                rclib_core_stop();
            break;
        case GST_MESSAGE_WARNING:
            error = NULL;
            error_msg = NULL;
            gst_message_parse_warning(msg, &error, &error_msg);
            g_warning("%s\nDEBUG: %s", error->message, error_msg);
            g_error_free(error);
            g_free(error_msg);
            break;
        default:
            break;
    }
    return TRUE;
}

static gboolean rclib_core_pad_buffer_probe_cb(GstPad *pad, GstBuffer *buf,
    gpointer data)
{
    GstMessage *msg;
    gint i;
    gint64 pos, len;
    gint rate = 0;
    gint channels = 0;
    gint width = 0;
    gint depth = 0;
    GstCaps *caps;
    GstStructure *structure;
    RCLibCoreSpectrumChannel *cd;
    RCLibCorePrivate *priv = NULL;
    GObject *object = G_OBJECT(data);
    const guint8 *buf_data = GST_BUFFER_DATA(buf);
    gfloat max_value;
    gfloat threshold = -60;
    guint bands = 128;
    guint nfft = 2 * bands - 2;
    guint size = GST_BUFFER_SIZE(buf);
    gint64 interval = GST_SECOND / 10;
    gint frame_size;
    gfloat *input;
    guint input_pos;
    guint fft_todo, msg_todo, block_size;
    gboolean have_full_interval;
    const gchar *mimetype;
    if(object==NULL) return TRUE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(object));
    if(priv==NULL) return TRUE;
    pos = (gint64)GST_BUFFER_TIMESTAMP(buf);
    len = rclib_core_query_duration();
    if(priv->end_time>0)
    {
        if(priv->end_time<pos)
        {
            msg = gst_message_new_eos(GST_OBJECT(priv->playbin));
            if(!gst_element_post_message(priv->playbin, msg))
                rclib_core_stop();
            g_debug("Reached CUE ending time, EOS message has been "
                "emitted.");
        }
    }
    else if(len>0 && pos-len-priv->start_time>2*GST_SECOND)
    {
        msg = gst_message_new_eos(GST_OBJECT(priv->playbin));
        if(!gst_element_post_message(priv->playbin, msg))
            rclib_core_stop(); 
        g_debug("Out of the duration, EOS message has been emitted.");
    }
    caps = gst_pad_get_negotiated_caps(pad);
    if(caps==NULL) return TRUE;
    structure = gst_caps_get_structure(caps, 0);
    mimetype = gst_structure_get_name(structure);
    gst_structure_get_int(structure, "rate", &rate);
    gst_structure_get_int(structure, "channels", &channels);
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "depth", &depth);
    gst_caps_unref(caps);
    if(depth==0) depth = width;
    if(rate==0 || width==0 || channels==0) return TRUE;
    width = width / 8;
    frame_size = width * channels;
    max_value = (1UL << (depth - 1)) - 1;
    if(GST_BUFFER_IS_DISCONT(buf)) /* Discontinuity detect */
        rclib_core_spectrum_flush(priv);
    if(priv->channel_data==NULL)
    {
        rclib_core_spectrum_alloc_channel_data(priv, bands);
        priv->frames_per_interval = gst_util_uint64_scale(interval,
            rate, GST_SECOND);
        priv->frames_todo = priv->frames_per_interval;
        priv->error_per_interval = (interval * rate) % GST_SECOND;
        if(priv->frames_per_interval==0)
            priv->frames_per_interval = 1;
        priv->input_pos = 0;
        rclib_core_spectrum_flush(priv);
    }
    if(priv->num_frames==0)
        priv->message_ts = GST_BUFFER_TIMESTAMP(buf);
    input_pos = priv->input_pos;
    while(size>=frame_size)
    {
        fft_todo = nfft - (priv->num_frames % nfft);
        msg_todo = priv->frames_todo - priv->num_frames;
        block_size = msg_todo;
        if(block_size > (size / frame_size))
            block_size = (size / frame_size);
        if(block_size > fft_todo)
            block_size = fft_todo;
        cd = priv->channel_data;
        input = cd->input;
        rclib_core_spectrum_input_data(mimetype, width, depth,
            (const guint8 *)buf_data, input, block_size, channels,
            max_value, input_pos, nfft);
        data += block_size * frame_size;
        size -= block_size * frame_size;
        input_pos = (input_pos + block_size) % nfft;
        priv->num_frames += block_size;
        have_full_interval = (priv->num_frames == priv->frames_todo);
        if((priv->num_frames % nfft == 0) ||
            (have_full_interval && !priv->num_fft))
        { 
            cd = priv->channel_data;
            rclib_core_spectrum_run_fft(cd, bands, threshold, input_pos);
            priv->num_fft++;
        }
        if(have_full_interval)
        {
            priv->frames_todo = priv->frames_per_interval;
            if(priv->accumulated_error >= GST_SECOND)
            {
                priv->accumulated_error -= GST_SECOND;
                priv->frames_todo++;
            }
            priv->accumulated_error+=priv->error_per_interval;
            cd = priv->channel_data;
            for(i=0;i<bands;i++)
                cd->spect_magnitude[i] /= priv->num_fft;
            msg = rclib_core_spectrum_message_new(priv, bands, rate,
                threshold, priv->message_ts, interval);
            gst_element_post_message(GST_ELEMENT(priv->playbin), msg);
            if(GST_CLOCK_TIME_IS_VALID(priv->message_ts))
                priv->message_ts +=
            gst_util_uint64_scale(priv->num_frames, GST_SECOND, rate);
            cd = priv->channel_data;
            rclib_core_spectrum_reset_message_data(bands, cd);
            priv->num_frames = 0;
            priv->num_fft = 0;
        }
    }
    priv->input_pos = input_pos;
    return TRUE;
}

static gboolean rclib_core_volume_changed_idle(GObject *object)
{
    gdouble volume;
    g_object_get(G_OBJECT(object), "volume", &volume, NULL);
    g_signal_emit(core_instance, core_signals[SIGNAL_VOLUME_CHANGED], 0,
        volume);
    return FALSE;
}

static void rclib_core_volume_notify_cb(GObject *object, GParamSpec *spec,
    gpointer data)
{
    /*
     * WARNING: The notify::volume signal may come from different thread or
     * process, send it into idle function.
     */
    g_idle_add((GSourceFunc)rclib_core_volume_changed_idle, object);
}

static void rclib_core_source_notify_cb(GObject *object, GParamSpec *spec,
    gpointer data)
{
    RCLibCorePrivate *priv;
    gchar *uri;
    if(data==NULL) return;
    priv = (RCLibCorePrivate *)data;
    priv->db_reference = priv->db_reference_pre;
    if(priv->db_reference!=NULL)
        priv->ext_reference = NULL;
    else
        priv->ext_reference = priv->ext_reference_pre;
    g_debug("Source changed.");
    g_object_get(object, "uri", &uri, NULL);
    g_signal_emit(core_instance, core_signals[SIGNAL_URI_CHANGED], 0,
        uri);
    g_debug("New URI is set: %s", uri);
    g_free(uri);
}

static void rclib_core_instance_init(RCLibCore *core)
{
    GstElement *playbin = NULL;
    GstElement *audiobin = NULL;
    GstElement *effectbin = NULL;
    GstElement *audiosink = NULL;
    GstElement *videosink = NULL;
    GstElement *audioconvert = NULL;
    GstElement *identity = NULL;
    GError *error = NULL;
    GstPad *pad;
    GstBus *bus;
    RCLibCorePrivate *priv;
    gboolean flag;
    flag = FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(core);
    G_STMT_START
    {
        playbin = gst_element_factory_make("playbin2", "rclib-playbin");
        if(playbin==NULL)
        {
            g_warning("Cannot load necessary plugin: %s", "playbin2");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_MISSING_CORE_PLUGIN,
                _("Cannot load necessary plugin: %s"), "playbin2");
            break;
        }
        videosink = gst_element_factory_make("fakesink", "rclib-videosink");
        if(videosink==NULL)
        {
            g_warning("Cannot load necessary plugin: %s", "fakesink");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_MISSING_CORE_PLUGIN,
                _("Cannot load necessary plugin: %s"), "fakesink");
            break;
        }
        audiosink = gst_element_factory_make("autoaudiosink",
            "rclib-audiosink");
        if(audiosink==NULL)
        {
            g_warning("Cannot load necessary plugin: %s", "autoaudiosink");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_MISSING_CORE_PLUGIN,
                _("Cannot load necessary plugin: %s"), "autoaudiosink");
            break;
        }
        audioconvert = gst_element_factory_make("audioconvert",
            "effect-audioconvert");
        if(audioconvert==NULL)
        {
            g_warning("Cannot load necessary plugin: %s", "audioconvert");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_MISSING_CORE_PLUGIN,
                _("Cannot load necessary plugin: %s"), "audioconvert");
            break;
        }
        identity = gst_element_factory_make("identity",
            "effect-identity");
        if(identity==NULL)
        {
            g_warning("Cannot load necessary plugin: %s", "identify");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_MISSING_CORE_PLUGIN,
                _("Cannot load necessary plugin: %s"), "identity");
            break;
        }
        effectbin = gst_bin_new("rclib-effectbin");
        if(effectbin==NULL)
        {
            g_warning("Cannot create bin: %s", "effectbin");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_CREATE_BIN_FAILED,
                _("Cannot create bin: %s"), "effectbin");
            break;
        }
        gst_bin_add_many(GST_BIN(effectbin), audioconvert, identity, NULL);
        if(!gst_element_link(audioconvert, identity))
        {
            g_warning("Cannot link elements!");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_LINK_FAILED,
                _("Cannot link elements!"));
            break;
        }
        pad = gst_element_get_static_pad(audioconvert, "sink");
        gst_element_add_pad(effectbin, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);
        pad = gst_element_get_static_pad(identity, "src");
        gst_element_add_pad(effectbin, gst_ghost_pad_new("src", pad));
        gst_object_unref(pad);
        audiobin = gst_bin_new("rclib-audiobin");
        if(audiobin==NULL)
        {
            g_warning("Cannot create bin: %s", "audiobin");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_CREATE_BIN_FAILED,
                _("Cannot create bin: %s"), "audiobin");
            break;
        }
        gst_bin_add_many(GST_BIN(audiobin), effectbin, audiosink, NULL);
        if(!gst_element_link_many(effectbin, audiosink, NULL))
        {
            g_warning("Cannot link elements!");
            g_set_error(&error, RCLIB_CORE_ERROR,
                RCLIB_CORE_ERROR_LINK_FAILED,
                _("Cannot link elements!"));
            break;
        }
        pad = gst_element_get_static_pad(effectbin, "sink");
        gst_element_add_pad(audiobin, gst_ghost_pad_new(NULL, pad));
        gst_object_unref(pad);
        flag = TRUE;
    }
    G_STMT_END;
    if(!flag)
    {
        if(playbin!=NULL) gst_object_unref(playbin);
        if(videosink!=NULL) gst_object_unref(videosink);
        if(audiobin!=NULL)
            gst_object_unref(audiobin);
        else
        {
            if(audiosink!=NULL) gst_object_unref(audiosink);
            if(effectbin!=NULL)
                gst_object_unref(effectbin);
            else
            {
                if(audioconvert!=NULL) gst_object_unref(audioconvert);
                if(identity!=NULL) gst_object_unref(identity);
            }

        }
        priv->error = error;
        return;
    }
    g_object_set(G_OBJECT(videosink), "sync", TRUE, NULL);
    g_object_set(G_OBJECT(playbin), "audio-sink", audiobin, "video-sink",
        videosink, NULL);
    bzero(&(priv->metadata), sizeof(RCLibCoreMetadata));
    priv->playbin = playbin;
    priv->effectbin = effectbin;
    priv->audiosink = audiosink;
    priv->videosink = videosink;
    priv->channel_data = NULL;
    priv->eq_plugin = gst_element_factory_make("equalizer-10bands",
        "effect-equalizer");
    priv->bal_plugin = gst_element_factory_make("audiopanorama",
        "effect-balance");
    priv->echo_plugin = gst_element_factory_make("audioecho",
        "effect-echo");
    if(priv->eq_plugin!=NULL)
        rclib_core_effect_add_element_nonblock(effectbin, priv->eq_plugin);
    if(priv->bal_plugin!=NULL)
    {
        g_object_set(priv->bal_plugin, "method", 1, NULL);
        rclib_core_effect_add_element_nonblock(effectbin, priv->bal_plugin);
    }
    if(priv->echo_plugin!=NULL)
    {
        g_object_set(G_OBJECT(priv->echo_plugin), "max-delay",
            1 * GST_SECOND, NULL);
        rclib_core_effect_add_element_nonblock(effectbin, priv->echo_plugin);
    }
    priv->extra_plugin_list = NULL;
    bus = gst_pipeline_get_bus(GST_PIPELINE(playbin));
    gst_bus_add_watch(bus, (GstBusFunc)rclib_core_bus_callback,
        core);
    gst_object_unref(bus);
    pad = gst_element_get_static_pad(audioconvert, "src");
    gst_pad_add_buffer_probe(pad, G_CALLBACK(rclib_core_pad_buffer_probe_cb),
       core);
    gst_object_unref(pad);
    g_signal_connect(priv->playbin, "notify::volume",
        G_CALLBACK(rclib_core_volume_notify_cb), NULL);
    g_signal_connect(priv->playbin, "notify::source",
        G_CALLBACK(rclib_core_source_notify_cb), priv);
    gst_element_set_state(playbin, GST_STATE_NULL);
    gst_element_set_state(playbin, GST_STATE_READY);
    rclib_core_spectrum_reset_state(priv);
}

GType rclib_core_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo core_info = {
        .class_size = sizeof(RCLibCoreClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_core_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibCore),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_core_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibCore"), &core_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_core_init:
 * @error: return location for a GError, or NULL
 *
 * Initialize the core, if the initialization failed, it returns FALSE,
 * and @error will be set.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rclib_core_init(GError **error)
{
    RCLibCorePrivate *priv;
    g_message("Loading core....");
    if(core_instance!=NULL)
    {
        g_warning("The core is already initialized!");
        g_set_error(error, RCLIB_CORE_ERROR, RCLIB_CORE_ERROR_ALREADY_INIT,
            _("The core is already initialized!"));
        return FALSE;
    }
    core_instance = g_object_new(RCLIB_CORE_TYPE, NULL);
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->playbin==NULL)
    {
        if(error!=NULL) *error = priv->error;
        g_object_unref(core_instance);
        core_instance = NULL;
        return FALSE;
    }
    g_message("Core loaded.");
    return TRUE;
}

/**
 * rclib_core_exit:
 *
 * Unload the core.
 */

void rclib_core_exit()
{
    if(core_instance!=NULL) g_object_unref(core_instance);
    core_instance = NULL;
    g_message("Core exited.");
}

/**
 * rclib_core_get_instance:
 *
 * Get the running #RCLibCore instance.
 *
 * Returns: The running instance.
 */

GObject *rclib_core_get_instance()
{
    return core_instance;
}

/**
 * rclib_core_signal_connect:
 * @name: the name of the signal
 * @callback: the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibCore object.
 *
 * Returns: The handler ID.
 */

gulong rclib_core_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(core_instance==NULL) return 0;
    return g_signal_connect(core_instance, name, callback, data);
}

/**
 * rclib_core_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running core instance so it will
 * not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_core_signal_disconnect(gulong handler_id)
{
    if(core_instance==NULL) return;
    g_signal_handler_disconnect(core_instance, handler_id);
}

/**
 * rclib_core_set_uri:
 * @uri: the URI to play
 * @db_reference: the databse reference to set, set to NULL if not used
 * @external_reference: the exterenal reference to set, the database
 * reference must be set to NULL if you want to use this reference
 *
 * Set the URI to play.
 */

void rclib_core_set_uri(const gchar *uri, GSequenceIter *db_reference,
    gpointer external_reference)
{
    RCLibCorePrivate *priv;
    gchar *scheme;
    gchar *cue_uri;
    gint track = 0;
    RCLibTagMetadata *cue_mmd;
    RCLibCueData cue_data;
    gboolean cue_flag = FALSE;
    gboolean emb_cue_flag = FALSE;
    if(core_instance==NULL || uri==NULL) return;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    gst_element_set_state(priv->playbin, GST_STATE_NULL);
    gst_element_set_state(priv->playbin, GST_STATE_READY);
    rclib_core_set_position(0);
    priv->db_reference = NULL;
    priv->ext_reference_pre = NULL;
    scheme = g_uri_parse_scheme(uri);
    /* We can only read CUE file on local machine. */
    if(g_strcmp0(scheme, "file")==0)
    {
        if(rclib_cue_get_track_num(uri, &cue_uri, &track))
        {
            if(g_regex_match_simple("(.CUE)$", cue_uri, G_REGEX_CASELESS,
                0))
            {
                if(rclib_cue_read_data(cue_uri, RCLIB_CUE_INPUT_URI,
                    &cue_data)>0)
                {
                    cue_flag = TRUE;
                }
            }
            else
            {
                cue_mmd = rclib_tag_read_metadata(cue_uri);
                if(cue_mmd!=NULL && cue_mmd->audio_flag &&
                    cue_mmd->emb_cue!=NULL)
                {
                    if(rclib_cue_read_data(cue_mmd->emb_cue,
                        RCLIB_CUE_INPUT_EMBEDDED, &cue_data)>0)
                    {
                        cue_flag = TRUE;
                        emb_cue_flag = TRUE;
                        cue_data.file = g_strdup(cue_uri);
                    }
                }
                if(cue_mmd!=NULL) rclib_tag_free(cue_mmd);
            }
            g_free(cue_uri);
        }
    }
    g_free(scheme);
    rclib_core_metadata_free(&(priv->metadata));
    priv->db_reference_pre = db_reference;
    priv->ext_reference_pre = external_reference;
    if(cue_flag)
    {
        priv->start_time = cue_data.track[track-1].time1;
        if(track!=cue_data.length)
            priv->end_time = cue_data.track[track].time1;
        else
            priv->end_time = 0;
        if(cue_data.track[track-1].title!=NULL)
            priv->metadata.title =
                g_strdup(cue_data.track[track-1].title);
        if(cue_data.track[track-1].performer!=NULL)
            priv->metadata.artist =
                g_strdup(cue_data.track[track-1].performer);
        if(cue_data.title!=NULL)
            priv->metadata.album = g_strdup(cue_data.title);
        g_object_set(G_OBJECT(priv->playbin), "uri", cue_data.file, NULL);
        rclib_cue_free(&cue_data);
        if(emb_cue_flag)
            priv->type = RCLIB_CORE_SOURCE_EMBEDDED_CUE;
        else
            priv->type = RCLIB_CORE_SOURCE_CUE;
    }
    else
    {
        priv->start_time = 0;
        priv->end_time = 0;
        priv->type = RCLIB_CORE_SOURCE_NORMAL;
        g_object_set(G_OBJECT(priv->playbin), "uri", uri, NULL);
    }
    gst_element_set_state(priv->playbin, GST_STATE_PAUSED);
}

/**
 * rclib_core_update_db_reference:
 * @new_ref: the new databse reference to set,
 * set to NULL if not used
 *
 * Update the database reference.
 */

void rclib_core_update_db_reference(GSequenceIter *new_ref)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return;
    priv->db_reference = new_ref;
}

/**
 * rclib_core_update_external_reference:
 * @new_ref: the new databse reference to set, set to NULL if not used
 *
 * Update the external reference.
 */

void rclib_core_update_external_reference(gpointer new_ref)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL || priv->db_reference!=NULL) return;
    priv->ext_reference = new_ref;
}

/**
 * rclib_core_get_uri:
 *
 * Get the URI.
 *
 * Returns: The URI, NULL if not set.
 */

gchar *rclib_core_get_uri()
{
    RCLibCorePrivate *priv;
    gchar *uri = NULL;
    if(core_instance==NULL) return NULL;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return NULL;
    g_object_get(G_OBJECT(priv->playbin), "uri", &uri, NULL);
    return uri;
}

/**
 * rclib_core_get_db_reference:
 *
 * Get the database reference.
 *
 * Returns: The database reference.
 */

GSequenceIter *rclib_core_get_db_reference()
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return NULL;
    return priv->db_reference;
}

/**
 * rclib_core_get_external_reference:
 *
 * Get the external reference.
 *
 * Returns: The external reference.
 */

gpointer rclib_core_get_external_reference()
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return NULL;
    return priv->ext_reference;
}

/**
 * rclib_core_get_source_type:
 *
 * Get source type of the URI.
 *
 * Returns: The source type.
 */

RCLibCoreSourceType rclib_core_get_source_type()
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return RCLIB_CORE_SOURCE_NONE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return RCLIB_CORE_SOURCE_NONE;
    return priv->type;
}

/**
 * rclib_core_get_metadata:
 *
 * Get metadata read from the loaded file in the core.
 *
 * Returns: The metadata.
 */

const RCLibCoreMetadata *rclib_core_get_metadata()
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return NULL;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return NULL;
    return &(priv->metadata);
}

/**
 * rclib_core_set_position:
 * @pos: the position to set
 *
 * Set the position for the player (in nanosecond).
 * Notice that this function can only be used when this player is playing
 * or paused.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_core_set_position(gint64 pos)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->start_time>0)
    {
        return gst_element_seek_simple(priv->playbin, GST_FORMAT_TIME, 
            GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE,
            pos + priv->start_time);
    }
    else
    {
        return gst_element_seek_simple(priv->playbin, GST_FORMAT_TIME, 
            GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, pos);
    }
}

/**
 * rclib_core_query_position:
 *
 * Get the playing position in the player (in nanosecond).
 *
 * Returns: The playing position (in nanosecond).
 */

gint64 rclib_core_query_position()
{
    RCLibCorePrivate *priv;
    gint64 position = 0;
    GstFormat format = GST_FORMAT_TIME;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(gst_element_query_position(priv->playbin, &format, &position))
    {
        if(position<0) position = 0;
    }
    else return 0;
    if(priv->start_time>0 && position>priv->start_time)
        position = position - priv->start_time;
    return position;
}

/**
 * rclib_core_query_duration:
 *
 * Return the duration of the music set on this player (in nanosecond).
 *
 * Returns: The duration (in nanosecond).
 */

gint64 rclib_core_query_duration()
{
    RCLibCorePrivate *priv;
    gint64 duration = 0;
    GstFormat format = GST_FORMAT_TIME;
    if(core_instance==NULL) return 0;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->end_time>0 && priv->end_time>priv->start_time)
    {
        duration = priv->end_time - priv->start_time;
        return duration;
    }
    if(gst_element_query_duration(priv->playbin, &format, &duration))
    {
        if(duration<0) duration = 0;
    }
    if(priv->start_time>0 && duration>priv->start_time)
        duration = duration - priv->start_time;
    return duration;  
}

/**
 * rclib_core_get_state:
 * @state: a pointer to #GstState to hold the state. Can be NULL
 * @pending: a pointer to #GstState to hold the pending state. Can be NULL
 * @timeout: a #GstClockTime to specify the timeout for an async state
 * change or GST_CLOCK_TIME_NONE for infinite timeout
 *
 * Gets the state of the element.
 *
 * Returns: #GST_STATE_CHANGE_SUCCESS if the element has no more pending
 * state and the last state change succeeded, #GST_STATE_CHANGE_ASYNC if
 * the element is still performing a state change or
 * #GST_STATE_CHANGE_FAILURE if the last state change failed. MT safe.
 */

GstStateChangeReturn rclib_core_get_state(GstState *state,
    GstState *pending, GstClockTime timeout)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return GST_STATE_CHANGE_FAILURE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    return gst_element_get_state(priv->playbin, state, pending, timeout);
}

/**
 * rclib_core_play:
 *
 * Set the state of the player to play.
 *
 * Returns: Whether the state is set to play successfully.
 */

gboolean rclib_core_play()
{
    RCLibCorePrivate *priv;
    GstState state;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    gst_element_get_state(priv->playbin, &state, NULL, 0);
    if(state!=GST_STATE_PAUSED && state!=GST_STATE_PLAYING &&
        state!=GST_STATE_READY && state!=GST_STATE_NULL)
    {
        gst_element_set_state(priv->playbin, GST_STATE_NULL);
    }
    return gst_element_set_state(priv->playbin, GST_STATE_PLAYING);
}

/**
 * rclib_core_pause:
 *
 * Set the state of the player to pause.
 *
 * Returns: Whether the state is set to pause successfully.
 */

gboolean rclib_core_pause()
{
    RCLibCorePrivate *priv;
    gint64 pos;
    gboolean flag, state_flag;
    GstFormat format = GST_FORMAT_TIME;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    flag = gst_element_query_position(priv->playbin, &format, &pos);
    state_flag = gst_element_set_state(priv->playbin, GST_STATE_PAUSED);
    if(flag)
    {
        gst_element_seek_simple(priv->playbin, GST_FORMAT_TIME, 
            GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, pos);
    }
    return state_flag;
}

/**
 * rclib_core_stop:
 *
 * Set the player to stop state.
 *
 * Returns: Whether the state is set to stop successfully.
 */

gboolean rclib_core_stop()
{
    RCLibCorePrivate *priv;
    gboolean flag;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    gst_element_set_state(priv->playbin, GST_STATE_NULL);
    flag = gst_element_set_state(priv->playbin, GST_STATE_READY);
    if(flag)
    {
        rclib_core_set_position(0);
        rclib_core_metadata_free(&(priv->metadata));
    }
    return flag;
}

/**
 * rclib_core_set_volume:
 * @volume: the volume of the player, it should be between 0.0 and 1.0.
 *
 * Set the volume of the player.
 *
 * Returns: Whether the volume is set successfully.
 */

gboolean rclib_core_set_volume(gdouble volume)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    g_object_set(G_OBJECT(priv->playbin), "volume", volume, NULL);
    return TRUE;
}

/**
 * rclib_core_get_volume:
 * @volume: the volume to return
 *
 * Get the volume of the player.
 *
 * Returns: Whether the volume is read successfully.
 */

gboolean rclib_core_get_volume(gdouble *volume)
{
    RCLibCorePrivate *priv;
    gdouble vol = 0.0;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    g_object_get(priv->playbin, "volume", &vol, NULL);
    if(volume!=NULL) *volume = vol;
    return TRUE;
}

/**
 * rclib_core_set_eq:
 * @type: the equalizer style type
 * @band: an array (10 elements) of the gains for each frequency band
 *
 * Set the equalizer of the player.
 *
 * Returns: Whether the equalizer is set successfully.
 */

gboolean rclib_core_set_eq(RCLibCoreEQType type, gdouble *band)
{
    RCLibCorePrivate *priv;
    gint i;
    gchar band_name[16];
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->eq_plugin==NULL) return FALSE;
    if(type>=RCLIB_CORE_EQ_TYPE_NONE && type<RCLIB_CORE_EQ_TYPE_CUSTOM)
    {
        band = core_eq_data[type].data;
        priv->eq_type = type;
    }
    else
    {
        if(band==NULL) return FALSE;
        priv->eq_type = RCLIB_CORE_EQ_TYPE_CUSTOM;
    }
    for(i=0;i<10;i++)
    {
        g_snprintf(band_name, 15, "band%d", i);
        g_object_set(G_OBJECT(priv->eq_plugin), band_name, band[i],
            NULL);
    }
    g_signal_emit(core_instance, core_signals[SIGNAL_EQ_CHANGED], 0,
        type, band);
    return TRUE;
}

/**
 * rclib_core_get_eq:
 * @type: the equalizer style type
 * @band: an array (10 elements) of the gains for each frequency band
 *
 * Get the equalizer of the player.
 *
 * Returns: Whether the data from equalizer is read successfully.
 */

gboolean rclib_core_get_eq(RCLibCoreEQType *type, gdouble *band)
{
    RCLibCorePrivate *priv;
    gint i;
    gchar band_name[16];
    gdouble value;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->eq_plugin==NULL) return FALSE;
    if(type!=NULL) *type = priv->eq_type;
    if(band==NULL) return TRUE;
    for(i=0;i<10;i++)
    {
        g_snprintf(band_name, 15, "band%d", i);
        g_object_get(G_OBJECT(priv->eq_plugin), band_name, &value,
            NULL);
        band[i] = value;
    }
    return TRUE;
}

/**
 * rclib_core_get_eq_name:
 * @type: the equalizer style type
 *
 * Get the name of the equalizer style type.
 *
 * Returns: The style name.
 */

const gchar *rclib_core_get_eq_name(RCLibCoreEQType type)
{
    if(type>=RCLIB_CORE_EQ_TYPE_NONE && type<RCLIB_CORE_EQ_TYPE_CUSTOM)
    {
        return gettext(core_eq_data[type].name);
    }
    else
        return _("Custom");
}

/**
 * rclib_core_set_balance:
 * @balance: set the position in stereo panorama
 *
 * Set the position in stereo panorama.
 *
 * Returns: Whether the balance position is set successfully.
 */

gboolean rclib_core_set_balance(gfloat balance)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->eq_plugin==NULL) return FALSE;
    g_object_set(G_OBJECT(priv->bal_plugin), "panorama", balance, NULL);
    g_signal_emit(core_instance, core_signals[SIGNAL_BALANCE_CHANGED], 0,
        balance);
    return TRUE;
}

/**
 * rclib_core_get_balance:
 * @balance: return the value of the position in stereo panorama
 *
 * Get the position in stereo panorama.
 *
 * Returns: Whether the balance position is read successfully.
 */

gboolean rclib_core_get_balance(gfloat *balance)
{
    RCLibCorePrivate *priv;
    gfloat bal;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->eq_plugin==NULL) return FALSE;
    g_object_get(G_OBJECT(priv->bal_plugin), "panorama", &bal, NULL);
    if(balance!=NULL) *balance = bal;
    return TRUE;
}

/**
 * rclib_core_set_echo:
 * @delay: the delay of the echo in nanoseconds
 * @feedback: the amount of feedback
 * @intensity: the intensity of the echo
 *
 * Set the properties of the echo effect.
 *
 * Returns: Whether the properties of the echo effect are set successfully.
 */

gboolean rclib_core_set_echo(guint64 delay, gfloat feedback,
    gfloat intensity)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->echo_plugin==NULL) return FALSE;
    g_object_set(G_OBJECT(priv->echo_plugin), "delay", delay, "feedback",
        feedback, "intensity", intensity, NULL);
    g_signal_emit(core_instance, core_signals[SIGNAL_ECHO_CHANGED], 0);
    return TRUE;
}

/**
 * rclib_core_get_echo:
 * @delay: return the delay of the echo in nanoseconds
 * @max_delay: return the maximum delay of the echo in nanoseconds
 * @feedback: return the amount of feedback
 * @intensity: return the intensity of the echo
 *
 * Get the properties of the echo effect.
 *
 * Returns: Whether the properties of the echo effect are read successfully.
 */

gboolean rclib_core_get_echo(guint64 *delay, guint64 *max_delay,
    gfloat *feedback, gfloat *intensity)
{
    RCLibCorePrivate *priv;
    guint64 delayv, max_delayv;
    gfloat feedbackv, intensityv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv->echo_plugin==NULL) return FALSE;
    g_object_get(G_OBJECT(priv->echo_plugin), "delay", &delayv, "max-delay",
        &max_delayv, "feedback", &feedbackv, "intensity", &intensityv, NULL);
    if(delay!=NULL) *delay = delayv;
    if(max_delay!=NULL) *max_delay = max_delayv;
    if(feedback!=NULL) *feedback = feedbackv;
    if(intensity!=NULL) *intensity = intensityv;
    return TRUE;
}

/**
 * rclib_core_effect_plugin_add:
 * @element: a new GStreamer sound effect plugin to add
 * 
 * Add a new GStreamer sound effect plugin to the player.
 * Notice that this operation must be done when the player is not
 * in playing or paused state.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_core_effect_plugin_add(GstElement *element)
{
    gboolean flag;
    if(core_instance==NULL) return FALSE;
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL || priv->effectbin==NULL) return FALSE;
    flag = rclib_core_effect_add_element_nonblock(priv->effectbin, element);
    if(flag)
    {
        gst_object_ref(element);
        priv->extra_plugin_list = g_list_append(priv->extra_plugin_list,
            element);
    }
    return TRUE;
}

/**
 * rclib_core_effect_plugin_remove:
 * @element: a new GStreamer sound effect plugin to remove
 * 
 * Remove an existed GStreamer sound effect plugin to the player.
 * Notice that this operation must be done when the player is not
 * in playing or paused state.
 */

void rclib_core_effect_plugin_remove(GstElement *element)
{
    if(core_instance==NULL) return;
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL || priv->effectbin==NULL) return;
    rclib_core_effect_remove_element_nonblock(priv->effectbin, element);
    priv->extra_plugin_list = g_list_remove(priv->extra_plugin_list,
        element);
    gst_object_unref(element);        
}

/**
 * rclib_core_effect_plugin_get_list:
 *
 * Get the list of the sound effect plugins that added to the player.
 *
 * Returns: The plugin list, please do not modify or free it.
 */

GList *rclib_core_effect_plugin_get_list()
{
    if(core_instance==NULL) return NULL;
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return NULL;
    return priv->extra_plugin_list;
}

