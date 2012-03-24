/*
 * RhythmCat Library Utilities Module
 * Some utility API for the library.
 *
 * rclib-util.c
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

#ifdef G_OS_WIN32
    #include <windows.h>
#endif

#include "rclib-util.h"
#include "rclib-common.h"
#include "rclib-tag.h"

/**
 * SECTION: rclib-util
 * @Short_description: Some utility API
 * @Title: Utilities
 * @Include: rclib-util.h
 *
 * Some utility API for the player.
 */

static const gchar *util_support_formatx = "(.FLAC|.OGG|.MP3|.WMA|.WAV|"
    ".OGA|.OGM|.APE|.AAC|.AC3|.MIDI|.MP2|.MID|.M4A|.CUE|.WV|.WVP|.TTA)$";
static const gchar *util_support_listx = "(.M3U)$";
static gchar *util_cover_search_dir = NULL;

/**
 * rclib_util_get_data_dir:
 * @name: the program name
 * @arg0: the first argument from the main function (argv[0])
 *
 * Get the data directory of the program.
 *
 * Returns: The data directory path.
 */

gchar *rclib_util_get_data_dir(const gchar *name, const gchar *arg0)
{
    gchar *data_dir = NULL;
    gchar *bin_dir = NULL;
    gchar *exec_path = NULL;
    char full_path[PATH_MAX];
    if(name==NULL) return NULL;
    #ifdef G_OS_UNIX
        exec_path = g_file_read_link("/proc/self/exe", NULL);
        if(exec_path!=NULL)
        {
            bin_dir = g_path_get_dirname(exec_path);
            g_free(exec_path);
            exec_path = NULL;
        }
        else exec_path = g_file_read_link("/proc/curproc/file", NULL);
        if(exec_path!=NULL)
        {
            bin_dir = g_path_get_dirname(exec_path);
            g_free(exec_path);
            exec_path = NULL;
        }
        else exec_path = g_file_read_link("/proc/self/path/a.out", NULL);
        if(exec_path!=NULL)
        {
            bin_dir = g_path_get_dirname(exec_path);
            g_free(exec_path);
            exec_path = NULL;
        }
    #endif
    #ifdef G_OS_WIN32
        bzero(full_path, PATH_MAX);
        GetModuleFileName(NULL, full_path, PATH_MAX);
        bin_dir = g_path_get_dirname(full_path);
    #endif
    if(bin_dir!=NULL)
    {
        data_dir = g_build_filename(bin_dir, "..", "share", name,
            NULL);
        if(!g_file_test(data_dir, G_FILE_TEST_IS_DIR))
        {
            g_free(data_dir);
            data_dir = g_strdup(bin_dir);
        }
        g_free(bin_dir);
    }
    #ifdef G_OS_UNIX
        if(data_dir==NULL)
        {
            if(g_path_is_absolute(arg0))
                exec_path = g_strdup(arg0);
            else
            {
                bin_dir = g_get_current_dir();
                exec_path = g_build_filename(bin_dir, arg0, NULL);
                g_free(bin_dir);
            }
            strncpy(full_path, exec_path, PATH_MAX-1);
            g_free(exec_path);
            exec_path = realpath(data_dir, full_path);
            if(exec_path!=NULL)
                bin_dir = g_path_get_dirname(exec_path);
            else
                bin_dir = NULL;
            if(bin_dir!=NULL)
            {
                data_dir = g_build_filename(bin_dir, "..", "share",
                    name, NULL);
                if(!g_file_test(data_dir, G_FILE_TEST_IS_DIR))
                {
                    g_free(data_dir);
                    data_dir = g_strdup(bin_dir);
                }
                g_free(bin_dir);
            }
        }
        if(data_dir==NULL)
        {
            data_dir = g_get_current_dir();
        }
    #endif
    return data_dir;
}

/**
 * rclib_util_is_supported_media:
 * @file: the filename to check
 * 
 * Check whether the given media file is supported by the player.
 *
 * Returns: Whether the file is supported.
 */

gboolean rclib_util_is_supported_media(const gchar *file)
{
    return g_regex_match_simple(util_support_formatx, file,
        G_REGEX_CASELESS, 0);
}

/**
 * rclib_util_is_supported_list:
 * @file: the filename to check
 * 
 * Check whether the given playlist file is supported by the player.
 *
 * Returns: Whether the file is supported.
 */

gboolean rclib_util_is_supported_list(const gchar *file)
{
    return g_regex_match_simple(util_support_listx, file,
        G_REGEX_CASELESS, 0);
}

/**
 * rclib_util_set_cover_search_dir:
 * @dir: the directory to set
 *
 * Set the directory for searching the album cover image files.
 */

void rclib_util_set_cover_search_dir(const gchar *dir)
{
    g_free(util_cover_search_dir);
    util_cover_search_dir = g_strdup(dir);
}

/**
 * rclib_util_get_cover_search_dir:
 *
 * Get the directory for searching the album cover image files.
 *
 * Returns: The directory path.
 */

const gchar *rclib_util_get_cover_search_dir()
{
    return util_cover_search_dir;
}

/**
 * rclib_util_search_cover:
 * @uri: the URI
 * @title: the title
 * @artist: the artist
 * @album: the album
 *
 * Search the album cover image file by given information.
 *
 * Returns: The album cover image file path, NULL if not found.
 */

gchar *rclib_util_search_cover(const gchar *uri, const gchar *title,
    const gchar *artist, const gchar *album)
{
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
    if(artist!=NULL && album!=NULL && strlen(artist)>0 &&
        strlen(album)>0)
    {
        if(flag)
            g_string_append_c(key, '|');
        tmp1 = g_regex_escape_string(album, -1);
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
    if(album!=NULL && strlen(album)>0)
    {
        if(flag)
            g_string_append_c(key, '|');
        tmp1 = g_regex_escape_string(album, -1);
        g_string_append(key, tmp1);
        g_free(tmp1);
        flag = TRUE;
    }
    if(!flag)
    {
        g_string_free(key, TRUE);
        return NULL;
    }
    g_string_append(key, ")\\.(([Jj][Pp][Gg])|([Pp][Nn][Gg])|"
        "([Jj][Pp][Ee][Gg])|([Bb][Mm][Pp]))$");
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
    if(util_cover_search_dir==NULL)
    {
        g_regex_unref(regex);
        return NULL;
    }
    dir = g_dir_open(util_cover_search_dir, 0, NULL);
    if(dir!=NULL)
    {
        while((fname_foreach=g_dir_read_name(dir))!=NULL)
        {
            if(g_regex_match(regex, fname_foreach, 0, NULL))
            {
                result = g_build_filename(util_cover_search_dir,
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
 * rclib_util_detect_encoding_by_locale:
 *
 * Get the most possible encoding by current locale settings.
 *
 * Returns: The encoding, NULL if the encoding cannot be detected.
 * Free it after usage.
 */

gchar *rclib_util_detect_encoding_by_locale()
{
    gchar *encoding = NULL;
    gchar *locale;
    locale = setlocale(LC_ALL, NULL);
    if(g_str_has_prefix(locale, "zh_CN"))
        encoding = g_strdup("GB18030");
    else if(strncmp(locale, "zh_TW", 5)==0)
        encoding = g_strdup("BIG5");
    else if(strncmp(locale, "ja_JP", 5)==0)
        encoding = g_strdup("ShiftJIS");
    return encoding;
}

