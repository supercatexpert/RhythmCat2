/*
 * RhythmCat UI Settings Module
 * The setting manager for the player.
 *
 * rc-ui-settings.c
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

#include "rc-ui-settings.h"
#include "rc-common.h"
#include "rc-ui-style.h"
#include "rc-ui-player.h"
#include "rc-ui-listmodel.h"
#include "rc-ui-listview.h"
#include "rc-ui-window.h"
#include "rc-ui-dialog.h"

/**
 * SECTION: rc-ui-settings
 * @Short_description: Player Configuration UI
 * @Title: Settings UI
 * @Include: rc-ui-settings.h
 *
 * The player configuation UI module, show player configuration panel.
 */

typedef struct RCUiSettingsPrivate
{
    GtkWidget *settings_window;
    GtkWidget *settings_notebook;
    GtkWidget *gen_audioout_combo_box;
    GtkWidget *gen_audioout_label;
    GtkWidget *gen_autoplay_check_button;
    GtkWidget *gen_loadlast_check_button;
    GtkWidget *gen_mintray_check_button;
    GtkWidget *gen_minclose_check_button;
    GtkWidget *pl_autoenc_check_button;
    GtkWidget *pl_id3enc_entry;
    GtkWidget *pl_lrcenc_entry;
    GtkWidget *pl_ldlgc_button;
    GtkWidget *apr_disthm_check_button;
    GtkWidget *apr_theme_combo_box;
    GtkWidget *if_hidecovimg_check_button;
    GtkWidget *if_hidelrc_check_button;
    GtkWidget *if_hidespr_check_button;
    GtkWidget *if_multicolumn_check_button;
    GtkWidget *if_titleformat_entry;
    GtkWidget *if_titleformat_grid;
    GtkWidget *if_showcolumn_frame;
    GtkWidget *if_showarcolumn_check_button;
    GtkWidget *if_showalcolumn_check_button;
    GtkWidget *if_showtrcolumn_check_button;
    GtkWidget *if_showyecolumn_check_button;
    GtkWidget *if_showftcolumn_check_button;
    GtkWidget *if_showrtcolumn_check_button;
    GtkWidget *if_showgecolumn_check_button;
}RCUiSettingsPrivate;

static RCUiSettingsPrivate settings_priv = {0};

static void rc_ui_settings_close_button_clicked(GtkButton *button,
    gpointer data)
{
    RCUiSettingsPrivate *priv = &settings_priv;
    gtk_widget_destroy(priv->settings_window);
}

static void rc_ui_settings_window_destroy_cb(GtkWidget *widget, gpointer data)
{
    RCUiSettingsPrivate *priv = &settings_priv;
    gtk_widget_destroyed(priv->settings_window, &(priv->settings_window));
}

static void rc_ui_settings_gen_autoplay_toggled(GtkToggleButton *button,
    gpointer data)
{
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("Player", "AutoPlayWhenStartup", flag);
}

static void rc_ui_settings_gen_loadlast_toggled(GtkToggleButton *button,
    gpointer data)
{
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("Player", "LoadLastPosition", flag);
}

static void rc_ui_settings_gen_mintray_toggled(GtkToggleButton *button,
    gpointer data)
{
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("MainUI", "MinimizeToTray", flag);
}

static void rc_ui_settings_gen_minclose_toggled(GtkToggleButton *button,
    gpointer data)
{
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("MainUI", "MinimizeWhenClose", flag);
}

static void rc_ui_settings_gen_audioout_changed(GtkComboBox *widget,
    gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint output_type;
    RCUiSettingsPrivate *priv = &settings_priv;
    model = gtk_combo_box_get_model(widget);
    if(model==NULL) return;
    if(!gtk_combo_box_get_active_iter(widget, &iter)) return;
    gtk_tree_model_get(model, &iter, 1, &output_type, -1);
    if(rclib_core_audio_output_set((RCLibCoreAudioOutputType)output_type))
    {
        gtk_label_set_markup(GTK_LABEL(priv->gen_audioout_label),
            _("Audio Output Plug-in changed successfully."));
        rclib_settings_set_integer("Player", "AudioOutputPluginType",
            output_type);
    }
    else
    {
        gtk_label_set_markup(GTK_LABEL(priv->gen_audioout_label),
            _("Failed to change Audio Output Plug-in!"));
    }
}

