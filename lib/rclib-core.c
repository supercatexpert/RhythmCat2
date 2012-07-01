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
    RCLIB_TYPE_CORE, RCLibCorePrivate)
#define RCLIB_CORE_ERROR rclib_core_error_quark()

typedef enum
{
    GST_PLAY_FLAG_VIDEO = (1 << 0),
    GST_PLAY_FLAG_AUDIO = (1 << 1),
    GST_PLAY_FLAG_TEXT = (1 << 2),
    GST_PLAY_FLAG_VIS = (1 << 3),
    GST_PLAY_FLAG_SOFT_VOLUME = (1 << 4),
    GST_PLAY_FLAG_NATIVE_AUDIO = (1 << 5),
    GST_PLAY_FLAG_NATIVE_VIDEO = (1 << 6),
    GST_PLAY_FLAG_DOWNLOAD = (1 << 7),
    GST_PLAY_FLAG_BUFFERING = (1 << 8),
    GST_PLAY_FLAG_DEINTERLACE = (1 << 9)
}GstPlayFlags;

typedef struct RCLibCorePrivate
{
    GstElement *playbin;
    GstElement *audiosink;
    GstElement *videosink;
    GstElement *effectbin;
    GstElement *bal_plugin; /* balance: audiopanorama */
    GstElement *eq_plugin; /* equalizer: equalizer-10bands */
    GstBus *bus;
    GstPad *query_pad;
    GList *extra_plugin_list;
    GError *error;
    gchar *uri;
    RCLibCoreSourceType type;
    GstState last_state;
    gint64 start_time;
    gint64 end_time;
    RCLibCoreMetadata metadata;
    gint64 duration;
    gint sample_rate;
    gint channels;
    gint depth;
    RCLibCoreEQType eq_type;
    GSequenceIter *db_reference;
    GSequenceIter *db_reference_pre;
    gpointer ext_reference;
    gpointer ext_reference_pre;
    gchar *ext_cookie;
    gchar *ext_cookie_pre;
    GAsyncQueue *tag_update_queue;
    gboolean tag_signal_emitted;
    gulong message_id;
    gulong volume_id;
    gulong audio_tag_changed_id;
    gulong source_id;
    guint tag_update_id;
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
    SIGNAL_BUFFERING,
    SIGNAL_BUFFER_PROBE,
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
    memset(metadata, 0, sizeof(RCLibCoreMetadata));
}

static inline gboolean rclib_core_parse_metadata(const GstTagList *tags,
    RCLibCoreMetadata *metadata, guint64 start_time,
    guint64 end_time)
{
    gchar *string = NULL;
    gboolean update = FALSE;
    GstBuffer *buffer = NULL;
    GDate *date = NULL;
    guint value = 0;
    gint intv = 0;    
    if(gst_tag_list_get_string(tags, GST_TAG_TITLE, &string))
    {
        if(g_strcmp0(string, metadata->title)!=0)
        {
            g_free(metadata->title);
            metadata->title = g_strdup(string);
            update = TRUE;
        }
        g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_ARTIST, &string))
    {
        if(g_strcmp0(string, metadata->artist)!=0)
        {
            g_free(metadata->artist);
            metadata->artist = g_strdup(string);
            update = TRUE;
        }
        g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_ALBUM, &string))
    {
        if(g_strcmp0(string, metadata->album)!=0)
        {
            g_free(metadata->album);
            metadata->album = g_strdup(string);
            update = TRUE;
        }
        g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &string))
    {
        if(g_strcmp0(string, metadata->ftype)!=0)
        {
            g_free(metadata->ftype);
            metadata->ftype = g_strdup(string);
            update = TRUE;
        }
        g_free(string);
    }
    if(gst_tag_list_get_buffer(tags, GST_TAG_IMAGE, &buffer))
    {
        if(metadata->image==NULL)
        {
            metadata->image = gst_buffer_ref(buffer);
            update = TRUE;
        }
        gst_buffer_unref(buffer);
    }
    if(gst_tag_list_get_buffer(tags, GST_TAG_PREVIEW_IMAGE, &buffer))
    {
        if(metadata->image==NULL)
        {
            metadata->image = gst_buffer_ref(buffer);
            update = TRUE;
        }
        gst_buffer_unref(buffer);
    }
    if(gst_tag_list_get_uint(tags, GST_TAG_NOMINAL_BITRATE, &value))
    {
        if(metadata->bitrate==0 && value>0)
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
    return update;
}


