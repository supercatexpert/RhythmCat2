/*
 * RhythmCat Library Core Header Declaration
 *
 * rclib-core.h
 * This file is part of RhythmCat Library (LibRhythmCat)
 *
 * Copyright (C) 2011 - SuperCat, license: GPL v3
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

#ifndef HAVE_RC_LIB_CORE_H
#define HAVE_RC_LIB_CORE_H

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>
#include <gst/fft/gstfftf32.h>

G_BEGIN_DECLS

#define RCLIB_CORE_TYPE (rclib_core_get_type())
#define RCLIB_CORE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), RCLIB_CORE_TYPE, \
    RCLibCore))
#define RCLIB_CORE_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), RCLIB_CORE_TYPE, \
    RCLibCoreClass))
#define RCLIB_IS_CORE(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), RCLIB_CORE_TYPE))
#define RCLIB_IS_CORE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), RCLIB_CORE_TYPE))
#define RCLIB_CORE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_CORE_TYPE, RCLibCoreClass))

/**
 * RCLibCoreSourceType:
 * @RCLIB_CORE_SOURCE_NONE: the source is empty
 * @RCLIB_CORE_SOURCE_NORMAL: the source is a normal audio file
 * @RCLIB_CORE_SOURCE_CUE: the source is from CUE sheet file
 * @RCLIB_CORE_SOURCE_EMBEDED_CUE: the source is from embeded CUE sheet
 *
 * The enum type for the source type.
 */

typedef enum {
    RCLIB_CORE_SOURCE_NONE = 0,
    RCLIB_CORE_SOURCE_NORMAL = 1,
    RCLIB_CORE_SOURCE_CUE = 2,
    RCLIB_CORE_SOURCE_EMBEDED_CUE = 3
}RCLibCoreSourceType;

/**
 * RCLibCoreErrorCode:
 * @RCLIB_CORE_ERROR_OK: no error
 * @RCLIB_CORE_ERROR_ALREADY_INIT: the core is already initialized
 * @RCLIB_CORE_ERROR_NOT_INIT: the core is not initialized yet
 * @RCLIB_CORE_ERROR_MISSING_CORE_PLUGIN: necessary plug-ins are missing
 * @RCLIB_CORE_ERROR_CREATE_BIN_FAILED: cannot create necessary #GstBin
 * @RCLIB_CORE_ERROR_LINK_FAILED: cannot link necessary #GstElement
 *
 * The enum type for core error messages.
 */

typedef enum {
    RCLIB_CORE_ERROR_OK = 0,
    RCLIB_CORE_ERROR_ALREADY_INIT = 1,
    RCLIB_CORE_ERROR_NOT_INIT = 2,
    RCLIB_CORE_ERROR_MISSING_CORE_PLUGIN = 3,
    RCLIB_CORE_ERROR_CREATE_BIN_FAILED = 4,
    RCLIB_CORE_ERROR_LINK_FAILED = 5
}RCLibCoreErrorCode;

/**
 * RCLibCoreEQType:
 * @RCLIB_CORE_EQ_TYPE_NONE: no equalizer effect
 * @RCLIB_CORE_EQ_TYPE_POP: the pop style
 * @RCLIB_CORE_EQ_TYPE_ROCK: the rock style
 * @RCLIB_CORE_EQ_TYPE_METAL: the metal style
 * @RCLIB_CORE_EQ_TYPE_DANCE: the dance style
 * @RCLIB_CORE_EQ_TYPE_ELECTRONIC: the electronic style
 * @RCLIB_CORE_EQ_TYPE_JAZZ: the jazz style
 * @RCLIB_CORE_EQ_TYPE_CLASSICAL: the classical style
 * @RCLIB_CORE_EQ_TYPE_BLUES: the blues style
 * @RCLIB_CORE_EQ_TYPE_VOCAL: the vocal style
 * @RCLIB_CORE_EQ_TYPE_CUSTOM: the custom style
 *
 * The enum type for the equalizer in the core.
 */

typedef enum {
    RCLIB_CORE_EQ_TYPE_NONE = 0,
    RCLIB_CORE_EQ_TYPE_POP = 1,
    RCLIB_CORE_EQ_TYPE_ROCK = 2,
    RCLIB_CORE_EQ_TYPE_METAL = 3,
    RCLIB_CORE_EQ_TYPE_DANCE = 4,
    RCLIB_CORE_EQ_TYPE_ELECTRONIC = 5,
    RCLIB_CORE_EQ_TYPE_JAZZ = 6,
    RCLIB_CORE_EQ_TYPE_CLASSICAL = 7,
    RCLIB_CORE_EQ_TYPE_BLUES = 8,
    RCLIB_CORE_EQ_TYPE_VOCAL = 9,
    RCLIB_CORE_EQ_TYPE_CUSTOM = 10
}RCLibCoreEQType;

