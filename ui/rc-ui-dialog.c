/*
 * RhythmCat UI Dialogs Module
 * Show dialogs in the player.
 *
 * ui-dialog.c
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

#include "rc-ui-dialog.h"
#include "rc-ui-listview.h"
#include "rc-ui-window.h"
#include "rc-ui-player.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-dialog
 * @Short_description: Dialogs in the player
 * @Title: Dialogs
 * @Include: rc-ui-dialog.h
 *
 * This module provides the dialogs in the player.
 */

static const gchar *dialog_about_license =
    "RhythmCat is free software; you can redistribute it\n"
    "and/or modify it under the terms of the GNU General\n"
    "Public License as published by the Free Software\n"
    "Foundation; either version 3 of the License, or\n"
    "(at your option) any later version.\n\n"
    "RhythmCat is distributed in the hope that it will be\n"
    "useful, but WITHOUT ANY WARRANTY; without even the\n"
    "implied warranty of MERCHANTABILITY or FITNESS FOR\n"
    "A PARTICULAR PURPOSE.  See the GNU General Public\n"
    "License for more details. \n\n"
    "You should have received a copy of the GNU General\n"
    "Public License along with RhythmCat; if not, write\n"
    "to the Free Software Foundation, Inc., 51 Franklin\n"
    "St, Fifth Floor, Boston, MA  02110-1301  USA.";
static const gchar const *dialog_about_authors[] = {"SuperCat", NULL};
static const gchar const *dialog_about_documenters[] = {"SuperCat", NULL};
static const gchar const *dialog_about_artists[] = {"SuperCat", NULL};

/**
 * rc_ui_dialog_about_player:
 *
 * The about dialog of the player.
 */

void rc_ui_dialog_about_player()
{
    GtkWidget *about_dialog;
    gchar *version;
    about_dialog = gtk_about_dialog_new();
    version = g_strdup_printf("LibRhythmCat %d.%d.%d - build date: %s",
        rclib_major_version, rclib_minor_version, rclib_micro_version,
        rclib_build_date);
    g_object_set(about_dialog, "program-name", "RhythmCat2 Music Player",
        "logo", rc_ui_player_get_icon_image(), "authors",
        dialog_about_authors, "documenters", dialog_about_documenters,
        "artists", dialog_about_artists, "comments",
        _("A music player based on GTK+ 3.0 & GStreamer 0.10"), "website",
        "http://supercat-lab.org", "license-type", GTK_LICENSE_GPL_3_0,
        "license", dialog_about_license, "version", version, NULL);
    g_free(version);
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);  
}

/**
 * rc_ui_dialog_show_message:
 * @type: type of message
 * @title: title of the message
 * @format: printf()-style format string, or NULL, allow-none
 * @...: arguments for @format
 *
 * Show message dialog in the player.
 */

void rc_ui_dialog_show_message(GtkMessageType type, const gchar *title,
    const gchar *format, ...)
{
    GtkWidget *dialog;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    dialog = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, type,
        GTK_BUTTONS_CLOSE, format, arg_ptr);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static gboolean rc_ui_dialog_music_file_filter(const GtkFileFilterInfo *info,
    gpointer data)
{
    return rclib_util_is_supported_media(info->display_name);
}

/**
 * rc_ui_dialog_add_music:
 *
 * Show a music import dialog for importing music files.
 */

