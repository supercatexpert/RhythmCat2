/*
 * RhythmCat Library CUE Parser Header Declaration
 *
 * rclib-cue.h
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

#ifndef HAVE_RCLIB_CUE_H
#define HAVE_RCLIB_CUE_H

#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * RCLibCueInputType:
 * @RCLIB_CUE_INPUT_URI: the input data is a URI
 * @RCLIB_CUE_INPUT_PATH: the input data is a file path
 * @RCLIB_CUE_INPUT_EMBEDDED: the input data is from a embedded CUE tag
 *
 * The input data type of CUE.
 */

typedef enum {
    RCLIB_CUE_INPUT_URI = 0,
    RCLIB_CUE_INPUT_PATH = 1,
    RCLIB_CUE_INPUT_EMBEDDED = 2
}RCLibCueInputType;

typedef struct _RCLibCueTrack RCLibCueTrack;
typedef struct _RCLibCueData RCLibCueData;

/**
 * RCLibCueTrack:
 * @index: the track index
 * @title: the track title
 * @performer: the track performer (artist)
 * @time0: the INDEX 00 time
 * @time1: the INDEX 01 time (start time)
 *
 * The track data structure of CUE data.
 */

struct _RCLibCueTrack {
    guint index;
    gchar *title;
    gchar *performer;
    guint64 time0;
    guint64 time1;
};

/**
 * RCLibCueData:
 * @type: the input type of the CUE file
 * @file: the audio file URI
 * @performer: the performer
 * @title: the title (it is usually the album name)
 * @length: the track length (number)
 * @track: the track data
 *
 * The structure for CUE sheet data.
 */

struct _RCLibCueData {
    RCLibCueInputType type;
    gchar *file;
    gchar *performer;
    gchar *title;
    guint length;
    RCLibCueTrack *track;
};

guint rclib_cue_read_data(const gchar *input, RCLibCueInputType type,
    RCLibCueData *data);
void rclib_cue_free(RCLibCueData *data);
gboolean rclib_cue_get_track_num(const gchar *path, gchar **cue_path,
    gint *track_num);
void rclib_cue_set_fallback_encoding(const gchar *encoding);
const gchar *rclib_cue_get_fallback_encoding();

G_END_DECLS

#endif

