/*
 * RhythmCat Library Music Database Module
 * Manage the music database.
 *
 * rclib-db.c
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

#include "rclib-db.h"
#include "rclib-db-priv.h"
#include "rclib-common.h"
#include "rclib-marshal.h"
#include "rclib-tag.h"
#include "rclib-cue.h"
#include "rclib-core.h"
#include "rclib-util.h"

/**
 * SECTION: rclib-db
 * @Short_description: The playlist database
 * @Title: RCLibDb
 * @Include: rclib-db.h
 *
 * The #RCLibDb is a class which manages the playlists in the player,
 * all playlists are put in a catalog list, and all music are put in
 * their playlists. The catalog item (playlist) and playlist item (music)
 * data can be accessed by #GSequenceIter. The database can import music
 * or playlist file asynchronously, or update the metadata in playlists
 * asynchronously.
 */

typedef struct RCLibDbXMLParserData
{
    gboolean db_flag;
    GSequence *catalog;
    GSequence *playlist;
    GSequenceIter *catalog_iter;
}RCLibDbXMLParserData;

enum
{
    SIGNAL_CATALOG_ADDED,
    SIGNAL_CATALOG_CHANGED,
    SIGNAL_CATALOG_DELETE,
    SIGNAL_CATALOG_REORDERED,
    SIGNAL_PLAYLIST_ADDED,
    SIGNAL_PLAYLIST_CHANGED,
    SIGNAL_PLAYLIST_DELETE,
    SIGNAL_PLAYLIST_REORDERED,
    SIGNAL_IMPORT_UPDATED,
    SIGNAL_REFRESH_UPDATED,
    SIGNAL_LAST
};

static GObject *db_instance = NULL;
static gpointer rclib_db_parent_class = NULL;
static gint db_signals[SIGNAL_LAST] = {0};
static const gint db_autosave_timeout = 120;

static void rclib_db_xml_parser_start_element_cb(GMarkupParseContext *context,
    const gchar *element_name, const gchar **attribute_names,
    const gchar **attribute_values, gpointer data, GError **error)
{
    RCLibDbXMLParserData *parser_data = (RCLibDbXMLParserData *)data;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *catalog_iter;
    guint i;
    if(data==NULL) return;
    if(g_strcmp0(element_name, "rclibdb")==0)
    {
        parser_data->db_flag = TRUE;
        return;
    }
    if(!parser_data->db_flag) return;
    if(parser_data->playlist!=NULL && g_strcmp0(element_name, "item")==0)
    {
        playlist_data = rclib_db_playlist_data_new();
        playlist_data->catalog = parser_data->catalog_iter;
        for(i=0;attribute_names[i]!=NULL;i++)
        {
            if(playlist_data->uri==NULL &&
                g_strcmp0(attribute_names[i], "uri")==0)
            {
                playlist_data->uri = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "type")==0)
            {
                sscanf(attribute_values[i], "%u", &(playlist_data->type));
            }
            else if(playlist_data->title==NULL &&
                g_strcmp0(attribute_names[i], "title")==0)
            {
                playlist_data->title = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->artist==NULL &&
                g_strcmp0(attribute_names[i], "artist")==0)
            {
                playlist_data->artist = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->album==NULL &&
                g_strcmp0(attribute_names[i], "album")==0)
            {
                playlist_data->album = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->ftype==NULL &&
                g_strcmp0(attribute_names[i], "filetype")==0)
            {
                playlist_data->ftype = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "length")==0)
            {
                playlist_data->length = g_ascii_strtoll(attribute_values[i],
                    NULL, 10);
            }
            else if(g_strcmp0(attribute_names[i], "tracknum")==0)
            {
                sscanf(attribute_values[i], "%d",
                    &(playlist_data->tracknum));
            }
            else if(g_strcmp0(attribute_names[i], "year")==0)
            {
                sscanf(attribute_values[i], "%d",
                    &(playlist_data->year));
            }
            else if(g_strcmp0(attribute_names[i], "rating")==0)
            {
                sscanf(attribute_values[i], "%f",
                    &(playlist_data->rating));
            }
            else if(playlist_data->lyricfile==NULL &&
                g_strcmp0(attribute_names[i], "lyricfile")==0)
            {
                playlist_data->lyricfile = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->albumfile==NULL &&
                g_strcmp0(attribute_names[i], "albumfile")==0)
            {
                playlist_data->albumfile = g_strdup(attribute_values[i]);
            }
            else if(playlist_data->lyricsecfile==NULL &&
                g_strcmp0(attribute_names[i], "lyricsecondfile")==0)
            {
                playlist_data->lyricsecfile = g_strdup(attribute_values[i]);
            }
        }
        g_sequence_append(parser_data->playlist, playlist_data);
    }
    else if(parser_data->catalog!=NULL &&
        g_strcmp0(element_name, "playlist")==0)
    {
        catalog_data = rclib_db_catalog_data_new();
        for(i=0;attribute_names[i]!=NULL;i++)
        {
            if(catalog_data->name==NULL &&
                g_strcmp0(attribute_names[i], "name")==0)
            {
                catalog_data->name = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attribute_names[i], "type")==0)
            {
                sscanf(attribute_values[i], "%u", &(catalog_data->type));
            }
        }
        catalog_data->playlist = g_sequence_new((GDestroyNotify)
            rclib_db_playlist_data_unref);
        parser_data->playlist = catalog_data->playlist;
        catalog_iter = g_sequence_append(parser_data->catalog, catalog_data);
        parser_data->catalog_iter = catalog_iter;
    }
}

