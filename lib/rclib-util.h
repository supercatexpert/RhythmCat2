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

gchar *rclib_util_get_data_dir(const gchar *name, const gchar *arg0);
gboolean rclib_util_is_supported_media(const gchar *file);
gboolean rclib_util_is_supported_list(const gchar *file);
void rclib_util_set_cover_search_dir(const gchar *dir);
const gchar *rclib_util_get_cover_search_dir();
gchar *rclib_util_search_cover(const gchar *uri, const gchar *title,
    const gchar *artist, const gchar *album);
gchar *rclib_util_detect_encoding_by_locale();

G_END_DECLS

#endif