static void rclib_core_finalize(GObject *object)
{
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(object));
    if(priv->query_pad!=NULL)
    {
        gst_object_unref(priv->query_pad);
    }
    if(priv->bus!=NULL)
    {
        if(priv->message_id>0)
            g_signal_handler_disconnect(priv->bus, priv->message_id);
        gst_object_unref(priv->bus);
    }
    if(priv->tag_update_id!=0)
        g_source_remove(priv->tag_update_id);
    if(priv->playbin!=NULL)
    {
        if(priv->volume_id>0)
            g_signal_handler_disconnect(priv->playbin, priv->volume_id);
        if(priv->audio_tag_changed_id>0)
        {
            g_signal_handler_disconnect(priv->playbin,
                priv->audio_tag_changed_id);
        }
        if(priv->source_id>0)
            g_signal_handler_disconnect(priv->playbin, priv->source_id);
        gst_element_set_state(priv->playbin, GST_STATE_NULL);
    }
    if(priv->tag_update_queue!=NULL)
        g_async_queue_unref(priv->tag_update_queue);
    if(priv->extra_plugin_list!=NULL)
        g_list_free(priv->extra_plugin_list);
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
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        state_changed), NULL, NULL, g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT, NULL);

    /**
     * RCLibCore::eos:
     * @core: the #RCLibCore that received the signal
     * 
     * The ::eos signal is emitted when end-of-stream reached in the
     * current pipeline of the core.
     */
    core_signals[SIGNAL_EOS] = g_signal_new("eos", RCLIB_TYPE_CORE,
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
     * found. Notice that this signal may be emitted many times during
     * the playing.
     */
    core_signals[SIGNAL_TAG_FOUND] = g_signal_new("tag-found",
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
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
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
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
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
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
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
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
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
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
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        balance_changed), NULL, NULL, g_cclosure_marshal_VOID__FLOAT,
        G_TYPE_NONE, 1, G_TYPE_FLOAT, NULL);

    /**
     * RCLibCore::buffering:
     * @core: the #RCLibCore that received the signal
     * @percent: the buffering percent (0-100)
     *
     * The ::buffering signal is emitted when the core is buffering.
     */
    core_signals[SIGNAL_BUFFERING] = g_signal_new("buffering",
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        buffering), NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE,
        1, G_TYPE_INT, NULL);
        
    /**
     * RCLibCore::buffer-probe:
     * @core: the #RCLibCore that received the signal
     * @pad: the #GstPad
     * @buffer: the #GstBuffer
     *
     * The ::buffering signal is emitted when the buffer data pass through
     *     the pipeline.
     * Warning: This signal is emitted from the working thread of GStreamer.
     */
    core_signals[SIGNAL_BUFFER_PROBE] = g_signal_new("buffer-probe",
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        buffer_probe), NULL, NULL, rclib_marshal_VOID__POINTER_POINTER,
        G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER, NULL);

    /**
     * RCLibCore::error:
     * @core: the #RCLibCore that received the signal
     *
     * The ::error signal is emitted when any error in the core occurs.
     */
    core_signals[SIGNAL_ERROR] = g_signal_new("error",
        RCLIB_TYPE_CORE, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibCoreClass,
        error), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);
}

