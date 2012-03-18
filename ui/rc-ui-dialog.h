/*
 * RhythmCat UI Dialogs Header Declaration
 *
 * rc-ui-dialog.h
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

#ifndef HAVE_RC_UI_DIALOG_H
#define HAVE_RC_UI_DIALOG_H

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

void rc_ui_dialog_about_player();
void rc_ui_dialog_show_message(GtkMessageType type, const gchar *title,
    const gchar *format, ...);
void rc_ui_dialog_add_music();
void rc_ui_dialog_add_directory();
void rc_ui_dialog_load_playlist();
void rc_ui_dialog_save_playlist();
void rc_ui_dialog_save_all_playlist();
void rc_ui_dialog_bind_lyric();
void rc_ui_dialog_bind_album();
void rc_ui_dialog_save_album();
void rc_ui_dialog_open_music();
void rc_ui_dialog_open_location();

G_END_DECLS

#endif