static inline GtkWidget *rc_ui_settings_general_build(
    RCUiSettingsPrivate *priv)
{
    GtkWidget *general_grid;
    GtkWidget *frame_label;
    GtkWidget *general_frame;
    GtkWidget *general_frame_grid;
    GtkWidget *audioout_frame;
    GtkWidget *audioout_frame_grid;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeIter iter;
    gint audioout_type;
    general_grid = gtk_grid_new();
    priv->gen_autoplay_check_button = gtk_check_button_new_with_mnemonic(
        _("_Auto play on startup"));
    priv->gen_loadlast_check_button = gtk_check_button_new_with_mnemonic(
        _("_Load the last playing position"));
    priv->gen_mintray_check_button = gtk_check_button_new_with_mnemonic(
        _("Minimize to _tray"));
    priv->gen_minclose_check_button = gtk_check_button_new_with_mnemonic(
        _("Minimize the window if the _close button is clicked"));
    priv->gen_audioout_label = gtk_label_new(NULL);
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    renderer = gtk_cell_renderer_text_new();
    priv->gen_audioout_combo_box = gtk_combo_box_new_with_model(
        GTK_TREE_MODEL(store));
    g_object_unref(store);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->gen_audioout_combo_box),
        renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(
        priv->gen_audioout_combo_box), renderer, "text", 0, NULL);   
    gtk_label_set_markup(GTK_LABEL(priv->gen_audioout_label),
        _("<b>Hint</b>: Change the audio output plug-in will "
        "<b>stop</b> playing."));
    g_object_set(priv->gen_audioout_label, "wrap", TRUE, "wrap-mode",
        PANGO_WRAP_WORD_CHAR, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    general_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), _("<b>General</b>"));
    g_object_set(general_frame, "label-widget", frame_label, "shadow-type",
        GTK_SHADOW_NONE, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    general_frame_grid = gtk_grid_new();
    audioout_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label),
        _("<b>Audio Output Plug-in</b>"));
    g_object_set(audioout_frame, "label-widget", frame_label, "shadow-type",
        GTK_SHADOW_NONE, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    audioout_frame_grid = gtk_grid_new();
    g_object_set(priv->gen_audioout_combo_box, "margin-left", 2,
        "margin-right", 2, "margin-top", 2, "margin-bottom", 2, "hexpand-set",
        TRUE, "hexpand", TRUE, NULL);
    g_object_set(priv->gen_autoplay_check_button, "active",
        rclib_settings_get_boolean("Player", "AutoPlayWhenStartup", NULL),
        NULL);
    g_object_set(priv->gen_loadlast_check_button, "active",
        rclib_settings_get_boolean("Player", "LoadLastPosition", NULL),
        NULL); 
    g_object_set(priv->gen_mintray_check_button, "active",
        rclib_settings_get_boolean("MainUI", "MinimizeToTray", NULL),
        NULL);
    g_object_set(priv->gen_minclose_check_button, "active",
        rclib_settings_get_boolean("MainUI", "MinimizeWhenClose", NULL),
        NULL);
    audioout_type = rclib_settings_get_integer("Player",
        "AudioOutputPluginType", NULL);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Automatic"), 1,
        RCLIB_CORE_AUDIO_OUTPUT_AUTO, -1);
    if(audioout_type<RCLIB_CORE_AUDIO_OUTPUT_PULSEAUDIO ||
        audioout_type>RCLIB_CORE_AUDIO_OUTPUT_WAVEFORM)
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
            priv->gen_audioout_combo_box), &iter);
    }
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("PulseAudio"), 1,
        RCLIB_CORE_AUDIO_OUTPUT_PULSEAUDIO, -1);
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("ALSA"), 1,
        RCLIB_CORE_AUDIO_OUTPUT_ALSA, -1);
    if(audioout_type==RCLIB_CORE_AUDIO_OUTPUT_ALSA)
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
            priv->gen_audioout_combo_box), &iter);
    }
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("OSS"), 1,
        RCLIB_CORE_AUDIO_OUTPUT_OSS, -1);
    if(audioout_type==RCLIB_CORE_AUDIO_OUTPUT_OSS)
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
            priv->gen_audioout_combo_box), &iter);
    }
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Jack"), 1,
        RCLIB_CORE_AUDIO_OUTPUT_JACK, -1);
    if(audioout_type==RCLIB_CORE_AUDIO_OUTPUT_JACK)
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
            priv->gen_audioout_combo_box), &iter);
    }
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, 0, _("Waveform"), 1,
        RCLIB_CORE_AUDIO_OUTPUT_WAVEFORM, -1);
    if(audioout_type==RCLIB_CORE_AUDIO_OUTPUT_WAVEFORM)
    {
        gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
            priv->gen_audioout_combo_box), &iter);
    }
    gtk_grid_attach(GTK_GRID(general_frame_grid),
        priv->gen_autoplay_check_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(general_frame_grid),
        priv->gen_loadlast_check_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(general_frame_grid),
        priv->gen_mintray_check_button, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(general_frame_grid),
        priv->gen_minclose_check_button, 0, 3, 1, 1);
    gtk_container_add(GTK_CONTAINER(general_frame), general_frame_grid);
    gtk_grid_attach(GTK_GRID(audioout_frame_grid),
        priv->gen_audioout_combo_box, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(audioout_frame_grid),
        priv->gen_audioout_label, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(audioout_frame), audioout_frame_grid);
    gtk_grid_attach(GTK_GRID(general_grid), audioout_frame, 0, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(general_grid), general_frame, 0, 1,
        1, 1);
    g_signal_connect(priv->gen_autoplay_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_gen_autoplay_toggled), NULL);
    g_signal_connect(priv->gen_loadlast_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_gen_loadlast_toggled), NULL);
    g_signal_connect(priv->gen_mintray_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_gen_mintray_toggled), NULL);
    g_signal_connect(priv->gen_minclose_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_gen_minclose_toggled), NULL);
    g_signal_connect(priv->gen_audioout_combo_box, "changed",
        G_CALLBACK(rc_ui_settings_gen_audioout_changed), NULL);
    return general_grid;
}

