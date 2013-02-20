/*
 * RhythmCat Library Tag Manager Module
 * Manage the tags of music files.
 *
 * rclib-tag.c
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

#include "rclib-tag.h"
#include "rclib-common.h"
#include <gst/pbutils/pbutils.h>

/**
 * SECTION: rclib-tag
 * @Short_description: Process tags
 * @Title: Tag Processor
 * @Include: rclib-tag.h
 *
 * Process the tags (metadata) of music, and provide some data
 * structures and functions for tag processing.
 */

#if GST_VERSION_MAJOR==1
    #define TAG_DECODEBIN "decodebin"
#else
    #define TAG_DECODEBIN "decodebin2"
#endif

static gchar *tag_fallback_encoding = NULL;

typedef struct RCTagDecodedPadData
{
    GstElement *pipeline;
    GstElement *fakesink;
    gboolean audio_flag;
    gboolean video_flag;
    gboolean non_audio_flag;
}RCTagDecodedPadData;

GType rclib_tag_metadata_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibTagMetadata"),
            (GBoxedCopyFunc)rclib_tag_copy_data,
            (GBoxedFreeFunc)rclib_tag_free);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/*
 * Get tag from GstTagList.
 */

static void rclib_tag_get_tag_cb(const GstTagList *tags, RCLibTagMetadata *mmd)
{
    gchar *string = NULL;
    GDate *date = NULL;
    gchar *cuelist = NULL;
    #if GST_VERSION_MAJOR==1
        GstSample *sample = NULL;
    #else
        GstBuffer *image = NULL;
    #endif
    guint bitrates = 0;
    guint tracknum = 0;
    guint cue_num = 0;
    guint i = 0;
    if(mmd==NULL || mmd->uri==NULL || tags==NULL) return;
    if(gst_tag_list_get_string(tags, GST_TAG_AUDIO_CODEC, &string))
    {
        if(mmd->ftype==NULL)
            mmd->ftype = string;
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_TITLE, &string))
    {
        if(mmd->title==NULL)
            mmd->title = string;
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_ARTIST, &string))
    {
        if(mmd->artist==NULL)
            mmd->artist = string;
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_ALBUM, &string))
    {
        if(mmd->album==NULL)
            mmd->album = string;
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_COMMENT, &string))
    {
        if(mmd->comment==NULL)
            mmd->comment = string;
        else g_free(string);
    }
    if(gst_tag_list_get_string(tags, GST_TAG_GENRE, &string))
    {
        if(mmd->genre==NULL)
            mmd->genre = string;
        else g_free(string);
    }
    #if GST_VERSION_MAJOR==1
        if(gst_tag_list_get_sample(tags, GST_TAG_IMAGE, &sample))
        {
            if(mmd->image==NULL)
                mmd->image = gst_buffer_ref(gst_sample_get_buffer(sample));
            gst_sample_unref(sample);
        }
        if(gst_tag_list_get_sample(tags, GST_TAG_PREVIEW_IMAGE, &sample))
        {
            if(mmd->image==NULL)
                mmd->image = gst_buffer_ref(gst_sample_get_buffer(sample));
            gst_sample_unref(sample);
        }
    #else
        if(gst_tag_list_get_buffer(tags, GST_TAG_IMAGE, &image))
        {
            if(mmd->image==NULL)
                mmd->image = image;
            else gst_buffer_unref(image);
        }
        if(gst_tag_list_get_buffer(tags, GST_TAG_PREVIEW_IMAGE, &image))
        {
            if(mmd->image==NULL)
                mmd->image = image;
            else gst_buffer_unref(image);
        }
    #endif
    if(gst_tag_list_get_uint(tags, GST_TAG_NOMINAL_BITRATE, &bitrates))
        mmd->bitrate = bitrates;
    if(gst_tag_list_get_uint(tags, GST_TAG_TRACK_NUMBER, &tracknum))
        mmd->tracknum = tracknum;
    if(gst_tag_list_get_date(tags, GST_TAG_DATE, &date))
    {
        mmd->year = g_date_get_year(date);
        g_date_free(date);
    }
    cue_num = gst_tag_list_get_tag_size(tags, GST_TAG_EXTENDED_COMMENT);
    for(i=0;i<cue_num && mmd->emb_cue==NULL;i++)
    {
        if(gst_tag_list_get_string_index(tags, GST_TAG_EXTENDED_COMMENT, i,
            &cuelist))
        {
            if(strncmp(cuelist, "cuesheet=", 9)==0)
            {
                mmd->emb_cue = g_strdup(cuelist+9);
                g_free(cuelist);
                break;
            }
            else g_free(cuelist);
        }
    }
}

