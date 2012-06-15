/*
 * RhythmCat Library Tag Manager Header Declaration
 *
 * rclib-tag.h
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

#ifndef HAVE_RC_LIB_TAG_H
#define HAVE_RC_LIB_TAG_H

#include <glib.h>
#include <glib/gi18n.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define RCLIB_TYPE_TAG_METADATA (rclib_tag_metadata_get_type())

typedef struct _RCLibTagMetadata RCLibTagMetadata;

/**
 * RCLibTagMetadata:
 * @length: the length of the music
 * @uri: the URI of the music
 * @tracknum: the track number of the music
 * @bitrate: the bitrate of the music
 * @samplerate: the sample rate of the music
 * @channels: the channel number of the music
 * @year: the year of the music
 * @title: the title text of the music
 * @artist: the artist text of the music
 * @album: the album text of the music
 * @comment: the comment text of the music
 * @ftype: the file type of the music
 * @emb_cue: the embedded CUE data of the music
 * @image: the GstBuffer which contains the cover image
 * @eos: the EOS signal
 * @audio_flag: whether this file has audio
 * @video_flag: whether this file has video
 * @user_data: the user data
 *
 * The structure for storing the music metadata.
 */

struct _RCLibTagMetadata {
    gint64 length;
    gchar *uri;
    guint tracknum;
    guint bitrate;
    gint samplerate;
    gint channels;
    gint year;
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *comment;
    gchar *ftype;
    gchar *emb_cue;
    GstBuffer *image;
    gboolean eos;
    gboolean audio_flag;
    gboolean video_flag;
    gpointer user_data;
};

/*< private >*/
GType rclib_tag_metadata_get_type();

/*< public >*/
RCLibTagMetadata *rclib_tag_read_metadata(const gchar *uri);
RCLibTagMetadata *rclib_tag_copy_data(const RCLibTagMetadata *mmd);
void rclib_tag_free(RCLibTagMetadata *mmd);
gchar *rclib_tag_get_name_from_fpath(const gchar *filename);
gchar *rclib_tag_get_name_from_uri(const gchar *uri);
void rclib_tag_set_fallback_encoding(const gchar *encoding);
const gchar *rclib_tag_get_fallback_encoding();

G_END_DECLS

#endif

