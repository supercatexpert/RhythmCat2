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
    #include <winbase.h>
#endif

#include "rclib-util.h"
#include "rclib-common.h"
#include "rclib-tag.h"
#include "rclib-string.h"

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

struct _RCLibMetadata
{
    gint ref_count;
    gint64 length;
    guint tracknum;
    guint bitrate;
    guint samplerate;
    guint channels;
    guint depth;
    gint year;
    RCLibString *uri;
    RCLibString *title;
    RCLibString *artist;
    RCLibString *album;
    RCLibString *comment;
    RCLibString *ftype;
    RCLibString *genre;
};

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
    char full_path[PATH_MAX] = {0};
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
        bin_dir = g_win32_get_package_installation_directory_of_module(NULL);
        data_dir = g_build_filename(bin_dir, "share", name, NULL);
        g_free(bin_dir);
        bin_dir = NULL;
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

GType rclib_metadata_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibMetadata"),
            (GBoxedCopyFunc)rclib_metadata_ref,
            (GBoxedFreeFunc)rclib_metadata_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_metadata_new:
 * 
 * Create a #RCLibMetadata data structure.
 *
 * Returns: (transfer full): The #RCLibMetadata, #NULL if any errors occured.
 */

RCLibMetadata *rclib_metadata_new()
{
    RCLibMetadata *metadata = g_new0(RCLibMetadata, 1);
    metadata->uri = rclib_string_new(NULL);
    metadata->title = rclib_string_new(NULL);
    metadata->artist = rclib_string_new(NULL);
    metadata->album = rclib_string_new(NULL);
    metadata->comment = rclib_string_new(NULL);
    metadata->ftype = rclib_string_new(NULL);
    metadata->genre = rclib_string_new(NULL);
    metadata->ref_count = 1;
    return metadata;
}

/**
 * rclib_metadata_free:
 * @metadata: the #RCLibMetadata structure
 * 
 * Free the #RCLibMetadata data structure.
 */

void rclib_metadata_free(RCLibMetadata *metadata)
{
    if(metadata==NULL) return;
    metadata->ref_count = 0;
    rclib_string_unref(metadata->uri);
    rclib_string_unref(metadata->title);
    rclib_string_unref(metadata->artist);
    rclib_string_unref(metadata->album);
    rclib_string_unref(metadata->comment);
    rclib_string_unref(metadata->ftype);
    rclib_string_unref(metadata->genre);
    g_free(metadata);
}

/**
 * rclib_metadata_ref:
 * @metadata: the #RCLibMetadata structure
 * 
 * Increase the reference count of #RCLibMetadata by 1.
 *
 * Returns: (transfer full): The #RCLibMetadata.
 */

RCLibMetadata *rclib_metadata_ref(RCLibMetadata *metadata)
{
    if(metadata==NULL) return NULL;
    g_atomic_int_add(&(metadata->ref_count), 1);
    return metadata;
}

/**
 * rclib_metadata_unref:
 * @metadata: the #RCLibString structure
 *
 * Decrease the reference of #RCLibMetadata by 1.
 * If the reference down to zero, the structure will be freed.
 */

void rclib_metadata_unref(RCLibMetadata *metadata)
{
    if(metadata==NULL) return;
    if(g_atomic_int_dec_and_test(&(metadata->ref_count)))
        rclib_metadata_free(metadata);
}

/**
 * rclib_metadata_set_int:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 * @value: the value to set
 *
 * Set integer value on the given property of @metadata.
 */

void rclib_metadata_set_int(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, gint value)
{
    if(metadata==NULL) return;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_YEAR:
        {
            metadata->year = value;
            break;
        }
        case RCLIB_METADATA_TYPE_TRACKNUM:
        {
            metadata->tracknum = value;
            break;
        }
        case RCLIB_METADATA_TYPE_BITRATE:
        {
            metadata->bitrate = value;
            break;
        }
        case RCLIB_METADATA_TYPE_SAMPLERATE:
        {
            metadata->samplerate = value;
            break;
        }
        case RCLIB_METADATA_TYPE_CHANNELS:
        {
            metadata->channels = value;
            break;
        }
        case RCLIB_METADATA_TYPE_DEPTH:
        {
            metadata->depth = value;
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            break;
        }
    }
}

/**
 * rclib_metadata_set_uint:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 * @value: the value to set
 *
 * Set unsigned integer value on the given property of @metadata.
 */