static void rc_ui_settings_pl_autoenc_toggled(GtkToggleButton *button,
    gpointer data)
{
    RCUiSettingsPrivate *priv = &settings_priv;
    gboolean flag = gtk_toggle_button_get_active(button);
    gchar *encoding, *id3_encoding;
    rclib_settings_set_boolean("Metadata", "AutoDetectEncoding", flag);
    g_object_set(priv->pl_id3enc_entry, "sensitive", !flag, NULL);
    g_object_set(priv->pl_lrcenc_entry, "sensitive", !flag, NULL);
    if(flag)
    {
        encoding = rclib_util_detect_encoding_by_locale();
        if(encoding!=NULL && strlen(encoding)>0)
        {
            id3_encoding = g_strdup_printf("%s:UTF-8", encoding);
            rclib_lyric_set_fallback_encoding(encoding);
            rclib_settings_set_string("Metadata", "LyricEncoding", encoding);
            gtk_entry_set_text(GTK_ENTRY(priv->pl_lrcenc_entry), encoding);
            rclib_tag_set_fallback_encoding(id3_encoding);
            rclib_settings_set_string("Metadata", "ID3Encoding",
                id3_encoding);
            gtk_entry_set_text(GTK_ENTRY(priv->pl_id3enc_entry),
                id3_encoding);
            g_free(id3_encoding);
        }
        g_free(encoding);
    }    
}

static void rc_ui_settings_pl_id3enc_changed(GtkEditable *editable,
    gpointer data)
{
    const gchar *text;
    text = gtk_entry_get_text(GTK_ENTRY(editable));
    rclib_settings_set_string("Metadata", "ID3Encoding", text);
}

static void rc_ui_settings_pl_lrcenc_changed(GtkEditable *editable,
    gpointer data)
{
    const gchar *text;
    text = gtk_entry_get_text(GTK_ENTRY(editable));
    rclib_settings_set_string("Metadata", "LyricEncoding", text);
}

