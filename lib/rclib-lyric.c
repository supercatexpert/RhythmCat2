/*
 * RhythmCat Library Lyric Backend Module
 * Process lyric files and data.
 *
 * rclib-lyric.c
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

#include "rclib-lyric.h"
#include "rclib-common-priv.h"
#include "rclib-marshal.h"
#include "rclib-core.h"
#include "rclib-tag.h"
#include "rclib-db.h"

/**
 * SECTION: rclib-lyric
 * @Short_description: The lyric processor
 * @Title: RCLibLyric
 * @Include: rclib-lyric.h
 *
 * The #RCLibLyric is a class which processes the lyric data. It can read
 * lyric data from lyric file, and then parse them. It can load two lyric
 * tracks. The timer inside can send signals for lyric display.
 */

struct _RCLibLyricPrivate
{
    gchar *search_dir;
    gchar *encoding;
    GRegex *regex;
    gulong timer;
    RCLibLyricParsedData parsed_data1;
    RCLibLyricParsedData parsed_data2;
    gulong tag_found_handler;
    gulong uri_changed_handler;
};

enum
{
    SIGNAL_LYRIC_READY,
    SIGNAL_LINE_CHANGED,
    SIGNAL_LYRIC_TIMER,
    SIGNAL_LYRIC_MAY_MISSING,
    SIGNAL_LAST
};

static GObject *lyric_instance = NULL;
static gpointer rclib_lyric_parent_class = NULL;
static gint lyric_signals[SIGNAL_LAST] = {0};

static gint rclib_lyric_time_compare_func(gconstpointer a, gconstpointer b,
    gpointer data)
{
    gint ret = 0;
    const RCLibLyricData *data1 = (const RCLibLyricData *)a;
    const RCLibLyricData *data2 = (const RCLibLyricData *)b;
    if(data1->time > data2->time) ret = 1;
    else if(data1->time < data2->time) ret = -1;
    return ret;
}

static void rclib_lyric_lyric_data_free(RCLibLyricData *data)
{
    if(data==NULL) return;
    if(data->text!=NULL) g_free(data->text);
    g_free(data);
}

static gboolean rclib_lyric_watch_timer(gpointer data)
{
    static const RCLibLyricData *reference[2] = {NULL, NULL};
    GstState state = 0;
    gint64 pos;
    gint i;
    gint64 offset;
    const RCLibLyricData *lyric_data;
    rclib_core_get_state(&state, NULL, 0);
    if(state!=GST_STATE_PLAYING && state!=GST_STATE_PAUSED)
    {
        return TRUE;
    }
    pos = rclib_core_query_position();
    for(i=0;i<2;i++)
    {
        offset = rclib_lyric_get_track_time_offset(i);
        lyric_data = rclib_lyric_get_line(i, pos);
        g_signal_emit(lyric_instance, lyric_signals[SIGNAL_LYRIC_TIMER],
            0, i, pos, lyric_data, offset);
        if(lyric_data==NULL) continue;
        if(reference[i]!=lyric_data)
        {
            g_signal_emit(lyric_instance, lyric_signals[SIGNAL_LINE_CHANGED],
                0, i, lyric_data, offset);
            reference[i] = lyric_data;
        }
    }
    return TRUE;
}

static void rclib_lyric_tag_found_cb(RCLibCore *core,
    const RCLibCoreMetadata *metadata, const gchar *uri, gpointer data)
{
    RCLibLyricPrivate *priv;
    RCLibLyric *lyric;
    gchar *lyric_path;
    if(data==NULL || uri==NULL) return;
    lyric = RCLIB_LYRIC(data);
    priv = lyric->priv;
    if(priv==NULL) return;
    if(priv->parsed_data1.filename==NULL)
    {
        lyric_path = rclib_lyric_search_lyric(uri, metadata->title,
            metadata->artist);
        if(lyric_path!=NULL)
        {
            rclib_lyric_load_file(lyric_path, 0);
            g_free(lyric_path);
        }
    }
}

