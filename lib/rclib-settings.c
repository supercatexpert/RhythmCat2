/*
 * RhythmCat Library Settings Module
 * Manage the settings in the player.
 *
 * rclib-settings.c
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

#include "rclib-settings.h"
#include "rclib-common.h"
#include "rclib-core.h"
#include "rclib-player.h"
#include "rclib-util.h"
#include "rclib-lyric.h"
#include "rclib-tag.h"
#include "rclib-db.h"

/**
 * SECTION: rclib-settings
 * @Short_description: Manage the settings
 * @Title: Settings
 * @Include: rclib-settings.h
 *
 * Manage the settings in the player. The settings can be used in this
 * library or the frontend UI. This module uses #GKeyFile to store
 * settings in .INI file style.
 *
 * Notice: The settings are only used when the application is starting,
 * or quiting, for load/save the settings. If you want to read or apply
 * settings, you should check if the settings can be applied directly,
 * if the settings are only used when the application is starting, then
 * save them in this module.
 */

static GKeyFile *settings_keyfile = NULL;
static gchar *settings_path = NULL;
static gboolean settings_dirty = FALSE;

static gboolean rclib_settings_load(const gchar *filename)
{
    GError *error = NULL;
    if(settings_keyfile==NULL || filename==NULL) return FALSE;
    if(g_key_file_load_from_file(settings_keyfile, filename,
        G_KEY_FILE_NONE, &error))
    {
        settings_dirty = FALSE;
        g_message("Settings file has been loaded.");
        return TRUE;
    }
    else
    {
        g_warning("Settings file cannot be loaded: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
}

static void rclib_settings_save(const gchar *filename)
{
    GError *error = NULL;
    gchar *settings_data = NULL;
    gsize settings_data_length;
    if(settings_keyfile==NULL || filename==NULL) return;
    settings_data = g_key_file_to_data(settings_keyfile,
        &settings_data_length, NULL);
    if(g_file_set_contents(filename, settings_data, settings_data_length,
        &error))
    {
        g_message("Settings file has been saved.");
    }
    else
    {
        g_warning("Settings file cannot be saved: %s", error->message);
        g_error_free(error);
    }
    g_free(settings_data);
    settings_dirty = FALSE;
}

/**
 * rclib_settings_init:
 *
 * Initialize the settings module.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rclib_settings_init()
{
    gdouble eq_array[10] = {0.0};
    if(settings_keyfile!=NULL)
    {
        g_warning("Settings module is already loaded!");
        return FALSE;
    }
    g_message("Loading settings module....");
    settings_keyfile = g_key_file_new();
    if(settings_keyfile==NULL) return FALSE;
    rclib_settings_set_boolean("Player", "AutoPlay", FALSE);
    rclib_settings_set_integer("Player", "RepeatMode",
        RCLIB_PLAYER_REPEAT_ALL);
    rclib_settings_set_integer("Player", "RandomMode",
        RCLIB_PLAYER_RANDOM_NONE);
    rclib_settings_set_double("Player", "Volume", 1.0);
    rclib_settings_set_boolean("Player", "AutoPlayWhenStartup", FALSE);
    rclib_settings_set_boolean("Player", "LoadLastPosition", FALSE);
    rclib_settings_set_integer("Player", "LastPlayedCatalog", 0);
    rclib_settings_set_integer("Player", "LastPlayedMusic", 0);
    rclib_settings_set_integer("SoundEffect", "EQStyle",
        RCLIB_CORE_EQ_TYPE_NONE);
    rclib_settings_set_double_list("SoundEffect", "EQ", eq_array, 10);
    rclib_settings_set_double("SoundEffect", "Balance", 0.0);
    rclib_settings_set_boolean("Playlist", "AutoEncodingDetect", TRUE);
    rclib_settings_set_boolean("Metadata", "AutoDetectEncoding", TRUE);
    settings_dirty = FALSE;
    g_message("Settings module loaded.");
    return TRUE;
}

/**
 * rclib_settings_exit:
 *
 * Unload the setings module and save the settings to the settings file.
 */

void rclib_settings_exit()
{
    if(settings_path!=NULL)
    {
        rclib_settings_update();
        rclib_settings_save(settings_path);
        g_free(settings_path);
    }
    if(settings_keyfile!=NULL)
        g_key_file_free(settings_keyfile);
    settings_keyfile = NULL;
    g_message("Settings module exited.");
}

/**
 * rclib_settings_load_from_file:
 * @filename: the path of the settings file
 *
 * Load settings from file.
 *
 * Returns: Whether the settings are loaded successfully.
 */

gboolean rclib_settings_load_from_file(const gchar *filename)
{
    gboolean flag;
    if(filename==NULL || settings_keyfile==NULL) return FALSE;
    g_free(settings_path);
    settings_path = g_strdup(filename);
    flag = rclib_settings_load(filename);
    return flag;
}

/**
 * rclib_settings_apply:
 *
 * Apply the settings to the modules in library.
 */

void rclib_settings_apply()
{
    GError *error = NULL;
    gboolean bvalue;
    gint ivalue;
    gdouble dvalue;
    gdouble *darray;
    gsize size;
    guint64 delay;
    gfloat intensity, feedback;
    gchar *encoding, *id3_encoding;
    ivalue = rclib_settings_get_integer("Player", "RepeatMode", &error);
    if(error==NULL)
    {
        rclib_player_set_repeat_mode(ivalue);
    }   
    else
    {
        g_error_free(error);
        error = NULL;
    }
    ivalue = rclib_settings_get_integer("Player", "RandomMode", &error);
    if(error==NULL)
    {
        rclib_player_set_random_mode(ivalue);
    }   
    else
    {
        g_error_free(error);
        error = NULL;
    }
    dvalue = rclib_settings_get_double("Player", "Volume", &error);
    if(error==NULL)
        rclib_core_set_volume(dvalue);
    else
    {
        g_error_free(error);
        error = NULL;
    }
    ivalue = rclib_settings_get_integer("SoundEffect", "EQStyle", &error);
    if(error==NULL)
    {
        if(ivalue>=RCLIB_CORE_EQ_TYPE_NONE && ivalue<RCLIB_CORE_EQ_TYPE_CUSTOM)
            rclib_core_set_eq(ivalue, NULL);
        else
        {
            darray = rclib_settings_get_double_list("SoundEffect", "EQ",
                &size, NULL);
            if(darray!=NULL)
            {
                rclib_core_set_eq(RCLIB_CORE_EQ_TYPE_CUSTOM, darray);
            }
            g_free(darray);
        }
    }   
    else
    {
        g_error_free(error);
        error = NULL;
    }
    dvalue = rclib_settings_get_double("SoundEffect", "Balance", NULL);
    rclib_core_set_balance(dvalue);
    delay = rclib_settings_get_integer("SoundEffect", "EchoDelay", NULL);
    delay *= GST_MSECOND;
    if(delay==0) delay = 1;
    feedback = rclib_settings_get_double("SoundEffect", "EchoFeedback", NULL);
    intensity = rclib_settings_get_double("SoundEffect", "EchoIntensity", NULL);
    rclib_core_set_echo(delay, feedback, intensity);
    bvalue = rclib_settings_get_boolean("Metadata", "AutoDetectEncoding", NULL);
    if(bvalue)
    {
        encoding = rclib_util_detect_encoding_by_locale();
        if(encoding!=NULL && strlen(encoding)>0)
        {
            id3_encoding = g_strdup_printf("%s:UTF-8", encoding);
            rclib_lyric_set_fallback_encoding(encoding);
            rclib_settings_set_string("Metadata", "LyricEncoding", encoding);
            rclib_tag_set_fallback_encoding(id3_encoding);
            rclib_settings_set_string("Metadata", "ID3Encoding",
                id3_encoding);
            g_free(id3_encoding);
        }
        if(encoding!=NULL)
            g_free(encoding);
        else
        {
            id3_encoding = rclib_settings_get_string("Metadata", "ID3Encoding",
                NULL);
            if(id3_encoding!=NULL && strlen(id3_encoding)>0)
                rclib_tag_set_fallback_encoding(id3_encoding);
            g_free(id3_encoding);
            encoding = rclib_settings_get_string("Metadata", "LyricEncoding",
                NULL);
            if(encoding!=NULL && strlen(encoding)>0)
                rclib_lyric_set_fallback_encoding(encoding);
            g_free(encoding);
        }
    }
    else
    {
        id3_encoding = rclib_settings_get_string("Metadata", "ID3Encoding",
            NULL);
        if(id3_encoding!=NULL && strlen(id3_encoding)>0)
            rclib_tag_set_fallback_encoding(id3_encoding);
        g_free(id3_encoding);
        encoding = rclib_settings_get_string("Metadata", "LyricEncoding",
            NULL);
        if(encoding!=NULL && strlen(encoding)>0)
            rclib_lyric_set_fallback_encoding(encoding);
        g_free(encoding);
    }
}

/**
 * rclib_settings_update:
 *
 * Update settings from the modules in the library.
 */
 
void rclib_settings_update()
{
    gint ivalue;
    gdouble dvalue;
    gdouble eq_array[10] = {0.0};
    guint64 delay;
    gfloat fvalue;
    gfloat intensity, feedback;
    GSequenceIter *db_reference;
    RCLibDbPlaylistData *playlist_data;
    ivalue = rclib_player_get_repeat_mode();
    rclib_settings_set_integer("Player", "RepeatMode", ivalue);
    ivalue = rclib_player_get_random_mode();
    rclib_settings_set_integer("Player", "RandomMode", ivalue);
    if(rclib_core_get_volume(&dvalue))
        rclib_settings_set_double("Player", "Volume", dvalue);
    if(rclib_core_get_eq((RCLibCoreEQType *)&ivalue, eq_array))
    {
        rclib_settings_set_integer("SoundEffect", "EQStyle", ivalue);
        rclib_settings_set_double_list("SoundEffect", "EQ", eq_array, 10);
    }
    if(rclib_core_get_balance(&fvalue))
        rclib_settings_set_double("SoundEffect", "Balance", fvalue);
    if(rclib_core_get_echo(&delay, NULL, &feedback, &intensity))
    {
        rclib_settings_set_integer("SoundEffect", "EchoDelay",
            delay/GST_MSECOND);
        rclib_settings_set_double("SoundEffect", "EchoFeedback", feedback);
        rclib_settings_set_double("SoundEffect", "EchoIntensity", intensity);
    }
    db_reference = rclib_core_get_db_reference();
    if(db_reference!=NULL)
    {
        playlist_data = g_sequence_get(db_reference);
        if(playlist_data!=NULL)
        {
            ivalue = g_sequence_iter_get_position(db_reference);
            rclib_settings_set_integer("Player", "LastPlayedMusic", ivalue);
            ivalue = g_sequence_iter_get_position(playlist_data->catalog);
            rclib_settings_set_integer("Player", "LastPlayedCatalog",
                ivalue);
        }
    }
}

/**
 * rclib_settings_sync:
 *
 * Save the settings to file if they are changed (become dirty).
 *
 */

void rclib_settings_sync()
{
    if(settings_path==NULL) return;
    if(!settings_dirty) return;
    rclib_settings_save(settings_path);
}

/**
 * rclib_settings_get_string:
 * @group_name: a group name
 * @key: a key
 * @error: return location for a GError, or NULL
 *
 * Returns the string value associated with key under group_name.
 *
 * Returns: A newly allocated string or NULL if the specified key cannot
 * be found.
 */

gchar *rclib_settings_get_string(const gchar *group_name,
    const gchar *key, GError **error)
{
    if(settings_keyfile==NULL) return NULL;
    return g_key_file_get_string(settings_keyfile, group_name, key, error);
}

/**
 * rclib_settings_get_integer:
 * @group_name: a group name
 * @key: a key
 * @error: return location for a GError, or NULL
 *
 * Returns the value associated with key under group_name as an integer.
 *
 * Returns: The value associated with the key as an integer, or 0
 * if the key was not found or could not be parsed.
 */

gint rclib_settings_get_integer(const gchar *group_name,
    const gchar *key, GError **error)
{
    if(settings_keyfile==NULL) return 0;
    return g_key_file_get_integer(settings_keyfile, group_name, key, error);
}

/**
 * rclib_settings_get_double:
 * @group_name: a group name
 * @key: a key
 * @error: return location for a GError, or NULL
 *
 * Returns the value associated with key under group_name as a double.
 * If group_name is NULL, the start_group is used.
 *
 * Returns: The value associated with the key as a double, or 0.0
 * if the key was not found or could not be parsed.
 */

gdouble rclib_settings_get_double(const gchar *group_name,
    const gchar *key, GError **error)
{
    if(settings_keyfile==NULL) return 0.0;
    return g_key_file_get_double(settings_keyfile, group_name, key, error);
}

/**
 * rclib_settings_get_boolean:
 * @group_name: a group name
 * @key: a key
 * @error: return location for a GError, or NULL
 *
 * Returns the value associated with key under group_name as a boolean.
 *
 * Returns: The value associated with the key as a boolean, or FALSE if the
 * key was not found or could not be parsed.
 */

gboolean rclib_settings_get_boolean(const gchar *group_name,
    const gchar *key, GError **error)
{
    if(settings_keyfile==NULL) return FALSE;
    return g_key_file_get_boolean(settings_keyfile, group_name, key, error);
}

/**
 * rclib_settings_get_string_list:
 * @group_name: a group name
 * @key: a key
 * @length: return location for the number of returned strings, or NULL
 * @error: return location for a GError, or NULL
 *
 * Returns the values associated with key under group_name.
 *
 * Returns: A NULL-terminated string array or NULL if the specified key
 * cannot be found. The array should be freed with g_strfreev().
 */

gchar **rclib_settings_get_string_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error)
{
    if(settings_keyfile==NULL) return NULL;
    return g_key_file_get_string_list(settings_keyfile, group_name, key,
        length, error);
}

/**
 * rclib_settings_get_boolean_list:
 * @group_name: a group name
 * @key: a key
 * @length: the number of booleans returned
 * @error: return location for a GError, or NULL
 *
 * Returns the values associated with key under group_name as booleans.
 *
 * Returns: The values associated with the key as a list of booleans, or
 * NULL if the key was not found or could not be parsed.
 */

gboolean *rclib_settings_get_boolean_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error)
{
    if(settings_keyfile==NULL) return NULL;
    return g_key_file_get_boolean_list(settings_keyfile, group_name, key,
        length, error);
}

