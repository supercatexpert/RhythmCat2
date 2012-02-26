/*
 * RhythmCat UI Menu Module
 * The menus for the player.
 *
 * ui-menu.c
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

#include "ui-menu.h"
#include "ui-listview.h"
#include "ui-dialog.h"
#include "common.h"

static GtkUIManager *ui_manager = NULL;
static GtkActionGroup *ui_actions = NULL;

static void rc_ui_menu_play_button_clicked_cb()
{
    GstState state;
    if(rclib_core_get_state(&state, NULL, GST_CLOCK_TIME_NONE))
    {
        if(state==GST_STATE_PLAYING)
        {
            rclib_core_pause();
            return;
        }
        else if(state==GST_STATE_PAUSED)
        {
            rclib_core_play();
            return;
        }
    }
    rclib_core_play();
    return;
}

static void rc_ui_menu_stop_button_clicked_cb()
{
    rclib_core_stop();
}

static void rc_ui_menu_prev_button_clicked_cb()
{
    rclib_player_play_prev(FALSE, TRUE, FALSE);
}

static void rc_ui_menu_next_button_clicked_cb()
{
    rclib_player_play_next(FALSE, TRUE, FALSE);
}

static void rc_ui_menu_repeat_clicked_cb(GtkAction *action,
    GtkRadioAction *current, gpointer data)
{
    gint value = gtk_radio_action_get_current_value(current);
    g_signal_handlers_block_by_func(action,
        G_CALLBACK(rc_ui_menu_repeat_clicked_cb), data);
    rclib_player_set_repeat_mode(value);
    g_signal_handlers_unblock_by_func(action,
        G_CALLBACK(rc_ui_menu_repeat_clicked_cb), data);   
}

static void rc_ui_menu_random_clicked_cb(GtkAction *action,
    GtkRadioAction *current, gpointer data)
{
    gint value = gtk_radio_action_get_current_value(current);
    g_signal_handlers_block_by_func(action,
        G_CALLBACK(rc_ui_menu_random_clicked_cb), data);
    rclib_player_set_random_mode(value);
    g_signal_handlers_unblock_by_func(action,
        G_CALLBACK(rc_ui_menu_random_clicked_cb), data);
}

static void rc_ui_menu_progress_import_cancel_clicked_cb()
{
    rclib_db_playlist_import_cancel();
}

static void rc_ui_menu_progress_refresh_cancel_clicked_cb()
{
    rclib_db_playlist_refresh_cancel();
}

static GtkActionEntry ui_menu_entries[] =
{
    { "FileMenu", NULL, N_("_File") },
    { "EditMenu", NULL, N_("_Edit") },
    { "ViewMenu", NULL, N_("_View") },
    { "ControlMenu", NULL, N_("_Control") },
    { "HelpMenu", NULL, N_("_Help") },
    { "RepeatMenu", NULL, N_("_Repeat") },
    { "RandomMenu", NULL, N_("Ran_dom") },
    { "FileNewList", GTK_STOCK_NEW,
      N_("_New Playlist"), "<control>N",
      N_("Create a new playlist"),
      G_CALLBACK(rc_ui_listview_catalog_new_playlist) },
    { "FileImportMusic", GTK_STOCK_OPEN,
      N_("Import _Music"), "<control>O",
      N_("Import music file"),
      G_CALLBACK(rc_ui_dialog_add_music) },
    { "FileImportList", GTK_STOCK_FILE,
      N_("Import _Playlist"), NULL,
      N_("Import music from playlist"),
      G_CALLBACK(rc_ui_dialog_load_playlist) },
    { "FileImportFolder", GTK_STOCK_DIRECTORY,
      N_("Import _Folder"), NULL,
      N_("Import all music from folder"),
      G_CALLBACK(rc_ui_dialog_add_directory) },
    { "FileExportList", GTK_STOCK_SAVE,
      N_("_Export Playlist"), "<control>S",
      N_("Export music to a playlist"),
      G_CALLBACK(rc_ui_dialog_save_playlist) },
    { "FileExportAll", GTK_STOCK_SAVE_AS,
      N_("Export _All Playlists"), NULL,
      N_("Export all playlists to a folder"),
      G_CALLBACK(rc_ui_dialog_save_all_playlist) },
    { "FileQuit", GTK_STOCK_QUIT,
      N_("_Quit"), "<control>Q",
      N_("Quit this player"),
      G_CALLBACK(NULL) },
    { "EditRenameList", GTK_STOCK_EDIT,
      N_("Re_name Playlist"), "F2",
      N_("Rename the playlist"),
      G_CALLBACK(rc_ui_listview_catalog_rename_playlist) },
    { "EditRemoveList", GTK_STOCK_REMOVE,
      N_("R_emove Playlist"), NULL,
      N_("Remove the playlist"),
      G_CALLBACK(rc_ui_listview_catalog_delete_items) },
    { "EditBindLyric", NULL,
      N_("Bind _lyric file"), NULL,
      N_("Bind lyric file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_lyric) },
    { "EditBindAlbum", NULL,
      N_("Bind al_bum file"), NULL,
      N_("Bind album file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_album) },
    { "EditRemoveMusic", GTK_STOCK_DELETE,
      N_("_Remove Music"), NULL,
      N_("Remove music from playlist"),
      G_CALLBACK(rc_ui_listview_playlist_delete_items) },
    { "EditSelectAll", GTK_STOCK_SELECT_ALL,
      N_("Select _All"), "<control>A",
      N_("Select all music in the playlist"),
      G_CALLBACK(rc_ui_listview_playlist_select_all) },
    { "EditRefreshList", GTK_STOCK_REFRESH,
      N_("Re_fresh Playlist"), "F5",
      N_("Refresh music information in the playlist"),
      G_CALLBACK(rc_ui_listview_playlist_refresh) },
    { "EditPlugin", GTK_STOCK_EXECUTE,
      N_("Plu_gins"), "F11",
      N_("Configure plugins"),
      G_CALLBACK(NULL) },
    { "EditPreferences", GTK_STOCK_PREFERENCES,
      N_("_Preferences"), "F8",
      N_("Configure the player"),
      G_CALLBACK(NULL) },
    { "ViewMiniMode", NULL,
      N_("_Mini Mode"), "<control><alt>M",
      N_("Enable mini mode"),
      G_CALLBACK(NULL) },
    { "ControlPlay", GTK_STOCK_MEDIA_PLAY,
      N_("_Play/Pause"), "<control>L",
      N_("Play or pause the music"),
      G_CALLBACK(rc_ui_menu_play_button_clicked_cb) },
    { "ControlStop", GTK_STOCK_MEDIA_STOP,
      N_("_Stop"), "<control>P",
      N_("Stop the music"),
      G_CALLBACK(rc_ui_menu_stop_button_clicked_cb) },
    { "ControlPrev", GTK_STOCK_MEDIA_PREVIOUS,
      N_("Pre_vious"), "<alt>Left",
      N_("Play previous music"),
      G_CALLBACK(rc_ui_menu_prev_button_clicked_cb) },
    { "ControlNext", GTK_STOCK_MEDIA_NEXT,
      N_("_Next"), "<alt>Right",
      N_("Play next music"),
      G_CALLBACK(rc_ui_menu_next_button_clicked_cb) },
    { "ControlBackward", GTK_STOCK_MEDIA_REWIND,
      N_("_Backward"), "<control>Left",
      N_("Backward 5 seconds"),
      G_CALLBACK(NULL) },
    { "ControlForward", GTK_STOCK_MEDIA_FORWARD,
      N_("_Forward"), "<control>Right",
      N_("Forward 5 seconds"),
      G_CALLBACK(NULL) },
    { "ControlVolumeUp", GTK_STOCK_GO_UP,
      N_("_Increase Volume"), "<control>Up",
      N_("Increase the volume"),
      G_CALLBACK(NULL) },
    { "ControlVolumeDown", GTK_STOCK_GO_DOWN,
      N_("_Decrease Volume"), "<control>Down",
      N_("Decrease the volume"),
      G_CALLBACK(NULL) },
    { "HelpAbout", GTK_STOCK_ABOUT,
      N_("_About"), NULL,
      N_("About this player"),
      G_CALLBACK(rc_ui_dialog_about_player) },
    { "HelpSupportedFormat", NULL,
      N_("_Supported Format"), NULL,
      N_("Check the supported music format of this player"),
      G_CALLBACK(NULL) },
    { "CatalogNewList", GTK_STOCK_NEW,
      N_("_New Playlist"), NULL,
      N_("Create a new playlist"),
      G_CALLBACK(rc_ui_listview_catalog_new_playlist) },
    { "CatalogRenameList", GTK_STOCK_EDIT,
      N_("Re_name Playlist"), NULL,
      N_("Raname the playlist"),
      G_CALLBACK(rc_ui_listview_catalog_rename_playlist) },
    { "CatalogRemoveList", GTK_STOCK_REMOVE,
      N_("R_emove Playlist"), NULL,
      N_("Remove the playlist"),
      G_CALLBACK(rc_ui_listview_catalog_delete_items) },
    { "CatalogExportList", GTK_STOCK_SAVE,
      N_("E_xport Playlist"), NULL,
      N_("Export music to a playlist"),
      G_CALLBACK(rc_ui_dialog_save_playlist) },
    { "PlaylistImportMusic", GTK_STOCK_OPEN,
      N_("Import _Music"), NULL,
      N_("Import music file"),
      G_CALLBACK(rc_ui_dialog_add_music) },
    { "PlaylistImportList", GTK_STOCK_FILE,
      N_("Import _Playlist"), NULL,
      N_("Import music from playlist"),
      G_CALLBACK(rc_ui_dialog_load_playlist) },
    { "PlaylistSelectAll", GTK_STOCK_SELECT_ALL,
      N_("Select _All"), NULL,
      N_("Select all music in the playlist"),
      G_CALLBACK(rc_ui_listview_playlist_select_all) },
    { "PlaylistBindLyric", NULL,
      N_("Bind _lyric file"), NULL,
      N_("Bind lyric file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_lyric) },
    { "PlaylistBindAlbum", NULL,
      N_("Bind al_bum file"), NULL,
      N_("Bind album file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_album) },
    { "PlaylistRemoveMusic", GTK_STOCK_REMOVE,
      N_("R_emove Music"), NULL,
      N_("Remove music from playlist"),
      G_CALLBACK(rc_ui_listview_playlist_delete_items) },
    { "PlaylistRefreshList", GTK_STOCK_REFRESH,
      N_("Re_fresh Playlist"), NULL,
      N_("Refresh music information in the playlist"),
      G_CALLBACK(rc_ui_listview_playlist_refresh) },
    { "AlbumSaveImage", GTK_STOCK_SAVE_AS,
      N_("_Save album image as"), NULL,
      N_("Save the album image"),
      G_CALLBACK(rc_ui_dialog_save_album) },
    { "ProgressImportStatus", NULL,
      N_("Importing: 0 remaining"), NULL,
      NULL,
      NULL },
    { "ProgressRefreshStatus", NULL,
      N_("Refreshing: 0 remaining"), NULL,
      NULL,
      NULL },
    { "ProgressImportCancel", NULL,
      N_("Stop _importing"), NULL,
      N_("Terminate the remaining importing jobs."),
      G_CALLBACK(rc_ui_menu_progress_import_cancel_clicked_cb) },
    { "ProgressRefreshCancel", NULL,
      N_("Stop _refreshing"), NULL,
      N_("Terminate the remaining refreshing jobs."),
      G_CALLBACK(rc_ui_menu_progress_refresh_cancel_clicked_cb) },
    { "TrayPlay", GTK_STOCK_MEDIA_PLAY,
      N_("_Play/Pause"), NULL,
      N_("Play or pause the music"),
      G_CALLBACK(rc_ui_menu_play_button_clicked_cb) },
    { "TrayStop", GTK_STOCK_MEDIA_STOP,
      N_("_Stop"), NULL,
      N_("Stop the music"),
      G_CALLBACK(rc_ui_menu_stop_button_clicked_cb) },
    { "TrayPrev", GTK_STOCK_MEDIA_PREVIOUS,
      N_("Pre_vious"), NULL,
      N_("Play previous music"),
      G_CALLBACK(rc_ui_menu_prev_button_clicked_cb) },
    { "TrayNext", GTK_STOCK_MEDIA_NEXT,
      N_("_Next"), NULL,
      N_("Play next music"),
      G_CALLBACK(rc_ui_menu_next_button_clicked_cb) },
    { "TrayShowPlayer", GTK_STOCK_HOME,
      N_("S_how Player"), NULL,
      N_("Show the window of player"),
      G_CALLBACK(NULL) },
    { "TrayModeSwitch", NULL,
      N_("_Mode Switch"), NULL,
      N_("Switch between normal mode and mini mode"),
      G_CALLBACK(NULL) },
    { "TrayAbout", GTK_STOCK_ABOUT,
      N_("_About"), NULL,
      N_("About this player"),
      G_CALLBACK(rc_ui_dialog_about_player) },
    { "TrayQuit", GTK_STOCK_QUIT,
      N_("_Quit"), NULL,
      N_("Quit this player"),
      G_CALLBACK(NULL) }
};

static guint ui_menu_n_entries = G_N_ELEMENTS(ui_menu_entries);

static GtkRadioActionEntry ui_menu_repeat_entries[] =
{
    { "RepeatNoRepeat", NULL,
      N_("_No Repeat"), NULL,
      N_("No repeat"), RCLIB_PLAYER_REPEAT_NONE },
    { "RepeatMusicRepeat", NULL,
      N_("Single _Music Repeat"), NULL,
      N_("Repeat playing single music"), RCLIB_PLAYER_REPEAT_SINGLE },
    { "RepeatListRepeat", NULL,
      N_("Single _Playlist Repeat"), NULL,
      N_("Repeat playing single playlist"), RCLIB_PLAYER_REPEAT_LIST },
    { "RepeatAllRepeat", NULL,
      N_("_All Playlists Repeat"), NULL,
      N_("Repeat playing all playlists"), RCLIB_PLAYER_REPEAT_ALL }
};

static guint ui_menu_repeat_n_entries = G_N_ELEMENTS(ui_menu_repeat_entries);

static GtkRadioActionEntry ui_menu_random_entries[] =
{
    { "RandomNoRandom", NULL,
      N_("_No Random"), NULL,
      N_("No random playing"), RCLIB_PLAYER_RANDOM_NONE },
    { "RandomSingleRandom", NULL,
      N_("_Single Playlist Random"), NULL,
      N_("Random playing a music in the playlist"),
      RCLIB_PLAYER_RANDOM_SINGLE },
    { "RandomAllRandom", NULL,
      N_("_All Playlists Random"), NULL,
      N_("Random playing a music in all playlists"),
      RCLIB_PlAYER_RANDOM_ALL }
};

static guint ui_menu_random_n_entries = G_N_ELEMENTS(ui_menu_random_entries);

static GtkToggleActionEntry ui_menu_toogle_entries[] =
{
    { "ViewAlwaysOnTop", GTK_STOCK_GOTO_TOP,
      N_("Always On _Top"), NULL,
      N_("Always on top"),
      G_CALLBACK(NULL), FALSE },
    { "TrayAlwaysOnTop", GTK_STOCK_GOTO_TOP,
      N_("Always On _Top"), NULL,
      N_("Always on top"),
      G_CALLBACK(NULL), FALSE }
};

static guint ui_menu_toogle_n_entries = G_N_ELEMENTS(ui_menu_toogle_entries);

static const gchar *ui_menu_info =
    "<ui>"
    "  <popup action='RC2MainPopupMenu'>"
    "    <menu action='FileMenu'>"
    "      <menuitem action='FileNewList'/>"
    "      <separator name='FileSep1'/>"
    "      <menuitem action='FileImportMusic'/>"
    "      <menuitem action='FileImportList'/>"
    "      <menuitem action='FileImportFolder'/>"
    "      <separator name='FileSep2'/>"
    "      <menuitem action='FileExportList'/>"
    "      <menuitem action='FileExportAll'/>"
    "      <separator name='FileSep3'/>"
    "      <menuitem action='FileQuit'/>"
    "    </menu>"
    "    <menu action='EditMenu'>"
    "      <menuitem action='EditRenameList'/>"
    "      <menuitem action='EditRemoveList'/>"
    "      <separator name='EditSep1'/>"
    "      <menuitem action='EditBindLyric'/>"
    "      <menuitem action='EditBindAlbum'/>"
    "      <menuitem action='EditRemoveMusic'/>"
    "      <menuitem action='EditSelectAll'/>"
    "      <menuitem action='EditRefreshList'/>"
    "      <separator name='EditSep2'/>"
    "      <menuitem action='EditPlugin'/>"
    "      <menuitem action='EditPreferences'/>"
    "    </menu>"
    "    <menu action='ViewMenu'>"
    "      <menuitem action='ViewAlwaysOnTop'/>"
    "      <menuitem action='ViewMiniMode'/>"
    "    </menu>"
    "    <menu action='ControlMenu'>"
    "      <menuitem action='ControlPlay'/>"
    "      <menuitem action='ControlStop'/>"
    "      <menuitem action='ControlPrev'/>"
    "      <menuitem action='ControlNext'/>"
    "      <menuitem action='ControlBackward'/>"
    "      <menuitem action='ControlForward'/>"
    "      <separator name='ControlSep1'/>"
    "      <menuitem action='ControlVolumeUp'/>"
    "      <menuitem action='ControlVolumeDown'/>"
    "      <separator name='ControlSep2'/>"
    "      <menu action='RepeatMenu'>"
    "        <menuitem action='RepeatNoRepeat'/>"
    "        <separator name='RepeatSep1'/>"
    "        <menuitem action='RepeatMusicRepeat'/>"
    "        <menuitem action='RepeatListRepeat'/>"
    "        <menuitem action='RepeatAllRepeat'/>"
    "      </menu>"
    "      <menu action='RandomMenu'>"
    "        <menuitem action='RandomNoRandom'/>"
    "        <separator name='RandomSep1'/>"
    "        <menuitem action='RandomSingleRandom'/>"
    "        <menuitem action='RandomAllRandom'/>"
    "      </menu>"
    "    </menu>"
    "    <menu action='HelpMenu'>"
    "      <menuitem action='HelpAbout'/>"
    "      <menuitem action='HelpSupportedFormat'/>"
    "    </menu>"
    "  </popup>"
    "  <popup action='CatalogPopupMenu'>"
    "    <menuitem action='CatalogNewList'/>"
    "    <menuitem action='CatalogRenameList'/>"
    "    <menuitem action='CatalogRemoveList'/>"
    "    <menuitem action='CatalogExportList'/>"
    "  </popup>"
    "  <popup action='PlaylistPopupMenu'>"
    "    <menuitem action='PlaylistImportMusic'/>"
    "    <menuitem action='PlaylistImportList'/>"
    "    <separator name='PlaylistSep1'/>"
    "    <menuitem action='PlaylistSelectAll'/>"
    "    <menuitem action='PlaylistBindLyric'/>"
    "    <menuitem action='PlaylistBindAlbum'/>"
    "    <menuitem action='PlaylistRemoveMusic'/>"
    "    <separator name='PlaylistSep2'/>"
    "    <menuitem action='PlaylistRefreshList'/>"
    "  </popup>"
    "  <popup action='AlbumPopupMenu'>"
    "    <menuitem action='AlbumSaveImage'/>"
    "  </popup>"
    "  <popup action='ProgressPopupMenu'>"
    "    <menuitem action='ProgressImportStatus'/>"
    "    <menuitem action='ProgressRefreshStatus'/>"
    "    <menuitem action='ProgressImportCancel'/>"
    "    <menuitem action='ProgressRefreshCancel'/>"
    "  </popup>"
    "  <popup action='TrayPopupMenu'>"
    "    <menuitem action='TrayPlay'/>"
    "    <menuitem action='TrayStop'/>"
    "    <menuitem action='TrayPrev'/>"
    "    <menuitem action='TrayNext'/>"
    "    <separator/>"
    "    <menuitem action='TrayShowPlayer'/>"
    "    <menuitem action='TrayModeSwitch'/>"
    "    <menuitem action='TrayAlwaysOnTop'/>"
    "    <menuitem action='TrayAbout'/>"
    "    <separator/>"
    "    <menuitem action='TrayQuit'/>"
    "  </popup>"
    "  <popup action='CoverPopupMenu'>"
    "  </popup>"
    "</ui>";

static void rc_ui_menu_core_state_changed_cb(RCLibCore *core,
    GstState state, gpointer data)
{
    if(state!=GST_STATE_PLAYING && state!=GST_STATE_PAUSED)
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlBackward"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlForward"), FALSE);
    }
    else
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlBackward"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlForward"), TRUE);
    }
    if(state==GST_STATE_PLAYING)
    {
        g_object_set(G_OBJECT(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlPlay")), "stock-id",
            GTK_STOCK_MEDIA_PAUSE, NULL);
        g_object_set(G_OBJECT(gtk_ui_manager_get_action(ui_manager,
            "/TrayPopupMenu/TrayPlay")), "stock-id",
            GTK_STOCK_MEDIA_PAUSE, NULL);
    }
    else
    {
        g_object_set(G_OBJECT(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlPlay")), "stock-id",
            GTK_STOCK_MEDIA_PLAY, NULL);
        g_object_set(G_OBJECT(gtk_ui_manager_get_action(ui_manager,
            "/TrayPopupMenu/TrayPlay")), "stock-id",
            GTK_STOCK_MEDIA_PLAY, NULL);
    }
}

static void rc_ui_menu_volume_changed_cb(RCLibCore *core, gdouble volume,
    gpointer data)
{
    if(volume>0.9999)
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlVolumeUp"), FALSE);
    else
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlVolumeUp"), TRUE);
    if(volume<0.0001)
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlVolumeDown"), FALSE);
    else
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/ControlMenu/ControlVolumeDown"), TRUE);
}

static void rc_ui_menu_player_repeat_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRepeatMode mode, gpointer data)
{
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(
        gtk_ui_manager_get_action(ui_manager,
        "/RC2MainPopupMenu/ControlMenu/RepeatMenu/RepeatNoRepeat")), mode);
}

static void rc_ui_menu_player_random_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRandomMode mode, gpointer data)
{
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(
        gtk_ui_manager_get_action(ui_manager,
        "/RC2MainPopupMenu/ControlMenu/RandomMenu/RandomNoRandom")), mode);
}

static void rc_ui_menu_catalog_listview_selection_changed_cb(
    GtkTreeSelection *selection, gpointer data)
{
    gint value = gtk_tree_selection_count_selected_rows(selection);
    if(value>0)    
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRemoveList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/CatalogPopupMenu/CatalogRemoveList"), TRUE);
    }
    else
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRemoveList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/CatalogPopupMenu/CatalogRemoveList"), FALSE);
    }
    if(value==1)
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileImportMusic"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileImportList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileImportFolder"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileExportList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRenameList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditSelectAll"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRefreshList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/CatalogPopupMenu/CatalogRenameList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/CatalogPopupMenu/CatalogExportList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistImportMusic"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistImportList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistRefreshList"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistSelectAll"), TRUE);
    }
    else
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileImportMusic"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileImportList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileImportFolder"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/FileMenu/FileExportList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRenameList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditSelectAll"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRefreshList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/CatalogPopupMenu/CatalogRenameList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/CatalogPopupMenu/CatalogExportList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistImportMusic"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistImportList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistRefreshList"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistSelectAll"), FALSE);
    }
}

static void rc_ui_menu_playlist_listview_selection_changed_cb(
    GtkTreeSelection *selection, gpointer data)
{
    gint value = gtk_tree_selection_count_selected_rows(selection);
    if(value>0)
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRemoveMusic"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistRemoveMusic"), TRUE);
    }
    else
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditRemoveMusic"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistRemoveMusic"), FALSE);
    }
    if(value==1)
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditBindLyric"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditBindAlbum"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistBindLyric"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistBindAlbum"), TRUE);
    }
    else
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditBindLyric"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MainPopupMenu/EditMenu/EditBindAlbum"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistBindLyric"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/PlaylistPopupMenu/PlaylistBindAlbum"), FALSE);
    }
}

/**
 * rc_ui_menu_init:
 *
 * Initialize the menus.
 */