typedef struct _RCLibCoreMetadata RCLibCoreMetadata;
typedef struct _RCLibCore RCLibCore;
typedef struct _RCLibCoreClass RCLibCoreClass;

/**
 * RCLibCoreMetadata:
 * @title: the title
 * @artist: the artist
 * @album: the album name
 * @ftype: the format information
 * @bitrate: the bitrate (unit: bit/s)
 * @duration: the duration of the music (unit: nanosecond)
 * @track: the track number
 * @year: the year
 * @rate: the sample rate
 * @channels: the number of channel
 * @image: the cover image buffer
 *
 * The structure for metadata read from core.
 */

struct _RCLibCoreMetadata {
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *ftype;
    guint bitrate;
    gint64 duration;
    gint track;
    gint year;
    gint rate;
    gint channels;
    GstBuffer *image;
};

/**
 * RCLibCore:
 *
 * The audio player core. The contents of the #RCLibCore structure are
 * private and should only be accessed via the provided API.
 */

struct _RCLibCore {
    /*< private >*/
    GObject parent;
};

/**
 * RCLibCoreClass:
 *
 * #RCLibCore class.
 */

struct _RCLibCoreClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*state_changed)(RCLibCore *core, GstState state);
    void (*eos)(RCLibCore *core);
    void (*tag_found)(RCLibCore *core, const RCLibCoreMetadata *metadata,
        const gchar *uri);
    void (*new_duration)(RCLibCore *core, gint64 duration);
    void (*uri_changed)(RCLibCore *core, const gchar *uri);
    void (*volume_changed)(RCLibCore *core, gdouble volume);
    void (*eq_changed)(RCLibCore *core, RCLibCoreEQType type,
        gdouble *values);
    void (*balance_changed)(RCLibCore *core, gfloat balance);
    void (*echo_changed)(RCLibCore *core);
    void (*spectrum_updated)(RCLibCore *core, guint rate, guint bands,
        const GValue *magnitudes);
    void (*error)(RCLibCore *core);
};

/*< private >*/
GType rclib_core_get_type();

/*< public >*/
gboolean rclib_core_init(gint *argc, gchar **argv[], GError **error);
void rclib_core_exit();
GObject *rclib_core_get_instance();
gulong rclib_core_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_core_signal_disconnect(gulong handler_id);
void rclib_core_set_uri(const gchar *uri, GSequenceIter *db_reference,
    gpointer external_reference);
void rclib_core_update_db_reference(GSequenceIter *new_ref);
void rclib_core_update_external_reference(gpointer new_ref);
gchar *rclib_core_get_uri();
GSequenceIter *rclib_core_get_db_reference();
gpointer rclib_core_get_external_reference();
RCLibCoreSourceType rclib_core_get_source_type();
const RCLibCoreMetadata *rclib_core_get_metadata();
gboolean rclib_core_set_position(gint64 pos);
gint64 rclib_core_query_position();
gint64 rclib_core_query_duration();
GstStateChangeReturn rclib_core_get_state(GstState *state,
    GstState *pending, GstClockTime timeout);
gboolean rclib_core_play();
gboolean rclib_core_pause();
gboolean rclib_core_stop();
gboolean rclib_core_set_volume(gdouble volume);
gboolean rclib_core_get_volume(gdouble *volume);
gboolean rclib_core_set_eq(RCLibCoreEQType type, gdouble *band);
gboolean rclib_core_get_eq(RCLibCoreEQType *type, gdouble *band);
const gchar *rclib_core_get_eq_name(RCLibCoreEQType type);
gboolean rclib_core_set_balance(gfloat balance);
gboolean rclib_core_get_balance(gfloat *balance);
gboolean rclib_core_set_echo(guint64 delay, guint64 max_delay,
    gfloat feedback, gfloat intensity);
gboolean rclib_core_get_echo(guint64 *delay, guint64 *max_delay,
    gfloat *feedback, gfloat *intensity);
gboolean rclib_core_effect_plugin_add(GstElement *element);
void rclib_core_effect_plugin_remove(GstElement *element);
GList *rclib_core_effect_plugin_get_list();

G_END_DECLS

#endif


