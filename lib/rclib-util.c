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

struct _RCLibString
{
    gint ref_count;
    GString *str;
};

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

GType rclib_string_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibString"),
            (GBoxedCopyFunc)rclib_string_ref,
            (GBoxedFreeFunc)rclib_string_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
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
 * rclib_string_new:
 * @init: the initial string
 * 
 * Create a #RCLibString data structure.
 *
 * Returns: (transfer full): The #RCLibString, #NULL if any errors occured.
 */
 
RCLibString *rclib_string_new(const gchar *init)
{
    RCLibString *str = g_new0(RCLibString, 1);
    if(init==NULL) init = "";
    str->str = g_string_new(init);
    str->ref_count = 1;
    return str;
}

/**
 * rclib_string_free:
 * @str: the #RCLibString structure
 * 
 * Free the #RCLibString data structure.
 */

void rclib_string_free(RCLibString *str)
{
    if(str==NULL) return;
    if(str->str!=NULL)
    {
        g_string_free(str->str, TRUE);
        str->str = NULL;
    }
    str->ref_count = 0;
    g_free(str);
}

/**
 * rclib_string_ref:
 * @str: the #RCLibString structure
 * 
 * Increase the reference count of #RCLibString by 1.
 *
 * Returns: (transfer full): The #RCLibString.
 */

RCLibString *rclib_string_ref(RCLibString *str)
{
    if(str==NULL) return NULL;
    g_atomic_int_add(&(str->ref_count), 1);
    return str;
}


/**
 * rclib_string_unref:
 * @str: the #RCLibString structure
 *
 * Decrease the reference of #RCLibString by 1.
 * If the reference down to zero, the structure will be freed.
 */

void rclib_string_unref(RCLibString *str)
{
    if(str==NULL) return;
    if(g_atomic_int_dec_and_test(&(str->ref_count)))
        rclib_string_free(str);
}

/**
 * rclib_string_set:
 * @str: the #RCLibString structure
 * @text: the C string to set
 * 
 * Set the @text to @str, replace the string inside @str if exists.
 */

void rclib_string_set(RCLibString *str, const gchar *text)
{
    if(str==NULL) return;
    if(text==NULL) text = "";
    (void)g_string_assign(str->str, text);
}

/**
 * rclib_string_get:
 * @str: the #RCLibString structure
 * 
 * Get the C string inside @str
 * 
 * Returns: The C string inside @str, do not free or modify it.
 */

const gchar *rclib_string_get(const RCLibString *str)
{
    if(str==NULL) return NULL;
    return str->str->str;
}

/**
 * rclib_string_dup:
 * @str: the #RCLibString structure
 * 
 * Duplicate the C string inside @str
 * 
 * Returns: The C string deplicated from @str, use #g_free() to free it.
 */

gchar *rclib_string_dup(RCLibString *str)
{
    if(str==NULL) return NULL;
    return g_strdup(str->str->str);
}

/**
 * rclib_string_length:
 * @str: the #RCLibString structure
 * 
 * Get the string length inside @str
 * 
 * Returns: The string length.
 */

gsize rclib_string_length(RCLibString *str)
{
    if(str==NULL) return 0;
    return str->str->len;
}

/**
 * rclib_string_printf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @...: the parameters to insert into the format string
 * 
 * Write a formatted string into @str, just similar to the standard sprintf()
 * function.
 */

void rclib_string_printf(RCLibString *str, const gchar *format, ...)
{
    va_list valist;
    if(str==NULL || format==NULL) return;
    va_start(valist, format);
    g_string_vprintf(str->str, format, valist);
    va_end(valist);
}

/**
 * rclib_string_vprintf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @args: the list of arguments to insert in the output
 * 
 * Write a formatted string into @str, just similar to the standard vsprintf()
 * function.
 */

void rclib_string_vprintf(RCLibString *str, const gchar *format, va_list args)
{
    if(str==NULL || format==NULL) return;
    g_string_vprintf(str->str, format, args);
}

/**
 * rclib_string_append:
 * @str: the #RCLibString structure
 * @text: the text to append
 * 
 * Append @text to @str.
 */

void rclib_string_append(RCLibString *str, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_append(str->str, text);
}

/**
 * rclib_string_append_len:
 * @str: the #RCLibString structure
 * @text: the text to append
 * @len: number of bytes of @text to use
 * 
 * Append @len bytes of @text to @str.
 */

void rclib_string_append_len(RCLibString *str, const gchar *text, gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_append_len(str->str, text, len);
}

/**
 * rclib_string_append_printf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @...: the parameters to insert into the format string
 * 
 * Appends a formatted string onto the end of @str, just similar to
 * #rclib_string_printf() except that the text is appended to the @str.
 */