static inline GtkWidget *rc_ui_settings_playlist_build(
    RCUiSettingsPrivate *priv)
{
    GtkWidget *playlist_grid;
    GtkWidget *frame_label;
    GtkWidget *metadata_frame;
    GtkWidget *metadata_frame_grid;
    GtkWidget *metadata_id3enc_label;
    GtkWidget *metadata_lrcenc_label;
    GtkWidget *legacy_frame;
    GtkWidget *legacy_frame_grid;
    GtkWidget *legacy_button_box;
    gchar *string;
    playlist_grid = gtk_grid_new();    
    priv->pl_autoenc_check_button = gtk_check_button_new_with_mnemonic(
        _("_Auto encoding detect (use system language settings)"));
    priv->pl_id3enc_entry = gtk_entry_new();
    priv->pl_lrcenc_entry = gtk_entry_new();
    priv->pl_ldlgc_button = gtk_button_new_with_mnemonic(
        _("Load playlist from _legacy version"));
    metadata_id3enc_label = gtk_label_new(
        _("ID3 Tag fallback character encodings"));
    metadata_lrcenc_label = gtk_label_new(
        _("Lyric text fallback character encodings"));
    metadata_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), _("<b>Metadata</b>"));
    g_object_set(metadata_frame, "label-widget", frame_label, "shadow-type",
        GTK_SHADOW_NONE, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    metadata_frame_grid = gtk_grid_new();
    g_object_set(priv->pl_id3enc_entry, "margin-left", 2, "margin-right",
        2, "margin-top", 2, "margin-bottom", 2, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    g_object_set(priv->pl_lrcenc_entry, "margin-left", 2, "margin-right",
        2, "margin-top", 2, "margin-bottom", 2, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    legacy_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL); 
    gtk_label_set_markup(GTK_LABEL(frame_label), _("<b>Legacy Support</b>"));
    g_object_set(legacy_frame, "label-widget", frame_label, "shadow-type",
        GTK_SHADOW_NONE, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    legacy_frame_grid = gtk_grid_new();
    legacy_button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_set(legacy_button_box, "margin-left", 2, "margin-right",
        2, "margin-top", 2, "margin-bottom", 2, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL, "layout-style", GTK_BUTTONBOX_CENTER,
        "spacing", 5, NULL);   
    if(rclib_settings_get_boolean("Metadata", "AutoDetectEncoding", NULL))
    {
        g_object_set(priv->pl_autoenc_check_button, "active", TRUE, NULL);
        g_object_set(priv->pl_id3enc_entry, "sensitive", FALSE, NULL);
        g_object_set(priv->pl_lrcenc_entry, "sensitive", FALSE, NULL);
    }
    string = rclib_settings_get_string("Metadata", "ID3Encoding", NULL);
    if(string!=NULL)
        gtk_entry_set_text(GTK_ENTRY(priv->pl_id3enc_entry), string);
    g_free(string);
    string = rclib_settings_get_string("Metadata", "LyricEncoding", NULL);
    if(string!=NULL)
        gtk_entry_set_text(GTK_ENTRY(priv->pl_lrcenc_entry), string);
    g_free(string);
    gtk_grid_attach(GTK_GRID(metadata_frame_grid),
        priv->pl_autoenc_check_button, 0, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(metadata_frame_grid),
        metadata_id3enc_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(metadata_frame_grid),
        priv->pl_id3enc_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(metadata_frame_grid),
        metadata_lrcenc_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(metadata_frame_grid),
        priv->pl_lrcenc_entry, 1, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(metadata_frame), metadata_frame_grid);
    gtk_box_pack_start(GTK_BOX(legacy_button_box), priv->pl_ldlgc_button,
        FALSE, FALSE, 2);
    gtk_grid_attach(GTK_GRID(legacy_frame_grid), legacy_button_box,
        0, 0, 2, 1);
    gtk_container_add(GTK_CONTAINER(legacy_frame), legacy_frame_grid);
    gtk_grid_attach(GTK_GRID(playlist_grid), metadata_frame, 0, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(playlist_grid), legacy_frame, 0, 1,
        1, 1);
    g_signal_connect(priv->pl_autoenc_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_pl_autoenc_toggled), NULL);        
    g_signal_connect(priv->pl_id3enc_entry, "changed",
        G_CALLBACK(rc_ui_settings_pl_id3enc_changed), NULL);
    g_signal_connect(priv->pl_lrcenc_entry, "changed",
        G_CALLBACK(rc_ui_settings_pl_lrcenc_changed), NULL);
    g_signal_connect(priv->pl_ldlgc_button, "clicked",
        G_CALLBACK(rc_ui_dialog_show_load_legacy), NULL);
    return playlist_grid;
}

static void rc_ui_settings_apr_theme_changed(GtkComboBox *widget,
    gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *theme;
    gchar *theme_settings;
    gchar *theme_file;
    gboolean embedded_flag;
    gboolean theme_flag = FALSE;
    model = gtk_combo_box_get_model(widget);
    if(model==NULL) return;
    if(!gtk_combo_box_get_active_iter(widget, &iter)) return;
    gtk_tree_model_get(model, &iter, 1, &embedded_flag, 2, &theme, -1);
    if(embedded_flag)
    {
        theme_settings = g_strdup_printf("embedded-theme:%s", theme);
        theme_flag = rc_ui_style_embedded_theme_set_by_name(theme);
        if(theme_flag)
            rclib_settings_set_string("MainUI", "Theme", theme_settings);
        g_free(theme_settings);
    }
    else
    {
        theme_file = g_build_filename(theme, "gtk3.css", NULL);
        theme_flag = rc_ui_style_css_set_file(theme_file);
        g_free(theme_file);
        if(theme_flag)
            rclib_settings_set_string("MainUI", "Theme", theme);
    }
    g_free(theme);
    
}

static void rc_ui_settings_apr_disthm_toggled(GtkToggleButton *button,
    gpointer data)
{
    RCUiSettingsPrivate *priv = &settings_priv;
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("MainUI", "DisableTheme", flag);
    g_object_set(priv->apr_theme_combo_box, "sensitive", !flag, NULL);
}

static inline GtkWidget *rc_ui_settings_appearance_build(
    RCUiSettingsPrivate *priv)
{
    GtkWidget *appearance_grid;
    GtkWidget *frame_label;
    GtkWidget *theme_frame;
    GtkWidget *theme_frame_grid;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeIter iter;
    GSList *theme_list, *foreach;
    const gchar *embedded_theme_name;
    guint theme_number;
    guint i;
    const gchar *path;
    gchar *theme_name;
    gchar *theme_settings;
    gboolean theme_embedded_flag = FALSE;
    appearance_grid = gtk_grid_new();
    theme_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL);
    store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_BOOLEAN,
        G_TYPE_STRING);
    renderer = gtk_cell_renderer_text_new();
    priv->apr_disthm_check_button = gtk_check_button_new_with_mnemonic(
        _("Disable theme (need to restart the player)"));
    priv->apr_theme_combo_box = gtk_combo_box_new_with_model(
        GTK_TREE_MODEL(store));
    g_object_unref(store);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->apr_theme_combo_box),
        renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(priv->apr_theme_combo_box),
        renderer, "text", 0, NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), _("<b>Theme</b>"));
    g_object_set(theme_frame, "label-widget", frame_label, "shadow-type",
        GTK_SHADOW_NONE, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    theme_frame_grid = gtk_grid_new();
    g_object_set(priv->apr_theme_combo_box, "margin-left", 2, "margin-right",
        2, "margin-top", 2, "margin-bottom", 2, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    theme_settings = rclib_settings_get_string("MainUI", "Theme", NULL);
    if(theme_settings!=NULL)
    {
        theme_embedded_flag = g_str_has_prefix(theme_settings,
            "embedded-theme:");
    }
    theme_number = rc_ui_style_embedded_theme_get_length();
    for(i=0;i<theme_number;i++)
    {
        embedded_theme_name = rc_ui_style_embedded_theme_get_name(i);
        theme_name = g_strdup_printf(_("%s (Embedded)"),
            embedded_theme_name);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, theme_name, 1, TRUE, 2,
            embedded_theme_name, -1);
        g_free(theme_name);
        if(theme_embedded_flag)
        {
            if(g_strcmp0(theme_settings+14, embedded_theme_name)==0)
            {
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
                    priv->apr_theme_combo_box), &iter);
            }
        }
    }
    theme_list = rc_ui_style_search_theme_paths();
    for(foreach=theme_list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        path = foreach->data;
        if(path==NULL) continue;
        theme_name = g_path_get_basename(path);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, theme_name, 1, FALSE, 2, path,
            -1);
        g_free(theme_name);
        if(g_strcmp0(theme_settings, path)==0)
        {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
                priv->apr_theme_combo_box), &iter);
        }
    }
    if(theme_list!=NULL)
        g_slist_free_full(theme_list, g_free);
    g_free(theme_settings);
    if(gtk_combo_box_get_active(GTK_COMBO_BOX(priv->apr_theme_combo_box))<0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(priv->apr_theme_combo_box), 0);
    if(rclib_settings_get_boolean("MainUI", "DisableTheme", NULL))
    {
        g_object_set(priv->apr_theme_combo_box, "sensitive", FALSE, NULL);
        g_object_set(priv->apr_disthm_check_button, "active", TRUE, NULL);
    }
    gtk_grid_attach(GTK_GRID(theme_frame_grid),
        priv->apr_disthm_check_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(theme_frame_grid),
        priv->apr_theme_combo_box, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(theme_frame), theme_frame_grid);
    gtk_grid_attach(GTK_GRID(appearance_grid), theme_frame, 0, 0,
        1, 1);
    g_signal_connect(priv->apr_theme_combo_box, "changed",
        G_CALLBACK(rc_ui_settings_apr_theme_changed), NULL);
    g_signal_connect(priv->apr_disthm_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_apr_disthm_toggled), NULL);
    return appearance_grid;
}

