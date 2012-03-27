/*
 * RhythmCat UI Menu Header Declaration
 *
 * rc-ui-menu.h
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

#ifndef HAVE_RC_UI_MENU_H
#define HAVE_RC_UI_MENU_H

#include <glib.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

GtkUIManager *rc_ui_menu_init();
GtkUIManager *rc_ui_menu_get_ui_manager();
void rc_ui_menu_state_refresh();
guint rc_ui_menu_add_menu_action(GtkAction *action, const gchar *path,
    const gchar *name, const gchar *action_name, gboolean top);
void rc_ui_menu_remove_menu_action(GtkAction *action, guint id);

G_END_DECLS

#endif