static void rclib_lyric_uri_changed_cb(RCLibCore *core, const gchar *uri,
    gpointer data)
{
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    gpointer iter = NULL;
    gchar *lyric_path = NULL;
    gchar *lyric_sec_path = NULL;
    gboolean flag1 = FALSE;
    gboolean flag2 = FALSE;
    gchar *ptitle = NULL;
    gchar *partist = NULL;
    rclib_lyric_clean(0);
    rclib_lyric_clean(1);
    rclib_core_get_play_source(&source_type, &iter, NULL);
    if(iter!=NULL)
    {
        if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
        {
            rclib_db_playlist_data_iter_get(iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE, &lyric_path,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            rclib_db_playlist_data_iter_get(iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE, &lyric_sec_path,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
        }
    }
    if(lyric_path==NULL)
        lyric_path = rclib_lyric_search_lyric(uri, NULL, NULL);
    if(lyric_path!=NULL)
        flag1 = rclib_lyric_load_file(lyric_path, 0);
    if(lyric_sec_path!=NULL)
        flag2 = rclib_lyric_load_file(lyric_sec_path, 1);
    g_free(lyric_path);
    g_free(lyric_sec_path);
    rclib_db_playlist_data_iter_get(iter, RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE,
        &ptitle, RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST, &partist,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    if(!flag1)
    {
        lyric_path = rclib_lyric_search_lyric(uri, ptitle, partist);
        if(lyric_path!=NULL)
            flag1 = rclib_lyric_load_file(lyric_path, 0);
        g_free(lyric_path);
    }
    g_free(ptitle);
    g_free(partist);
    if(!flag1 && !flag2)
    {
        g_signal_emit(lyric_instance,
            lyric_signals[SIGNAL_LYRIC_MAY_MISSING], 0);
    }
}

static void rclib_lyric_finalize(GObject *object)
{
    RCLibLyricPrivate *priv = RCLIB_LYRIC(object)->priv;
    RCLIB_LYRIC(object)->priv = NULL;
    if(priv->tag_found_handler>0)
        rclib_core_signal_disconnect(priv->tag_found_handler);
    if(priv->uri_changed_handler>0)
        rclib_core_signal_disconnect(priv->uri_changed_handler);
    if(priv->regex!=NULL) g_regex_unref(priv->regex);
    g_free(priv->search_dir);
    g_free(priv->encoding);
    if(priv->parsed_data1.seq!=NULL) g_sequence_free(priv->parsed_data1.seq);
    g_free(priv->parsed_data1.filename);
    g_free(priv->parsed_data1.title);
    g_free(priv->parsed_data1.artist);
    g_free(priv->parsed_data1.album);
    g_free(priv->parsed_data1.author);
    if(priv->parsed_data2.seq!=NULL) g_sequence_free(priv->parsed_data2.seq);
    g_free(priv->parsed_data2.filename);
    g_free(priv->parsed_data2.title);
    g_free(priv->parsed_data2.artist);
    g_free(priv->parsed_data2.album);
    g_free(priv->parsed_data2.author);
    g_source_remove(priv->timer);
    G_OBJECT_CLASS(rclib_lyric_parent_class)->finalize(object);
}

static GObject *rclib_lyric_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(lyric_instance!=NULL) return lyric_instance;
    retval = G_OBJECT_CLASS(rclib_lyric_parent_class)->constructor
        (type, n_construct_params, construct_params);
    lyric_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&lyric_instance);
    return retval;
}

static void rclib_lyric_class_init(RCLibLyricClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_lyric_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_lyric_finalize;
    object_class->constructor = rclib_lyric_constructor;
    g_type_class_add_private(klass, sizeof(RCLibLyricPrivate));
    
    /**
     * RCLibLyric::lyric-ready:
     * @lyric: the #RCLibLyric that received the signal
     * @index: the track index which is ready
     *
     * The ::lyric-ready signal is emitted when a lyric track is
     * ready to be used.
     */
    lyric_signals[SIGNAL_LYRIC_READY] = g_signal_new("lyric-ready",
        RCLIB_TYPE_LYRIC, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibLyricClass, lyric_ready), NULL, NULL,
        g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT, NULL);
        
    /**
     * RCLibLyric::line-changed:
     * @lyric: the #RCLibLyric that received the signal
     * @index: the track index of the current lyric line
     * @lyric_data: the lyric data of the current lyric line
     * @offset: the time offset of the lyric data
     *
     * The ::line-changed signal is emitted when a lyric line has been
     * found which matches the current playing position.
     */
    lyric_signals[SIGNAL_LINE_CHANGED] = g_signal_new("line-changed",
        RCLIB_TYPE_LYRIC, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibLyricClass, line_changed), NULL, NULL,
        rclib_marshal_VOID__UINT_POINTER_INT64, G_TYPE_NONE, 3, G_TYPE_UINT,
        G_TYPE_POINTER, G_TYPE_INT64, NULL);
        
    /**
     * RCLibLyric::lyric-timer:
     * @lyric: the #RCLibLyric that received the signal
     * @index: the track index of the current lyric line
     * @pos: current time position
     * @lyric_data: the lyric data of the current lyric line
     * @offset: the time offset of the lyric data
     *
     * The ::lyric-timer signal is emitted every 100ms, used for
     * lyric display.
     */
    lyric_signals[SIGNAL_LYRIC_TIMER] = g_signal_new("lyric-timer",
        RCLIB_TYPE_LYRIC, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibLyricClass, lyric_timer), NULL, NULL,
        rclib_marshal_VOID__UINT_INT64_POINTER_INT64, G_TYPE_NONE, 4,
        G_TYPE_UINT, G_TYPE_INT64, G_TYPE_POINTER, G_TYPE_INT64, NULL);
        
    /**
     * RCLibLyric::lyric-may-missing:
     * @lyric: the #RCLibLyric that received the signal
     *
     * The ::lyric-may-missing signal is emitted if the lyric file of
     * the current playing music may not exist.
     */
    lyric_signals[SIGNAL_LYRIC_MAY_MISSING] = g_signal_new(
        "lyric-may-missing", RCLIB_TYPE_LYRIC, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibLyricClass, lyric_may_missing), NULL, NULL,
        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0,
        G_TYPE_NONE, NULL);   
}