/**
 * rclib_settings_get_integer_list:
 * @group_name: a group name
 * @key: a key
 * @length: the number of integers returned
 * @error: return location for a GError, or NULL
 *
 * Returns the values associated with key under group_name as integers.
 *
 * Returns: The values associated with the key as a list of integers, or
 * NULL if the key was not found or could not be parsed.
 */

gint *rclib_settings_get_integer_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error)
{
    if(settings_keyfile==NULL) return NULL;
    return g_key_file_get_integer_list(settings_keyfile, group_name, key,
        length, error);
}

/**
 * rclib_settings_get_double_list:
 * @group_name: a group name
 * @key: a key
 * @length: the number of doubles returned
 * @error: return location for a GError, or NULL
 *
 * Returns the values associated with key under group_name as doubles.
 *
 * Returns: The values associated with the key as a list of doubles, or
 * NULL if the key was not found or could not be parsed.
 */

gdouble *rclib_settings_get_double_list(const gchar *group_name,
    const gchar *key, gsize *length, GError **error)
{
    if(settings_keyfile==NULL) return NULL;
    return g_key_file_get_double_list(settings_keyfile, group_name, key,
        length, error);
}

/**
 * rclib_settings_set_string:
 * @group_name: a group name
 * @key: a key
 * @string: a string
 *
 * Associates a new string value with key under group_name. If key cannot
 * be found then it is created. If group_name cannot be found then it is
 * created.
 */