/*
 * Callback for creating new decoded pad.
 */

#if GST_VERSION_MAJOR==1
static void rc_tag_gst_new_decoded_pad_cb(GstElement *decodebin, 
    GstPad *pad, RCTagDecodedPadData *data)
#else
static void rc_tag_gst_new_decoded_pad_cb(GstElement *decodebin, 
    GstPad *pad, gboolean last, RCTagDecodedPadData *data)
#endif
{
    GstCaps *caps;
    GstStructure *structure;
    const gchar *mimetype;
    gboolean cancel = FALSE;
    GstPad *sink_pad;
    #if GST_VERSION_MAJOR==1
        caps = gst_pad_query_caps(pad, NULL);
    #else
        caps = gst_pad_get_caps(pad);
    #endif
    /* we get "ANY" caps for text/plain files etc. */
    if(gst_caps_is_empty(caps) || gst_caps_is_any(caps))
    {
        g_warning("Decoded pad with no caps or any caps. "
            "This file is boring.");
        cancel = TRUE;
        data->non_audio_flag = TRUE;
    }
    else
    {
        sink_pad = gst_element_get_static_pad(data->fakesink, "sink");
        gst_pad_link(pad, sink_pad);
        gst_object_unref(sink_pad);
        /* Is this pad audio? */
        structure = gst_caps_get_structure(caps, 0);
        mimetype = gst_structure_get_name(structure);
        if(g_str_has_prefix(mimetype, "audio/x-raw"))
        {
            g_debug("Got decoded audio pad of type %s", mimetype);
            data->audio_flag = TRUE;
        }
        else if(g_str_has_prefix(mimetype, "video/"))
        {
            g_debug("Got decoded video pad of type %s", mimetype);
            data->video_flag = TRUE;
        }
        else
        {
            g_debug("Got decoded pad of non-audio type %s", mimetype);
            data->non_audio_flag = TRUE;
        }
    }
    gst_caps_unref(caps);
    /* If this is non-audio, cancel the operation.
     * This seems to cause some deadlocks with video files, so only do it
     * when we get no/any caps.
     */
    if(cancel) gst_element_set_state(data->pipeline, GST_STATE_NULL);
}

/**
 * rclib_tag_read_metadata:
 * @uri: the URI of the music file
 *
 * Read tag (metadata) from given URI.
 *
 * Returns: The Metadata of the music, NULL if the file is not a music file,
 * free after usage.
 */