static void rclib_lyric_instance_init(RCLibLyric *lyric)
{
    RCLibLyricPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE(lyric,
        RCLIB_TYPE_LYRIC, RCLibLyricPrivate);
    lyric->priv = priv;
    priv->parsed_data1.seq = g_sequence_new((GDestroyNotify)
        rclib_lyric_lyric_data_free);
    priv->parsed_data2.seq = g_sequence_new((GDestroyNotify)
        rclib_lyric_lyric_data_free);
    priv->regex = g_regex_new("(?P<time>\\[[0-9]+:[0-9]+([\\.:][0-9]+)*\\])|"
        "(?P<text>[^\\]]*$)|(?P<title>\\[ti:.+\\])|(?P<artist>\\[ar:.+\\])|"
        "(?P<album>\\[al:.+\\])|(?P<author>\\[by:.+\\])|"
        "(?P<offset>\\[offset:.+\\])", 0, 0, NULL);
    priv->timer = g_timeout_add(100, (GSourceFunc)rclib_lyric_watch_timer,
        NULL);
    priv->tag_found_handler = rclib_core_signal_connect("tag-found",
        G_CALLBACK(rclib_lyric_tag_found_cb), lyric);
    priv->uri_changed_handler = rclib_core_signal_connect("uri-changed",
        G_CALLBACK(rclib_lyric_uri_changed_cb), lyric);
}