void rclib_metadata_set_uint(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, guint value)
{
    if(metadata==NULL) return;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_YEAR:
        {
            metadata->year = value;
        }
        case RCLIB_METADATA_TYPE_TRACKNUM:
        {
            metadata->tracknum = value;
            break;
        }
        case RCLIB_METADATA_TYPE_BITRATE:
        {
            metadata->bitrate = value;
            break;
        }
        case RCLIB_METADATA_TYPE_SAMPLERATE:
        {
            metadata->samplerate = value;
            break;
        }
        case RCLIB_METADATA_TYPE_CHANNELS:
        {
            metadata->channels = value;
            break;
        }
        case RCLIB_METADATA_TYPE_DEPTH:
        {
            metadata->depth = value;
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            break;
        }
    }
}

/**
 * rclib_metadata_set_int64:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 * @value: the value to set
 *
 * Set 64-bit integer value on the given property of @metadata.
 */

void rclib_metadata_set_int64(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, gint64 value)
{
    if(metadata==NULL) return;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_LENGTH:
        {
            metadata->length = value;
        }
        case RCLIB_METADATA_TYPE_YEAR:
        {
            metadata->year = value;
        }
        case RCLIB_METADATA_TYPE_TRACKNUM:
        {
            metadata->tracknum = value;
            break;
        }
        case RCLIB_METADATA_TYPE_BITRATE:
        {
            metadata->bitrate = value;
            break;
        }
        case RCLIB_METADATA_TYPE_SAMPLERATE:
        {
            metadata->samplerate = value;
            break;
        }
        case RCLIB_METADATA_TYPE_CHANNELS:
        {
            metadata->channels = value;
            break;
        }
        case RCLIB_METADATA_TYPE_DEPTH:
        {
            metadata->depth = value;
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            break;
        }
    }
}

/**
 * rclib_metadata_set_string:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 * @value: the value to set
 *
 * Set string value on the given property of @metadata.
 */

void rclib_metadata_set_string(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, RCLibString *value)
{
    if(metadata==NULL) return;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_URI:
        {
            rclib_string_unref(metadata->uri);
            metadata->uri = rclib_string_ref(value); 
            break;
        }
        case RCLIB_METADATA_TYPE_TITLE:
        {
            rclib_string_unref(metadata->title);
            metadata->title = rclib_string_ref(value); 
            break;
        }
        case RCLIB_METADATA_TYPE_ARTIST:
        {
            rclib_string_unref(metadata->artist);
            metadata->artist = rclib_string_ref(value); 
            break;
        }
        case RCLIB_METADATA_TYPE_ALBUM:
        {
            rclib_string_unref(metadata->album);
            metadata->album = rclib_string_ref(value); 
            break;
        }
        case RCLIB_METADATA_TYPE_GENRE:
        {
            rclib_string_unref(metadata->genre);
            metadata->genre = rclib_string_ref(value); 
            break;
        }
        case RCLIB_METADATA_TYPE_COMMENT:
        {
            rclib_string_unref(metadata->comment);
            metadata->comment = rclib_string_ref(value); 
            break;
        }
        case RCLIB_METADATA_TYPE_FTYPE:
        {
            rclib_string_unref(metadata->ftype);
            metadata->ftype = rclib_string_ref(value); 
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            break;
        }
    }
}

/**
 * rclib_metadata_set_cstring:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 * @value: the value to set
 *
 * Set C-style string value on the given property of @metadata.
 */

void rclib_metadata_set_cstring(RCLibMetadata *metadata, 
    RCLibMetadataPropType prop, const gchar *value)
{
    RCLibString *str;
    if(metadata==NULL) return;
    str = rclib_string_new(value);
    rclib_metadata_set_string(metadata, prop, str);
}

/**
 * rclib_metadata_get_int:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 *
 * Get integer value on the given property of @metadata.
 * 
 * Returns: The value of the given property.
 */

gint rclib_metadata_get_int(RCLibMetadata *metadata,
    RCLibMetadataPropType prop)
{
    if(metadata==NULL) return 0;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_YEAR:
        {
            return metadata->year;
            break;
        }
        case RCLIB_METADATA_TYPE_TRACKNUM:
        {
            return metadata->tracknum;
            break;
        }
        case RCLIB_METADATA_TYPE_BITRATE:
        {
            return metadata->bitrate;
            break;
        }
        case RCLIB_METADATA_TYPE_SAMPLERATE:
        {
            return metadata->samplerate;
            break;
        }
        case RCLIB_METADATA_TYPE_CHANNELS:
        {
            return metadata->channels;
            break;
        }
        case RCLIB_METADATA_TYPE_DEPTH:
        {
            return metadata->depth;
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            return 0;
            break;
        }
    }
    return 0;
}

/**
 * rclib_metadata_get_uint:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 *
 * Get unsigned integer value on the given property of @metadata.
 * 
 * Returns: The value of the given property.
 */