void rclib_settings_set_string(const gchar *group_name,
    const gchar *key, const gchar *string)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_string(settings_keyfile, group_name, key, string);
}

/**
 * rclib_settings_set_boolean:
 * @group_name: a group name
 * @key: a key
 * @value: TRUE or FALSE
 *
 * Associates a new boolean value with key under group_name. If key cannot
 * be found then it is created. If group_name cannot be found then it is
 * created.
 */

void rclib_settings_set_boolean(const gchar *group_name,
    const gchar *key, gboolean value)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_boolean(settings_keyfile, group_name, key, value);
}

/**
 * rclib_settings_set_integer:
 * @group_name: a group name
 * @key: a key
 * @value: an integer value
 *
 * Associates a new integer value with key under group_name. If key cannot
 * be found then it is created. If group_name cannot be found then it is
 * created.
 */

void rclib_settings_set_integer(const gchar *group_name,
    const gchar *key, gint value)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_integer(settings_keyfile, group_name, key, value);
}

/**
 * rclib_settings_set_double:
 * @group_name: a group name
 * @key: a key
 * @value: an double value
 *
 * Associates a new double value with key under group_name. If key cannot
 * be found then it is created. If group_name cannot be found then it is
 * created.
 */

void rclib_settings_set_double(const gchar *group_name,
    const gchar *key, gdouble value)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_double(settings_keyfile, group_name, key, value);
}