void rc_ui_dialog_add_music()
{
    GtkTreeIter iter;
    GtkWidget *file_chooser;
    GtkFileFilter *file_filter1;
    gint result = 0;
    GSList *filelist = NULL;
    const GSList *filelist_foreach = NULL;
    gchar *uri = NULL;
    gchar *dialog_title = NULL;
    const gchar *home_dir;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    file_filter1 = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter1,
        _("All supported music files(*.FLAC;*.OGG;*.MP3;*.WAV;*.WMA...)"));
    gtk_file_filter_add_custom(file_filter1, GTK_FILE_FILTER_DISPLAY_NAME,
        rc_ui_dialog_music_file_filter, NULL, NULL);
    dialog_title = _("Select the music you want to add...");
    file_chooser = gtk_file_chooser_dialog_new(dialog_title, NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser),
        home_dir);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(file_chooser),TRUE);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), file_filter1);
    result = gtk_dialog_run(GTK_DIALOG(file_chooser));
    switch(result)
    {
        case GTK_RESPONSE_ACCEPT:
            filelist = gtk_file_chooser_get_uris(
                GTK_FILE_CHOOSER(file_chooser));
            for(filelist_foreach=filelist;filelist_foreach!=NULL;
                filelist_foreach=g_slist_next(filelist_foreach))
            {
                uri = filelist_foreach->data;
                rclib_db_playlist_add_music((RCLibDbCatalogIter *)
                    iter.user_data, NULL, uri);
                g_free(uri);
            }
            g_slist_free(filelist);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

/**
 * rc_ui_dialog_add_directory:
 *
 * Show a directory import dialog for importing music files from directory.
 */

void rc_ui_dialog_add_directory()
{
    GtkWidget *file_chooser;
    GtkTreeIter iter;
    gint result = 0;
    gchar *directory_path = NULL;
    const gchar *home_dir;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    file_chooser = gtk_file_chooser_dialog_new(
        _("Select the directory you want to import..."),
        NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,
        GTK_RESPONSE_CANCEL, NULL);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser),
        home_dir);
    result = gtk_dialog_run(GTK_DIALOG(file_chooser));
    switch(result)
    {
        case GTK_RESPONSE_ACCEPT:
            directory_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                file_chooser));
            rclib_db_playlist_add_directory((RCLibDbCatalogIter *)
                iter.user_data, NULL, directory_path);
            g_free(directory_path);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

static gboolean rc_ui_dialog_playlist_file_filter(
    const GtkFileFilterInfo *info, gpointer data)
{
    return rclib_util_is_supported_list(info->display_name);
}

/**
 * rc_ui_dialog_load_playlist:
 *
 * Show a playlist import dialog for importing all music files in the
 * playlist file.
 */

void rc_ui_dialog_load_playlist()
{
    GtkTreeIter iter;
    GtkWidget *file_chooser;
    GtkFileFilter *file_filter1;
    gint result = 0;
    gchar *file_name = NULL;
    const gchar *home_dir;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    file_filter1 = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter1,
        _("M3U Playlist(*.M3U)"));
    gtk_file_filter_add_custom(file_filter1, GTK_FILE_FILTER_DISPLAY_NAME,
        rc_ui_dialog_playlist_file_filter, NULL, NULL);
    file_chooser = gtk_file_chooser_dialog_new(_("Load the playlist..."),
        NULL, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser),
        home_dir);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), file_filter1);
    result = gtk_dialog_run(GTK_DIALOG(file_chooser));
    switch(result)
    {
        case GTK_RESPONSE_ACCEPT:
            file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                file_chooser));
            rclib_db_playlist_add_m3u_file(iter.user_data, NULL, file_name);
            g_free(file_name);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

/**
 * rc_ui_dialog_save_playlist:
 *
 * Show a playlist save dialog for exporting the playlist to a
 * playlist file.
 */

void rc_ui_dialog_save_playlist()
{
    GtkWidget *file_chooser;
    GtkFileFilter *file_filter1;
    gint result = 0;
    gchar *file_name = NULL;
    const gchar *home_dir;
    GtkTreeIter iter;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    file_filter1 = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter1,
        _("M3U Playlist(*.M3U)"));
    gtk_file_filter_add_custom(file_filter1, GTK_FILE_FILTER_DISPLAY_NAME,
        rc_ui_dialog_playlist_file_filter, NULL, NULL);
    file_chooser = gtk_file_chooser_dialog_new(_("Save the playlist..."),
        NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser),
        home_dir);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), file_filter1);
    gtk_file_chooser_set_do_overwrite_confirmation(
        GTK_FILE_CHOOSER(file_chooser), TRUE);
    result = gtk_dialog_run(GTK_DIALOG(file_chooser));
    switch(result)
    {
        case GTK_RESPONSE_ACCEPT:
            file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                file_chooser));
            rclib_db_playlist_export_m3u_file(iter.user_data, file_name);
            g_free(file_name);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