static void rclib_db_xml_parser_end_element_cb(GMarkupParseContext *context,
    const gchar *element_name, gpointer data, GError **error)
{
    RCLibDbXMLParserData *parser_data = (RCLibDbXMLParserData *)data;
    if(data==NULL) return;
    if(g_strcmp0(element_name, "rclibdb")==0)
    {
        parser_data->db_flag = FALSE;
    }
    else if(g_strcmp0(element_name, "playlist")==0)
    {
        parser_data->playlist = NULL;
        parser_data->catalog_iter = NULL;
    }
}

static gboolean rclib_db_load_library_db(GSequence *catalog,
    const gchar *file, gboolean *dirty_flag)
{
    RCLibDbXMLParserData parser_data = {0};
    GMarkupParseContext *parse_context;
    GMarkupParser markup_parser =
    {
        .start_element = rclib_db_xml_parser_start_element_cb, 
        .end_element = rclib_db_xml_parser_end_element_cb,
        .text = NULL,
        .passthrough = NULL,
        .error = NULL
    };
    GError *error = NULL;
    GFile *input_file;
    GZlibDecompressor *decompressor;
    GFileInputStream *file_istream;
    GInputStream *decompress_istream;
    gchar buffer[4096];
    gssize read_size;
    input_file = g_file_new_for_path(file);
    if(input_file==NULL) return FALSE;
    file_istream = g_file_read(input_file, NULL, &error);
    g_object_unref(input_file);
    if(file_istream==NULL)
    {
        g_warning("Cannot open input stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        return FALSE;
    }
    decompressor = g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB);
    decompress_istream = g_converter_input_stream_new(G_INPUT_STREAM(
        file_istream), G_CONVERTER(decompressor));
    g_object_unref(file_istream);
    g_object_unref(decompressor);
    parser_data.catalog = catalog;
    parse_context = g_markup_parse_context_new(&markup_parser, 0,
        &parser_data, NULL);
    while((read_size=g_input_stream_read(decompress_istream, buffer, 4096,
        NULL, NULL))>0)
    {
        g_markup_parse_context_parse(parse_context, buffer, read_size,
            NULL);
    }
    g_markup_parse_context_end_parse(parse_context, NULL);
    g_object_unref(decompress_istream);
    g_markup_parse_context_free(parse_context);
    g_message("Playlist loaded.");
    if(dirty_flag!=NULL) *dirty_flag = FALSE;
    return TRUE;
}