GType rclib_lyric_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo lyric_info = {
        .class_size = sizeof(RCLibLyricClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_lyric_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibLyric),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_lyric_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibLyric"), &lyric_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_lyric_init:
 *
 * Initialize the lyric process instance.
 */

void rclib_lyric_init()
{
    RCLibLyricPrivate *priv;
    g_message("Loading lyric processor....");
    if(lyric_instance!=NULL)
    {
        g_warning("Lyric processor is already loaded!");
        return;
    }
    lyric_instance = g_object_new(RCLIB_TYPE_LYRIC, NULL);
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv->parsed_data1.seq==NULL || priv->parsed_data2.seq==NULL)
    {
        g_warning("Cannot load lyric sequence!");
        g_object_unref(lyric_instance);
        return;
    }
    if(priv->regex==NULL)
    {
        g_warning("Cannot load lyric regex!");
        g_object_unref(lyric_instance);
        return;
    }
    g_message("Lyric processor loaded.");
}

/**
 * rclib_lyric_exit:
 *
 * Unload the lyric process instance.
 */

void rclib_lyric_exit()
{
    if(lyric_instance!=NULL) g_object_unref(lyric_instance);
    lyric_instance = NULL;
    g_message("Lyric processor exited.");
}

/**
 * rclib_lyric_get_instance:
 *
 * Get the running #RCLibLyric instance.
 *
 * Returns: (transfer none): The running instance.
 */

GObject *rclib_lyric_get_instance()
{
    return lyric_instance;
}

/**
 * rclib_lyric_signal_connect:
 * @name: the name of the signal
 * @callback: (scope call): the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibLyric object.
 *
 * Returns: The handler ID.
 */

gulong rclib_lyric_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(lyric_instance==NULL) return 0;
    return g_signal_connect(lyric_instance, name, callback, data);
}

/**
 * rclib_lyric_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running #RCLibLyric instance so it
 * will not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_lyric_signal_disconnect(gulong handler_id)
{
    if(lyric_instance==NULL) return;
    g_signal_handler_disconnect(lyric_instance, handler_id);
}

/**
 * rclib_lyric_set_fallback_encoding:
 * @encoding: the fallback encoding, set to NULL if not used
 *
 * Set the fallback encoding for the lyric parser.
 */

void rclib_lyric_set_fallback_encoding(const gchar *encoding)
{
    RCLibLyricPrivate *priv;
    if(lyric_instance==NULL) return;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return;
    g_free(priv->encoding);
    priv->encoding = g_strdup(encoding);
}

/**
 * rclib_lyric_get_fallback_encoding:
 *
 * Get the fallback encoding for the lyric parser, NULL if it is no set.
 *
 * Returns: The fallback encoding, do not free or modify it.
 */

const gchar *rclib_lyric_get_fallback_encoding()
{
    RCLibLyricPrivate *priv;
    if(lyric_instance==NULL) return NULL;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return NULL;
    return priv->encoding;
}