/**
 * rc_ui_dialog_save_all_playlist:
 *
 * Show a playlist export dialog for exporting all playlists in the player
 * to playlist files, then put these files into the given directory.
 */

void rc_ui_dialog_save_all_playlist()
{
    GtkWidget *file_chooser;
    gint result = 0;
    const gchar *home_dir;
    gchar *directory_name = NULL;
    file_chooser = gtk_file_chooser_dialog_new(
        _("Select the directory you want to store the playlists..."),
        NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_SAVE,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser),
        home_dir);
    result = gtk_dialog_run(GTK_DIALOG(file_chooser));
    switch(result)
    {
        case GTK_RESPONSE_ACCEPT:
            directory_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                file_chooser));
            rclib_db_playlist_export_all_m3u_files(directory_name);
            g_free(directory_name);        
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}



/**
 * rc_ui_dialog_bind_lyric:
 *
 * Show a dialog to set the lyric binding of a music item.
 */

void rc_ui_dialog_bind_lyric()
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *main_grid;
    GtkWidget *frame1, *frame2;
    GtkWidget *grid1, *grid2;
    GtkWidget *radio_buttons[4];
    GtkWidget *filebutton[2];
    GtkTreeIter iter;
    gint ret;
    gint i;
    const gchar *home_dir;
    const gchar *lyric_file1 = NULL;
    const gchar *lyric_file2 = NULL;
    gchar *new_file;
    GtkFileFilter *file_filter;
    GtkTreePath *path;
    GtkTreeModel *model;
    GtkTreeRowReference *reference;
    if(!rc_ui_listview_playlist_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    model = rc_ui_listview_playlist_get_model();
    if(model==NULL) return;
    path = gtk_tree_model_get_path(model, &iter);
    reference = gtk_tree_row_reference_new(model, path);
    gtk_tree_path_free(path);
    dialog = gtk_dialog_new_with_buttons(_("Set lyric file binding"),
        GTK_WINDOW(rc_ui_main_window_get_widget()), GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    radio_buttons[0] = gtk_radio_button_new_with_mnemonic(NULL,
        _("_Bind lyric file to the music"));
    radio_buttons[1] = gtk_radio_button_new_with_mnemonic_from_widget(
        GTK_RADIO_BUTTON(radio_buttons[0]), _("_Do not bind lyric file"));
    radio_buttons[2] = gtk_radio_button_new_with_mnemonic(NULL,
        _("Bind _secondary lyric file to the music"));
    radio_buttons[3] = gtk_radio_button_new_with_mnemonic_from_widget(
        GTK_RADIO_BUTTON(radio_buttons[2]),
        _("Do _not bind secondary lyric file"));
    filebutton[0] = gtk_file_chooser_button_new(_("Select a lyric file"),
        GTK_FILE_CHOOSER_ACTION_OPEN);
    filebutton[1] = gtk_file_chooser_button_new(_("Select a lyric file"),
        GTK_FILE_CHOOSER_ACTION_OPEN);
    frame1 = gtk_frame_new(_("The first lyric file binding"));
    frame2 = gtk_frame_new(_("The second lyric file binding"));
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    main_grid = gtk_grid_new();
    grid1 = gtk_grid_new();
    grid2 = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 2);
    gtk_grid_set_row_spacing(GTK_GRID(grid1), 2);
    gtk_grid_set_row_spacing(GTK_GRID(grid2), 2);
    for(i=0;i<4;i++)
        gtk_widget_set_hexpand(radio_buttons[i], TRUE);
    for(i=0;i<2;i++)
        gtk_widget_set_hexpand(filebutton[i], TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[3]), TRUE);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filebutton[0]),
        home_dir);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filebutton[1]),
        home_dir);
    file_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter,
        _("Image File (*.JPG, *.BMP, *.PNG)..."));
    gtk_file_filter_set_name(file_filter, _("Lyric File (*.LRC)"));
    gtk_file_filter_add_pattern(file_filter, "*.[L,l][R,r][C,c]");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filebutton[0]), file_filter);
    file_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter,
        _("Image File (*.JPG, *.BMP, *.PNG)..."));
    gtk_file_filter_set_name(file_filter, _("Lyric File (*.LRC)"));
    gtk_file_filter_add_pattern(file_filter, "*.[L,l][R,r][C,c]");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filebutton[1]), file_filter);
    rclib_db_playlist_data_iter_get((RCLibDbPlaylistIter *)
        iter.user_data, RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE,
        &lyric_file1, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    rclib_db_playlist_data_iter_get((RCLibDbPlaylistIter *)
        iter.user_data, RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE,
        &lyric_file2, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    if(lyric_file1!=NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]),
             TRUE);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filebutton[0]),
            lyric_file1);
    }
    if(lyric_file2!=NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[2]),
             TRUE);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filebutton[1]),
            lyric_file2);
    }
    gtk_grid_attach(GTK_GRID(grid1), radio_buttons[0], 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid1), filebutton[0], 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid1), radio_buttons[1], 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid2), radio_buttons[2], 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid2), filebutton[1], 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid2), radio_buttons[3], 0, 2, 1, 1);    
    gtk_container_add(GTK_CONTAINER(frame1), grid1);
    gtk_container_add(GTK_CONTAINER(frame2), grid2);
    gtk_grid_attach(GTK_GRID(main_grid), frame1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), frame2, 0, 1, 1, 1);
    gtk_widget_set_size_request(dialog, 300, -1);
    gtk_container_add(GTK_CONTAINER(content_area), main_grid);
    gtk_widget_show_all(content_area);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret==GTK_RESPONSE_ACCEPT)
    {
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_buttons[0])))
        {
            new_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                filebutton[0]));
            if(new_file!=NULL)
            {
                if(gtk_tree_row_reference_valid(reference))
                {
                    path = gtk_tree_row_reference_get_path(reference);
                    model = gtk_tree_row_reference_get_model(reference);
                    if(path!=NULL && model!=NULL)
                    {
                        if(gtk_tree_model_get_iter(model, &iter, path))
                        {
                            rclib_db_playlist_data_iter_set(
                                (RCLibDbPlaylistIter *)iter.user_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE,
                                new_file, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        }
                    }
                }
            }
            g_free(new_file);
        }
        else
        {
            if(gtk_tree_row_reference_valid(reference))
            {
                path = gtk_tree_row_reference_get_path(reference);
                model = gtk_tree_row_reference_get_model(reference);
                if(path!=NULL && model!=NULL)
                {
                    if(gtk_tree_model_get_iter(model, &iter, path))
                    {
                        rclib_db_playlist_data_iter_set(
                            (RCLibDbPlaylistIter *)iter.user_data,
                            RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE,
                            NULL, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                    }
                }
            }
        }
        
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_buttons[2])))
        {
            new_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                filebutton[1]));
            if(new_file!=NULL)
            {
                if(gtk_tree_row_reference_valid(reference))
                {
                    path = gtk_tree_row_reference_get_path(reference);
                    model = gtk_tree_row_reference_get_model(reference);
                    if(path!=NULL && model!=NULL)
                    {
                        if(gtk_tree_model_get_iter(model, &iter, path))
                        {
                            rclib_db_playlist_data_iter_set(
                                (RCLibDbPlaylistIter *)iter.user_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE,
                                new_file, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        }
                    }
                }
            }
            g_free(new_file);
        }
        else
        {
            if(gtk_tree_row_reference_valid(reference))
            {
                path = gtk_tree_row_reference_get_path(reference);
                model = gtk_tree_row_reference_get_model(reference);
                if(path!=NULL && model!=NULL)
                {
                    if(gtk_tree_model_get_iter(model, &iter, path))
                    {
                        rclib_db_playlist_data_iter_set(
                            (RCLibDbPlaylistIter *)iter.user_data,
                            RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE,
                            NULL, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                    }
                }
            }
        }
    }
    gtk_tree_row_reference_free(reference);
    gtk_widget_destroy(dialog);
}


