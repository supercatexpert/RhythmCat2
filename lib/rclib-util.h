/*
 * RhythmCat Library Utilities Header Declaration
 *
 * rclib-util.h
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

#ifndef HAVE_RCLIB_UTILITIES_H
#define HAVE_RCLIB_UTILITIES_H

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * RCLibString:
 * 
 * The structure for reference countable string.
 */

typedef struct _RCLibString RCLibString;

/**
 * RCLibMetadata:
 * 
 * The structure for metadata.
 */

typedef struct _RCLibMetadata RCLibMetadata;

/**
 * RCLibMetadataPropType:
 * @RCLIB_METADATA_TYPE_NONE: none type, not used by data
 * @RCLIB_METADATA_TYPE_URI: the URI (string)
 * @RCLIB_METADATA_TYPE_TITLE: the title (string)
 * @RCLIB_METADATA_TYPE_ARTIST: the artist (string)
 * @RCLIB_METADATA_TYPE_ALBUM: the album (string)
 * @RCLIB_METADATA_TYPE_FTYPE: the file type (string)
 * @RCLIB_METADATA_TYPE_LENGTH: the time length (#gint64)
 * @RCLIB_METADATA_TYPE_TRACKNUM: the track number (#gint)
 * @RCLIB_METADATA_TYPE_YEAR: the year (#gint),
 * @RCLIB_METADATA_TYPE_COMMENT: the comment (string)
 * @RCLIB_METADATA_TYPE_GENRE: the genre (string)
 * @RCLIB_METADATA_TYPE_SAMPLERATE: the sample rate (guint)
 * @RCLIB_METADATA_TYPE_BITRATE: the bitrate (guint)
 * @RCLIB_METADATA_TYPE_DEPTH: the bit depth (guint)
 * @RCLIB_METADATA_TYPE_CHANNELS: the number of channels (guint)
 * 
 * The enum type for set/get the data in the #RCLibDbPlaylistData
 */

typedef enum {
    RCLIB_METADATA_TYPE_NONE = 0,
    RCLIB_METADATA_TYPE_URI = 1,
    RCLIB_METADATA_TYPE_TITLE = 2,
    RCLIB_METADATA_TYPE_ARTIST = 3,
    RCLIB_METADATA_TYPE_ALBUM = 4,
    RCLIB_METADATA_TYPE_FTYPE = 5,
    RCLIB_METADATA_TYPE_LENGTH = 6,
    RCLIB_METADATA_TYPE_TRACKNUM = 7,
    RCLIB_METADATA_TYPE_YEAR = 8,
    RCLIB_METADATA_TYPE_COMMENT = 9,
    RCLIB_METADATA_TYPE_GENRE = 10,
    RCLIB_METADATA_TYPE_SAMPLERATE = 11,
    RCLIB_METADATA_TYPE_BITRATE = 12,
    RCLIB_METADATA_TYPE_DEPTH = 13,
    RCLIB_METADATA_TYPE_CHANNELS = 14
}RCLibMetadataPropType;

gchar *rclib_util_get_data_dir(const gchar *name, const gchar *arg0);
gboolean rclib_util_is_supported_media(const gchar *file);
gboolean rclib_util_is_supported_list(const gchar *file);
void rclib_util_set_cover_search_dir(const gchar *dir);
const gchar *rclib_util_get_cover_search_dir();
gchar *rclib_util_search_cover(const gchar *uri, const gchar *title,
    const gchar *artist, const gchar *album);
gchar *rclib_util_detect_encoding_by_locale();

/*< private >*/
GType rclib_string_get_type();
GType rclib_metadata_get_type();

/*< public >*/
RCLibString *rclib_string_new(const gchar *init);
void rclib_string_free(RCLibString *str);
RCLibString *rclib_string_ref(RCLibString *str);
void rclib_string_unref(RCLibString *str);
void rclib_string_set(RCLibString *str, const gchar *text);
const gchar *rclib_string_get(const RCLibString *str);
gchar *rclib_string_dup(RCLibString *str);
gsize rclib_string_length(RCLibString *str);
void rclib_string_printf(RCLibString *str, const gchar *format, ...);
void rclib_string_vprintf(RCLibString *str, const gchar *format, va_list args);
void rclib_string_append(RCLibString *str, const gchar *text);
void rclib_string_append_len(RCLibString *str, const gchar *text, gssize len);
void rclib_string_append_printf(RCLibString *str, const gchar *format, ...);
void rclib_string_append_vprintf(RCLibString *str, const gchar *format,
    va_list args);
void rclib_string_prepend(RCLibString *str, const gchar *text);
void rclib_string_prepend_len(RCLibString *str, const gchar *text, gssize len);
void rclib_string_insert(RCLibString *str, gssize pos, const gchar *text);
void rclib_string_insert_len(RCLibString *str, gssize pos, const gchar *text,
    gssize len);
void rclib_string_overwrite(RCLibString *str, gsize pos, const gchar *text);
void rclib_string_overwrite_len(RCLibString *str, gsize pos, const gchar *text,
    gssize len);
void rclib_string_erase(RCLibString *str, gssize pos, gssize len);
void rclib_string_truncate(RCLibString *str, gsize len);
guint rclib_string_hash(const RCLibString *str);
gboolean rclib_string_equal(const RCLibString *str1, const RCLibString *str2);

RCLibMetadata *rclib_metadata_new();
void rclib_metadata_free(RCLibMetadata *metadata);
RCLibMetadata *rclib_metadata_ref(RCLibMetadata *metadata);
void rclib_metadata_unref(RCLibMetadata *metadata);
void rclib_metadata_set_int(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, gint value);
void rclib_metadata_set_uint(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, guint value);
void rclib_metadata_set_int64(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, gint64 value);
void rclib_metadata_set_string(RCLibMetadata *metadata,
    RCLibMetadataPropType prop, RCLibString *value);
void rclib_metadata_set_cstring(RCLibMetadata *metadata, 
    RCLibMetadataPropType prop, const gchar *value);
gint rclib_metadata_get_int(RCLibMetadata *metadata,
    RCLibMetadataPropType prop);
guint rclib_metadata_get_uint(RCLibMetadata *metadata,
    RCLibMetadataPropType prop);
gint64 rclib_metadata_get_int64(RCLibMetadata *metadata,
    RCLibMetadataPropType prop);
RCLibString *rclib_metadata_get_string(RCLibMetadata *metadata,
    RCLibMetadataPropType prop);
const gchar *rclib_metadata_get_cstring(RCLibMetadata *metadata,
    RCLibMetadataPropType prop);
gchar *rclib_metadata_dup_cstring(RCLibMetadata *metadata,
    RCLibMetadataPropType prop);

G_END_DECLS

#endif