static void rclib_lyric_add_line(RCLibLyricParsedData *parsed_data,
    GRegex *regex, const gchar *encoding, const gchar *line, gsize length)
{
    GMatchInfo *match_info;
    gchar *tmp;
    gchar *lyric_line;
    gsize bytes_read, bytes_written;
    gint minute = 0;
    gfloat second = 0.0;
    gint64 time;
    gchar *str;
    gint value;
    gchar *text = NULL;
    GArray *time_array;
    gint i;
    RCLibLyricData *lyric_data;
    if(parsed_data==NULL || regex==NULL || line==NULL) return;
    if(g_utf8_validate(line, length, NULL))
        lyric_line = g_strdup(line);
    else if(encoding!=NULL)
    {
        tmp = g_convert(line, length, "UTF-8", encoding, &bytes_read,
            &bytes_written, NULL);
        if(tmp==NULL) return;
        lyric_line = tmp;
    }
    else return;
    if(!g_regex_match(regex, lyric_line, 0, &match_info))
    {
        g_free(lyric_line);
        return;
    }
    time_array = g_array_new(FALSE, FALSE, sizeof(gint64));
    while(g_match_info_matches(match_info))
    {
        str = g_match_info_fetch_named(match_info, "time");
        if(str!=NULL && strlen(str)>0)
        {
            if(sscanf(str, "[%d:%f]", &minute, &second)==2)
            {
                time = minute * 6000 + (gint)(second * 100);
                time *= 10 * GST_MSECOND;
                g_array_append_val(time_array, time);
            }
        }
        g_free(str);
        str = g_match_info_fetch_named(match_info, "title");
        if(str!=NULL)
            tmp = g_strstr_len(str, -1, ":");
        else
            tmp = NULL;
        if(tmp!=NULL && strlen(tmp)>2)
        {
            tmp++;
            if(parsed_data->title==NULL)
                parsed_data->title = g_strndup(tmp, strlen(tmp)-1);
        }
        g_free(str);
        str = g_match_info_fetch_named(match_info, "artist");
        if(str!=NULL)
            tmp = g_strstr_len(str, -1, ":");
        else
            tmp = NULL;
        if(tmp!=NULL && strlen(tmp)>2)
        {
            tmp++;
            if(parsed_data->artist==NULL)
                parsed_data->artist = g_strndup(tmp, strlen(tmp)-1);
        }
        g_free(str);
        str = g_match_info_fetch_named(match_info, "album");
        if(str!=NULL)
            tmp = g_strstr_len(str, -1, ":");
        else
            tmp = NULL;
        if(tmp!=NULL && strlen(tmp)>2)
        {
            tmp++;
            if(parsed_data->album==NULL)
                parsed_data->album = g_strndup(tmp, strlen(tmp)-1);
        }
        g_free(str);
        str = g_match_info_fetch_named(match_info, "author");
        if(str!=NULL)
            tmp = g_strstr_len(str, -1, ":");
        else
            tmp = NULL;
        if(tmp!=NULL && strlen(tmp)>2)
        {
            tmp++;
            if(parsed_data->author==NULL)
                parsed_data->author = g_strndup(tmp, strlen(tmp)-1);
        }
        g_free(str);
        str = g_match_info_fetch_named(match_info, "offset");
        if(str!=NULL)
            tmp = g_strstr_len(str, -1, ":");
        else
            tmp = NULL;
        if(tmp!=NULL && strlen(tmp)>2)
        {
            tmp++;
            if(sscanf(tmp, "%d", &value)==1)
                parsed_data->offset = value;
        }
        g_free(str);
        str = g_match_info_fetch_named(match_info, "text");
        if(str!=NULL && strlen(str)>0)
        {
            text = g_strdup(str);
        }
        g_free(str);
        g_match_info_next(match_info, NULL);
    }
    if(text==NULL) text = g_strdup("");
    for(i=0;i<time_array->len;i++)
    {
        lyric_data = g_new0(RCLibLyricData, 1);
        lyric_data->time = g_array_index(time_array, gint64, i);
        lyric_data->length = -1;
        lyric_data->text = g_strdup(text);
        g_sequence_insert_sorted(parsed_data->seq, lyric_data,
            rclib_lyric_time_compare_func, NULL);
    }
    g_free(text);
    g_array_free(time_array, TRUE);
    g_free(lyric_line);
}

/**
 * rclib_lyric_load_file:
 * @filename: the lyric file to load
 * @index: the lyric track index (0 or 1)
 *
 * Load lyric file and then parse it.
 *
 * Returns: Whether the file is loaded and parsed.
 */

