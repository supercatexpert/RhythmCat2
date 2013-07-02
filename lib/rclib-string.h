/*
 * RhythmCat Library String Header Declaration
 *
 * rclib-string.h
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

#ifndef HAVE_RCLIB_STRING_H
#define HAVE_RCLIB_STRING_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * RCLibString:
 * 
 * The structure for reference countable string.
 */

typedef struct _RCLibString RCLibString;

/*< private >*/
GType rclib_string_get_type();

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

G_END_DECLS

#endif

