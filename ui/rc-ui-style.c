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
#include "rc-ui-resources.h"
#include "rc-main.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-style
 * @Short_description: The UI style of the player
 * @Title: Styles
 * @Include: rc-ui-style.h
 *
 * This module provides the configration of the style (theme) of the
 * player. The player can use GTK+ 3 CSS style file for setting the
 * style, there are also some embedded styles in the player.
 */

typedef struct RCUiStyleEmbeddedTheme
{
    const gchar *name;
    const gchar *path;
}RCUiStyleEmbeddedTheme; 

static GtkCssProvider *style_css_provider = NULL;
static RCUiStyleEmbeddedTheme style_embedded_themes[] =
{
    {
        .name = "Monochrome",
        .path = "/org/RhythmCat2/ui/resources/themes/Monochrome/"
            "monochrome.css"
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
 * rc_ui_style_css_set_resource:
 * @resource_path: the CSS file path in the registered #GResource
 *
 * Apply the CSS style file in the registered #GResource to the player.
 *
 * Returns: Whether the operation succeeded.
 */

gboolean rc_ui_style_css_set_resource(const gchar *resource_path)
{
    GFile *file;
    GError *error = NULL;
    gchar *uri, *tmp;
    GdkScreen *screen = gdk_screen_get_default();
    if(resource_path==NULL)
    {
        g_warning("Invalid CSS Style file name!");
        return FALSE;
    }
    tmp = g_uri_escape_string(resource_path,
        G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, FALSE);
    uri = g_strconcat("resource://", tmp, NULL);
    g_free(tmp);
    file = g_file_new_for_uri(uri);
    g_free(uri);
    g_message("Loading CSS Style from resource: %s", resource_path);
    if(file==NULL)
    {
        g_warning("Cannot open CSS Style from resource path: %s",
            resource_path);
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
 * rc_ui_style_embedded_theme_set_by_name:
 * @name: the theme name
 *
 * Use the embedded theme in the player by the given name.
 *
 * Returns: Whether the theme is set successfully.
 */

gboolean rc_ui_style_embedded_theme_set_by_name(const gchar *name)
{
    guint length;
    guint i;
    gboolean flag = FALSE;
    length = sizeof(style_embedded_themes)/sizeof(RCUiStyleEmbeddedTheme);
    for(i=0;i<length;i++)
    {
        if(g_strcmp0(name, style_embedded_themes[i].name)==0)
        {
            flag = rc_ui_style_css_set_resource(
                style_embedded_themes[i].path);
            if(flag) break;
        }
    }
    return flag;
}

/**
 * rc_ui_style_embedded_theme_set_by_index:
 * @index: the theme index
 *
 * Use the embedded theme in the player by the given index number.
 *
 * Returns: Whether the theme is set successfully.
 */

gboolean rc_ui_style_embedded_theme_set_by_index(guint index)
{
    guint length;
    length = sizeof(style_embedded_themes)/sizeof(RCUiStyleEmbeddedTheme);
    if(index>=length) return FALSE;
    return rc_ui_style_css_set_resource(style_embedded_themes[index].path);
}

/**
 * rc_ui_style_embedded_theme_set_default:
 *
 * Use the default embedded theme in the player.
 *
 * Returns: Whether the theme is set successfully.
 */

gboolean rc_ui_style_embedded_theme_set_default()
{
    return rc_ui_style_css_set_resource(style_embedded_themes[0].path);
}

/**
 * rc_ui_style_embedded_theme_get_length:
 *
 * Get the number of the embedded themes in the player.
 *
 * Returns: The number of the embedded themes.
 */

guint rc_ui_style_embedded_theme_get_length()
{
    return sizeof(style_embedded_themes)/sizeof(RCUiStyleEmbeddedTheme);
}

/**
 * rc_ui_style_embedded_theme_get_name:
 * @index: the theme index
 *
 * Get the name of the embedded themes in the player by the given index
 * number.
 *
 * Returns: The name of the embedded themes.
 */

const gchar *rc_ui_style_embedded_theme_get_name(guint index)
{
    guint length;
    length = sizeof(style_embedded_themes)/sizeof(RCUiStyleEmbeddedTheme);
    if(index>=length) return NULL;
    return style_embedded_themes[index].name;
}

/**
 * rc_ui_style_search_theme_paths:
 *
 * Get a list of theme paths, which contains theme files.
 *
 * Returns: (transfer full) (element-type filename): A list of theme paths.
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