/**
 * rc_ui_dialog_bind_album:
 *
 * Show a dialog to set the album cover image binding of a music item.
 */

void rc_ui_dialog_bind_album()
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *grid;
    GtkWidget *radio_buttons[2];
    GtkWidget *filebutton;
    GtkTreeIter iter;
    gint ret;
    const gchar *home_dir;
    const gchar *album_file = NULL;
    gchar *new_file;
    GtkFileFilter *file_filter;
    GtkTreePath *path;
    GtkTreeModel *model;
    GtkTreeRowReference *reference;
    if(!rc_ui_listview_playlist_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    model = rc_ui_listview_playlist_get_model();
    if(model==NULL) return;
    path = gtk_tree_model_get_path(model, &iter);
    reference = gtk_tree_row_reference_new(model, path);
    gtk_tree_path_free(path);
    dialog = gtk_dialog_new_with_buttons(_("Set album file binding"),
        GTK_WINDOW(rc_ui_main_window_get_widget()), GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    radio_buttons[0] = gtk_radio_button_new_with_mnemonic(NULL,
        _("_Bind album image file to the music"));
    radio_buttons[1] = gtk_radio_button_new_with_mnemonic_from_widget(
        GTK_RADIO_BUTTON(radio_buttons[0]), _("_Do not bind album file"));
    filebutton = gtk_file_chooser_button_new(_("Select a album image file"),
        GTK_FILE_CHOOSER_ACTION_OPEN);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
    file_filter = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter,
        _("Image File (*.JPG, *.BMP, *.PNG)..."));
    gtk_file_filter_add_pattern(file_filter, "*.[J,j][P,p][G,g]");
    gtk_file_filter_add_pattern(file_filter, "*.[J,j][P,p][E,e][G,g]");
    gtk_file_filter_add_pattern(file_filter, "*.[B,b][M,m][P,p]");
    gtk_file_filter_add_pattern(file_filter, "*.[P,p][N,n][G,g]");
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filebutton),
        home_dir);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filebutton), file_filter);
    rclib_db_playlist_data_iter_get((RCLibDbPlaylistIter *)
        iter.user_data, RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUMFILE,
        &album_file, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    if(album_file!=NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]),
             TRUE);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filebutton),
            album_file);
    }
    gtk_widget_set_hexpand(radio_buttons[0], TRUE);
    gtk_grid_attach(GTK_GRID(grid), radio_buttons[0], 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), filebutton, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), radio_buttons[1], 0, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_set_size_request(dialog, 300, -1);
    gtk_widget_show_all(content_area);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret==GTK_RESPONSE_ACCEPT)
    {
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_buttons[0])))
        {
            new_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                filebutton));
            if(new_file!=NULL)
            {
                if(gtk_tree_row_reference_valid(reference))
                {
                    path = gtk_tree_row_reference_get_path(reference);
                    model = gtk_tree_row_reference_get_model(reference);
                    if(path!=NULL && model!=NULL)
                    {
                        if(gtk_tree_model_get_iter(model, &iter, path))
                        {
                            rclib_db_playlist_data_iter_set(
                                (RCLibDbPlaylistIter *)iter.user_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUMFILE,
                                new_file, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                        }
                    }
                }
            }
            g_free(new_file);
        }
        else
        {
            if(gtk_tree_row_reference_valid(reference))
            {
                path = gtk_tree_row_reference_get_path(reference);
                model = gtk_tree_row_reference_get_model(reference);
                if(path!=NULL && model!=NULL)
                {
                    if(gtk_tree_model_get_iter(model, &iter, path))
                    {
                            rclib_db_playlist_data_iter_set(
                                (RCLibDbPlaylistIter *)iter.user_data,
                                RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUMFILE,
                                NULL, RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                    }
                }
            }
        }
    }
    gtk_tree_row_reference_free(reference);
    gtk_widget_destroy(dialog);
}

