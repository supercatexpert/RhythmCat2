/*
 * RhythmCat UI Menu Module
 * The menus for the player.
 *
 * rc-ui-menu.c
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

#include "rc-ui-menu.h"
#include "rc-ui-listview.h"
#include "rc-ui-dialog.h"
#include "rc-ui-plugin.h"
#include "rc-ui-settings.h"
#include "rc-ui-effect.h"
#include "rc-common.h"
#include "rc-ui-player.h"

static GtkUIManager *ui_manager = NULL;
static GtkActionGroup *ui_actions = NULL;

static void rc_ui_menu_play_button_clicked_cb()
{
    GstState state;
    if(rclib_core_get_state(&state, NULL, 0))
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

static void rc_ui_menu_backward_clicked_cb()
{
    gint64 time = rclib_core_query_position();
    time -= 5 * GST_SECOND;
    if(time<0) time = 0;
    rclib_core_set_position(time);
}

static void rc_ui_menu_forward_clicked_cb()
{
    gint64 time = rclib_core_query_position();
    time += 5 * GST_SECOND;
    rclib_core_set_position(time);
}

static void rc_ui_menu_vol_up_clicked_cb()
{
    gdouble value;
    if(!rclib_core_get_volume(&value)) return;
    value+=0.05;
    if(value>1.0) value = 1.0;
    rclib_core_set_volume(value);
}

static void rc_ui_menu_vol_down_clicked_cb()
{
    gdouble value;
    if(!rclib_core_get_volume(&value)) return;
    value-=0.05;
    if(value<0.0) value = 0.0;
    rclib_core_set_volume(value);
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

static void rc_ui_menu_keep_above_clicked_cb(GtkAction *action,
    gpointer data)
{
    GtkWidget *main_window;
    gboolean active;
    main_window = rc_ui_player_get_main_window();
    if(main_window==NULL || action==NULL) return;
    active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
    g_signal_handlers_block_by_func(action,
        G_CALLBACK(rc_ui_menu_keep_above_clicked_cb), data);
    gtk_window_set_keep_above(GTK_WINDOW(main_window), active);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(
        gtk_ui_manager_get_action(ui_manager,
        "/RC2MenuBar/ViewMenu/ViewAlwaysOnTop")), active);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(
        gtk_ui_manager_get_action(ui_manager,
        "/TrayPopupMenu/TrayAlwaysOnTop")), active);
    g_signal_handlers_unblock_by_func(action,
        G_CALLBACK(rc_ui_menu_keep_above_clicked_cb), data);
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
    { "RhythmCatMenu", NULL, "_RhythmCat" },
    { "PlaylistMenu", NULL, N_("_Playlist") },
    { "ViewMenu", NULL, N_("_View") },
    { "ControlMenu", NULL, N_("_Control") },
    { "HelpMenu", NULL, N_("_Help") },
    { "RepeatMenu", NULL, N_("_Repeat") },
    { "RandomMenu", NULL, N_("Ran_dom") },
    { "RhythmCatOpenMusic", GTK_STOCK_OPEN,
      N_("Open _Music"), "<control>O",
      N_("Open and play the music file"),    
      G_CALLBACK(rc_ui_dialog_open_music) },
    { "RhythmCatOpenLocation", NULL,
      N_("Open _Location"), NULL,
      N_("Open a location and play"),
      G_CALLBACK(rc_ui_dialog_open_location) },
    { "RhythmCatPreferences", GTK_STOCK_PREFERENCES,
      N_("_Preferences"), "F8",
      N_("Configure the player"),
      G_CALLBACK(rc_ui_settings_window_show) },  
    { "RhythmCatPlugin", GTK_STOCK_EXECUTE,
      N_("Plu_gins"), "F11",
      N_("Configure plugins"),
      G_CALLBACK(rc_ui_plugin_window_show) },
    { "RhythmCatQuit", GTK_STOCK_QUIT,
      N_("_Quit"), "<control>Q",
      N_("Quit this player"),
      G_CALLBACK(rc_ui_player_exit) },
    { "PlaylistNewList", GTK_STOCK_NEW,
      N_("_New Playlist"), "<control>N",
      N_("Create a new playlist"),
      G_CALLBACK(rc_ui_listview_catalog_new_playlist) },
    { "PlaylistRenameList", GTK_STOCK_EDIT,
      N_("Re_name Playlist"), "F2",
      N_("Rename the playlist"),
      G_CALLBACK(rc_ui_listview_catalog_rename_playlist) },
    { "PlaylistRemoveList", GTK_STOCK_REMOVE,
      N_("R_emove Playlist"), NULL,
      N_("Remove the playlist"),
      G_CALLBACK(rc_ui_listview_catalog_delete_items) },
    { "PlaylistExportList", GTK_STOCK_SAVE,
      N_("_Export Playlist"), "<control>S",
      N_("Export music to a playlist"),
      G_CALLBACK(rc_ui_dialog_save_playlist) },
    { "PlaylistExportAll", GTK_STOCK_SAVE_AS,
      N_("Export _All Playlists"), NULL,
      N_("Export all playlists to a folder"),
      G_CALLBACK(rc_ui_dialog_save_all_playlist) },
    { "PlaylistImportMusic", GTK_STOCK_OPEN,
      N_("Import _Music"), "<control>I",
      N_("Import music file"),
      G_CALLBACK(rc_ui_dialog_add_music) },
    { "PlaylistImportList", GTK_STOCK_FILE,
      N_("Import From _Playlist"), NULL,
      N_("Import music from playlist"),
      G_CALLBACK(rc_ui_dialog_load_playlist) },
    { "PlaylistImportFolder", GTK_STOCK_DIRECTORY,
      N_("Import From _Folder"), NULL,
      N_("Import all music from folder"),
      G_CALLBACK(rc_ui_dialog_add_directory) },
    { "PlaylistRemoveMusic", GTK_STOCK_DELETE,
      N_("_Remove Music"), NULL,
      N_("Remove music from playlist"),
      G_CALLBACK(rc_ui_listview_playlist_delete_items) },
    { "PlaylistSelectAll", GTK_STOCK_SELECT_ALL,
      N_("Select _All"), "<control>A",
      N_("Select all music in the playlist"),
      G_CALLBACK(rc_ui_listview_playlist_select_all) },
    { "PlaylistRefreshList", GTK_STOCK_REFRESH,
      N_("Re_fresh Playlist"), "F5",
      N_("Refresh music information in the playlist"),
      G_CALLBACK(rc_ui_listview_playlist_refresh) },
    { "PlaylistBindLyric", NULL,
      N_("Bind _Lyric File"), NULL,
      N_("Bind lyric file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_lyric) },
    { "PlaylistBindAlbum", NULL,
      N_("Bind Al_bum File"), NULL,
      N_("Bind album file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_album) },
    { "ViewEffect", NULL,
      N_("_Audio Effects"), "<control>E",
      N_("Adjust the audio effects"),
      G_CALLBACK(rc_ui_effect_window_show) },      
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
      G_CALLBACK(rc_ui_menu_backward_clicked_cb) },
    { "ControlForward", GTK_STOCK_MEDIA_FORWARD,
      N_("_Forward"), "<control>Right",
      N_("Forward 5 seconds"),
      G_CALLBACK(rc_ui_menu_forward_clicked_cb) },
    { "ControlVolumeUp", GTK_STOCK_GO_UP,
      N_("_Increase Volume"), "<control>Up",
      N_("Increase the volume"),
      G_CALLBACK(rc_ui_menu_vol_up_clicked_cb) },
    { "ControlVolumeDown", GTK_STOCK_GO_DOWN,
      N_("_Decrease Volume"), "<control>Down",
      N_("Decrease the volume"),
      G_CALLBACK(rc_ui_menu_vol_down_clicked_cb) },
    { "HelpAbout", GTK_STOCK_ABOUT,
      N_("_About"), NULL,
      N_("About this player"),
      G_CALLBACK(rc_ui_dialog_about_player) },
    { "HelpReport", NULL,
      N_("_Bug Report"), NULL,
      N_("Report bugs"),
      G_CALLBACK(NULL) },
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
    { "PlaylistPImportMusic", GTK_STOCK_OPEN,
      N_("Import _Music"), NULL,
      N_("Import music file"),
      G_CALLBACK(rc_ui_dialog_add_music) },
    { "PlaylistPImportList", GTK_STOCK_FILE,
      N_("Import from _Playlist"), NULL,
      N_("Import music from playlist"),
      G_CALLBACK(rc_ui_dialog_load_playlist) },
    { "PlaylistPSelectAll", GTK_STOCK_SELECT_ALL,
      N_("Select _All"), NULL,
      N_("Select all music in the playlist"),
      G_CALLBACK(rc_ui_listview_playlist_select_all) },
    { "PlaylistPBindLyric", NULL,
      N_("Bind _Lyric File"), NULL,
      N_("Bind lyric file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_lyric) },
    { "PlaylistPBindAlbum", NULL,
      N_("Bind Al_bum File"), NULL,
      N_("Bind album file to the selected music"),
      G_CALLBACK(rc_ui_dialog_bind_album) },
    { "PlaylistPRemoveMusic", GTK_STOCK_REMOVE,
      N_("R_emove Music"), NULL,
      N_("Remove music from playlist"),
      G_CALLBACK(rc_ui_listview_playlist_delete_items) },
    { "PlaylistPRefreshList", GTK_STOCK_REFRESH,
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
      G_CALLBACK(rc_ui_player_exit) }
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
      G_CALLBACK(rc_ui_menu_keep_above_clicked_cb), FALSE },
    { "TrayAlwaysOnTop", GTK_STOCK_GOTO_TOP,
      N_("Always On _Top"), NULL,
      N_("Always on top"),
      G_CALLBACK(rc_ui_menu_keep_above_clicked_cb), FALSE }
};

static guint ui_menu_toogle_n_entries = G_N_ELEMENTS(ui_menu_toogle_entries);

static const gchar *ui_menu_info =
    "<ui>"
    "  <menubar name='RC2MenuBar'>"
    "    <menu action='RhythmCatMenu'>"
    "      <menuitem action='RhythmCatOpenMusic'/>"
    "      <menuitem action='RhythmCatOpenLocation'/>"
    "      <separator name='RhythmCatSep1'/>"
    "      <menuitem action='RhythmCatPreferences'/>"
    "      <menuitem action='RhythmCatPlugin'/>"
    "      <separator name='RhythmCatSep2'/>"
    "      <menuitem action='RhythmCatQuit'/>"
    "    </menu>"
    "    <menu action='PlaylistMenu'>"
    "      <menuitem action='PlaylistNewList'/>"
    "      <menuitem action='PlaylistRenameList'/>"
    "      <menuitem action='PlaylistRemoveList'/>"
    "      <menuitem action='PlaylistExportList'/>"
    "      <menuitem action='PlaylistExportAll'/>"
    "      <separator name='PlaylistSep1'/>"
    "      <menuitem action='PlaylistImportMusic'/>"
    "      <menuitem action='PlaylistImportList'/>"
    "      <menuitem action='PlaylistImportFolder'/>"
    "      <menuitem action='PlaylistRemoveMusic'/>"
    "      <menuitem action='PlaylistSelectAll'/>"
    "      <menuitem action='PlaylistRefreshList'/>"
    "      <separator name='PlaylistSep2'/>" 
    "      <menuitem action='PlaylistBindLyric'/>"
    "      <menuitem action='PlaylistBindAlbum'/>"
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
    "    <menu action='ViewMenu'>"
    "      <menuitem action='ViewEffect'/>"
    "      <separator name='ViewSep1'/>"
    "      <separator name='ViewSep2'/>"
    "      <menuitem action='ViewAlwaysOnTop'/>"
    "      <menuitem action='ViewMiniMode'/>"
    "    </menu>"
    "    <menu action='HelpMenu'>"
    "      <menuitem action='HelpReport'/>"
    "      <menuitem action='HelpAbout'/>"
    "      <menuitem action='HelpSupportedFormat'/>"
    "    </menu>"
    "  </menubar>"
    "  <popup action='CatalogPopupMenu'>"
    "    <menuitem action='CatalogNewList'/>"
    "    <menuitem action='CatalogRenameList'/>"
    "    <menuitem action='CatalogRemoveList'/>"
    "    <menuitem action='CatalogExportList'/>"
    "  </popup>"
    "  <popup action='PlaylistPopupMenu'>"
    "    <menuitem action='PlaylistPImportMusic'/>"
    "    <menuitem action='PlaylistPImportList'/>"
    "    <separator name='PlaylistSep1'/>"
    "    <menuitem action='PlaylistPSelectAll'/>"
    "    <menuitem action='PlaylistPBindLyric'/>"
    "    <menuitem action='PlaylistPBindAlbum'/>"
    "    <menuitem action='PlaylistPRemoveMusic'/>"
    "    <separator name='PlaylistSep2'/>"
    "    <menuitem action='PlaylistPRefreshList'/>"
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
            "/RC2MenuBar/ControlMenu/ControlBackward"), FALSE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlForward"), FALSE);
    }
    else
    {
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlBackward"), TRUE);
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlForward"), TRUE);
    }
    if(state==GST_STATE_PLAYING)
    {
        g_object_set(G_OBJECT(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlPlay")), "stock-id",
            GTK_STOCK_MEDIA_PAUSE, NULL);
        g_object_set(G_OBJECT(gtk_ui_manager_get_action(ui_manager,
            "/TrayPopupMenu/TrayPlay")), "stock-id",
            GTK_STOCK_MEDIA_PAUSE, NULL);
    }
    else
    {
        g_object_set(G_OBJECT(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlPlay")), "stock-id",
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
            "/RC2MenuBar/ControlMenu/ControlVolumeUp"), FALSE);
    else
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlVolumeUp"), TRUE);
    if(volume<0.0001)
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlVolumeDown"), FALSE);
    else
        gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ControlMenu/ControlVolumeDown"), TRUE);
}

static void rc_ui_menu_player_repeat_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRepeatMode mode, gpointer data)
{
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(
        gtk_ui_manager_get_action(ui_manager,
        "/RC2MenuBar/ControlMenu/RepeatMenu/RepeatNoRepeat")), mode);
}

static void rc_ui_menu_player_random_mode_changed_cb(RCLibPlayer *player,
    RCLibPlayerRandomMode mode, gpointer data)
{
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(
        gtk_ui_manager_get_action(ui_manager,
        "/RC2MenuBar/ControlMenu/RandomMenu/RandomNoRandom")), mode);
}

/**
 * rc_ui_menu_init:
 *
 * Initialize the menus.
 *
 * Returns: The #GtkUIManager
 */

