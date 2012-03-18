/*
 * RhythmCat UI Style Module
 * Manage the styles and themes used in the player.
 *
 * rc-ui-style.c
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
 
#include "rc-ui-style.h"

static GtkCssProvider *style_css_provider = NULL;

void rc_ui_style_css_set_file(const gchar *filename)
{
    GFile *file;
    GError *error = NULL;
    GdkScreen *screen = gdk_screen_get_default();
    if(filename==NULL)
    {
        g_warning("Invalid CSS Style file name!");
        return;
    }
    file = g_file_new_for_path(filename);
    g_message("Loading CSS Style: %s",
        filename);
    if(file==NULL)
    {
        g_warning("Cannot open CSS Style: %s", filename);
        return;
    }
    if(style_css_provider==NULL)
        style_css_provider = gtk_css_provider_new();
    if(!gtk_css_provider_load_from_file(style_css_provider, file,
        &error))
    {
        g_warning("Cannot open CSS Style: %s", error->message);
        g_error_free(error);
        g_object_unref(file);
        return;
    }
    g_object_unref(file);
    gtk_style_context_add_provider_for_screen(screen,
        GTK_STYLE_PROVIDER(style_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_reset_widgets(screen);
    g_message("Loaded new CSS Style.");
}

void rc_ui_style_css_set_data(const gchar *data, gssize length)
{
    GError *error = NULL;
    GdkScreen *screen = gdk_screen_get_default();
    if(data==NULL)
    {
        g_warning("Invalid CSS Style data!");
        return;
    }
    if(style_css_provider==NULL)
        style_css_provider = gtk_css_provider_new();
    if(!gtk_css_provider_load_from_data(style_css_provider, data, length,
        &error))
    {
        g_warning("Cannot open CSS Style: %s", error->message);
        g_error_free(error);
        return;
    }
    gtk_style_context_add_provider_for_screen(screen,
        GTK_STYLE_PROVIDER(style_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_reset_widgets(screen);
    g_message("Loaded new CSS Style.");
}

void rc_ui_style_css_unset()
{
    GdkScreen *screen = gdk_screen_get_default();
    if(style_css_provider!=NULL)
    {
        gtk_style_context_remove_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(style_css_provider));
        g_object_unref(style_css_provider);
        style_css_provider = NULL;
    }
    gtk_style_context_reset_widgets(screen);
    g_message("Custom CSS Style has been removed.");
}