static void rc_ui_settings_if_hidecovimg_toggled(GtkToggleButton *button,
    gpointer data)
{
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("MainUI", "HideCoverImage", flag);
    rc_ui_main_window_cover_image_set_visible(!flag);
}

static void rc_ui_settings_if_hidelrc_toggled(GtkToggleButton *button,
    gpointer data)
{
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("MainUI", "HideLyricLabels", flag);
    rc_ui_main_window_lyric_labels_set_visible(!flag);
}

static void rc_ui_settings_if_hidespr_toggled(GtkToggleButton *button,
    gpointer data)
{
    gboolean flag = gtk_toggle_button_get_active(button);
    rclib_settings_set_boolean("MainUI", "HideSpectrumWidget", flag);
    rc_ui_main_window_spectrum_set_visible(!flag);
}

static void rc_ui_settings_if_multicolumn_toggled(GtkToggleButton *button,
    gpointer data)
{
    RCUiSettingsPrivate *priv = (RCUiSettingsPrivate *)data;
    gboolean flag;
    gboolean column_flag;
    guint column_flags = 0;
    gchar *string;
    if(data==NULL) return;
    flag = gtk_toggle_button_get_active(button);
    rc_ui_listview_playlist_set_column_display_mode(flag);
    rclib_settings_set_boolean("MainUI", "UseMultiColumns", flag);
    if(flag)
    {
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnArtistEnabled", NULL);
        g_object_set(priv->if_showarcolumn_check_button, "active",
             column_flag, NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnAlbumEnabled", NULL);
        g_object_set(priv->if_showalcolumn_check_button, "active",
             column_flag, NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnTrackNumberEnabled", NULL);
        g_object_set(priv->if_showtrcolumn_check_button, "active",
             column_flag, NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnYearEnabled", NULL);
        g_object_set(priv->if_showyecolumn_check_button, "active",
             column_flag, NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnFileTypeEnabled", NULL);
        g_object_set(priv->if_showftcolumn_check_button, "active",
             column_flag, NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnRatingEnabled", NULL);
        g_object_set(priv->if_showrtcolumn_check_button, "active",
             column_flag, NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnGenreEnabled", NULL);
        g_object_set(priv->if_showgecolumn_check_button, "active",
             column_flag, NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_GENRE;    
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_GENRE,
            column_flags);
        g_object_set(priv->if_multicolumn_check_button, "active", TRUE,
            NULL);
        g_object_set(priv->if_titleformat_grid, "visible", FALSE, NULL);
        g_object_set(priv->if_showcolumn_frame, "visible", TRUE, NULL);
    }
    else
    {
        string = rclib_settings_get_string("MainUI", "PlaylistTitleFormat",
            NULL);
        if(string!=NULL && g_strstr_len(string, -1, "%TITLE")!=NULL)
        {
            rc_ui_listview_playlist_set_title_format(string);
            gtk_entry_set_text(GTK_ENTRY(priv->if_titleformat_entry),
                string);
        }
        g_free(string);
        g_object_set(priv->if_titleformat_grid, "visible", TRUE, NULL);
        g_object_set(priv->if_showcolumn_frame, "visible", FALSE, NULL);
    }
}

static void rc_ui_settings_if_titleformat_changed(GtkEditable *editable,
    gpointer data)
{
    const gchar *text;
    if(rc_ui_listview_playlist_get_column_display_mode()) return;
    text = gtk_entry_get_text(GTK_ENTRY(editable));
    if(text==NULL || g_strstr_len(text, -1, "%TITLE")==NULL) return;
    rc_ui_listview_playlist_set_title_format(text);
    rclib_settings_set_string("MainUI", "PlaylistTitleFormat", text);
}

static void rc_ui_settings_if_showcolumn_toggled(GtkToggleButton *button,
    gpointer data)
{
    RCUiSettingsPrivate *priv = (RCUiSettingsPrivate *)data;
    gboolean flag;
    guint column_flags;
    if(data==NULL) return;
    if(!rc_ui_listview_playlist_get_column_display_mode()) return;
    flag = gtk_toggle_button_get_active(button);
    if(button==(GtkToggleButton *)priv->if_showarcolumn_check_button)
    {
        if(flag)
            column_flags = RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST;
        else
            column_flags = 0;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST, column_flags);
        rclib_settings_set_boolean("MainUI", "PlaylistColumnArtistEnabled",
            flag);
    }
    else if(button==(GtkToggleButton *)priv->if_showalcolumn_check_button)
    {
        if(flag)
            column_flags = RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM;
        else
            column_flags = 0;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM, column_flags);
        rclib_settings_set_boolean("MainUI", "PlaylistColumnAlbumEnabled",
            flag);
    }
    else if(button==(GtkToggleButton *)priv->if_showtrcolumn_check_button)
    {
        if(flag)
            column_flags = RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK;
        else
            column_flags = 0;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK, column_flags);
        rclib_settings_set_boolean("MainUI",
           "PlaylistColumnTrackNumberEnabled", flag);
    }
    else if(button==(GtkToggleButton *)priv->if_showyecolumn_check_button)
    {
        if(flag)
            column_flags = RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR;
        else
            column_flags = 0;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR, column_flags);
        rclib_settings_set_boolean("MainUI", "PlaylistColumnYearEnabled",
            flag);
    }
    else if(button==(GtkToggleButton *)priv->if_showftcolumn_check_button)
    {
        if(flag)
            column_flags = RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE;
        else
            column_flags = 0;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE, column_flags);
        rclib_settings_set_boolean("MainUI",
           "PlaylistColumnFileTypeEnabled", flag);
    }
    else if(button==(GtkToggleButton *)priv->if_showrtcolumn_check_button)
    {
        if(flag)
            column_flags = RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING;
        else
            column_flags = 0;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING, column_flags);
        rclib_settings_set_boolean("MainUI",
           "PlaylistColumnRatingEnabled", flag);
    }
    else if(button==(GtkToggleButton *)priv->if_showgecolumn_check_button)
    {
        if(flag)
            column_flags = RC_UI_LISTVIEW_PLAYLIST_COLUMN_GENRE;
        else
            column_flags = 0;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_GENRE, column_flags);
        rclib_settings_set_boolean("MainUI",
           "PlaylistColumnGenreEnabled", flag);
    }
}