/**
 * rc_ui_dialog_save_album:
 *
 * Show a dialog to save the album image.
 */

void rc_ui_dialog_save_album()
{
    GtkWidget *file_chooser;
    gint result = 0;
    gchar *file_name = NULL;
    const gchar *home_dir;
    if(!rclib_album_get_album_data(NULL, NULL)) return;
    file_chooser = gtk_file_chooser_dialog_new(_("Save the playlist..."),
        NULL, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser),
        home_dir);
    gtk_file_chooser_set_do_overwrite_confirmation(
        GTK_FILE_CHOOSER(file_chooser), TRUE);
    result = gtk_dialog_run(GTK_DIALOG(file_chooser));
    switch(result)
    {
        case GTK_RESPONSE_ACCEPT:
            file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(
                file_chooser));
            rclib_album_save_file(file_name);
            g_free(file_name);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

/**
 * rc_ui_dialog_open_music:
 *
 * Show a open dialog for open and play the music.
 */

void rc_ui_dialog_open_music()
{
    GtkTreeIter iter;
    GtkWidget *file_chooser;
    GtkFileFilter *file_filter1;
    gint result = 0;
    gchar *uri = NULL;
    gchar *dialog_title = NULL;
    const gchar *home_dir;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    file_filter1 = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter1,
        _("All supported music files(*.FLAC;*.OGG;*.MP3;*.WAV;*.WMA...)"));
    gtk_file_filter_add_custom(file_filter1, GTK_FILE_FILTER_DISPLAY_NAME,
        rc_ui_dialog_music_file_filter, NULL, NULL);
    dialog_title = _("Select the music you want to add...");
    file_chooser = gtk_file_chooser_dialog_new(dialog_title, NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser),
        home_dir);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), file_filter1);
    result = gtk_dialog_run(GTK_DIALOG(file_chooser));
    switch(result)
    {
        case GTK_RESPONSE_ACCEPT:
            uri = gtk_file_chooser_get_uri(
                GTK_FILE_CHOOSER(file_chooser));
            if(uri!=NULL)
            {
                rclib_core_set_uri(uri);
                rclib_core_play();
            }
            g_free(uri);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

/**
 * rc_ui_dialog_open_location:
 *
 * Show a dialog for open and play the location.
 */

void rc_ui_dialog_open_location()
{
    GtkWidget *dialog;
    GtkWidget *grid;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget *entry;
    const gchar *uri;
    gint ret;
    dialog = gtk_dialog_new_with_buttons(_("Open Location"),
        GTK_WINDOW(rc_ui_main_window_get_widget()), GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    label = gtk_label_new(_("Enter the URL of the file you "
        "would like to open:"));
    entry = gtk_entry_new();
    g_object_set(entry, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    g_object_set(grid, "row-spacing", 3, NULL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_set_size_request(dialog, 300, -1);
    gtk_widget_show_all(content_area);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret==GTK_RESPONSE_ACCEPT)
    {
        uri = gtk_entry_get_text(GTK_ENTRY(entry));
        rclib_core_set_uri(uri);
        rclib_core_play();
    }
    gtk_widget_destroy(dialog);
}

static void rc_ui_dialog_show_supported_format_close_button_clicked(
    GtkWidget *widget, gpointer data)
{
    if(data==NULL) return;
    gtk_widget_destroy(GTK_WIDGET(data));
}

/**
 * rc_ui_dialog_show_supported_format:
 *
 * Show a dialog to get the supported music format list of the player.
 */

void rc_ui_dialog_show_supported_format()
{
    static GtkWidget *dialog = NULL;
    GtkWidget *main_grid;
    GtkWidget *button_hbox;
    GtkWidget *scrolled_window;
    GtkWidget *treeview;
    GtkWidget *close_button;
    GtkListStore *list_store;
    GtkTreeViewColumn *columns[2];
    GtkCellRenderer *renderers[2];
    GtkTreeIter iter;
    gboolean flag;
    gint i;
    if(dialog!=NULL)
    {
        gtk_widget_show_all(dialog);
        return;
    }
    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
    renderers[0] = gtk_cell_renderer_text_new();
    renderers[1] = gtk_cell_renderer_toggle_new();
    columns[0] = gtk_tree_view_column_new_with_attributes(
        _("Format"), renderers[0], "text", 0, NULL);
    columns[1] = gtk_tree_view_column_new_with_attributes(
        _("Supported"), renderers[1], "active", 1, NULL);
    gtk_tree_view_column_set_expand(columns[0], TRUE);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
    for(i=0;i<2;i++)
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), columns[i]);
    close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    main_grid = gtk_grid_new();
    button_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(dialog), _("Supported Audio Format"));
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(dialog, 350, 250);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    g_object_set(main_grid, "row-spacing", 2, NULL);
    g_object_set(scrolled_window, "expand", TRUE, NULL);
    g_object_set(button_hbox, "layout-style", GTK_BUTTONBOX_END, "spacing",
        5, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);
    gtk_box_pack_start(GTK_BOX(button_hbox), close_button, FALSE, FALSE, 2);
    gtk_grid_attach(GTK_GRID(main_grid), scrolled_window, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), button_hbox, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(dialog), main_grid);
    g_signal_connect(close_button, "clicked",
        G_CALLBACK(rc_ui_dialog_show_supported_format_close_button_clicked),
        dialog);
    g_signal_connect(dialog, "destroy",
        G_CALLBACK(gtk_widget_destroyed), &dialog);

    /* Check FLAC support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("flacdec", 0, 10, 0);
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("ffdec_flac", 0,
            10, 0);
    }
    gtk_list_store_set(list_store, &iter, 0, "FLAC", 1, flag, -1);

    /* Check OGG Vorbis support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("oggdemux", 0, 10, 0)
        && gst_default_registry_check_feature_version("vorbisdec", 0, 10, 0);
    gtk_list_store_set(list_store, &iter, 0, "OGG Vorbis", 1, flag, -1);

    /* Check MP3 support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("flump3dec", 0, 10, 0);
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("mad", 0, 10, 0);
    }
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("ffdec_mp3", 0,
            10, 0);
    }
    gtk_list_store_set(list_store, &iter, 0, "MP3", 1, flag, -1);

    /* Check WMA support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("fluwmadec", 0, 10, 0);
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("ffdec_wmapro", 0,
            10, 0) && gst_default_registry_check_feature_version("ffdec_wmav1",
            0, 10, 0) && gst_default_registry_check_feature_version(
            "ffdec_wmav2", 0, 10, 0) &&
            gst_default_registry_check_feature_version("ffdec_wmavoice", 0,
            10, 0);
    }
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("ffdec_mp3", 0,
            10, 0);
    }
    gtk_list_store_set(list_store, &iter, 0, "WMA", 1, flag, -1);

    /* Check Wavpack support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("wavpackdec", 0, 10, 0);
    gtk_list_store_set(list_store, &iter, 0, "Wavpack", 1, flag, -1);

    /* Check APE support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("ffdec_ape", 0, 10, 0)
        && gst_default_registry_check_feature_version("ffdemux_ape", 0, 10, 0);
    gtk_list_store_set(list_store, &iter, 0, "APE", 1, flag, -1);

    /* Check TTA support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("ttadec", 0, 10, 0) &&
        gst_default_registry_check_feature_version("ttaparse", 0, 10, 0);
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("ffdemux_tta", 0,
            10, 0) && gst_default_registry_check_feature_version("ffdec_tta",
            0, 10, 0);
    }
    gtk_list_store_set(list_store, &iter, 0, "TTA", 1, flag, -1);

    /* Check AAC support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("fluaacdec", 0, 10, 0);
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("ffdec_aac", 0,
            10, 0);
    }
    gtk_list_store_set(list_store, &iter, 0, "AAC", 1, flag, -1);

    /* Check AC3 support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("ffdec_ac3", 0, 10, 0);
    gtk_list_store_set(list_store, &iter, 0, "AC3", 1, flag, -1);

    /* Check MIDI support */
    gtk_list_store_append(list_store, &iter);
    flag = gst_default_registry_check_feature_version("fluidsynth", 0, 10, 0);
    if(!flag)
    {
        flag = gst_default_registry_check_feature_version("wildmidi", 0,
            10, 0);
    }
    gtk_list_store_set(list_store, &iter, 0, "MIDI", 1, flag, -1);

    g_object_unref(list_store);
    gtk_widget_show_all(dialog);
}

