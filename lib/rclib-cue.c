/*
 * RhythmCat Library CUE Parser Module
 * Parse track data in CUE files.
 *
 * rclib-cue.c
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


#include "rclib-cue.h"
#include "rclib-common.h"

/**
 * SECTION: rclib-cue
 * @Short_description: CUE Parser.
 * @Title: Cue Parser
 * @Include: rclib-cue.h
 *
 * The data structures and functions for parse track data in CUE
 * sheet files.
 */

static gchar *cue_fallback_encoding = NULL;

/**
 * rclib_cue_read_data:
 * @input: the input data
 * @type: the data type of the input data
 * @data: (out): the parsed CUE data
 *
 * Read and parse data from CUE file or string.
 *
 * Returns: The track number, 0 if the CUE data is incorrect.
 */

guint rclib_cue_read_data(const gchar *input, RCLibCueInputType type,
    RCLibCueData *data)
{
    gchar *path = NULL;
    gchar *buf = NULL, *line = NULL;
    gchar *dir = NULL;
    gchar *tmp = NULL;
    gchar *cue_raw_data = NULL, *cue_tmp_data = NULL, *cue_new_data = NULL;
    gsize cue_raw_length = 0, cue_tmp_length = 0, cue_new_length = 0;
    gint track_index, track_sm, track_ss, track_sd;
    guint64 track_time;
    gchar **line_data_array = NULL;
    gboolean flag;
    guint i = 0;
    gchar chr;
    guint track_num = 0;
    GSList *track_list = NULL, *list_foreach = NULL;
    GRegex *music_filename_regex;
    GRegex *data_regex;
    GMatchInfo *match_info;
    RCLibCueTrack *cue_track_data = NULL, *cue_track_array = NULL;
    if(input==NULL) return 0;
    if(data==NULL) return 0;
    switch(type)
    {
        case RCLIB_CUE_INPUT_URI:
            path = g_filename_from_uri(input, NULL, NULL);
            if(path==NULL) return 0;
        case RCLIB_CUE_INPUT_PATH:
            if(path==NULL) path = g_strdup(input);
            if(path==NULL) return 0;
            flag = g_file_get_contents(path, &cue_raw_data,
                &cue_raw_length, NULL);
            dir = g_path_get_dirname(path);
            g_free(path);
            if(!flag)
            {
                g_free(dir);
                return 0;
            }
        case RCLIB_CUE_INPUT_EMBEDDED:
            break;
        default:
            return 0;
    }
    if(cue_raw_data==NULL)
    {
        if(!g_utf8_validate(input, -1, NULL) && cue_fallback_encoding!=NULL)
            cue_tmp_data = g_convert(input, -1, "UTF-8",
                cue_fallback_encoding, NULL, NULL, NULL);
        else
            cue_tmp_data = g_strdup(input);
    }
    else
    {
        if(!g_utf8_validate(cue_raw_data, -1, NULL))
            cue_tmp_data = g_convert(cue_raw_data, -1, "UTF-8",
                cue_fallback_encoding, NULL, NULL, NULL);
        else
            cue_tmp_data = g_strdup(cue_raw_data);
        g_free(cue_raw_data);
    }
    if(cue_tmp_data==NULL)
    {
        g_free(dir);
        return 0;
    }
    cue_tmp_length = strlen(cue_tmp_data);
    cue_new_data = g_new0(gchar, cue_tmp_length);
    for(i=0;i<cue_tmp_length;i++)
    {
        chr = cue_tmp_data[i];
        if(chr!='\r')
        {
            cue_new_data[cue_new_length] = chr;
            cue_new_length++;
        }
        else if(i+1<cue_tmp_length && cue_new_data[i+1]!='\n')
        {
            cue_new_data[cue_new_length] = '\n';
            cue_new_length++;
        }
    }
    g_free(cue_tmp_data);
    memset(data, 0, sizeof(RCLibCueData));
    data->type = type;
    if(type!=RCLIB_CUE_INPUT_EMBEDDED)
    {
        music_filename_regex = g_regex_new("(FILE \").*[\"]",
            G_REGEX_CASELESS, 0, NULL);
        g_regex_match(music_filename_regex, cue_new_data, 0, &match_info);
        if(g_match_info_matches(match_info))
        {
            buf = g_match_info_fetch(match_info, 0);
            if(dir!=NULL)
            {
                path = g_strndup(buf+6, strlen(buf)-7);
                tmp = g_build_filename(dir, path, NULL);
                data->file = g_filename_to_uri(tmp, NULL, NULL);
                g_free(tmp);
                g_free(path);
            }
            else
                data->file = g_strndup(buf+6, strlen(buf)-7);
            g_free(buf);
        }
        g_match_info_free(match_info);
        g_regex_unref(music_filename_regex);
        g_free(dir);
        if(data->file==NULL)
        {
            g_free(cue_new_data);
            return 0;
        }
    }
    else
        data->file = NULL;
    data_regex = g_regex_new("\".*[^\"]", G_REGEX_CASELESS, 0, NULL);
    line_data_array = g_strsplit(cue_new_data, "\n", 0);
    for(i=0;line_data_array[i]!=NULL;i++)
    {
        line = line_data_array[i];
        if(g_regex_match_simple("(TRACK )[0-9]+( AUDIO)", line,
            G_REGEX_CASELESS, 0))
        {
            track_num++;
            cue_track_data = g_new0(RCLibCueTrack, 1);
            sscanf(line, "%*s%d", &(cue_track_data->index));
            track_list = g_slist_append(track_list, cue_track_data);
        }
        else if(cue_track_data!=NULL && g_regex_match_simple("(INDEX )[0-9]+ "
            "[0-9]+:[0-9]{2}:[0-9]{2}", line, G_REGEX_CASELESS, 0))
        {
            sscanf(line, "%*s%d %d:%d:%d", &track_index, &track_sm,
                &track_ss, &track_sd);
            track_time = (track_sm * 60 + track_ss) * 1000 + 10 * track_sd;
            track_time *= GST_MSECOND;
            if(track_index==0)
                cue_track_data->time0 = track_time;
            else if(track_index==1)
                cue_track_data->time1 = track_time;
        }
        else if(g_regex_match_simple("(TITLE \").*[\"]", line,
            G_REGEX_CASELESS, 0))
        {
            g_regex_match(data_regex, line, 0, &match_info);
            if(g_match_info_matches(match_info))
            {
                buf = g_match_info_fetch(match_info, 0);
                if(buf!=NULL && strlen(buf)>1)
                {
                    if(cue_track_data!=NULL)
                    {
                        g_free(cue_track_data->title);
                        cue_track_data->title = g_strdup(buf+1);
                    }
                    else
                    {
                        g_free(data->title);
                        data->title = g_strdup(buf+1);
                    }
                }
                g_free(buf);
            }
            g_match_info_free(match_info);
        }
        else if(g_regex_match_simple("(PERFORMER \").*[\"]", line,
            G_REGEX_CASELESS, 0))
        {
            g_regex_match(data_regex, line, 0, &match_info);
            if(g_match_info_matches(match_info))
            {
                buf = g_match_info_fetch(match_info, 0);
                if(buf!=NULL && strlen(buf)>1)
                {
                    if(cue_track_data!=NULL)
                    {
                        g_free(cue_track_data->performer);
                        cue_track_data->performer = g_strdup(buf+1);
                    }
                    else
                    {
                        g_free(data->performer);
                        data->performer = g_strdup(buf+1);
                    }
                }
                g_free(buf);
            }
            g_match_info_free(match_info);
        }
        else if(g_regex_match_simple("(REM GENRE \").*[\"]", line,
            G_REGEX_CASELESS, 0))
        {
            g_regex_match(data_regex, line, 0, &match_info);
            if(g_match_info_matches(match_info))
            {
                buf = g_match_info_fetch(match_info, 0);
                if(buf!=NULL && strlen(buf)>1)
                {
                    g_free(data->genre);
                    data->genre = g_strdup(buf+1);
                }
                g_free(buf);
            }
            g_match_info_free(match_info);
        }
    }
    g_strfreev(line_data_array);
    g_free(cue_new_data);
    g_regex_unref(data_regex);
    i = 0;
    cue_track_array = g_new0(RCLibCueTrack, track_num);
    for(list_foreach=track_list;list_foreach!=NULL;
        list_foreach=g_slist_next(list_foreach))
    {
        memcpy(cue_track_array+i, list_foreach->data, sizeof(RCLibCueTrack));
        g_free(list_foreach->data);
        i++;
    }
    g_slist_free(track_list);
    data->track = cue_track_array;
    data->length = track_num;
    return track_num;
}

