/*
 * RhythmCat UI Style Header Declaration
 *
 * rc-ui-style.h
 * This file is part of RhythmCat Music Player (GTK+ Version)
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

#ifndef HAVE_RC_UI_STYLE_H
#define HAVE_RC_UI_STYLE_H

#include <glib.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

gboolean rc_ui_style_css_set_file(const gchar *filename);
gboolean rc_ui_style_css_set_data(const gchar *data, gssize length);
gboolean rc_ui_style_css_set_resource(const gchar *resource_path);
void rc_ui_style_css_unset();
gboolean rc_ui_style_embedded_theme_set_by_name(const gchar *name);
gboolean rc_ui_style_embedded_theme_set_by_index(guint index);
gboolean rc_ui_style_embedded_theme_set_default();
guint rc_ui_style_embedded_theme_get_length();
const gchar *rc_ui_style_embedded_theme_get_name(guint index);
GSList *rc_ui_style_search_theme_paths();

G_END_DECLS

#endif