RCLibTagMetadata *rclib_tag_read_metadata(const gchar *uri)
{
    GstElement *pipeline = NULL;
    GstElement *urisrc = NULL;
    GstElement *decodebin = NULL;
    GstElement *fakesink = NULL;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    gint64 dura = 0;
    GstStateChangeReturn state_ret;
    GstMessage *msg;
    GstFormat fmt = GST_FORMAT_TIME;
    RCLibTagMetadata *mmd;
    RCTagDecodedPadData decoded_pad_data;
    GstTagList *tags = NULL;
    gboolean error_check = FALSE;
    GTimer *timer;
    GstMessageType msg_type = GST_MESSAGE_UNKNOWN;
    if(uri==NULL)
    {
        return NULL;
    }
    mmd = g_new0(RCLibTagMetadata, 1);
    mmd->uri = g_strdup(uri);
    #if GST_VERSION_MAJOR==1
        urisrc = gst_element_make_from_uri(GST_URI_SRC, mmd->uri, "urisrc",
            NULL);
    #else
        urisrc = gst_element_make_from_uri(GST_URI_SRC, mmd->uri, "urisrc");
    #endif
    if(urisrc==NULL)
    {
        g_warning("Cannot load urisrc from given URI!");
        rclib_tag_free(mmd);
        return NULL;
    }
    G_STMT_START
    {
        decodebin = gst_element_factory_make(TAG_DECODEBIN, NULL);
        if(decodebin==NULL) break;
        fakesink = gst_element_factory_make("fakesink", NULL);
        if(fakesink==NULL) break;
        pipeline = gst_pipeline_new("pipeline");
        if(pipeline==NULL) break;
        error_check = TRUE;
    }
    G_STMT_END;
    if(!error_check)
    {
        if(decodebin!=NULL) gst_object_unref(decodebin);
        if(fakesink!=NULL) gst_object_unref(fakesink);
        if(pipeline!=NULL) gst_object_unref(pipeline);
        rclib_tag_free(mmd);
        return NULL;
    }
    gst_bin_add_many(GST_BIN(pipeline), urisrc, decodebin, fakesink, NULL);
    if(!gst_element_link(urisrc, decodebin))
    {
        g_warning("Cannot link urisrc to decodebin!");
        gst_object_unref(pipeline);
        rclib_tag_free(mmd);
        return NULL;
    }
    decoded_pad_data.pipeline = pipeline;
    decoded_pad_data.fakesink = fakesink;
    #if GST_VERSION_MAJOR==1
        g_signal_connect(decodebin, "pad-added",
            G_CALLBACK(rc_tag_gst_new_decoded_pad_cb), &decoded_pad_data);
    #else
        g_signal_connect(decodebin, "new-decoded-pad",
            G_CALLBACK(rc_tag_gst_new_decoded_pad_cb), &decoded_pad_data);
    #endif
    gst_element_set_state(pipeline, GST_STATE_NULL);
    state_ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if(state_ret==GST_STATE_CHANGE_FAILURE)
    {
        g_warning("Cannot change the state of the pipeline of the "
            "tag reader! URI: %s", mmd->uri);
        gst_object_unref(GST_OBJECT(pipeline));
        rclib_tag_free(mmd);
        return NULL;
    }
    timer = g_timer_new();
    g_timer_start(timer);
    while(g_timer_elapsed(timer, NULL)<5.0)
    {
        msg = gst_bus_timed_pop_filtered(GST_ELEMENT_BUS(pipeline),
            GST_SECOND / 2, GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG |
            GST_MESSAGE_ERROR);
        msg_type = GST_MESSAGE_TYPE(msg);
        if(msg_type!=GST_MESSAGE_TAG)
        {
            gst_message_unref(msg);
            break;
        }
        gst_message_parse_tag(msg, &tags);
        rclib_tag_get_tag_cb(tags, mmd);
        #if GST_VERSION_MAJOR==1
            gst_tag_list_unref(tags);
        #else
            gst_tag_list_free(tags);
        #endif
        gst_message_unref(msg);
    }
    g_timer_stop(timer);
    g_timer_destroy(timer);
    if(msg_type==GST_MESSAGE_ERROR)
        g_warning("Cannot get tag from file: %s!", mmd->uri);
    #if GST_VERSION_MAJOR==1
        gst_element_query_duration(pipeline, fmt, &dura);
    #else
        gst_element_query_duration(pipeline, &fmt, &dura);
    #endif
    sink_pad = gst_element_get_static_pad(fakesink, "sink");
    if(sink_pad!=NULL)
    {
        #if GST_VERSION_MAJOR==1
            caps = gst_pad_get_current_caps(sink_pad);
        #else
            caps = gst_pad_get_negotiated_caps(sink_pad);
        #endif
        if(caps!=NULL)
        {
            structure = gst_caps_get_structure(caps, 0);
            gst_structure_get_int(structure, "rate", &(mmd->samplerate));
            gst_structure_get_int(structure, "channels", &(mmd->channels));
            gst_caps_unref(caps);
        }
        gst_object_unref(sink_pad);
    }
    mmd->length = dura;
    mmd->audio_flag = decoded_pad_data.audio_flag;
    mmd->video_flag = decoded_pad_data.video_flag;
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_debug("Successfully read tag from URI: %s", mmd->uri);
    return mmd;
}

/**
 * rclib_tag_copy_data:
 * @mmd: the #RCLibTagMetadata data to copy
 *
 * Copy the #RCLibTagMetadata data from the given parameter.
 *
 * Returns: (transfer full): The copied data.
 */