static inline GString *rclib_db_build_xml_data(GSequence *catalog)
{
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *catalog_iter, *playlist_iter;
    GString *data_str;
    gchar *tmp;
    extern guint rclib_major_version;
    extern guint rclib_minor_version;
    extern guint rclib_micro_version;
    data_str = g_string_new("<?xml version=\"1.0\" standalone=\"yes\"?>\n");
    g_string_append_printf(data_str, "<rclibdb version=\"%u.%u.%u\">\n",
        rclib_major_version, rclib_minor_version, rclib_micro_version);
    for(catalog_iter = g_sequence_get_begin_iter(catalog);
        !g_sequence_iter_is_end(catalog_iter);
        catalog_iter = g_sequence_iter_next(catalog_iter))
    {
        catalog_data = g_sequence_get(catalog_iter);
        tmp = g_markup_printf_escaped("  <playlist name=\"%s\" type=\"%u\">\n",
            catalog_data->name, catalog_data->type);
        g_string_append(data_str, tmp);
        g_free(tmp);
        for(playlist_iter = g_sequence_get_begin_iter(catalog_data->playlist);
            !g_sequence_iter_is_end(playlist_iter);
            playlist_iter = g_sequence_iter_next(playlist_iter))
        {
            playlist_data = g_sequence_get(playlist_iter);
            g_string_append_printf(data_str, "    <item type=\"%u\" ",
                playlist_data->type);
            if(playlist_data->uri!=NULL)
            {
                tmp = g_markup_printf_escaped("uri=\"%s\" ",
                    playlist_data->uri);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->title!=NULL)
            {
                tmp = g_markup_printf_escaped("title=\"%s\" ",
                    playlist_data->title);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->artist!=NULL)
            {
                tmp = g_markup_printf_escaped("artist=\"%s\" ",
                    playlist_data->artist);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->album!=NULL)
            {
                tmp = g_markup_printf_escaped("album=\"%s\" ",
                    playlist_data->album);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->ftype!=NULL)
            {
                tmp = g_markup_printf_escaped("filetype=\"%s\" ",
                    playlist_data->ftype);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            tmp = g_markup_printf_escaped("length=\"%"G_GINT64_FORMAT"\" ",
                playlist_data->length);
            g_string_append(data_str, tmp);
            g_free(tmp);
            tmp = g_markup_printf_escaped("tracknum=\"%d\" year=\"%d\" "
                "rating=\"%f\" ", playlist_data->tracknum,
                playlist_data->year, playlist_data->rating);
            g_string_append(data_str, tmp);
            g_free(tmp);
            if(playlist_data->lyricfile!=NULL)
            {
                tmp = g_markup_printf_escaped("lyricfile=\"%s\" ",
                    playlist_data->lyricfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->albumfile!=NULL)
            {
                tmp = g_markup_printf_escaped("albumfile=\"%s\" ",
                    playlist_data->albumfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            if(playlist_data->lyricsecfile!=NULL)
            {
                tmp = g_markup_printf_escaped("lyricsecondfile=\"%s\" ",
                    playlist_data->lyricsecfile);
                g_string_append(data_str, tmp);
                g_free(tmp);
            }
            g_string_append(data_str, "/>\n");
        }
        g_string_append(data_str, "  </playlist>\n");
    }
    g_string_append(data_str, "</rclibdb>\n");
    return data_str;
}

static gboolean rclib_db_save_library_db(GSequence *catalog,
    const gchar *file, gboolean *dirty_flag)
{
    GString *xml_str;
    GZlibCompressor *compressor;
    GFileOutputStream *file_ostream;
    GOutputStream *compress_ostream;
    GFile *output_file;
    gssize write_size;
    GError *error = NULL;
    gboolean flag = TRUE;
    output_file = g_file_new_for_path(file);
    if(output_file==NULL) return FALSE;
    xml_str = rclib_db_build_xml_data(catalog);
    if(xml_str==NULL) return FALSE;
    file_ostream = g_file_replace(output_file, NULL, FALSE,
        G_FILE_CREATE_PRIVATE, NULL, &error);
    g_object_unref(output_file);
    if(file_ostream==NULL)
    {
        g_warning("Cannot open output file stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        return FALSE;
    }
    compressor = g_zlib_compressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 5);
    compress_ostream = g_converter_output_stream_new(G_OUTPUT_STREAM(
        file_ostream), G_CONVERTER(compressor));
    g_object_unref(file_ostream);
    g_object_unref(compressor);
    write_size = g_output_stream_write(compress_ostream, xml_str->str,
        xml_str->len, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Cannot save playlist: %s", error->message);
        g_error_free(error);
        error = NULL;
        flag = FALSE;
    }
    g_output_stream_close(compress_ostream, NULL, &error);
    if(error!=NULL)
    {
        g_warning("Cannot close file stream: %s", error->message);
        g_error_free(error);
        error = NULL;
        flag = FALSE;
    }
    g_object_unref(compress_ostream);
    if(flag)
    {
        g_message("Playlist saved, wrote %ld bytes.", (glong)write_size);
    }
    if(dirty_flag!=NULL) *dirty_flag = FALSE;  
    return flag;
}

static gpointer rclib_db_playlist_autosave_thread_cb(gpointer data)
{
    RCLibDbPrivate *priv = (RCLibDbPrivate *)data;
    gchar *filename;
    GZlibCompressor *compressor;
    GFileOutputStream *file_ostream;
    GOutputStream *compress_ostream;
    GFile *output_file;
    gssize write_size;
    GError *error = NULL;
    gboolean flag = TRUE;
    if(data==NULL)
    {
        g_thread_exit(NULL);
        return NULL;
    }
    while(priv->work_flag)
    {
        g_mutex_lock(&(priv->autosave_mutex));
        g_cond_wait(&(priv->autosave_cond), &(priv->autosave_mutex));
        G_STMT_START
        {
            if(priv->autosave_xml_data==NULL) break;
            filename = g_strdup_printf("%s.autosave", priv->filename);
            output_file = g_file_new_for_path(filename);
            g_free(filename);
            if(output_file==NULL) break;
            file_ostream = g_file_replace(output_file, NULL, FALSE,
                G_FILE_CREATE_PRIVATE, NULL, NULL);
            g_object_unref(output_file);
            if(file_ostream==NULL) break;
            compressor = g_zlib_compressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB,
                5);
            compress_ostream = g_converter_output_stream_new(G_OUTPUT_STREAM(
                file_ostream), G_CONVERTER(compressor));
            g_object_unref(file_ostream);
            g_object_unref(compressor);
            write_size = g_output_stream_write(compress_ostream,
                priv->autosave_xml_data->str, priv->autosave_xml_data->len,
                NULL, &error);
            if(error!=NULL)
            {
                g_error_free(error);
                error = NULL;
                flag = FALSE;
            }
            g_output_stream_close(compress_ostream, NULL, &error);
            if(error!=NULL)
            {
                g_error_free(error);
                error = NULL;
                flag = FALSE;
            }
            g_object_unref(compress_ostream);
            if(flag)
            {
                g_message("Auto save successfully, wrote %ld bytes.",
                    (glong)write_size);
            }
        }
        G_STMT_END;
        if(priv->autosave_xml_data!=NULL)
            g_string_free(priv->autosave_xml_data, TRUE);
        priv->autosave_xml_data = NULL;
        priv->dirty_flag = FALSE;
        g_mutex_unlock(&(priv->autosave_mutex));
    }
    g_thread_exit(NULL);
    return NULL;
}

static gboolean rclib_db_autosave_timeout_cb(gpointer data)
{
    RCLibDbPrivate *priv = (RCLibDbPrivate *)data;
    if(data==NULL) return FALSE;
    if(!priv->dirty_flag) return TRUE;
    if(priv->filename==NULL) return TRUE;
    if(priv->autosave_xml_data!=NULL) return TRUE;
    priv->autosave_xml_data = rclib_db_build_xml_data(priv->catalog);
    g_mutex_lock(&(priv->autosave_mutex));
    g_cond_signal(&(priv->autosave_cond));
    g_mutex_unlock(&(priv->autosave_mutex));
    return TRUE;
}

static void rclib_db_finalize(GObject *object)
{
    RCLibDbPlaylistImportData *import_data;
    RCLibDbPlaylistRefreshData *refresh_data;
    gchar *autosave_file;
    RCLibDbPrivate *priv = RCLIB_DB(object)->priv;
    priv->work_flag = FALSE;
    g_mutex_lock(&(priv->autosave_mutex));
    g_cond_signal(&(priv->autosave_cond));
    g_mutex_unlock(&(priv->autosave_mutex));
    g_source_remove(priv->autosave_timeout);
    g_thread_join(priv->autosave_thread);
    g_mutex_clear(&(priv->autosave_mutex));
    g_cond_clear(&(priv->autosave_cond));
    rclib_db_playlist_import_cancel();
    rclib_db_playlist_refresh_cancel();
    import_data = g_new0(RCLibDbPlaylistImportData, 1);
    refresh_data = g_new0(RCLibDbPlaylistRefreshData, 1);
    g_async_queue_push(priv->import_queue, import_data);
    g_async_queue_push(priv->refresh_queue, refresh_data);
    g_thread_join(priv->import_thread);
    g_thread_join(priv->refresh_thread);
    rclib_db_save_library_db(priv->catalog, priv->filename,
        &(priv->dirty_flag));
    autosave_file = g_strdup_printf("%s.autosave", priv->filename);
    g_remove(autosave_file);
    g_free(autosave_file);
    g_free(priv->filename);
    g_async_queue_unref(priv->import_queue);
    g_sequence_free(priv->catalog);
    priv->import_queue = NULL;
    priv->refresh_queue = NULL;
    priv->catalog = NULL;
    G_OBJECT_CLASS(rclib_db_parent_class)->finalize(object);
}

static GObject *rclib_db_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(db_instance!=NULL) return db_instance;
    retval = G_OBJECT_CLASS(rclib_db_parent_class)->constructor
        (type, n_construct_params, construct_params);
    db_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&db_instance);
    return retval;
}

static void rclib_db_class_init(RCLibDbClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_db_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_db_finalize;
    object_class->constructor = rclib_db_constructor;
    g_type_class_add_private(klass, sizeof(RCLibDbPrivate));
    
    /**
     * RCLibDb::catalog-added:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the added item
     * 
     * The ::catalog-added signal is emitted when a new item has been
     * added to the catalog.
     */
    db_signals[SIGNAL_CATALOG_ADDED] = g_signal_new("catalog-added",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_added), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

    /**
     * RCLibDb::catalog-changed:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the changed item
     * 
     * The ::catalog-changed signal is emitted when an item has been
     * changed in the catalog.
     */
    db_signals[SIGNAL_CATALOG_CHANGED] = g_signal_new("catalog-changed",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_changed), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::catalog-delete:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the item which is about to be deleted
     * 
     * The ::catalog-delete signal is emitted before an item in the catalog
     * is about to be deleted.
     */
    db_signals[SIGNAL_CATALOG_DELETE] = g_signal_new("catalog-delete",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_delete), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

    /**
     * RCLibDb::catalog-reordered:
     * @db: the #RCLibDb that received the signal
     * @new_order: an array of integers mapping the current position of each
     * catalog item to its old position before the re-ordering,
     * i.e. @new_order<literal>[newpos] = oldpos</literal>
     * 
     * The ::catalog-reordered signal is emitted when the catalog items have
     * been reordered.
     */
    db_signals[SIGNAL_CATALOG_REORDERED] = g_signal_new("catalog-reordered",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        catalog_reordered), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::playlist-added:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the added item
     * 
     * The ::playlist-added signal is emitted when a new item has been
     * added to the playlist.
     */
    db_signals[SIGNAL_PLAYLIST_ADDED] = g_signal_new("playlist-added",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_added), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::playlist-changed:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the changed item
     * 
     * The ::playlist-changed signal is emitted when an item has been
     * changed in the playlist.
     */
    db_signals[SIGNAL_PLAYLIST_CHANGED] = g_signal_new("playlist-changed",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_changed), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

    /**
     * RCLibDb::playlist-delete:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the item which is about to be deleted
     * 
     * The ::playlist-delete signal is emitted before an item in the playlist
     * is about to be deleted.
     */
    db_signals[SIGNAL_PLAYLIST_DELETE] = g_signal_new("playlist-delete",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_delete), NULL, NULL, g_cclosure_marshal_VOID__POINTER,
        G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::playlist-reordered:
     * @db: the #RCLibDb that received the signal
     * @iter: the #iter pointed to the catalog item which contains the
     * reordered playlist items
     * @new_order: an array of integers mapping the current position of each
     * playlist item to its old position before the re-ordering,
     * i.e. @new_order<literal>[newpos] = oldpos</literal>
     * 
     * The ::playlist-reordered signal is emitted when the catalog items have
     * been reordered.
     */
    db_signals[SIGNAL_PLAYLIST_REORDERED] = g_signal_new("playlist-reordered",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        playlist_reordered), NULL, NULL, rclib_marshal_VOID__POINTER_POINTER,
        G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER, NULL);
        
    /**
     * RCLibDb::import-updated:
     * @db: the #RCLibDb that received the signal
     * @remaining: the number of remaing jobs in current import queue
     * 
     * The ::import-updated signal is emitted when a job in the import
     * queue have been processed.
     */
    db_signals[SIGNAL_IMPORT_UPDATED] = g_signal_new("import-updated",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        import_updated), NULL, NULL, g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT, NULL);

    /**
     * RCLibDb::refresh-updated:
     * @db: the #RCLibDb that received the signal
     * @remaining: the number of remaing jobs in current refresh queue
     * 
     * The ::refresh-updated signal is emitted when a job in the refresh
     * queue have been processed.
     */
    db_signals[SIGNAL_REFRESH_UPDATED] = g_signal_new("refresh-updated",
        RCLIB_TYPE_DB, G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbClass,
        refresh_updated), NULL, NULL, g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT, NULL);
}

