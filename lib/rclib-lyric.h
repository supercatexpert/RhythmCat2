/*
 * RhythmCat Library Lyric Backend Header Declaration
 *
 * rclib-lyric.h
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

#ifndef HAVE_RCLIB_LYRIC_H
#define HAVE_RCLIB_LYRIC_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define RCLIB_TYPE_LYRIC (rclib_lyric_get_type())
#define RCLIB_LYRIC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RCLIB_TYPE_LYRIC, RCLibLyric))
#define RCLIB_LYRIC_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RCLIB_TYPE_LYRIC, RCLibLyricClass))
#define RCLIB_IS_LYRIC(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RCLIB_TYPE_LYRIC))
#define RCLIB_IS_LYRIC_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RCLIB_TYPE_LYRIC))
#define RCLIB_LYRIC_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_TYPE_LYRIC, RCLibLyricClass))

typedef struct _RCLibLyricData RCLibLyricData;
typedef struct _RCLibLyricParsedData RCLibLyricParsedData;
typedef struct _RCLibLyric RCLibLyric;
typedef struct _RCLibLyricClass RCLibLyricClass;
typedef struct _RCLibLyricPrivate RCLibLyricPrivate;

/**
 * RCLibLyricData:
 * @time: the time line (unit: nanosecond)
 * @length: the length (unit: nanosecond)
 * @text: the lyric text
 *
 * The structure for lyric data.
 */

struct _RCLibLyricData {
    gint64 time;
    gint64 length;
    gchar *text;
};

/**
 * RCLibLyricParsedData:
 * @seq: the #GSequence which contains the lyric data
 * @filename: the file path of the lyric file
 * @title: the title
 * @artist: the artist
 * @album: the album
 * @author: the author of the lyric file
 * @offset: the offset time of the lyric data
 *
 * The structure for parsed lyric data.
 */

struct _RCLibLyricParsedData {
    GSequence *seq;
    gchar *filename;
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *author;
    gint offset;
};

/**
 * RCLibLyric:
 *
 * The lyric processor. The contents of the #RCLibLyric structure are
 * private and should only be accessed via the provided API.
 */

struct _RCLibLyric {
    /*< private >*/
    GObject parent;
    RCLibLyricPrivate *priv;
};

/**
 * RCLibLyricClass:
 *
 * #RCLibLyric class.
 */

struct _RCLibLyricClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*lyric_ready)(RCLibLyric *lyric, guint index);
    void (*line_changed)(RCLibLyric *lyric, guint index,
        const RCLibLyricData *lyric_data, gint64 offset);
    void (*lyric_timer)(RCLibLyric *lyric, guint index, gint64 pos,
        const RCLibLyricData *lyric_data, gint64 offset);
    void (*lyric_may_missing)(RCLibLyric *lyric);
};

/*< private >*/
GType rclib_lyric_get_type();

/*< public >*/
void rclib_lyric_init();
void rclib_lyric_exit();
GObject *rclib_lyric_get_instance();
gulong rclib_lyric_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_lyric_signal_disconnect(gulong handler_id);
void rclib_lyric_set_fallback_encoding(const gchar *encoding);
const gchar *rclib_lyric_get_fallback_encoding();
gboolean rclib_lyric_load_file(const gchar *filename, guint index);
void rclib_lyric_clean(guint index);
const RCLibLyricData *rclib_lyric_get_line(guint index, gint64 time);
GSequenceIter *rclib_lyric_get_line_iter(guint index, gint64 time);
void rclib_lyric_set_search_dir(const gchar *dir);
const gchar *rclib_lyric_get_search_dir();
gchar *rclib_lyric_search_lyric(const gchar *uri, const gchar *title,
    const gchar *artist);
gboolean rclib_lyric_is_available(guint index);
const RCLibLyricParsedData *rclib_lyric_get_parsed_data(guint index);
gint64 rclib_lyric_get_track_time_offset(guint index);

G_END_DECLS

#endif