RCLibTagMetadata *rclib_tag_copy_data(const RCLibTagMetadata *mmd)
{
    RCLibTagMetadata *new_mmd;
    new_mmd = (RCLibTagMetadata *)g_new0(RCLibTagMetadata, 1);
    memcpy(new_mmd, mmd, sizeof(RCLibTagMetadata));
    new_mmd->uri = g_strdup(mmd->uri);
    new_mmd->title = g_strdup(mmd->title);
    new_mmd->artist = g_strdup(mmd->artist);
    new_mmd->album = g_strdup(mmd->album);
    new_mmd->comment = g_strdup(mmd->comment);
    new_mmd->ftype = g_strdup(mmd->ftype);
    new_mmd->genre = g_strdup(mmd->genre);
    new_mmd->emb_cue = g_strdup(mmd->emb_cue);
    new_mmd->image = gst_buffer_copy(mmd->image);
    return new_mmd;
}

/**
 * rclib_tag_free:
 * @mmd: the metadata
 *
 * Free the memory allocated for metadata struct (RCLibTagMetadata).
 */

void rclib_tag_free(RCLibTagMetadata *mmd)
{
    if(mmd==NULL) return;
    if(mmd->image!=NULL) gst_buffer_unref(mmd->image);
    g_free(mmd->uri);
    g_free(mmd->title);
    g_free(mmd->artist);
    g_free(mmd->album);
    g_free(mmd->comment);
    g_free(mmd->ftype);
    g_free(mmd->emb_cue);
    g_free(mmd->genre);
    g_free(mmd);
}

/**
 * rclib_tag_get_name_from_fpath:
 * @filename: the full path or file name
 *
 * Return the base-name without extension from a full path or file name.
 *
 * Returns: The base-name without extension.
 */

gchar *rclib_tag_get_name_from_fpath(const gchar *filename)
{
    gchar *ufilename;
    gchar *basename;
    gchar *dptr;
    gchar *realname;
    gsize len;
    gint nlen;
    if(filename==NULL) return NULL;
    ufilename = g_filename_to_utf8(filename, -1, NULL, NULL, NULL);
    if(ufilename==NULL) ufilename = g_strdup(filename);
    basename = g_filename_display_basename(ufilename);   
    g_free(ufilename);
    if(basename==NULL) return NULL;
    len = strlen(basename);
    dptr = strrchr(basename, '.');
    nlen = (gint)(dptr - basename);
    if(dptr==NULL || nlen<0 || nlen>=len)
        realname = g_strdup(basename);
    else
        realname = g_strndup(basename, nlen);
    g_free(basename);
    return realname;
}

/**
 * rclib_tag_get_name_from_uri:
 * @uri: the URI
 *
 * Return the base-name without extension from a URI.
 *
 * Returns: The base-name without extension.
 */

gchar *rclib_tag_get_name_from_uri(const gchar *uri)
{
    gchar *filepath;
    gchar *realname;
    if(uri==NULL) return NULL;
    filepath = g_filename_from_uri(uri, NULL, NULL);
    if(filepath==NULL) return NULL;
    realname = rclib_tag_get_name_from_fpath(filepath);
    g_free(filepath);
    return realname;
}


/**
 * rclib_tag_search_lyric_file:
 * @dirname: the directory name
 * @mmd: the metadata
 *
 * Search lyric file in given directory by the given metadata.
 *
 * Returns: The file name which is found in the directory, NULL if not found,
 * free after usage.
 */