static inline GtkWidget *rc_ui_settings_interface_build(
    RCUiSettingsPrivate *priv)
{
    GtkWidget *interface_grid;
    GtkWidget *frame_label;
    GtkWidget *mainwin_frame;
    GtkWidget *mainwin_frame_grid;
    GtkWidget *listview_frame;
    GtkWidget *listview_frame_grid;
    GtkWidget *titleformat_label;
    GtkWidget *note_label;
    GtkWidget *showcolumn_frame_grid;
    guint column_flags;
    interface_grid = gtk_grid_new();
    priv->if_hidecovimg_check_button = gtk_check_button_new_with_mnemonic(
        _("Hide cover _image"));
    priv->if_hidelrc_check_button = gtk_check_button_new_with_mnemonic(
        _("Hide _lyric labels")); 
    priv->if_hidespr_check_button = gtk_check_button_new_with_mnemonic(
        _("Hide _spectrum show"));
    priv->if_multicolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Show _metadata in multi-columns"));
    priv->if_titleformat_entry = gtk_entry_new();
    priv->if_showarcolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Artist"));
    priv->if_showalcolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Album"));
    priv->if_showtrcolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Track"));
    priv->if_showyecolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Year"));
    priv->if_showftcolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Format"));
    priv->if_showrtcolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Rating"));
    priv->if_showgecolumn_check_button = gtk_check_button_new_with_mnemonic(
        _("Genre"));
    priv->if_titleformat_grid = gtk_grid_new();
    showcolumn_frame_grid = gtk_grid_new();
    priv->if_showcolumn_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label),
        _("<b>Playlist Visible Columns</b>"));
    g_object_set(priv->if_showcolumn_frame, "label-widget", frame_label,
        "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    titleformat_label = gtk_label_new(_("Title column format: "));
    note_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(note_label), _("<b>Hint</b>: Use "
        "<i>%TITLE</i> as title string, <i>%ARTIST</i> as artist string, "
        "<i>%ALBUM</i> as album string, somehow <i>%TITLE</i> must be "
        "included in the format string."));
    g_object_set(note_label, "wrap", TRUE, "wrap-mode", PANGO_WRAP_WORD_CHAR,
        "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    g_object_set(priv->if_titleformat_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->if_titleformat_grid, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->if_showcolumn_frame, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    gtk_grid_attach(GTK_GRID(priv->if_titleformat_grid), titleformat_label,
        0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(priv->if_titleformat_grid),
        priv->if_titleformat_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(priv->if_titleformat_grid), note_label,
        0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(showcolumn_frame_grid),
        priv->if_showarcolumn_check_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(showcolumn_frame_grid),
        priv->if_showalcolumn_check_button, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(showcolumn_frame_grid),
        priv->if_showtrcolumn_check_button, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(showcolumn_frame_grid),
        priv->if_showgecolumn_check_button, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(showcolumn_frame_grid),
        priv->if_showyecolumn_check_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(showcolumn_frame_grid),
        priv->if_showftcolumn_check_button, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(showcolumn_frame_grid),
        priv->if_showrtcolumn_check_button, 2, 1, 1, 1); 
    gtk_container_add(GTK_CONTAINER(priv->if_showcolumn_frame),
        showcolumn_frame_grid);
    gtk_widget_show_all(priv->if_titleformat_grid);
    gtk_widget_show_all(priv->if_showcolumn_frame);
    gtk_widget_set_no_show_all(priv->if_titleformat_grid, TRUE);
    gtk_widget_set_no_show_all(priv->if_showcolumn_frame, TRUE);
    mainwin_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), _("<b>Main Window</b>"));
    g_object_set(mainwin_frame, "label-widget", frame_label, "shadow-type",
        GTK_SHADOW_NONE, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    mainwin_frame_grid = gtk_grid_new();
    listview_frame = gtk_frame_new(NULL);
    frame_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(frame_label), _("<b>List Views</b>"));
    g_object_set(listview_frame, "label-widget", frame_label, "shadow-type",
        GTK_SHADOW_NONE, "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    listview_frame_grid = gtk_grid_new();
    g_object_set(priv->if_hidecovimg_check_button, "active",
        rclib_settings_get_boolean("MainUI", "HideCoverImage", NULL),
        NULL);
    g_object_set(priv->if_hidelrc_check_button, "active",
        rclib_settings_get_boolean("MainUI", "HideLyricLabels", NULL),
        NULL);
    g_object_set(priv->if_hidespr_check_button, "active",
        rclib_settings_get_boolean("MainUI", "HideSpectrumWidget", NULL),
        NULL);
    g_object_set(priv->if_titleformat_entry, "text",
        rc_ui_list_model_get_playlist_title_format(), NULL);
    column_flags = rc_ui_listview_playlist_get_enabled_columns();
    g_object_set(priv->if_showarcolumn_check_button, "active",
        column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST ? TRUE: FALSE,
        NULL);
    g_object_set(priv->if_showalcolumn_check_button, "active",
        column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM ? TRUE: FALSE,
        NULL);
    g_object_set(priv->if_showtrcolumn_check_button, "active",
        column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK ? TRUE: FALSE,
        NULL);
    g_object_set(priv->if_showyecolumn_check_button, "active",
        column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR ? TRUE: FALSE,
        NULL);
    g_object_set(priv->if_showftcolumn_check_button, "active",
        column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE ? TRUE: FALSE,
        NULL);
    g_object_set(priv->if_showrtcolumn_check_button, "active",
        column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING ? TRUE: FALSE,
        NULL);
    g_object_set(priv->if_showgecolumn_check_button, "active",
        column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_GENRE ? TRUE: FALSE,
        NULL);
    if(rc_ui_listview_playlist_get_column_display_mode())
    {
        g_object_set(priv->if_multicolumn_check_button, "active", TRUE,
            NULL);
        g_object_set(priv->if_titleformat_grid, "visible", FALSE, NULL);    
    }
    else
    {
        g_object_set(priv->if_showcolumn_frame, "visible", FALSE, NULL);
    }
    gtk_grid_attach(GTK_GRID(mainwin_frame_grid),
        priv->if_hidecovimg_check_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(mainwin_frame_grid),
        priv->if_hidelrc_check_button, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(mainwin_frame_grid),
        priv->if_hidespr_check_button, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(listview_frame_grid),
        priv->if_multicolumn_check_button, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(listview_frame_grid),
        priv->if_titleformat_grid, 0, 1, 1, 1);    
    gtk_grid_attach(GTK_GRID(listview_frame_grid),
        priv->if_showcolumn_frame, 0, 2, 1, 1);   
    gtk_container_add(GTK_CONTAINER(mainwin_frame), mainwin_frame_grid);
    gtk_container_add(GTK_CONTAINER(listview_frame), listview_frame_grid);
    gtk_grid_attach(GTK_GRID(interface_grid), mainwin_frame, 0, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(interface_grid), listview_frame, 0, 1,
        1, 1);
    g_signal_connect(priv->if_hidecovimg_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_hidecovimg_toggled), NULL);
    g_signal_connect(priv->if_hidelrc_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_hidelrc_toggled), NULL);
    g_signal_connect(priv->if_hidespr_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_hidespr_toggled), NULL);
    g_signal_connect(priv->if_multicolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_multicolumn_toggled), priv);        
    g_signal_connect(priv->if_titleformat_entry, "changed",
        G_CALLBACK(rc_ui_settings_if_titleformat_changed), priv);
    g_signal_connect(priv->if_showarcolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_showcolumn_toggled), priv);
    g_signal_connect(priv->if_showalcolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_showcolumn_toggled), priv);
    g_signal_connect(priv->if_showtrcolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_showcolumn_toggled), priv);
    g_signal_connect(priv->if_showyecolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_showcolumn_toggled), priv);
    g_signal_connect(priv->if_showftcolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_showcolumn_toggled), priv);
    g_signal_connect(priv->if_showrtcolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_showcolumn_toggled), priv);
    g_signal_connect(priv->if_showgecolumn_check_button, "toggled",
        G_CALLBACK(rc_ui_settings_if_showcolumn_toggled), priv);
    return interface_grid;
}