void rc_ui_menu_init()
{
    GError *error = NULL;
    GtkTreeSelection *catalog_selection, *playlist_selection;
    ui_manager = gtk_ui_manager_new();
    ui_actions = gtk_action_group_new("RC2Actions");
    gtk_action_group_set_translation_domain(ui_actions, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(ui_actions, ui_menu_entries,
        ui_menu_n_entries, NULL);
    gtk_action_group_add_radio_actions(ui_actions, ui_menu_repeat_entries,
        ui_menu_repeat_n_entries, 0,
        G_CALLBACK(rc_ui_menu_repeat_clicked_cb), NULL);
    gtk_action_group_add_radio_actions(ui_actions, ui_menu_random_entries,
        ui_menu_random_n_entries, 0,
        G_CALLBACK(rc_ui_menu_random_clicked_cb), NULL);
    gtk_action_group_add_toggle_actions(ui_actions, ui_menu_toogle_entries,
        ui_menu_toogle_n_entries, NULL);
    gtk_ui_manager_insert_action_group(ui_manager, ui_actions, 0);
    g_object_unref(ui_actions);
    if(!gtk_ui_manager_add_ui_from_string(ui_manager, ui_menu_info, -1,
        &error))
    {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Cannot load menu: %s",
            error->message);
        g_error_free(error);
    }
    catalog_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        rc_ui_listview_get_catalog_widget()));
    playlist_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        rc_ui_listview_get_playlist_widget()));
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/AlbumPopupMenu/AlbumSaveImage"), FALSE);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/ProgressPopupMenu/ProgressImportStatus"), FALSE);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/ProgressPopupMenu/ProgressRefreshStatus"), FALSE);
    gtk_action_set_visible(gtk_ui_manager_get_action(ui_manager,
        "/ProgressPopupMenu/ProgressImportStatus"), FALSE);
    gtk_action_set_visible(gtk_ui_manager_get_action(ui_manager,
        "/ProgressPopupMenu/ProgressRefreshStatus"), FALSE);
    gtk_action_set_visible(gtk_ui_manager_get_action(ui_manager,
        "/ProgressPopupMenu/ProgressImportCancel"), FALSE);
    gtk_action_set_visible(gtk_ui_manager_get_action(ui_manager,
        "/ProgressPopupMenu/ProgressRefreshCancel"), FALSE);
    rclib_core_signal_connect("state-changed",
        G_CALLBACK(rc_ui_menu_core_state_changed_cb), NULL);
    rclib_core_signal_connect("volume-changed",
        G_CALLBACK(rc_ui_menu_volume_changed_cb), NULL);
    rclib_player_signal_connect("repeat-mode-changed",
        G_CALLBACK(rc_ui_menu_player_repeat_mode_changed_cb), NULL);
    rclib_player_signal_connect("random-mode-changed",
        G_CALLBACK(rc_ui_menu_player_random_mode_changed_cb), NULL);    
    g_signal_connect(G_OBJECT(catalog_selection), "changed",
        G_CALLBACK(rc_ui_menu_catalog_listview_selection_changed_cb), NULL);
    g_signal_connect(G_OBJECT(playlist_selection), "changed",
        G_CALLBACK(rc_ui_menu_playlist_listview_selection_changed_cb), NULL);
}

/**
 * rc_ui_menu_get_ui_manager:
 *
 * Get the UI Manager of the menus.
 *
 * Returns: The UI Manager object.
 */

GtkUIManager *rc_ui_menu_get_ui_manager()
{
    return ui_manager;
}

/**
 * rc_ui_menu_state_refresh:
 *
 * Refresh the state of the menus.
 */
 
void rc_ui_menu_state_refresh()
{
    gdouble volume;
    GtkTreeSelection *catalog_selection, *playlist_selection;
    catalog_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        rc_ui_listview_get_catalog_widget()));
    playlist_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        rc_ui_listview_get_playlist_widget()));
    rc_ui_menu_catalog_listview_selection_changed_cb(catalog_selection,
        NULL);
    rc_ui_menu_playlist_listview_selection_changed_cb(playlist_selection,
        NULL);
    if(rclib_core_get_volume(&volume))
        rc_ui_menu_volume_changed_cb(NULL, volume, NULL);
}