gchar *rclib_tag_search_lyric_file(const gchar *dirname,
    const RCLibTagMetadata *mmd)
{
    gchar *result = NULL;
    GRegex *realname_regex = NULL;
    GRegex *artist_title_regex = NULL;
    GRegex *title_artist_regex = NULL;
    GRegex *title_only_regex = NULL;
    gchar *realname = NULL;
    gchar *realname_pattern = NULL;
    gchar *tmp, *string;
    GDir *gdir;
    const gchar *fname_foreach = NULL;
    const gchar *match_result = NULL;
    guint level = 0;
    if(dirname==NULL || mmd==NULL) return NULL;
    gdir = g_dir_open(dirname, 0, NULL);
    if(gdir==NULL) return NULL;
    if(mmd->uri!=NULL)
        realname = rclib_tag_get_name_from_uri(mmd->uri);
    if(realname!=NULL)
    {
        tmp = g_regex_escape_string(realname, -1);
        realname_pattern = g_strdup_printf("^(%s)(\\.LRC)$", tmp);
        g_free(tmp);
        realname_regex = g_regex_new(realname_pattern, G_REGEX_CASELESS,
            0, NULL);
        g_free(realname);
        g_free(realname_pattern);
    }
    if(mmd->title!=NULL && strlen(mmd->title)>0 && mmd->artist!=NULL &&
        strlen(mmd->artist)>0)
    {
        tmp = g_strdup_printf("%s - %s", mmd->artist, mmd->title);
        string = g_regex_escape_string(tmp, -1);
        g_free(tmp);
        tmp = g_strdup_printf("^(%s)(\\.LRC)$", string);
        g_free(string);
        artist_title_regex = g_regex_new(tmp, G_REGEX_CASELESS,
            0, NULL);
        g_free(tmp);
        tmp = g_strdup_printf("%s - %s", mmd->title, mmd->artist);
        string = g_regex_escape_string(tmp, -1);
        g_free(tmp);
        tmp = g_strdup_printf("^(%s)(\\.LRC)$", string);
        g_free(string);
        title_artist_regex = g_regex_new(tmp, G_REGEX_CASELESS,
            0, NULL);
        g_free(tmp);
    }
    if(mmd->title!=NULL && strlen(mmd->title)>0)
    {
        tmp = g_regex_escape_string(mmd->title, -1);
        string = g_strdup_printf("^(%s)(\\.LRC)$", tmp);
        g_free(tmp);
        title_only_regex = g_regex_new(string, G_REGEX_CASELESS,
            0, NULL);
        g_free(string);
    }
    while((fname_foreach=g_dir_read_name(gdir))!=NULL)
    {
        if(realname_regex!=NULL)
        {
            if(g_regex_match(realname_regex, fname_foreach, 0, NULL))
            {
                match_result = fname_foreach;
                level = 4;
                break;
            }
        }
        if(artist_title_regex!=NULL)
        {
            if(g_regex_match(artist_title_regex, fname_foreach, 0, NULL) &&
                level<3)
            {
                match_result = fname_foreach;
                level = 3;
            }
        }
        if(title_artist_regex!=NULL)
        {
            if(g_regex_match(title_artist_regex, fname_foreach, 0, NULL) &&
                level<2)
            {
                match_result = fname_foreach;
                level = 2;
            }
        }
        if(title_only_regex!=NULL)
        {
            if(g_regex_match(title_only_regex, fname_foreach, 0, NULL) &&
                level<1)
            {
                match_result = fname_foreach;
                level = 1;
            }
        }
    }
    if(realname_regex!=NULL) g_regex_unref(realname_regex);
    if(artist_title_regex!=NULL) g_regex_unref(artist_title_regex);
    if(title_artist_regex!=NULL) g_regex_unref(title_artist_regex);
    if(title_only_regex!=NULL) g_regex_unref(title_only_regex);
    if(match_result!=NULL)
        result = g_build_filename(dirname, match_result, NULL);
    g_dir_close(gdir);
    return result;
}

/**
 * rclib_tag_search_album_file:
 * @dirname: the directory name
 * @mmd: the metadata
 *
 * Search album image file in given directory by the given metadata.
 *
 * Returns: The file name which is found in the directory, NULL if not found,
 * free after usage.
 */