static gboolean rclib_core_effect_add_element_internal(GstElement *effectbin,
    GstElement *new_element, gboolean block)
{
    GstPad *binsinkpad;
    GstPad *binsrcpad;
    GstPad *realpad;
    GstPad *prevpad;
    GstPad *srcpad, *blockpad;
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
    if(block)
    {
        srcpad = gst_element_get_static_pad(effectbin, "sink");
        blockpad = gst_pad_get_peer(srcpad);
        gst_object_unref(srcpad);
        gst_pad_set_blocked(blockpad, TRUE);
        gst_object_unref(blockpad);
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
        if(block)
        {
            srcpad = gst_element_get_static_pad(effectbin, "sink");
            blockpad = gst_pad_get_peer(srcpad);
            gst_object_unref(srcpad);
            gst_pad_set_blocked(blockpad, FALSE);
            gst_object_unref(blockpad);
        }
        gst_object_unref(realpad);
        gst_bin_remove(GST_BIN(effectbin), bin);
        return FALSE;
    }
    link = gst_pad_link(binsrcpad, realpad);
    gst_object_unref(realpad);
    if(link!=GST_PAD_LINK_OK)
    {
        if(block)
        {
            srcpad = gst_element_get_static_pad(effectbin, "sink");
            blockpad = gst_pad_get_peer(srcpad);
            gst_object_unref(srcpad);
            gst_pad_set_blocked(blockpad, FALSE);
            gst_object_unref(blockpad);
        }
        return FALSE;
    }
    gst_element_sync_state_with_parent(bin);
    if(block)
    {
        srcpad = gst_element_get_static_pad(effectbin, "sink");
        blockpad = gst_pad_get_peer(srcpad);
        gst_object_unref(srcpad);
        gst_pad_set_blocked(blockpad, FALSE);
        gst_object_unref(blockpad);
    }
    return TRUE;
}

static void rclib_core_effect_remove_element_internal(GstElement *effectbin,
    GstElement *element, gboolean block)
{
    GstPad *mypad;
    GstPad *prevpad, *nextpad;
    GstPad *srcpad, *blockpad;
    GstElement *bin;
    bin = GST_ELEMENT(gst_element_get_parent(element));
    if(bin==NULL) return;
    gst_object_unref(bin);
    gst_element_set_state(bin, GST_STATE_NULL);
    if(block)
    {
        srcpad = gst_element_get_static_pad(effectbin, "sink");
        blockpad = gst_pad_get_peer(srcpad);
        gst_object_unref(srcpad);
        gst_pad_set_blocked(blockpad, TRUE);
        gst_object_unref(blockpad);
    }
    mypad = gst_element_get_static_pad(bin, "sink");
    prevpad = gst_pad_get_peer(mypad);
    gst_pad_unlink(prevpad, mypad);
    gst_object_unref(mypad);
    mypad = gst_element_get_static_pad (bin, "src");
    nextpad = gst_pad_get_peer(mypad);
    gst_pad_unlink(mypad, nextpad);
    gst_object_unref(mypad);
    gst_pad_link(prevpad, nextpad);
    gst_object_unref(prevpad);
    gst_object_unref(nextpad);
    if(block)
    {
        srcpad = gst_element_get_static_pad(effectbin, "sink");
        blockpad = gst_pad_get_peer(srcpad);
        gst_object_unref(srcpad);
        gst_pad_set_blocked(blockpad, FALSE);
        gst_object_unref(blockpad);
    }
    gst_bin_remove(GST_BIN(effectbin), bin);
}