static void rc_ui_dialog_autosaved_response_cb(GtkDialog *dialog,
    gint response_id, gpointer user_data)
{
    if(response_id==GTK_RESPONSE_YES)
    {
        rclib_db_load_autosaved();
        rc_ui_listview_refresh();
    }
    rclib_db_autosaved_remove();
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

/**
 * rc_ui_dialog_show_load_autosaved:
 *
 * Show a dialog to ask the user whether to use the auto-saved playlist file.
 */

void rc_ui_dialog_show_load_autosaved()
{
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("The player does not exit normally, "
        "do you want to use the auto-saved playlist data?"));
    g_signal_connect(dialog, "response",
        G_CALLBACK(rc_ui_dialog_autosaved_response_cb), NULL);
    gtk_widget_show_all(dialog);
}

/**
 * rc_ui_dialog_show_load_legacy:
 *
 * Ask the user whether to load the playlist file from legacy version.
 */

void rc_ui_dialog_show_load_legacy()
{
    GtkWidget *dialog;
    gint ret;
    dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("Do you want to load playlist data from legacy version "
        "of RhythmCat?"));
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret==GTK_RESPONSE_YES)
    {
        rclib_db_load_legacy();
    }
    gtk_widget_destroy(dialog);
}

static void rc_ui_dialog_rating_limited_playing_enable_toggled_cb(
    GtkToggleButton *button, gpointer data)
{
    GtkWidget *grid = GTK_WIDGET(data);
    gboolean enabled;
    if(data==NULL) return;
    enabled = gtk_toggle_button_get_active(button);
    gtk_widget_set_sensitive(grid, enabled);
}

