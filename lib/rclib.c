/*
 * RhythmCat Library Main Module
 * Library initialization and main event loop.
 *
 * rclib.c
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

#include "rclib.h"
#include "rclib-common.h"

/**
 * SECTION: rclib
 * @Short_description: The main module
 * @Title: Main Module
 * @Include: rclib.h
 *
 * The main module is used for general library initialization and library
 * information access, it will initialize the core, database, player
 * schedulor, lyric processor, and connect some signals to callbacks for
 * update the metadata in the database. If you do not want to initialize
 * the modules by yourself, just using function rclib_init().
 */

const guint rclib_major_version = 1;
const guint rclib_minor_version = 9;
const guint rclib_micro_version = 2;
const gchar *rclib_build_date = __DATE__;
const gchar *rclib_build_time = __TIME__;

static gchar *db_file = NULL;
static gulong main_tag_found_handler = 0;
static gulong main_new_duration_handler = 0;
static gulong main_catalog_delete_handler = 0;
static gulong main_playist_delete_handler = 0;
static gulong main_error_handler = 0;

static void rclib_main_update_db_metadata_cb(RCLibCore *core,
    const RCLibCoreMetadata *metadata, const gchar *uri, gpointer data)
{
    RCLibDbPlaylistData *playlist_data;
    gint ret;
    RCLibCoreSourceType type;
    GSequenceIter *iter;
    if(metadata==NULL || uri==NULL) return;
    iter = rclib_core_get_db_reference();
    if(iter==NULL) return;
    playlist_data = g_sequence_get(iter);
    type = rclib_core_get_source_type();
    ret = g_strcmp0(uri, playlist_data->uri);
    if(type==RCLIB_CORE_SOURCE_NORMAL && ret!=0) return;
    playlist_data = g_new0(RCLibDbPlaylistData, 1);
    playlist_data->title = metadata->title;
    playlist_data->artist = metadata->artist;
    playlist_data->album = metadata->album;
    playlist_data->ftype = metadata->ftype;
    playlist_data->tracknum = metadata->track;
    playlist_data->year = metadata->year;
    if(type==RCLIB_CORE_SOURCE_CUE || type==RCLIB_CORE_SOURCE_EMBEDED_CUE)
        playlist_data->type = RCLIB_DB_PLAYLIST_TYPE_CUE;
    else
        playlist_data->type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
    rclib_db_playlist_update_metadata(iter, playlist_data);
    if(metadata->duration>0)
        rclib_db_playlist_update_length(iter, metadata->duration);
    g_free(playlist_data);
}

static void rclib_main_update_db_duration_cb(RCLibCore *core,
    gint64 duration, gpointer data)
{
    GSequenceIter *iter = rclib_core_get_db_reference();
    if(iter==NULL || duration<0) return;
    rclib_db_playlist_update_length(iter, duration);
}

static void rclib_main_catalog_delete_cb(RCLibDb *db, GSequenceIter *iter,
    gpointer data)
{
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *reference_iter;
    if(iter==NULL) return;
    reference_iter = rclib_core_get_db_reference();
    if(reference_iter!=NULL)
    {
        playlist_data = g_sequence_get(reference_iter);
        if(playlist_data->catalog==iter)
            rclib_core_update_db_reference(NULL);
    }
}

static void rclib_main_playlist_delete_cb(RCLibDb *db, GSequenceIter *iter,
    gpointer data)
{
    if(iter==NULL) return;
    if(iter==rclib_core_get_db_reference())
        rclib_core_update_db_reference(NULL);
}

static void rclib_main_error_cb(RCLibCore *core, const gchar *message,
    gpointer data)
{
    GSequenceIter *iter;
    iter = rclib_core_get_db_reference();
    if(iter==NULL) return;
    rclib_db_playlist_set_type(iter, RCLIB_DB_PLAYLIST_TYPE_MISSING);
}

/**
 * rclib_init:
 * @argc: (inout): address of the <parameter>argc</parameter> parameter of
 *     your main() function (or 0 if @argv is %NULL). This will be changed if 
 *     any arguments were handled
 * @argv: (array length=argc) (inout) (allow-none): address of the
 *     <parameter>argv</parameter> parameter of main(), or %NULL
 * @dir: the directory of the user data
 * @error: return location for a GError, or %NULL
 *
 * Initialize the library, please call this function before using any other
 * library functions. If you want to initialize the library by yourself, you
 * should call the initializion function in each module.
 *
 * Returns: Whether the initializion succeeded.
 */

gboolean rclib_init(gint *argc, gchar **argv[], const gchar *dir,
    GError **error)
{
    gchar *lyric_dir, *album_dir;
    gchar *settings_file;
    if(dir==NULL) return FALSE;
    g_type_init();
    if(!gst_init_check(argc, argv, error))
        return FALSE;
    if(!rclib_core_init(error))
        return FALSE;
    g_mkdir_with_parents(dir, 0700);
    db_file = g_build_filename(dir, "library.db", NULL);
    if(!rclib_db_init(db_file))
    {
        rclib_core_exit();
        return FALSE;
    }
    rclib_player_init();
    rclib_lyric_init();
    rclib_album_init();
    rclib_settings_init();
    lyric_dir = g_build_filename(dir, "Lyrics", NULL);
    g_mkdir_with_parents(lyric_dir, 0700);
    rclib_lyric_set_search_dir(lyric_dir);
    g_free(lyric_dir);
    album_dir = g_build_filename(dir, "AlbumImages", NULL);
    g_mkdir_with_parents(album_dir, 0700);
    rclib_util_set_cover_search_dir(album_dir);
    g_free(album_dir);
    settings_file = g_build_filename(dir, "settings.conf", NULL);
    rclib_settings_load_from_file(settings_file);
    g_free(settings_file);
    main_tag_found_handler = rclib_core_signal_connect("tag-found",
        G_CALLBACK(rclib_main_update_db_metadata_cb), NULL);
    main_new_duration_handler = rclib_core_signal_connect("new-duration",
        G_CALLBACK(rclib_main_update_db_duration_cb), NULL);
    main_catalog_delete_handler = rclib_db_signal_connect("catalog-delete",
        G_CALLBACK(rclib_main_catalog_delete_cb), NULL);
    main_playist_delete_handler = rclib_db_signal_connect("playlist-delete",
        G_CALLBACK(rclib_main_playlist_delete_cb), NULL);
    main_error_handler = rclib_core_signal_connect("error",
        G_CALLBACK(rclib_main_error_cb), NULL);
    return TRUE;
}

/**
 * rclib_exit:
 *
 * Exit and unload the library.
 */

void rclib_exit()
{
    if(main_tag_found_handler>0)
        rclib_core_signal_disconnect(main_tag_found_handler);
    if(main_new_duration_handler>0)
        rclib_core_signal_disconnect(main_new_duration_handler);
    if(main_catalog_delete_handler>0)
        rclib_db_signal_disconnect(main_catalog_delete_handler);
    if(main_playist_delete_handler>0)
        rclib_db_signal_disconnect(main_playist_delete_handler);
    if(main_error_handler>0)
        rclib_core_signal_disconnect(main_error_handler);
    g_free(db_file);
    rclib_settings_exit();
    rclib_album_exit();
    rclib_lyric_exit();
    rclib_core_exit();
    rclib_db_exit();
}