static void rclib_core_bus_callback(GstBus *bus, GstMessage *msg,
    gpointer data)
{
    RCLibCorePrivate *priv = NULL;
    GObject *object = G_OBJECT(data);    
    if(object==NULL) return;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(object));    
    switch(GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
        {
            gst_element_set_state(priv->playbin, GST_STATE_NULL); 
            gst_element_set_state(priv->playbin, GST_STATE_READY);
            g_signal_emit(object, core_signals[SIGNAL_EOS], 0);
            break;
        }
        case GST_MESSAGE_TAG:
        {
            /*
             * Become useless because the metadata can be got from
             * the signals from playbin2.
             */
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
        {
            GstState old_state, state, pending;
            if(priv==NULL) break;
            if(GST_MESSAGE_SRC(msg)!=GST_OBJECT(priv->playbin))
                break;
            gst_message_parse_state_changed(msg, &old_state, &state,
                &pending);
            if(pending!=GST_STATE_VOID_PENDING) break;
            priv->last_state = state;
            g_signal_emit(object, core_signals[SIGNAL_STATE_CHANGED],
                0, state);
            g_debug("Core state changed from %s to %s",
                gst_element_state_get_name(old_state),
                gst_element_state_get_name(state));
            break;
        }
        case GST_MESSAGE_DURATION:
        {
            break;
        }
        case GST_MESSAGE_ASYNC_DONE:
        {
            gint64 duration;
            GstCaps *caps;
            gint intv;
            GstStructure *structure;
            duration = rclib_core_query_duration();
            if(duration>0)
            {
                g_signal_emit(object, core_signals[SIGNAL_NEW_DURATION], 0,
                    duration);
            }
            if(priv->query_pad!=NULL)
            {
                caps = gst_pad_get_negotiated_caps(priv->query_pad);
                structure = gst_caps_get_structure(caps, 0);
                if(gst_structure_get_int(structure, "rate", &intv))
                {
                    priv->sample_rate = intv;
                }
                if(gst_structure_get_int(structure, "channels", &intv))
                {
                    priv->channels = intv;
                }
                if(gst_structure_get_int(structure, "depth", &intv))
                {
                    priv->depth = intv;
                }
                gst_caps_unref(caps);
            }
            break;
        }
        case GST_MESSAGE_BUFFERING:
        {
            gint buffering = 0;
            gint64 buffering_left = 0;
            GstBufferingMode mode;
            gst_message_parse_buffering_stats(msg, &mode, NULL, NULL,
                &buffering_left);
            g_signal_emit(object, core_signals[SIGNAL_BUFFERING], 0,
                buffering);
            break;
        }
        case GST_MESSAGE_NEW_CLOCK:
        {
            gint64 duration = 0;
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
        }
        case GST_MESSAGE_ELEMENT:
        {
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            GError *error = NULL;
            gchar *error_msg = NULL;
            gst_message_parse_error(msg, &error, &error_msg);
            g_warning("%s\nDEBUG: %s", error->message, error_msg);
            g_error_free(error);
            g_signal_emit(object, core_signals[SIGNAL_ERROR], 0, error_msg);
            g_free(error_msg);
            if(!gst_element_post_message(priv->playbin,
                gst_message_new_eos(GST_OBJECT(priv->playbin))))
                rclib_core_stop();
            break;
        }
        case GST_MESSAGE_WARNING:
        {
            GError *error = NULL;
            gchar *error_msg = NULL;
            gst_message_parse_warning(msg, &error, &error_msg);
            g_warning("%s\nDEBUG: %s", error->message, error_msg);
            g_error_free(error);
            g_free(error_msg);
            break;
        }
        default:
            break;
    }
    return;
}

static gboolean rclib_core_pad_buffer_probe_cb(GstPad *pad, GstBuffer *buf,
    gpointer data)
{
    /* WARNING: This function is not called in main thread! */
    GstMessage *msg;
    gint64 pos, len;
    gint rate = 0;
    gint channels = 0;
    gint width = 0;
    gint depth = 0;
    GstCaps *caps;
    GstStructure *structure;
    RCLibCorePrivate *priv = NULL;
    GObject *object = G_OBJECT(data);
    if(object==NULL) return TRUE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(object));
    if(priv==NULL) return TRUE;
    pos = (gint64)GST_BUFFER_TIMESTAMP(buf);
    len = rclib_core_query_duration();
    if(len>0) priv->duration = len;
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
    g_signal_emit(object, core_signals[SIGNAL_BUFFER_PROBE], 0, pad, buf);
    caps = gst_pad_get_negotiated_caps(pad);
    if(caps==NULL) return TRUE;
    structure = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(structure, "rate", &rate);
    gst_structure_get_int(structure, "channels", &channels);
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "depth", &depth);
    gst_caps_unref(caps);
    if(depth==0) depth = width;
    if(rate==0 || width==0 || channels==0) return TRUE;
    priv->sample_rate = rate;
    priv->channels = channels;
    priv->depth = depth;
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

}

