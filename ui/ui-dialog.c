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

#include "ui-dialog.h"
#include "ui-listview.h"
#include "ui-player.h"
#include "common.h"

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
    about_dialog = gtk_about_dialog_new();
    gchar *version;
    version = g_strdup_printf("LibRhythmCat %d.%d.%d - build date: %s",
        rclib_major_version, rclib_minor_version, rclib_micro_version,
        rclib_build_date);
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog),
        "RhythmCat2 Music Player");
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about_dialog),
        (GdkPixbuf *)rc_ui_player_get_icon_image());
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog),
        dialog_about_authors);
    gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(about_dialog),
        dialog_about_documenters);
    gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about_dialog),
        dialog_about_artists);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog),
        _("A music player based on GTK+ 3.0 & GStreamer 0.10"));
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog),
        "http://supercat-lab.org");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog),
        GTK_LICENSE_GPL_3_0);
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dialog),
        dialog_about_license);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog),
        version);
    g_free(version);
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);  
}

/**
 * rc_ui_dialog_show_message:
 * @type: type of message
 * @title: title of the message
 * @format: printf()-style format string, or NULL, allow-none
 * @Varargs: arguments for @format
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
                rclib_db_playlist_add_music((GSequenceIter *)iter.user_data,
                    NULL, uri);
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
            rclib_db_playlist_add_directory((GSequenceIter *)iter.user_data,
                NULL, directory_path);
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
    GtkWidget *vbox;
    GtkWidget *frame1, *frame2;
    GtkWidget *vbox2, *vbox3;
    GtkWidget *radio_buttons[4];
    GtkWidget *filebutton[2];
    GtkTreeIter iter;
    gint ret;
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
        GTK_WINDOW(rc_ui_player_get_main_window()), GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    radio_buttons[0] = gtk_radio_button_new_with_mnemonic(NULL,
        _("_Bind lyric file to the music"));
    radio_buttons[1] = gtk_radio_button_new_with_mnemonic_from_widget(
        GTK_RADIO_BUTTON(radio_buttons[0]), _("_Do not bind lryic file"));
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
    vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
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
    lyric_file1 = rclib_db_playlist_get_lyric_bind((GSequenceIter *)
        iter.user_data);
    lyric_file2 = rclib_db_playlist_get_lyric_secondary_bind(
        (GSequenceIter *)iter.user_data);
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
    gtk_box_pack_start(GTK_BOX(vbox2), radio_buttons[0], FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox2), filebutton[0], FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox2), radio_buttons[1], FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox3), radio_buttons[2], FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox3), filebutton[1], FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox3), radio_buttons[3], FALSE, FALSE, 2);
    gtk_container_add(GTK_CONTAINER(frame1), vbox2);
    gtk_container_add(GTK_CONTAINER(frame2), vbox3);
    gtk_box_pack_start(GTK_BOX(vbox), frame1, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), frame2, FALSE, FALSE, 2);
    gtk_widget_set_size_request(dialog, 300, -1);
    gtk_widget_show_all(vbox);
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
                            rclib_db_playlist_set_lyric_bind(
                                (GSequenceIter *)iter.user_data, new_file);
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
                        rclib_db_playlist_set_lyric_bind(
                            (GSequenceIter *)iter.user_data, NULL);
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
                            rclib_db_playlist_set_lyric_secondary_bind(
                                (GSequenceIter *)iter.user_data, new_file);
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
                        rclib_db_playlist_set_lyric_secondary_bind(
                            (GSequenceIter *)iter.user_data, NULL);
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
    GtkWidget *vbox;
    GtkWidget *vbox2;
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
        GTK_WINDOW(rc_ui_player_get_main_window()), GTK_DIALOG_MODAL |
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    radio_buttons[0] = gtk_radio_button_new_with_mnemonic(NULL,
        _("_Bind album image file to the music"));
    radio_buttons[1] = gtk_radio_button_new_with_mnemonic_from_widget(
        GTK_RADIO_BUTTON(radio_buttons[0]), _("_Do not bind album file"));
    filebutton = gtk_file_chooser_button_new(_("Select a album image file"),
        GTK_FILE_CHOOSER_ACTION_OPEN);
    vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
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
    album_file = rclib_db_playlist_get_album_bind((GSequenceIter *)
        iter.user_data);
    if(album_file!=NULL)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]),
             TRUE);
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filebutton),
            album_file);
    }
    gtk_box_pack_start(GTK_BOX(vbox2), radio_buttons[0], FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox2), filebutton, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), vbox2, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[1], FALSE, FALSE, 2);
    gtk_widget_set_size_request(dialog, 300, -1);
    gtk_widget_show_all(vbox);
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
                            rclib_db_playlist_set_album_bind(
                                (GSequenceIter *)iter.user_data, new_file);
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
                        rclib_db_playlist_set_album_bind(
                            (GSequenceIter *)iter.user_data, NULL);
                    }
                }
            }
        }
    }
    gtk_tree_row_reference_free(reference);
    gtk_widget_destroy(dialog);
}