gboolean rclib_lyric_load_file(const gchar *filename, guint index)
{
    RCLibLyricParsedData *parsed_data;
    gchar *line;
    GFile *file;
    GFileInputStream *input_stream;
    GDataInputStream *data_stream;
    gsize line_len;
    gint64 time = -1;
    RCLibLyricPrivate *priv;
    GSequenceIter *iter;
    RCLibLyricData *lyric_data;
    rclib_lyric_clean(index);
    if(lyric_instance==NULL) return FALSE;
    if(filename==NULL) return FALSE;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return FALSE;
    file = g_file_new_for_path(filename);
    if(file==NULL) return FALSE;
    if(!g_file_query_exists(file, NULL))
    {
        g_object_unref(file);
        return FALSE;
    }
    input_stream = g_file_read(file, NULL, NULL);
    g_object_unref(file);
    if(input_stream==NULL) return FALSE;
    data_stream = g_data_input_stream_new(G_INPUT_STREAM(input_stream));
    g_object_unref(input_stream);
    if(data_stream==NULL) return FALSE;
    g_data_input_stream_set_newline_type(data_stream,
        G_DATA_STREAM_NEWLINE_TYPE_ANY);
    if(index==1)
        parsed_data = &(priv->parsed_data2);
    else
        parsed_data = &(priv->parsed_data1);
    if(index!=1) index = 0;
    while((line=g_data_input_stream_read_line(data_stream, &line_len,
        NULL, NULL))!=NULL)
    {
        rclib_lyric_add_line(parsed_data, priv->regex, priv->encoding,
            line, line_len);
        g_free(line);
    }
    g_object_unref(data_stream);
    for(iter = g_sequence_get_end_iter(parsed_data->seq);
        !g_sequence_iter_is_begin(iter);)
    {
        iter = g_sequence_iter_prev(iter);
        lyric_data = g_sequence_get(iter);
        if(lyric_data==NULL) continue;
        if(time>=0)
        {
            lyric_data->length = time - lyric_data->time;
        }
        time = lyric_data->time;
    }
    parsed_data->filename = g_strdup(filename);
    g_signal_emit(lyric_instance, lyric_signals[SIGNAL_LYRIC_READY], 0,
        index);
    return TRUE;
}

/**
 * rclib_lyric_clean:
 * @index: the lyric track index
 *
 * Clean the lyric data in the running lyric prcessor instance.
 */

void rclib_lyric_clean(guint index)
{
    RCLibLyricParsedData *parsed_data;
    RCLibLyricPrivate *priv;
    GSequenceIter *begin_iter, *end_iter;
    if(lyric_instance==NULL) return;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return;
    if(index==1)
        parsed_data = &(priv->parsed_data2);
    else
        parsed_data = &(priv->parsed_data1);
    g_free(parsed_data->title);
    parsed_data->title = NULL;
    g_free(parsed_data->artist);
    parsed_data->artist = NULL;
    g_free(parsed_data->album);
    parsed_data->album = NULL;
    g_free(parsed_data->author);
    parsed_data->author = NULL;
    g_free(parsed_data->filename);
    parsed_data->filename = NULL;
    parsed_data->offset = 0;
    if(parsed_data->seq==NULL) return;
    begin_iter = g_sequence_get_begin_iter(parsed_data->seq);
    end_iter = g_sequence_get_end_iter(parsed_data->seq);
    if(begin_iter==NULL || end_iter==NULL) return;
    g_sequence_remove_range(begin_iter, end_iter);
}

/**
 * rclib_lyric_get_line:
 * @time: the time to search
 * @index: the lyric track index
 * 
 * Find the lyric line which matches to the given time (in nanoseconds).
 *
 * Returns: The matched line data, NULL if not found.
 */

const RCLibLyricData *rclib_lyric_get_line(guint index, gint64 time)
{
    RCLibLyricData *lyric_data = NULL;
    GSequenceIter *iter;
    iter = rclib_lyric_get_line_iter(index, time);
    if(iter==NULL) return NULL;
    if(g_sequence_iter_is_end(iter)) return NULL;
    lyric_data = g_sequence_get(iter);
    return lyric_data;
}