static gboolean rc_ui_settings_window_key_press_cb(GtkWidget *widget,
    GdkEvent *event, gpointer data)
{
    guint keyval = event->key.keyval;
    RCUiSettingsPrivate *priv = &settings_priv;
    if(keyval==GDK_KEY_Escape)
        gtk_widget_destroy(priv->settings_window);
    return FALSE;
}

static inline void rc_ui_settings_window_init()
{
    RCUiSettingsPrivate *priv = &settings_priv;
    GtkWidget *close_button;
    GtkWidget *main_grid;
    GtkWidget *general_label;
    GtkWidget *playlist_label;
    GtkWidget *appearance_label;
    GtkWidget *interface_label;
    GtkWidget *button_hbox;
    GtkWidget *scrolled_window;
    priv->settings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    main_grid = gtk_grid_new();
    priv->settings_notebook = gtk_notebook_new();
    close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    general_label = gtk_label_new(_("General"));
    playlist_label = gtk_label_new(_("Playlist"));
    appearance_label = gtk_label_new(_("Appearance"));
    interface_label = gtk_label_new(_("Interface"));
    button_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_set(priv->settings_window, "title", _("Player Preferences"),
        "window-position", GTK_WIN_POS_CENTER, "has-resize-grip", FALSE,
        "default-width", 350, "default-height", 300, "icon-name",
        GTK_STOCK_PREFERENCES, "type-hint", GDK_WINDOW_TYPE_HINT_DIALOG,
        NULL);
    g_object_set(priv->settings_notebook, "margin-left", 2, "margin-right",
        2, "margin-top", 4, "margin-bottom", 4, "tab-pos", GTK_POS_LEFT,
        "expand", TRUE, NULL);
    g_object_set(button_hbox, "layout-style", GTK_BUTTONBOX_END,
        "hexpand-set", TRUE, "hexpand", TRUE, "spacing", 4,
        "margin-left", 4, "margin-right", 4, "margin-top", 4,
        "margin-bottom", 4, NULL);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(scrolled_window, "hscrollbar-policy", GTK_POLICY_NEVER,
        "vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
        rc_ui_settings_general_build(priv));
    gtk_notebook_append_page(GTK_NOTEBOOK(priv->settings_notebook),
        scrolled_window, general_label);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(scrolled_window, "hscrollbar-policy", GTK_POLICY_NEVER,
        "vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
        rc_ui_settings_playlist_build(priv));
    gtk_notebook_append_page(GTK_NOTEBOOK(priv->settings_notebook),
        scrolled_window, playlist_label);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(scrolled_window, "hscrollbar-policy", GTK_POLICY_NEVER,
        "vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
        rc_ui_settings_appearance_build(priv));
    gtk_notebook_append_page(GTK_NOTEBOOK(priv->settings_notebook),
        scrolled_window, appearance_label);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(scrolled_window, "hscrollbar-policy", GTK_POLICY_NEVER,
        "vscrollbar-policy", GTK_POLICY_AUTOMATIC, NULL);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),
        rc_ui_settings_interface_build(priv));
    gtk_notebook_append_page(GTK_NOTEBOOK(priv->settings_notebook),
        scrolled_window, interface_label);
    gtk_box_pack_start(GTK_BOX(button_hbox), close_button, FALSE, FALSE, 2);
    gtk_grid_attach(GTK_GRID(main_grid), priv->settings_notebook, 0, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), button_hbox, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(priv->settings_window), main_grid);
    g_signal_connect(close_button, "clicked",
        G_CALLBACK(rc_ui_settings_close_button_clicked), NULL);
    g_signal_connect(priv->settings_window, "key-press-event",
        G_CALLBACK(rc_ui_settings_window_key_press_cb), NULL);
    g_signal_connect(priv->settings_window, "destroy",
        G_CALLBACK(rc_ui_settings_window_destroy_cb), NULL);
    gtk_widget_show_all(priv->settings_window);
}

/**
 * rc_ui_plugin_window_show:
 *
 * Show a player configuration window.
 */

void rc_ui_settings_window_show()
{
    RCUiSettingsPrivate *priv = &settings_priv;
    if(priv->settings_window==NULL)
        rc_ui_settings_window_init();
    else
        gtk_window_present(GTK_WINDOW(priv->settings_window));
}

/**
 * rc_ui_settings_window_destroy:
 *
 * Destroy the player configuration window.
 */

void rc_ui_settings_window_destroy()
{
    RCUiSettingsPrivate *priv = &settings_priv;
    if(priv->settings_window!=NULL)
        gtk_widget_destroy(priv->settings_window);
}