static void rclib_db_instance_init(RCLibDb *db)
{
    RCLibDbPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE(db, RCLIB_TYPE_DB,
        RCLibDbPrivate);
    db->priv = priv;
    priv->work_flag = TRUE;
    _rclib_db_instance_init_playlist(db, priv);
    priv->autosave_thread = g_thread_new("RC2-Autosave-Thread",
        rclib_db_playlist_autosave_thread_cb, priv);
    priv->import_work_flag = FALSE;
    priv->refresh_work_flag = FALSE;
    priv->dirty_flag = FALSE;
    priv->autosave_timeout = g_timeout_add_seconds(db_autosave_timeout,
        rclib_db_autosave_timeout_cb, priv);
}

GType rclib_db_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo db_info = {
        .class_size = sizeof(RCLibDbClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_db_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibDb),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_db_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibDb"), &db_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rclib_db_catalog_data_get_type(void)
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibDbCatalogData"),
            (GBoxedCopyFunc)rclib_db_catalog_data_ref,
            (GBoxedFreeFunc)rclib_db_catalog_data_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rclib_db_playlist_data_get_type(void)
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibDbPlaylistData"),
            (GBoxedCopyFunc)rclib_db_playlist_data_ref,
            (GBoxedFreeFunc)rclib_db_playlist_data_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_db_init:
 * @file: the file of the music library database to load
 *
 * Initialize the music library database.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rclib_db_init(const gchar *file)
{
    RCLibDbPrivate *priv;
    g_message("Loading music library database....");
    if(db_instance!=NULL)
    {
        g_warning("The database is already initialized!");
        return FALSE;
    }
    db_instance = g_object_new(RCLIB_TYPE_DB, NULL);
    priv = RCLIB_DB(db_instance)->priv;
    if(priv->catalog==NULL || priv->import_queue==NULL ||
        priv->import_thread==NULL)
    {
        g_object_unref(db_instance);
        db_instance = NULL;
        g_warning("Failed to load database!");
        return FALSE;
    }
    rclib_db_load_library_db(priv->catalog, file, &(priv->dirty_flag));
    priv->filename = g_strdup(file);
    g_message("Database loaded.");
    return TRUE;
}

/**
 * rclib_db_exit:
 *
 * Unload the music library database.
 */

void rclib_db_exit()
{
    if(db_instance!=NULL) g_object_unref(db_instance);
    db_instance = NULL;
    g_message("Database exited.");
}

/**
 * rclib_db_get_instance:
 *
 * Get the running #RCLibDb instance.
 *
 * Returns: (transfer none): The running instance.
 */

GObject *rclib_db_get_instance()
{
    return db_instance;
}

/**
 * rclib_db_signal_connect:
 * @name: the name of the signal
 * @callback: (scope call): the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibDb object.
 *
 * Returns: The handler ID.
 */

gulong rclib_db_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(db_instance==NULL) return 0;
    return g_signal_connect(db_instance, name, callback, data);
}