/**
 * rc_ui_dialog_rating_limited_playing:
 *
 * Show a dialog to configure the rating limited playing mode.
 */

void rc_ui_dialog_rating_limited_playing()
{
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *main_grid;
    GtkWidget *grid;
    GtkWidget *enable_checkbutton;
    GtkWidget *condition_radiobutton1;
    GtkWidget *condition_radiobutton2;
    GtkWidget *spin_label;
    GtkWidget *spin_button;
    gint ret;
    gboolean condition = FALSE;
    gfloat rating = 3.0;
    dialog = gtk_dialog_new_with_buttons(_("Rating Limited Playing Mode"),
        GTK_WINDOW(rc_ui_main_window_get_widget()), GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    enable_checkbutton = gtk_check_button_new_with_mnemonic(_("_Enable "
        "rating limited playing mode"));
    condition_radiobutton1 = gtk_radio_button_new_with_mnemonic(NULL,
        _("Play the music whose rating is _greater than or equal\nto the "
        "rating limit value"));
    condition_radiobutton2 = gtk_radio_button_new_with_mnemonic_from_widget(
        GTK_RADIO_BUTTON(condition_radiobutton1), _("Play the music whose "
        "rating is _lesser than or equal to\nthe rating limit value"));
    spin_label = gtk_label_new(_("Rating limit value"));
    spin_button = gtk_spin_button_new_with_range(0.0, 5.0, 0.1);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    main_grid = gtk_grid_new();
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 2);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
    gtk_widget_set_hexpand(condition_radiobutton1, TRUE);
    gtk_widget_set_hexpand(condition_radiobutton2, TRUE);
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);
    if(rclib_player_get_rating_limit(&rating, &condition))
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            enable_checkbutton), TRUE);
        gtk_widget_set_sensitive(grid, TRUE);
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            enable_checkbutton), FALSE);
        gtk_widget_set_sensitive(grid, FALSE);
    }
    if(condition)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            condition_radiobutton2), TRUE);
    }
    else
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            condition_radiobutton1), TRUE);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), rating);
    gtk_grid_attach(GTK_GRID(grid), condition_radiobutton1,
        0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), condition_radiobutton2,
        0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_label,
        0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), spin_button,
        1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), enable_checkbutton,
        0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), grid,
        0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(content_area), main_grid);
    gtk_widget_set_size_request(dialog, 300, -1);
    g_signal_connect(enable_checkbutton, "toggled",
        G_CALLBACK(rc_ui_dialog_rating_limited_playing_enable_toggled_cb),
        grid);
    gtk_widget_show_all(content_area);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret==GTK_RESPONSE_ACCEPT)
    {
        rclib_player_set_rating_limit(
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
            enable_checkbutton)), gtk_spin_button_get_value(
            GTK_SPIN_BUTTON(spin_button)), gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(condition_radiobutton2)));
    }
    gtk_widget_destroy(dialog);
}