/**
 * rclib_lyric_get_line_iter:
 * @time: the time to search
 * @index: the lyric track index
 * 
 * Find iter of the lyric line which matches to the given time
 * (in nanoseconds).
 *
 * Returns: (transfer none): The #GSequenceIter of the lyric line,
 *     #NULL if not found.
 */

GSequenceIter *rclib_lyric_get_line_iter(guint index, gint64 time)
{
    RCLibLyricParsedData *parsed_data;
    RCLibLyricPrivate *priv;
    RCLibLyricData *lyric_data = NULL;
    GSequenceIter *begin, *end, *iter;
    gint64 offset;
    if(lyric_instance==NULL) return NULL;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return NULL;
    if(index==1)
        parsed_data = &(priv->parsed_data2);
    else
        parsed_data = &(priv->parsed_data1);
    begin = g_sequence_get_begin_iter(parsed_data->seq);
    end = g_sequence_get_end_iter(parsed_data->seq);
    if(begin==end) return NULL;
    lyric_data = g_sequence_get(begin);
    offset = (gint64)parsed_data->offset * GST_MSECOND;
    if(lyric_data!=NULL)
    {
        if(time<lyric_data->time+offset) return NULL;
    }
    do
    {
        iter = g_sequence_range_get_midpoint(begin, end);
        lyric_data = g_sequence_get(iter);
        if(lyric_data==NULL) break;
        if(time<lyric_data->time+offset)
        {
            end = iter;
        }
        else if(time>=lyric_data->time+offset && (lyric_data->length<0 ||
            time<lyric_data->time+offset+lyric_data->length))
        {
            break;
        }
        else begin = iter;
    }
    while(begin!=end);
    return iter;
}

/**
 * rclib_lyric_set_search_dir:
 * @dir: the directory to set
 *
 * Set the directory for searching the lyric files.
 */

void rclib_lyric_set_search_dir(const gchar *dir)
{
    RCLibLyricPrivate *priv;
    if(lyric_instance==NULL) return;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return;
    g_free(priv->search_dir);
    priv->search_dir = g_strdup(dir);
}

/**
 * rclib_lyric_get_search_dir:
 *
 * Get the directory for searching the lyric files.
 *
 * Returns: The directory path.
 */

const gchar *rclib_lyric_get_search_dir()
{
    RCLibLyricPrivate *priv;
    if(lyric_instance==NULL) return NULL;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return NULL;
    return priv->search_dir;
}

/**
 * rclib_lyric_search_lyric:
 * @uri: the URI
 * @title: the title
 * @artist: the artist
 *
 * Search the lyric file by given information.
 *
 * Returns: The lyric file path, NULL if not found.
 */