guint rclib_metadata_get_uint(RCLibMetadata *metadata,
    RCLibMetadataPropType prop)
{
    if(metadata==NULL) return 0;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_YEAR:
        {
            return metadata->year;
            break;
        }
        case RCLIB_METADATA_TYPE_TRACKNUM:
        {
            return metadata->tracknum;
            break;
        }
        case RCLIB_METADATA_TYPE_BITRATE:
        {
            return metadata->bitrate;
            break;
        }
        case RCLIB_METADATA_TYPE_SAMPLERATE:
        {
            return metadata->samplerate;
            break;
        }
        case RCLIB_METADATA_TYPE_CHANNELS:
        {
            return metadata->channels;
            break;
        }
        case RCLIB_METADATA_TYPE_DEPTH:
        {
            return metadata->depth;
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            return 0;
            break;
        }
    }
    return 0;
}

/**
 * rclib_metadata_get_int64:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 *
 * Get 64-bit integer value on the given property of @metadata.
 * 
 * Returns: The value of the given property.
 */

gint64 rclib_metadata_get_int64(RCLibMetadata *metadata,
    RCLibMetadataPropType prop)
{
    if(metadata==NULL) return 0;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_LENGTH:
        {
            return metadata->length;
            break;
        }
        case RCLIB_METADATA_TYPE_YEAR:
        {
            return metadata->year;
            break;
        }
        case RCLIB_METADATA_TYPE_TRACKNUM:
        {
            return metadata->tracknum;
            break;
        }
        case RCLIB_METADATA_TYPE_BITRATE:
        {
            return metadata->bitrate;
            break;
        }
        case RCLIB_METADATA_TYPE_SAMPLERATE:
        {
            return metadata->samplerate;
            break;
        }
        case RCLIB_METADATA_TYPE_CHANNELS:
        {
            return metadata->channels;
            break;
        }
        case RCLIB_METADATA_TYPE_DEPTH:
        {
            return metadata->depth;
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            return 0;
            break;
        }
    }
    return 0;
}

/**
 * rclib_metadata_get_string:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 *
 * Get string value on the given property of @metadata.
 * 
 * Returns: (transfer full): The value of the given property.
 */

RCLibString *rclib_metadata_get_string(RCLibMetadata *metadata,
    RCLibMetadataPropType prop)
{
    if(metadata==NULL) return NULL;
    switch(prop)
    {
        case RCLIB_METADATA_TYPE_URI:
        {
            return rclib_string_ref(metadata->uri);
            break;
        }
        case RCLIB_METADATA_TYPE_TITLE:
        {
            return rclib_string_ref(metadata->title);
            break;
        }
        case RCLIB_METADATA_TYPE_ARTIST:
        {
            return rclib_string_ref(metadata->artist);
            break;
        }
        case RCLIB_METADATA_TYPE_ALBUM:
        {
            return rclib_string_ref(metadata->album);
            break;
        }
        case RCLIB_METADATA_TYPE_GENRE:
        {
            return rclib_string_ref(metadata->genre);
            break;
        }
        case RCLIB_METADATA_TYPE_COMMENT:
        {
            return rclib_string_ref(metadata->comment);
            break;
        }
        case RCLIB_METADATA_TYPE_FTYPE:
        {
            return rclib_string_ref(metadata->ftype);
            break;
        }
        default:
        {
            g_warning("Wrong value type!");
            break;
        }
    }
    return NULL;
}

/**
 * rclib_metadata_get_cstring:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 *
 * Get C-style string value on the given property of @metadata.
 * 
 * Returns: (transfer none): The value of the given property.
 */

const gchar *rclib_metadata_get_cstring(RCLibMetadata *metadata,
    RCLibMetadataPropType prop)
{
    RCLibString *str;
    const gchar *value;
    if(metadata==NULL) return NULL;
    str = rclib_metadata_get_string(metadata, prop);
    value = rclib_string_get(str);
    rclib_string_unref(str);
    return value;
}

/**
 * rclib_metadata_dup_cstring:
 * @metadata: the #RCLibString structure
 * @prop: the property type
 *
 * Duplicate C-style string value on the given property of @metadata.
 * 
 * Returns: (transfer none): The value of the given property.
 */

gchar *rclib_metadata_dup_cstring(RCLibMetadata *metadata,
    RCLibMetadataPropType prop)
{
    RCLibString *str;
    gchar *value;
    if(metadata==NULL) return NULL;
    str = rclib_metadata_get_string(metadata, prop);
    value = rclib_string_dup(str);
    rclib_string_unref(str);
    return value;
}