static gboolean rclib_core_audio_tags_changed_idle_cb(gpointer data)
{
    /* WARNING: This function is not called in main thread! */
    RCLibCorePrivate *priv = (RCLibCorePrivate *)data;
    GstTagList *tags;
    GstTagList *merged_tags = NULL;
    gboolean flag = FALSE;
    if(data==NULL) return FALSE;
    if(priv->tag_update_queue==NULL) return FALSE;
    g_async_queue_lock(priv->tag_update_queue);
    while((tags=g_async_queue_try_pop_unlocked(priv->tag_update_queue))!=
        NULL)
    {
        if(merged_tags==NULL)
            merged_tags = gst_tag_list_copy(tags);
        else
            gst_tag_list_insert(merged_tags, tags, GST_TAG_MERGE_REPLACE);
        gst_tag_list_free(tags);
    }
    priv->tag_update_id = 0;
    g_async_queue_unlock(priv->tag_update_queue);
    if(merged_tags==NULL) return FALSE;
    flag = rclib_core_parse_metadata(merged_tags, &(priv->metadata),
        priv->start_time, priv->end_time);
    gst_tag_list_free(merged_tags);
    if(!flag) return FALSE;
    g_signal_emit(core_instance, core_signals[SIGNAL_TAG_FOUND], 0,
        &(priv->metadata), priv->uri);
    priv->tag_signal_emitted = TRUE;
    return FALSE;
}