gchar *rclib_lyric_search_lyric(const gchar *uri, const gchar *title,
    const gchar *artist)
{
    RCLibLyricPrivate *priv;
    GRegex *regex;
    GString *key;
    GDir *dir = NULL;
    gchar *fdir = NULL, *rname = NULL;
    gchar *path, *tmp1, *tmp2;
    const gchar *fname_foreach;
    gchar *result = NULL;
    gboolean flag = FALSE;
    if(uri!=NULL)
    {
        path = g_filename_from_uri(uri, NULL, NULL);
        if(path!=NULL)
        {
            fdir = g_path_get_dirname(path);   
            rname = rclib_tag_get_name_from_fpath(path);
            g_free(path);
        }
    }
    key = g_string_new("^(");
    if(rname!=NULL)
    {
        tmp1 = g_regex_escape_string(rname, -1);
        g_free(rname);
        g_string_append(key, tmp1);
        g_free(tmp1);
        flag = TRUE;
    }
    if(artist!=NULL && title!=NULL && strlen(artist)>0 &&
        strlen(title)>0)
    {
        if(flag)
            g_string_append_c(key, '|');
        tmp1 = g_regex_escape_string(title, -1);
        tmp2 = g_regex_escape_string(artist, -1);
        g_string_append_printf(key, "%s - %s|%s - %s", tmp1, tmp2,
            tmp2, tmp1);
        g_free(tmp1);
        g_free(tmp2);
        flag = TRUE;
    }
    if(title!=NULL && strlen(title)>0)
    {
        if(flag)
            g_string_append_c(key, '|');
        tmp1 = g_regex_escape_string(title, -1);
        g_string_append(key, tmp1);
        g_free(tmp1);
        flag = TRUE;
    }
    if(!flag)
    {
        g_string_free(key, TRUE);
        return NULL;
    }
    g_string_append(key, ")\\.([Ll][Rr][Cc])$");
    regex = g_regex_new(key->str, 0, 0, NULL);
    g_string_free(key, TRUE);
    if(regex==NULL)
    {
        g_free(fdir);
        return NULL;
    }
    if(fdir!=NULL)
        dir = g_dir_open(fdir, 0, NULL);
    if(dir!=NULL)
    {
        while((fname_foreach=g_dir_read_name(dir))!=NULL)
        {
            if(g_regex_match(regex, fname_foreach, 0, NULL))
            {
                result = g_build_filename(fdir, fname_foreach, NULL);
                break;
            }
        }
        g_dir_close(dir);
        g_free(fdir);
        if(result!=NULL)
        {
            g_regex_unref(regex);
            return result;
        }
    }
    if(lyric_instance==NULL)
    {
        g_regex_unref(regex);
        return NULL;
    }
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL || priv->search_dir==NULL)
    {
        g_regex_unref(regex);
        return NULL;
    }
    dir = g_dir_open(priv->search_dir, 0, NULL);
    if(dir!=NULL)
    {
        while((fname_foreach=g_dir_read_name(dir))!=NULL)
        {
            if(g_regex_match(regex, fname_foreach, 0, NULL))
            {
                result = g_build_filename(priv->search_dir,
                    fname_foreach, NULL);
                break;
            }
        }
        g_dir_close(dir);
    }
    g_regex_unref(regex);
    return result;
}

/**
 * rclib_lyric_is_available:
 * @index: the lyric track index
 *
 * Check whether the lyric in the given track index is available.
 *
 * Returns: Whether the lyric is available.
 */

gboolean rclib_lyric_is_available(guint index)
{
    RCLibLyricPrivate *priv;
    RCLibLyricParsedData *parsed_data;
    if(lyric_instance==NULL) return FALSE;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return FALSE;
    if(index==1)
        parsed_data = &(priv->parsed_data2);
    else
        parsed_data = &(priv->parsed_data1);
    return (parsed_data->filename!=NULL);
}

/**
 * rclib_lyric_get_parsed_data:
 * @index: the lyric track index
 *
 * Get the parsed lyric data by the given track index.
 *
 * Returns: (transfer none): The parsed lyric data.
 */

const RCLibLyricParsedData *rclib_lyric_get_parsed_data(guint index)
{
    RCLibLyricPrivate *priv;
    const RCLibLyricParsedData *parsed_data;
    if(lyric_instance==NULL) return NULL;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return NULL;
    if(index==1)
        parsed_data = &(priv->parsed_data2);
    else
        parsed_data = &(priv->parsed_data1);
    return parsed_data;
}

/**
 * rclib_lyric_get_track_time_offset:
 * @index: the lyric track index
 *
 * Get the time offset (in nanosecond) by the given track index
 *
 * Returns: The delay time.
 */

gint64 rclib_lyric_get_track_time_offset(guint index)
{
    RCLibLyricPrivate *priv;
    RCLibLyricParsedData *parsed_data;
    if(lyric_instance==NULL) return 0;
    priv = RCLIB_LYRIC(lyric_instance)->priv;
    if(priv==NULL) return 0;
    if(index==1)
        parsed_data = &(priv->parsed_data2);
    else
        parsed_data = &(priv->parsed_data1);
    return (gint64)parsed_data->offset * GST_MSECOND;
}

