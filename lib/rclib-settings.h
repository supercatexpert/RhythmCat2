/*
 * RhythmCat Library Settings Header Declaration
 *
 * rclib-settings.h
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

#ifndef HAVE_RCLIB_SETTINGS_H
#define HAVE_RCLIB_SETTINGS_H

#include <glib.h>
#include <gst/gst.h>

G_BEGIN_DECLS

gboolean rclib_settings_init();
void rclib_settings_exit();
gboolean rclib_settings_load_from_file(const gchar *filename);
void rclib_settings_apply();
void rclib_settings_update();
void rclib_settings_sync();
gchar *rclib_settings_get_string(const gchar *group_name,
    const gchar *key, GError **error);
gint rclib_settings_get_integer(const gchar *group_name,
    const gchar *key, GError **error);
gdouble rclib_settings_get_double(const gchar *group_name,
    const gchar *key, GError **error);
gboolean rclib_settings_get_boolean(const gchar *group_name,
    const gchar *key, GError **error);
gchar **rclib_settings_get_string_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error);
gboolean *rclib_settings_get_boolean_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error);
gint *rclib_settings_get_integer_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error);
gdouble *rclib_settings_get_double_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error);
void rclib_settings_set_string(const gchar *group_name,
    const gchar *key, const gchar *string);
void rclib_settings_set_boolean(const gchar *group_name,
    const gchar *key, gboolean value);
void rclib_settings_set_integer(const gchar *group_name,
    const gchar *key, gint value);
void rclib_settings_set_double(const gchar *group_name,
    const gchar *key, gdouble value);
void rclib_settings_set_string_list(const gchar *group_name,
    const gchar *key, const gchar * const list[], gsize length);
void rclib_settings_set_boolean_list(const gchar *group_name,
    const gchar *key, gboolean list[], gsize length);
void rclib_settings_set_integer_list(const gchar *group_name,
    const gchar *key, gint list[], gsize length);
void rclib_settings_set_double_list(const gchar *group_name,
    const gchar *key, gdouble list[], gsize length);
gboolean rclib_settings_has_key(const gchar *group_name, gchar *key,
    GError **error);

G_END_DECLS

#endif