/**
 * rclib_db_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running #RCLibDb instance so it will
 * not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_db_signal_disconnect(gulong handler_id)
{
    if(db_instance==NULL) return;
    g_signal_handler_disconnect(db_instance, handler_id);
}

/**
 * rclib_db_get_catalog:
 *
 * Get the catalog.
 *
 * Returns: (transfer none): (skip): The catalog, NULL if the catalog does
 *     not exist.
 */

GSequence *rclib_db_get_catalog()
{
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return NULL;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL) return NULL;
    return priv->catalog;
}

/**
 * rclib_db_sync:
 *
 * Save the database (if modified) to disk.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rclib_db_sync()
{
    RCLibDbPrivate *priv;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return FALSE;
    if(!priv->dirty_flag) return TRUE;
    return rclib_db_save_library_db(priv->catalog, priv->filename,
        &(priv->dirty_flag));
}

/**
 * rclib_db_load_autosaved:
 *
 * Load all playlist data from auto-saved playlist.
 */

gboolean rclib_db_load_autosaved()
{
    RCLibDbPrivate *priv;
    GSequenceIter *iter;
    gchar *filename;
    gboolean flag;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return FALSE;
    while(g_sequence_get_length(priv->catalog)>0)
    {
        iter = g_sequence_get_begin_iter(priv->catalog);
        rclib_db_catalog_delete(iter);
    }
    filename = g_strdup_printf("%s.autosave", priv->filename);
    flag = rclib_db_load_library_db(priv->catalog, filename,
        &(priv->dirty_flag));
    g_free(filename);
    if(flag)
    {
        for(iter=g_sequence_get_begin_iter(priv->catalog);
            !g_sequence_iter_is_end(iter);
            iter=g_sequence_iter_next(iter))
        {
            g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_ADDED],
                0, iter);
        }
    }
    return flag;
}