/**
 * rclib_cue_free:
 * @data: the CUE data to free
 *
 * Free the CUE data.
 * Notice that the pointer itself will not be free.
 */

void rclib_cue_free(RCLibCueData *data)
{
    guint i;
    RCLibCueTrack *track;
    g_free(data->file);
    g_free(data->title);
    g_free(data->performer);
    g_free(data->genre);
    if(data->track!=NULL)
    {
        for(i=0;i<data->length;i++)
        {
            track = data->track+i;
            g_free(track->title);
            g_free(track->performer);
        }
        g_free(data->track);
    }
    memset(data, 0, sizeof(RCLibCueData));
}

/**
 * rclib_cue_get_track_num:
 * @path: the file path or URI
 * @cue_path: (out) (allow-none): the file path or URI of the CUE file
 * @track_num: (out) (allow-none): the track number
 *
 * Get the CUE path/URI and track number from given path/URI.
 *
 * e.g. For the given path "/home/test/1.cue:1", you will get
 * path "/home/test/1.cue", and track number 1.
 *
 * Returns: Whether the path/URI is valid.
 */

gboolean rclib_cue_get_track_num(const gchar *path, gchar **cue_path,
    gint *track_num)
{
    gchar *needle;
    gint len;
    gint track;
    needle = strrchr(path, ':');
    if(sscanf(needle, ":%d", &track)!=1)
        return FALSE;
    if(track<0) return FALSE;
    if(track_num!=NULL) *track_num = track;
    if(needle==NULL) return FALSE;
    len = needle - path;
    if(len<=0) return FALSE;
    if(cue_path!=NULL)
        *cue_path = g_strndup(path, len);
    return TRUE;
}

/**
 * rclib_cue_set_fallback_encoding:
 * @encoding: the new fallback encoding
 *
 * Set the fallback encoding for CUE parser.
 */

void rclib_cue_set_fallback_encoding(const gchar *encoding)
{
    if(encoding==NULL) return;
    g_free(cue_fallback_encoding);
    cue_fallback_encoding = g_strdup(encoding);
}

/**
 * rclib_cue_get_fallback_encoding:
 *
 * Get the fallback encoding used in the CUE parser, NULL if not set.
 */

const gchar *rclib_cue_get_fallback_encoding()
{
    return cue_fallback_encoding;
}


