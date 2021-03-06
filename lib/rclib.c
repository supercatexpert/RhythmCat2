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
const guint rclib_micro_version = 5;
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
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    RCLibCoreSourceType type;
    RCLibCoreSourceType new_type;
    RCLibDbPlaylistIter *iter = NULL;
    gchar *puri = NULL;
    if(metadata==NULL || uri==NULL) return;
    if(!rclib_core_get_play_source(&source_type, (gpointer *)&iter, NULL))
        return;
    if(source_type!=RCLIB_CORE_PLAY_SOURCE_PLAYLIST || iter!=NULL)
        return;
    if(!rclib_db_playlist_is_valid_iter(iter)) return;
    rclib_db_playlist_data_iter_get(iter, RCLIB_DB_PLAYLIST_DATA_TYPE_URI,
        &puri, RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE, &type,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    ret = g_strcmp0(uri, puri);
    g_free(puri);
    if(type==RCLIB_CORE_SOURCE_NORMAL && ret!=0) return;
    playlist_data = rclib_db_playlist_data_new();
    if(type==RCLIB_CORE_SOURCE_CUE || type==RCLIB_CORE_SOURCE_EMBEDDED_CUE)
        new_type = RCLIB_DB_PLAYLIST_TYPE_CUE;
    else
        new_type = RCLIB_DB_PLAYLIST_TYPE_MUSIC;
    rclib_db_playlist_data_set(playlist_data,
        RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE, new_type,
        RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE, metadata->title,
        RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST, metadata->artist,
        RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM, metadata->album,
        RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE, metadata->ftype,
        RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE, metadata->genre,
        RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM, metadata->track,
        RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR, metadata->year,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    rclib_db_playlist_update_metadata(iter, playlist_data);
    if(metadata->duration>0)
    {
        rclib_db_playlist_data_iter_set(iter,
            RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH, metadata->duration,
            RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    }
    rclib_db_playlist_data_free(playlist_data);
}

static void rclib_main_update_db_duration_cb(RCLibCore *core,
    gint64 duration, gpointer data)
{
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    gpointer iter = NULL;
    if(!rclib_core_get_play_source(&source_type, &iter, NULL))
        return;
    if(iter==NULL || duration<0)
        return;
    if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
    {
        rclib_db_playlist_data_iter_set((RCLibDbPlaylistIter *)iter,
            RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH, duration,
            RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    }
}

static void rclib_main_catalog_delete_cb(RCLibDb *db,
    RCLibDbCatalogIter *iter, gpointer data)
{
    RCLibDbCatalogIter *catalog_iter = NULL;
    RCLibDbPlaylistIter *reference_iter;
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    if(iter==NULL) return;
    if(!rclib_core_get_play_source(&source_type, (gpointer *)&reference_iter,
        NULL))
    {
        return;
    }
    if(source_type!=RCLIB_CORE_PLAY_SOURCE_PLAYLIST || reference_iter==NULL)
        return;
    rclib_db_playlist_data_iter_get(reference_iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    if(catalog_iter==iter)
    {
        rclib_core_update_play_source(RCLIB_CORE_PLAY_SOURCE_PLAYLIST, NULL,
            NULL, NULL);
    }
}

static void rclib_main_playlist_delete_cb(RCLibDb *db,
    RCLibDbPlaylistIter *iter, gpointer data)
{
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    RCLibDbPlaylistIter *reference_iter = NULL;
    if(iter==NULL) return;
    if(!rclib_core_get_play_source(&source_type, (gpointer *)&reference_iter,
        NULL))
    {
        return;
    }
    if(source_type!=RCLIB_CORE_PLAY_SOURCE_PLAYLIST || reference_iter==NULL)
        return;
    if(iter==reference_iter)
    {
        rclib_core_update_play_source(RCLIB_CORE_PLAY_SOURCE_PLAYLIST, NULL,
            NULL, NULL);
    }
}

static void rclib_main_error_cb(RCLibCore *core, const gchar *message,
    gpointer data)
{
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    gpointer reference = NULL;
    gchar *cookie = NULL;
    if(!rclib_core_get_play_source(&source_type, &reference, &cookie))
        return;
    if(reference==NULL)
    {
        g_free(cookie);
        return;
    }
    if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
    {
        rclib_db_playlist_data_iter_set((RCLibDbPlaylistIter *)reference,
            RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE, RCLIB_DB_PLAYLIST_TYPE_MISSING,
            RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    }
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
    db_file = g_build_filename(dir, "library.zdb", NULL);
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