/**
 * rclib_db_autosaved_exist:
 *
 * Whether auto-saved playlist file exists.
 *
 * Returns: The existence of auto-saved playlist file.
 */

gboolean rclib_db_autosaved_exist()
{
    RCLibDbPrivate *priv;
    gchar *filename;
    gboolean flag = FALSE;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return FALSE;
    filename = g_strdup_printf("%s.autosave", priv->filename);
    flag = g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
    g_free(filename);
    return flag;
}

/**
 * rclib_db_autosaved_remove:
 *
 * Remove the auto-saved playlist file.
 */

void rclib_db_autosaved_remove()
{
    RCLibDbPrivate *priv;
    gchar *filename;
    if(db_instance==NULL) return;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL || priv->catalog==NULL || priv->filename==NULL)
        return;
    filename = g_strdup_printf("%s.autosave", priv->filename);
    g_remove(filename);
    g_free(filename);
}

/**
 * rclib_db_load_legacy:
 *
 * Load legacy playlist file from RhythmCat 1.0.
 *
 * Returns: Whether the operation succeeds.
 */

gboolean rclib_db_load_legacy()
{
    RCLibDbPrivate *priv;
    GFile *file;
    GFileInputStream *input_stream;
    GDataInputStream *data_stream;
    gsize line_len;
    RCLibDbCatalogData *catalog_data = NULL;
    RCLibDbPlaylistData *playlist_data = NULL;
    GSequenceIter *catalog_iter = NULL;
    GSequenceIter *playlist_iter = NULL;
    gchar *line = NULL;
    gint64 timeinfo;
    gint trackno;
    gchar *plist_set_file_full_path = NULL;
    const gchar *home_dir = NULL;
    if(db_instance==NULL) return FALSE;
    priv = RCLIB_DB(db_instance)->priv;
    if(priv==NULL) return FALSE;
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    plist_set_file_full_path = g_build_filename(home_dir, ".RhythmCat",
        "playlist.dat", NULL);
    if(plist_set_file_full_path==NULL) return FALSE;
    file = g_file_new_for_path(plist_set_file_full_path);
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
    while((line=g_data_input_stream_read_line(data_stream, &line_len,
        NULL, NULL))!=NULL)
    {
        if(catalog_data!=NULL && strncmp(line, "UR=", 3)==0)
        {
            if(playlist_iter!=NULL)
            {
                g_signal_emit(db_instance,
                    db_signals[SIGNAL_PLAYLIST_CHANGED], 0, playlist_iter);
            }
            playlist_data = rclib_db_playlist_data_new();
            playlist_data->uri = g_strdup(line+3);
            playlist_data->catalog = catalog_iter;
            playlist_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
            playlist_iter = g_sequence_append(catalog_data->playlist,
                playlist_data);
            g_signal_emit(db_instance, db_signals[SIGNAL_PLAYLIST_ADDED],
                0, playlist_iter);
        }
        else if(playlist_data!=NULL && strncmp(line, "TI=", 3)==0)
            playlist_data->title = g_strdup(line+3);
        else if(playlist_data!=NULL && strncmp(line, "AR=", 3)==0)
            playlist_data->artist = g_strdup(line+3);
        else if(playlist_data!=NULL && strncmp(line, "AL=", 3)==0)
            playlist_data->album = g_strdup(line+3);
        else if(playlist_data!=NULL && strncmp(line, "TL=", 3)==0)  /* time length */
        {
            timeinfo = g_ascii_strtoll(line+3, NULL, 10) * 10;
            timeinfo *= GST_MSECOND;
            playlist_data->length = timeinfo;
        }
        else if(strncmp(line, "TN=", 3)==0)  /* track number */
        {
            sscanf(line+3, "%d", &trackno);
            playlist_data->tracknum = trackno;
        }
        else if(strncmp(line, "LF=", 3)==0)
            playlist_data->lyricfile = g_strdup(line+3);
        else if(strncmp(line, "AF=", 3)==0)
            playlist_data->albumfile = g_strdup(line+3);
        else if(strncmp(line, "LI=", 3)==0)
        {
            catalog_data = rclib_db_catalog_data_new();
            catalog_data->name = g_strdup(line+3);
            catalog_data->type = RCLIB_DB_CATALOG_TYPE_PLAYLIST;
            catalog_data->playlist = g_sequence_new((GDestroyNotify)
                rclib_db_playlist_data_unref);
            catalog_iter = g_sequence_append(priv->catalog, catalog_data);
            g_signal_emit(db_instance, db_signals[SIGNAL_CATALOG_ADDED],
                0, catalog_iter);
        }
        g_free(line);
    }
    g_object_unref(data_stream);
    return TRUE;
}


