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

/**
 * SECTION: rclib-tag
 * @Short_description: Process tags
 * @Title: Tag Processor
 * @Include: rclib-tag.h
 *
 * Process the tags (metadata) of music, and provide some data
 * structures and functions for tag processing.
 */

typedef struct RCLibTagDecodedPadData
{
    GstElement *pipeline;
    GstElement *fakesink;
    gboolean audio_flag;
    gboolean video_flag;
    gboolean non_audio_flag;
}RCLibTagDecodedPadData;

static gchar *tag_fallback_encoding = NULL;

/*
 * Get tag from GstTagList.
 */

static void rclib_tag_get_tag_cb(const GstTagList *tags, RCLibTagMetadata *mmd)
{
    gchar *string = NULL;
    GDate *date = NULL;
    gchar *cuelist = NULL;
    GstBuffer *image = NULL;
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

static void rclib_tag_gst_new_decoded_pad_cb(GstElement *decodebin, 
    GstPad *pad, gboolean last, RCLibTagDecodedPadData *data)
{
    GstCaps *caps;
    GstStructure *structure;
    const gchar *mimetype;
    gboolean cancel = FALSE;
    GstPad *sink_pad;
    caps = gst_pad_get_caps(pad);
    /* we get "ANY" caps for text/plain files etc. */
    if(gst_caps_is_empty(caps) || gst_caps_is_any(caps))
    {
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
            data->audio_flag = TRUE;
        }
        else if(g_str_has_prefix(mimetype, "video/"))
        {
            data->video_flag = TRUE;
        }
        else
        {
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
    GstElement *pipeline;
    GstElement *urisrc;
    GstElement *decodebin;
    GstElement *fakesink;
    GstPad *sink_pad;
    GstCaps *caps;
    GstStructure *structure;
    gint64 dura = 0;
    GstStateChangeReturn state_ret;
    GstMessage *msg;
    GstFormat fmt = GST_FORMAT_TIME;
    RCLibTagMetadata *mmd;
    RCLibTagDecodedPadData decoded_pad_data;
    GstTagList *tags = NULL;
    if(uri==NULL)
    {
        return NULL;
    }
    urisrc = gst_element_make_from_uri(GST_URI_SRC, uri, "urisrc");
    if(urisrc==NULL)
    {
        return NULL;
    }
    decodebin = gst_element_factory_make("decodebin2", NULL);
    if(decodebin==NULL)
        decodebin = gst_element_factory_make("decodebin", NULL);
    if(decodebin==NULL)
    {
        gst_object_unref(urisrc);
        return NULL;
    }
    fakesink = gst_element_factory_make("fakesink", NULL);
    if(fakesink==NULL)
    {
        gst_object_unref(urisrc);
        gst_object_unref(decodebin);
        return NULL;
    }
    pipeline = gst_pipeline_new("pipeline");
    if(pipeline==NULL)
    {
        gst_object_unref(urisrc);
        gst_object_unref(decodebin);
        gst_object_unref(fakesink);
        return NULL;
    }
    gst_bin_add_many(GST_BIN(pipeline), urisrc, decodebin, fakesink, NULL);
    if(!gst_element_link(urisrc, decodebin))
    {
        gst_object_unref(pipeline);
        return NULL;
    }
    mmd = g_new0(RCLibTagMetadata, 1);
    mmd->uri = g_strdup(uri);
    decoded_pad_data.pipeline = pipeline;
    decoded_pad_data.fakesink = fakesink;
    g_signal_connect(decodebin, "new-decoded-pad",
        G_CALLBACK(rclib_tag_gst_new_decoded_pad_cb), &decoded_pad_data);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    state_ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if(!state_ret)
    {
        gst_object_unref(pipeline);
        g_free(mmd);
        return NULL;
    }
    while(1)
    {
        msg = gst_bus_timed_pop_filtered(GST_ELEMENT_BUS(pipeline),
            GST_CLOCK_TIME_NONE, GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG |
            GST_MESSAGE_ERROR | GST_MESSAGE_DURATION);
        if(GST_MESSAGE_TYPE(msg)==GST_MESSAGE_DURATION)
        {
            gst_message_parse_duration(msg, &fmt, &dura);
            if(fmt==GST_FORMAT_TIME)
                mmd->length = dura;
        }
        else if(GST_MESSAGE_TYPE(msg)==GST_MESSAGE_TAG)
        {
            gst_message_parse_tag(msg, &tags);
            rclib_tag_get_tag_cb(tags, mmd);
            gst_tag_list_free(tags);
        }
        else
            break;
        gst_message_unref(msg);
    }
    if(GST_MESSAGE_TYPE(msg)==GST_MESSAGE_ERROR)
        ;
    gst_message_unref(msg);
    if(mmd->length<=0)
    {
        fmt = GST_FORMAT_TIME;
        gst_element_query_duration(pipeline, &fmt, &dura);
        mmd->length = dura;
    }
    sink_pad = gst_element_get_static_pad(fakesink, "sink");
    if(sink_pad!=NULL)
    {
        caps = gst_pad_get_negotiated_caps(sink_pad);
        if(caps!=NULL)
        {
            structure = gst_caps_get_structure(caps, 0);
            gst_structure_get_int(structure, "rate", &mmd->samplerate);
            gst_structure_get_int(structure, "channels", &mmd->channels);
            gst_caps_unref(caps);
        }
        gst_object_unref(sink_pad);
    }
    mmd->audio_flag = decoded_pad_data.audio_flag;
    mmd->video_flag = decoded_pad_data.video_flag;
    state_ret = gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    return mmd;
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