/**
 * rclib_settings_set_string_list:
 * @group_name: a group name
 * @key: a key
 * @list: an array of string values
 * @length: number of string values in list
 *
 * Associates a list of string values for key under group_name. If key
 * cannot be found then it is created. If group_name cannot be found then
 * it is created.
 */

void rclib_settings_set_string_list(const gchar *group_name,
    const gchar *key, const gchar * const list[], gsize length)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_string_list(settings_keyfile, group_name, key,
        list, length);
}

/**
 * rclib_settings_set_boolean_list:
 * @group_name: a group name
 * @key: a key
 * @list: an array of boolean values
 * @length: number of string values in list
 *
 * Associates a list of boolean values with key under group_name. If key
 * cannot be found then it is created. If group_name is NULL, the
 * start_group is used.
 */

void rclib_settings_set_boolean_list(const gchar *group_name,
    const gchar *key, gboolean list[], gsize length)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_boolean_list(settings_keyfile, group_name, key,
        list, length);
}

/**
 * rclib_settings_set_integer_list:
 * @group_name: a group name
 * @key: a key
 * @list: an array of integer values
 * @length: number of integer values in list
 *
 * Associates a list of integer values with key under group_name. If key
 * cannot be found then it is created. If group_name is NULL, the
 * start_group is used.
 */

void rclib_settings_set_integer_list(const gchar *group_name,
    const gchar *key, gint list[], gsize length)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_integer_list(settings_keyfile, group_name, key,
        list, length);
}

/**
 * rclib_settings_set_double_list:
 * @group_name: a group name
 * @key: a key
 * @list: an array of double values
 * @length: number of double values in list
 *
 * Associates a list of double values with key under group_name. If key
 * cannot be found then it is created. If group_name is NULL, the
 * start_group is used.
 */

void rclib_settings_set_double_list(const gchar *group_name,
    const gchar *key, gdouble list[], gsize length)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return;
    g_key_file_set_double_list(settings_keyfile, group_name, key,
        list, length);
}

/**
 * rclib_settings_has_key:
 * @group_name: a group name
 * @key: a key
 * @error: return location for a GError
 *
 * Looks whether the key file has the key in the group.
 *
 * Returns: TRUE if key is a part of group_name, FALSE otherwise.
 */

gboolean rclib_settings_has_key(const gchar *group_name, gchar *key,
    GError **error)
{
    settings_dirty = TRUE;
    if(settings_keyfile==NULL) return FALSE;
    return g_key_file_has_key(settings_keyfile, group_name, key, error);
}

