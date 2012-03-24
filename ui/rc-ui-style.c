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
#include "rc-ui-css.h"
#include "rc-main.h"

static GtkCssProvider *style_css_provider = NULL;
static RCUiStyleEmbededTheme style_embeded_themes[] = {
    {
        .name = "Monochrome",
        .data = rc_ui_css_monochrome,
        .length = sizeof(rc_ui_css_monochrome)
    }
};

/**
 * rc_ui_style_css_set_file:
 * @filename: the CSS file path
 *
 * Apply the CSS style file to the player.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rc_ui_style_css_set_file(const gchar *filename)
{
    GFile *file;
    GError *error = NULL;
    GdkScreen *screen = gdk_screen_get_default();
    if(filename==NULL)
    {
        g_warning("Invalid CSS Style file name!");
        return FALSE;
    }
    file = g_file_new_for_path(filename);
    g_message("Loading CSS Style: %s",
        filename);
    if(file==NULL)
    {
        g_warning("Cannot open CSS Style: %s", filename);
        return FALSE;
    }
    if(style_css_provider==NULL)
        style_css_provider = gtk_css_provider_new();
    if(!gtk_css_provider_load_from_file(style_css_provider, file,
        &error))
    {
        g_warning("Cannot open CSS Style: %s", error->message);
        g_error_free(error);
        g_object_unref(file);
        return FALSE;
    }
    g_object_unref(file);
    gtk_style_context_add_provider_for_screen(screen,
        GTK_STYLE_PROVIDER(style_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_reset_widgets(screen);
    g_message("Loaded new CSS Style.");
    return TRUE;
}

/**
 * rc_ui_style_css_set_data:
 * @data: the CSS data in the buffer
 * @length: the length of the data
 *
 * Apply the CSS style data to the player.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rc_ui_style_css_set_data(const gchar *data, gssize length)
{
    GError *error = NULL;
    GdkScreen *screen = gdk_screen_get_default();
    if(data==NULL)
    {
        g_warning("Invalid CSS Style data!");
        return FALSE;
    }
    if(style_css_provider==NULL)
        style_css_provider = gtk_css_provider_new();
    if(!gtk_css_provider_load_from_data(style_css_provider, data, length,
        &error))
    {
        g_warning("Cannot open CSS Style: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
    gtk_style_context_add_provider_for_screen(screen,
        GTK_STYLE_PROVIDER(style_css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_style_context_reset_widgets(screen);
    g_message("Loaded new CSS Style.");
    return TRUE;
}

/**
 * rc_ui_style_css_unset:
 *
 * Remove CSS style used before.
 */

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

/**
 * rc_ui_style_get_embeded_theme:
 * @number: the theme number
 *
 * Get the embeded theme in the player.
 *
 * Returns: An array of embeded theme data.
 */

const RCUiStyleEmbededTheme *rc_ui_style_get_embeded_theme(guint *number)
{
    if(number!=NULL)
        *number = sizeof(style_embeded_themes)/sizeof(RCUiStyleEmbededTheme);
    return style_embeded_themes;
}

/**
 * rc_ui_style_search_theme_paths:
 *
 * Get a list of theme paths, which contains theme files.
 *
 * Returns: A list of theme paths.
 */

GSList *rc_ui_style_search_theme_paths()
{
    GSList *list = NULL;
    gchar *path = NULL;
    GDir *dir = NULL;
    const gchar *path_name = NULL;
    gchar *theme_path = NULL;
    gchar *theme_file = NULL;
    path = g_build_filename(rc_main_get_data_dir(), "themes", NULL);
    dir = g_dir_open(path, 0, NULL);
    if(dir!=NULL)
    {
        path_name = g_dir_read_name(dir);
        while(path_name!=NULL)
        {
            theme_path = g_build_filename(path, path_name, NULL);
            if(g_file_test(theme_path, G_FILE_TEST_IS_DIR))
                list = g_slist_append(list, theme_path);
            else
                g_free(theme_path);
            path_name = g_dir_read_name(dir);
        }
        g_dir_close(dir);
    }
    g_free(path);
    path = g_build_filename(rc_main_get_user_dir(), "Themes", NULL);
    dir = g_dir_open(path, 0, NULL);
    if(dir!=NULL)
    {
        path_name = g_dir_read_name(dir);
        while(path_name!=NULL)
        {
            theme_path = g_build_filename(path, path_name, NULL);
            theme_file = g_build_filename(theme_path, "gtk3.css", NULL);
            if(g_file_test(theme_path, G_FILE_TEST_IS_DIR) &&
                g_file_test(theme_file, G_FILE_TEST_IS_REGULAR))
            {
                list = g_slist_append(list, theme_path);
            }
            else
                g_free(theme_path);
            g_free(theme_file);
            path_name = g_dir_read_name(dir);
        }
        g_dir_close(dir);
    }
    g_free(path);
    return list;
}