static void rclib_core_audio_tags_changed_cb(GstElement *playbin2,
    gint stream_id, gpointer data)
{
    /* WARNING: This signal callback is not called on main thread! */
    RCLibCorePrivate *priv = (RCLibCorePrivate *)data;
    GstTagList *tags = NULL;
    gint current_stream_id = 0;
    GstPad *pad;
    GstCaps *caps;
    gint intv;
    GstStructure *structure;
    if(data==NULL) return;
    if(priv->tag_update_queue==NULL) return;
    g_object_get(playbin2, "current-audio", &current_stream_id, NULL);
    if(current_stream_id!=stream_id) return;
    g_signal_emit_by_name(playbin2, "get-audio-tags", stream_id, &tags);
    if(tags!=NULL)
    {
        g_async_queue_lock(priv->tag_update_queue);
        g_async_queue_push_unlocked(priv->tag_update_queue, tags);
        if(priv->tag_update_id==0)
        {
            priv->tag_update_id = g_idle_add((GSourceFunc)
                rclib_core_audio_tags_changed_idle_cb, priv);
        }
        g_async_queue_unlock(priv->tag_update_queue);
    }
    g_signal_emit_by_name(playbin2, "get-audio-pad", stream_id, &pad);
    if(pad!=NULL)
    {
        caps = gst_pad_get_negotiated_caps(pad);
        if(caps!=NULL)
        {
            structure = gst_caps_get_structure(caps, 0);
            if(structure!=NULL)
            {
                if(gst_structure_get_int(structure, "rate", &intv))
                {
                    if(intv>0) priv->sample_rate = intv;
                }
                if(gst_structure_get_int(structure, "channels", &intv))
                {
                    if(intv>0) priv->channels = intv;
                }
                if(gst_structure_get_int(structure, "depth", &intv))
                {
                    if(intv>0) priv->depth = intv;
                }
            }
        }
        gst_object_unref(pad);
    }
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
    GstPlayFlags flags;
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
    g_object_set(videosink, "sync", TRUE, NULL);
    g_object_set(playbin, "audio-sink", audiobin, "video-sink",
        videosink, NULL);
    g_object_get(playbin, "flags", &flags, NULL);
    flags |= GST_PLAY_FLAG_DOWNLOAD;
    flags |= GST_PLAY_FLAG_BUFFERING;
    flags &= ~GST_PLAY_FLAG_VIDEO;
    flags &= ~GST_PLAY_FLAG_TEXT;
    g_object_set(playbin, "flags", flags, NULL);
    memset(&(priv->metadata), 0, sizeof(RCLibCoreMetadata));
    priv->playbin = playbin;
    priv->effectbin = effectbin;
    priv->audiosink = audiosink;
    priv->videosink = videosink;
    priv->eq_plugin = gst_element_factory_make("equalizer-10bands",
        "effect-equalizer");
    priv->bal_plugin = gst_element_factory_make("audiopanorama",
        "effect-balance");
    if(priv->eq_plugin!=NULL)
        rclib_core_effect_add_element_internal(effectbin, priv->eq_plugin,
            FALSE);
    if(priv->bal_plugin!=NULL)
    {
        g_object_set(priv->bal_plugin, "method", 1, NULL);
        rclib_core_effect_add_element_internal(effectbin, priv->bal_plugin,
            FALSE);
    }
    priv->extra_plugin_list = NULL;
    priv->tag_update_id = 0;
    priv->tag_update_queue = g_async_queue_new_full((GDestroyNotify)
        gst_tag_list_free);
    bus = gst_element_get_bus(playbin);
    gst_bus_add_signal_watch(bus);
    priv->message_id = g_signal_connect(bus, "message",
        G_CALLBACK(rclib_core_bus_callback), core);
    priv->bus = bus;
    pad = gst_element_get_static_pad(audioconvert, "sink");
    priv->query_pad = pad;
    gst_pad_add_buffer_probe(pad, G_CALLBACK(rclib_core_pad_buffer_probe_cb),
       core);
    priv->volume_id = g_signal_connect(priv->playbin, "notify::volume",
        G_CALLBACK(rclib_core_volume_notify_cb), priv);
    priv->source_id = g_signal_connect(priv->playbin, "notify::source",
        G_CALLBACK(rclib_core_source_notify_cb), priv);
    priv->audio_tag_changed_id = g_signal_connect(priv->playbin,
        "audio-tags-changed", G_CALLBACK(rclib_core_audio_tags_changed_cb),
        priv);
    gst_element_set_state(playbin, GST_STATE_NULL);
    gst_element_set_state(playbin, GST_STATE_READY);
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
    core_instance = g_object_new(RCLIB_TYPE_CORE, NULL);
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
 * Returns: (transfer none): The running instance.
 */

GObject *rclib_core_get_instance()
{
    return core_instance;
}

/**
 * rclib_core_signal_connect:
 * @name: the name of the signal
 * @callback: (scope call): the the #GCallback to connect
 * @data: the user data
 *
 * Connect the #GCallback function to the given signal for the running
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

static inline void rclib_core_set_uri_internal(const gchar *uri,
    GSequenceIter *db_ref, const gchar *cookie,
    GSequenceIter *external_ref)
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
    rclib_core_stop();
    g_free(priv->uri);
    priv->uri = NULL;
    priv->db_reference = NULL;
    priv->ext_reference = NULL;
    g_free(priv->ext_cookie);
    priv->ext_cookie = NULL;
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
    priv->tag_signal_emitted = FALSE;
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
        g_object_set(priv->playbin, "uri", cue_data.file, NULL);
        priv->uri = g_strdup(cue_data.file);
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
        g_object_set(priv->playbin, "uri", uri, NULL);
        priv->uri = g_strdup(uri);
    }
    priv->db_reference = db_ref;
    priv->ext_cookie = g_strdup(cookie);
    priv->ext_reference = external_ref;
    gst_element_set_state(priv->playbin, GST_STATE_PAUSED);
    g_signal_emit(core_instance, core_signals[SIGNAL_URI_CHANGED], 0,
        priv->uri);
}

/**
 * rclib_core_set_uri:
 * @uri: the URI to play
 *
 * Set the URI to play.
 */

void rclib_core_set_uri(const gchar *uri)
{
    rclib_core_set_uri_internal(uri, NULL, NULL, NULL);
}

/**
 * rclib_core_set_uri_with_db_ref:
 * @uri: the URI to play
 * @db_ref: the database reference to set, set to NULL if not used.
 * Reference must be set to NULL if you want to use this reference
 *
 * Set the URI and the music DB list iter to play.
 */

void rclib_core_set_uri_with_db_ref(const gchar *uri,
    GSequenceIter *db_ref)
{
    rclib_core_set_uri_internal(uri, db_ref, NULL, NULL);
}

/**
 * rclib_core_set_uri_with_ext_ref:
 * @uri: the URI to play
 * @cookie: the external reference cookie
 * @external_ref: the databse reference to set, set to NULL if not used.
 * Reference must be set to NULL if you want to use this reference
 *
 * Set the URI to play, with external cookie and reference pointer.
 */

void rclib_core_set_uri_with_ext_ref(const gchar *uri, const gchar *cookie,
    GSequenceIter *external_ref)
{
    rclib_core_set_uri_internal(uri, NULL, cookie, external_ref);
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
 * @cookie: the reference cookie
 * @new_ref: the new databse reference to set, set to NULL if not used
 *
 * Update the external reference.
 */

void rclib_core_update_external_reference(const gchar *cookie,
    gpointer new_ref)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL || priv->db_reference!=NULL) return;
    if(g_strcmp0(priv->ext_cookie, cookie)!=0) return;
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
 * Returns: (transfer none): The database reference.
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
 * @cookie: (out) (allow-none): the reference cookie
 * @ref: (out) (allow-none): the reference
 *
 * Get the external reference.
 *
 * Returns: (transfer none): Whether the results are set.
 */

gboolean rclib_core_get_external_reference(gchar **cookie, gpointer *ref)
{
    RCLibCorePrivate *priv;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return FALSE;
    if(priv->ext_cookie==NULL || priv->ext_reference==NULL)
        return FALSE;
    if(cookie!=NULL)
    {
        *cookie = g_strdup(priv->ext_cookie);
    }
    if(priv->ext_reference!=NULL)
    {
        *ref = priv->ext_reference;
    }
    return TRUE;
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
 * rclib_core_query_buffering_percent:
 * @percent: (out) (allow-none): the buffering percent (0-100)
 * @busy: (out) (allow-none): if buffering is busy
 *
 * Get the percentage of buffered data. This is a value between 0 and 100.
 * The busy indicator is #TRUE when the buffering is in progress.
 *
 * Returns: Whether the query succeeded.
 */

gboolean rclib_core_query_buffering_percent(gint *percent, gboolean *busy)
{
    RCLibCorePrivate *priv;
    GstQuery *query;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return FALSE;
    query = gst_query_new_buffering(GST_FORMAT_TIME);
    gst_element_query(priv->playbin, query);
    gst_query_parse_buffering_percent(query, percent, busy);
    gst_query_unref(query);
    return TRUE;
}

/**
 * rclib_core_query_buffering_rang:
 * @start: (out) (allow-none): the start to set, or NULL
 * @stop: (out) (allow-none): the stop to set, or NULL
 * @estimated_total: (out) (allow-none): estimated total amount
 *     of download time, or NULL
 *
 * Parse an available query, writing the results into the passed
 * parameters, if the respective parameters are non-NULL.
 *
 * Returns: Whether the query succeeded. 
 */

gboolean rclib_core_query_buffering_range(gint64 *start, gint64 *stop,
    gint64 *estimated_total)
{
    RCLibCorePrivate *priv;
    GstQuery *query;
    GstFormat format = GST_FORMAT_PERCENT; 
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    if(priv==NULL) return FALSE;
    query = gst_query_new_buffering(GST_FORMAT_TIME);
    gst_element_query(priv->playbin, query);
    gst_query_parse_buffering_range(query, &format, start, stop,
        estimated_total);
    gst_query_unref(query);
    return TRUE;
}

/**
 * rclib_core_get_state:
 * @state: (out) (allow-none): a pointer to #GstState to hold the state.
 *     Can be #NULL
 * @pending: (out) (allow-none): a pointer to #GstState to hold the
 *     pending state. Can be #NULL
 * @timeout: a #GstClockTime to specify the timeout for an async state
 *     change or GST_CLOCK_TIME_NONE for infinite timeout
 *
 * Gets the state of the element.
 *
 * Returns: #GST_STATE_CHANGE_SUCCESS if the element has no more pending
 *     state and the last state change succeeded, #GST_STATE_CHANGE_ASYNC if
 *     the element is still performing a state change or
 *     #GST_STATE_CHANGE_FAILURE if the last state change failed. MT safe.
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
    GstBus *bus;
    GstMessage *msg;
    GstTagList *tags;
    if(core_instance==NULL) return FALSE;
    priv = RCLIB_CORE_GET_PRIVATE(RCLIB_CORE(core_instance));
    bus = gst_element_get_bus(priv->playbin);
    gst_element_set_state(priv->playbin, GST_STATE_READY);
    rclib_core_metadata_free(&(priv->metadata));
    priv->duration = 0;
    priv->sample_rate = 0;
    priv->channels = 0;
    priv->depth = 0;
    priv->tag_signal_emitted = FALSE;
    g_async_queue_lock(priv->tag_update_queue);
    while((tags=g_async_queue_try_pop_unlocked(priv->tag_update_queue))!=
        NULL)
    {
        gst_tag_list_free(tags);
    }
    g_async_queue_unlock(priv->tag_update_queue);
    while((msg=gst_bus_pop_filtered(bus, GST_MESSAGE_STATE_CHANGED))!=NULL)
    {
        gst_bus_async_signal_func(bus, msg, NULL);
        gst_message_unref(msg);
    }
    gst_bus_set_flushing(bus, TRUE);
    gst_object_unref(bus);
    gst_element_set_state(priv->playbin, GST_STATE_NULL);
    rclib_core_set_position(0);
    return TRUE;
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
 * @volume: (out) (allow-none): the volume to return
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
 * @type: (out) (allow-none): the equalizer style type
 * @band: (out) (allow-none): an array (10 elements) of the gains
 *     for each frequency band
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
 * @balance: (out) (allow-none): return the value of the position in
 *     stereo panorama
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
    rclib_core_stop();
    flag = rclib_core_effect_add_element_internal(priv->effectbin, element,
        FALSE);
    if(flag)
    {
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
    rclib_core_stop();
    rclib_core_effect_remove_element_internal(priv->effectbin, element,
        FALSE);
    priv->extra_plugin_list = g_list_remove(priv->extra_plugin_list,
        element);   
}

/**
 * rclib_core_effect_plugin_get_list:
 *
 * Get the list of the sound effect plugins that added to the player.
 *
 * Returns: (element-type GstElement) (transfer none): The plugin list,
 *     please do not modify or free it.
 */

GList *rclib_core_effect_plugin_get_list()
{
    if(core_instance==NULL) return NULL;
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return NULL;
    return priv->extra_plugin_list;
}

/**
 * rclib_core_query_sample_rate:
 *
 * Get the sample rate of the playing stream.
 *
 * Returns: The sample rate (unit: Hz).
 */

gint rclib_core_query_sample_rate()
{
    if(core_instance==NULL) return 0;
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return 0;
    return priv->sample_rate;
}

/**
 * rclib_core_query_channels:
 *
 * Get the channel number of the playing stream.
 *
 * Returns: The channel number.
 */

gint rclib_core_query_channels()
{
    if(core_instance==NULL) return 0;
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return 0;
    return priv->channels;
}

/**
 * rclib_core_query_depth:
 *
 * Get the depth of the playing stream.
 *
 * Returns: The depth.
 */

gint rclib_core_query_depth()
{
    if(core_instance==NULL) return 0;
    RCLibCorePrivate *priv = RCLIB_CORE_GET_PRIVATE(core_instance);
    if(priv==NULL) return 0;
    return priv->depth;
}