gchar *rclib_tag_search_album_file(const gchar *dirname,
    const RCLibTagMetadata *mmd)
{
    gchar *result = NULL;
    GRegex *realname_regex = NULL;
    GRegex *artist_regex = NULL;
    GRegex *title_regex = NULL;
    GRegex *album_regex = NULL;
    gchar *realname = NULL;
    gchar *realname_pattern = NULL;
    gchar *tmp, *string;
    GDir *gdir;
    const gchar *fname_foreach = NULL;
    const gchar *match_result = NULL;
    guint level = 0;
    if(dirname==NULL || mmd==NULL) return NULL;
    gdir = g_dir_open(dirname, 0, NULL);
    if(gdir==NULL) return NULL;
    if(mmd->uri!=NULL)
        realname = rclib_tag_get_name_from_uri(mmd->uri);
    if(realname!=NULL)
    {
        tmp = g_regex_escape_string(realname, -1);
        realname_pattern = g_strdup_printf("^(%s)\\.(BMP|JPG|JPEG|PNG)$",
            tmp);
        g_free(tmp);
        realname_regex = g_regex_new(realname_pattern, G_REGEX_CASELESS,
            0, NULL);
        g_free(realname);
        g_free(realname_pattern);
    }
    if(mmd->title!=NULL && strlen(mmd->title)>0)
    {
        string = g_regex_escape_string(mmd->title, -1);
        tmp = g_strdup_printf("^(%s)\\.(BMP|JPG|JPEG|PNG)$", string);
        g_free(string);
        title_regex = g_regex_new(tmp, G_REGEX_CASELESS, 0, NULL);
        g_free(tmp);
    }
    if(mmd->artist!=NULL && strlen(mmd->artist)>0)
    {
        string = g_regex_escape_string(mmd->artist, -1);
        tmp = g_strdup_printf("^(%s)\\.(BMP|JPG|JPEG|PNG)$", string);
        g_free(string);
        artist_regex = g_regex_new(tmp, G_REGEX_CASELESS, 0, NULL);
        g_free(tmp);
    }
    if(mmd->album!=NULL && strlen(mmd->album)>0)
    {
        string = g_regex_escape_string(mmd->album, -1);
        tmp = g_strdup_printf("^(%s)\\.(BMP|JPG|JPEG|PNG)$", string);
        g_free(string);
        album_regex = g_regex_new(tmp, G_REGEX_CASELESS, 0, NULL);
        g_free(tmp);
    }
    while((fname_foreach=g_dir_read_name(gdir))!=NULL)
    {
        if(realname_regex!=NULL)
        {
            if(g_regex_match(realname_regex, fname_foreach, 0, NULL))
            {
                match_result = fname_foreach;
                level = 4;
                break;
            }
        }
        if(title_regex!=NULL)
        {
            if(g_regex_match(title_regex, fname_foreach, 0, NULL) &&
                level<3)
            {
                match_result = fname_foreach;
                level = 3;
            }
        }
        if(album_regex!=NULL)
        {
            if(g_regex_match(album_regex, fname_foreach, 0, NULL) &&
                level<2)
            {
                match_result = fname_foreach;
                level = 2;
            }
        }
        if(artist_regex!=NULL)
        {
            if(g_regex_match(artist_regex, fname_foreach, 0, NULL) &&
                level<1)
            {
                match_result = fname_foreach;
                level = 1;
            }
        }
    }
    if(realname_regex!=NULL) g_regex_unref(realname_regex);
    if(artist_regex!=NULL) g_regex_unref(artist_regex);
    if(title_regex!=NULL) g_regex_unref(title_regex);
    if(album_regex!=NULL) g_regex_unref(album_regex);
    if(match_result!=NULL)
        result = g_build_filename(dirname, match_result, NULL);
    g_dir_close(gdir);
    return result;
}

/**
 * rclib_tag_set_fallback_encoding:
 * @encoding: the fallback encoding to set
 *
 * Set the fallback encoding for tag (metadata) reading,
 * notice that this setting will also affect the tag reading
 * in the Core module.
 */

void rclib_tag_set_fallback_encoding(const gchar *encoding)
{
    gchar *str;
    str = g_strdup_printf("%s:UTF-8", encoding);
    g_setenv("GST_ID3_TAG_ENCODING", str, TRUE);
    g_setenv("GST_ID3V2_TAG_ENCODING", str, TRUE);
    g_free(str);
    g_free(tag_fallback_encoding);    
    tag_fallback_encoding = g_strdup(encoding);
}

/**
 * rclib_tag_get_fallback_encoding:
 *
 * Get the fallback encoding for the tag reading.
 *
 * Returns: The fallback encoding.
 */

const gchar *rclib_tag_get_fallback_encoding()
{
    return tag_fallback_encoding;
}