GtkUIManager *rc_ui_menu_init()
{
    GError *error = NULL;
    gdouble volume;
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
        g_warning("Cannot load menu: %s", error->message);
        g_error_free(error);
        ui_manager = NULL;
        return NULL;
    }
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
        
    /* Seal the menus that are not available now */
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/RC2MenuBar/ViewMenu/ViewMiniMode"), FALSE);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/RC2MenuBar/HelpMenu/HelpReport"), FALSE);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/RC2MenuBar/HelpMenu/HelpSupportedFormat"), FALSE);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/TrayPopupMenu/TrayShowPlayer"), FALSE);
    gtk_action_set_sensitive(gtk_ui_manager_get_action(ui_manager,
        "/TrayPopupMenu/TrayModeSwitch"), FALSE);

    rclib_core_signal_connect("state-changed",
        G_CALLBACK(rc_ui_menu_core_state_changed_cb), NULL);
    rclib_core_signal_connect("volume-changed",
        G_CALLBACK(rc_ui_menu_volume_changed_cb), NULL);
    rclib_player_signal_connect("repeat-mode-changed",
        G_CALLBACK(rc_ui_menu_player_repeat_mode_changed_cb), NULL);
    rclib_player_signal_connect("random-mode-changed",
        G_CALLBACK(rc_ui_menu_player_random_mode_changed_cb), NULL);
    if(rclib_core_get_volume(&volume))
        rc_ui_menu_volume_changed_cb(NULL, volume, NULL);   
    return ui_manager;
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
 * rc_ui_menu_add_menu_action:
 * @action: the #GtkAction for the menu item
 * @path: the path to append to
 * @name: the name for the added UI element
 * @action_name: the name of the action to be proxied
 * @top: if TRUE, the UI element is added before its siblings,
 * otherwise it is added after its siblings
 *
 * Add a menu item to the main menu of the player.
 *
 * Returns: The merge ID of the menu item. (Should be used to remove
 * the menu when you do not need it.)
 */

guint rc_ui_menu_add_menu_action(GtkAction *action, const gchar *path,
    const gchar *name, const gchar *action_name, gboolean top)
{
    guint id;
    if(ui_manager==NULL || ui_actions==NULL || action==NULL || path==NULL ||
        name==NULL) return 0;
    id = gtk_ui_manager_new_merge_id(ui_manager);
    gtk_ui_manager_add_ui(ui_manager, id, path, name, action_name,
        GTK_UI_MANAGER_MENUITEM, top);
    gtk_action_group_add_action(ui_actions, action);
    return id;
}

/**
 * rc_ui_menu_remove_menu_action:
 * @action: the #GtkAction for the menu item
 * @id: the merge ID of the menu item
 *
 * Remove the menu item by the given #GtkAction and merge ID.
 */

void rc_ui_menu_remove_menu_action(GtkAction *action, guint id)
{
    if(ui_manager==NULL || ui_actions==NULL) return;
    if(action==NULL) return;
    if(id>0)
        gtk_ui_manager_remove_ui(ui_manager, id);
    gtk_action_group_remove_action(ui_actions, action);
}