void rclib_string_append_printf(RCLibString *str, const gchar *format, ...)
{
    va_list valist;
    if(str==NULL || format==NULL) return;
    va_start(valist, format);
    g_string_append_vprintf(str->str, format, valist);
    va_end(valist);
}

/**
 * rclib_string_append_vprintf:
 * @str: the #RCLibString structure
 * @format: the string format. See the printf() documentation
 * @args: the list of arguments to insert in the output
 * 
 * Appends a formatted string onto the end of @str, just similar to
 * #rclib_string_printf() except that the text is appended to the @str.
 */

void rclib_string_append_vprintf(RCLibString *str, const gchar *format,
    va_list args)
{
    if(str==NULL || format==NULL) return;
    g_string_append_vprintf(str->str, format, args);
}

/**
 * rclib_string_prepend:
 * @str: the #RCLibString structure
 * @text: the text to prepend
 * 
 * Add @text on to the start of @str.
 */

void rclib_string_prepend(RCLibString *str, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_prepend(str->str, text);
}

/**
 * rclib_string_prepend_len:
 * @str: the #RCLibString structure
 * @text: the text to prepend
 * @len: number of bytes of @text to use
 * 
 * Add @len bytes of @text on to the start of @str.
 */

void rclib_string_prepend_len(RCLibString *str, const gchar *text, gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_prepend_len(str->str, text, len);
}

/**
 * rclib_string_insert:
 * @str: the #RCLibString structure
 * @pos: the position to insert the copy of the string
 * @text: the text to insert
 * 
 * Inserts a copy of a string into @str at @pos, expanding it if necessary.
 */

void rclib_string_insert(RCLibString *str, gssize pos, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_insert(str->str, pos, text);
}

/**
 * rclib_string_insert_len:
 * @str: the #RCLibString structure
 * @pos: the position to insert the copy of the string
 * @text: the text to insert
 * @len: number of bytes of @text to insert
 * 
 * Inserts @len bytes of @text into @str at @pos, expanding it if necessary.
 */

void rclib_string_insert_len(RCLibString *str, gssize pos, const gchar *text,
    gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_insert_len(str->str, pos, text, len);
}

/**
 * rclib_string_overwrite:
 * @str: the #RCLibString structure
 * @pos: the position at which to start overwriting
 * @text: the text for overwriting
 * 
 * Overwrites part of a string, lengthening it if necessary.
 */

void rclib_string_overwrite(RCLibString *str, gsize pos, const gchar *text)
{
    if(str==NULL || text==NULL) return;
    g_string_overwrite(str->str, pos, text);
}

/**
 * rclib_string_overwrite_len:
 * @str: the #RCLibString structure
 * @pos: the position at which to start overwriting
 * @text: the text for overwriting
 * @len: the number of bytes to write from @text
 * 
 * Overwrites part of a string, lengthening it if necessary. 
 */

void rclib_string_overwrite_len(RCLibString *str, gsize pos, const gchar *text,
    gssize len)
{
    if(str==NULL || text==NULL) return;
    g_string_overwrite_len(str->str, pos, text, len);
}

/**
 * rclib_string_erase:
 * @str: the #RCLibString structure
 * @pos: the position of the content to remove
 * @len: the number of bytes to remove, or -1 to remove all following bytes
 * 
 * Removes @len bytes from @str, starting at position @pos. The rest of
 *     the @str is shifted down to fill the gap.
 */

void rclib_string_erase(RCLibString *str, gssize pos, gssize len)
{
    if(str==NULL) return;
    g_string_erase(str->str, pos, len);
}

/**
 * rclib_string_truncate:
 * @str: the #RCLibString structure
 * @len: the new size of @str
 * 
 * Cuts off the end of the @str, leaving the first @len bytes.
 */

void rclib_string_truncate(RCLibString *str, gsize len)
{
    if(str==NULL) return;
    g_string_truncate(str->str, len);
}

/**
 * rclib_string_hash:
 * @str: the #RCLibString structure
 * 
 * Creates a hash code for @str; for use with #GHashTable.
 * 
 * Returns: Hash code for @str.
 */

guint rclib_string_hash(const RCLibString *str)
{
    if(str==NULL) return 0;
    return g_string_hash(str->str);
}

/**
 * rclib_string_equal:
 * @str1: the first #RCLibString
 * @str2: the second #RCLibString
 * 
 * Compares two strings for equality, returning TRUE if they are equal.
 *     For use with #GHashTable.
 * 
 * Returns: #TRUE if they strings are the same length and contain
 *     the same bytes.
 */

gboolean rclib_string_equal(const RCLibString *str1, const RCLibString *str2)
{
    if(str1==NULL || str2==NULL) return FALSE;
    return g_string_equal(str1->str, str2->str);
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
