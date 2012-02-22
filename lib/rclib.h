/*
 * RhythmCat Library Main Header Declaration
 *
 * rclib.h
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

#ifndef HAVE_RC_LIB_H
#define HAVE_RC_LIB_H

#include <glib.h>
#include <gst/gst.h>
#include "rclib-core.h"
#include "rclib-db.h"
#include "rclib-tag.h"
#include "rclib-cue.h"
#include "rclib-player.h"
#include "rclib-util.h"
#include "rclib-lyric.h"
#include "rclib-settings.h"
#include "rclib-dbus.h"

G_BEGIN_DECLS

/**
 * rclib_major_version:
 *
 * The major version of LibRhythmCat.
 */
extern const guint rclib_major_version;

/**
 * rclib_minor_version:
 *
 * The minor version of LibRhythmCat.
 */
extern const guint rclib_minor_version;

/**
 * rclib_micro_version:
 *
 * The micro version of LibRhythmCat.
 */
extern const guint rclib_micro_version;

/**
 * rclib_build_date:
 *
 * The build (complied) date of LibRhythmCat.
 */
extern const gchar *rclib_build_date;

/**
 * rclib_build_time:
 *
 * The build (complied) time of LibRhythmCat.
 */
extern const gchar *rclib_build_time;

gboolean rclib_init(gint *argc, gchar **argv[], const gchar *dir,
    GError **error);
void rclib_exit();

G_END_DECLS

#endif

