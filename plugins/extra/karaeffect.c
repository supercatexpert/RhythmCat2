/*
 * Karaoke Effect Plug-in
 * Remove voice from playing music.
 *
 * karaeffect.c
 * This file is part of RhythmCat
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

#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <rclib.h>
#include <rc-ui-menu.h>

#define KARAEFF_ID "rc2-native-karaoke-effect"

typedef struct RCPluginKaraEffectPrivate
{
    GstElement *kara_element;
    GtkToggleAction *action;
    guint menu_id;
    GtkWidget *kara_window;
    GtkWidget *filter_band_scale;
    GtkWidget *filter_width_scale;
    GtkWidget *level_scale;
    GtkWidget *mono_level_scale;
    gfloat filter_band;
    gfloat filter_width;
    gfloat level;
    gfloat mono_level;
    gulong shutdown_id;
    GKeyFile *keyfile;
}RCPluginKaraEffectPrivate;

static RCPluginKaraEffectPrivate karaeff_priv = {0};

static void rc_plugin_karaeff_load_conf(RCPluginKaraEffectPrivate *priv)
{
    gdouble dvalue;
    GError *error = NULL;
    if(priv==NULL || priv->keyfile==NULL) return;
    dvalue = g_key_file_get_double(priv->keyfile, KARAEFF_ID, "FilterBand",
        &error);
    if(error==NULL && dvalue>=0.0 && dvalue<=441.0)
        priv->filter_band = dvalue;
    else if(error!=NULL)
    {
        g_error_free(error);
        error = NULL;
    }
    dvalue = g_key_file_get_double(priv->keyfile, KARAEFF_ID, "FilterWidth",
        &error);
    if(error==NULL && dvalue>=0.0 && dvalue<=100.0)
        priv->filter_width = dvalue;
    else if(error!=NULL)
    {
        g_error_free(error);
        error = NULL;
    }
    dvalue = g_key_file_get_double(priv->keyfile, KARAEFF_ID, "Level",
        &error);
    if(error==NULL && dvalue>=0.0 && dvalue<=1.0)
        priv->level = dvalue;
    else if(error!=NULL)
    {
        g_error_free(error);
        error = NULL;
    }
    dvalue = g_key_file_get_double(priv->keyfile, KARAEFF_ID, "MonoLevel",
        &error);
    if(error==NULL && dvalue>=0.0 && dvalue<=1.0)
        priv->mono_level = dvalue;
    else if(error!=NULL)
    {
        g_error_free(error);
        error = NULL;
    }
}

static void rc_plugin_karaeff_save_conf(RCPluginKaraEffectPrivate *priv)
{
    if(priv==NULL || priv->keyfile==NULL) return;
    g_key_file_set_double(priv->keyfile, KARAEFF_ID, "FilterBand",
        (gdouble)priv->filter_band);
    g_key_file_set_double(priv->keyfile, KARAEFF_ID, "FilterWidth",
        (gdouble)priv->filter_width);
    g_key_file_set_double(priv->keyfile, KARAEFF_ID, "Level",
        (gdouble)priv->level);
    g_key_file_set_double(priv->keyfile, KARAEFF_ID, "MonoLevel",
        (gdouble)priv->mono_level);
}

static void rc_plugin_karaeff_fb_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginKaraEffectPrivate *priv = (RCPluginKaraEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->kara_element, "filter-band", (gfloat)value, NULL);
    priv->filter_band = value;
}

static void rc_plugin_karaeff_fw_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginKaraEffectPrivate *priv = (RCPluginKaraEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->kara_element, "filter-width", (gfloat)value, NULL);
    priv->filter_width = value;
}

static void rc_plugin_karaeff_level_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginKaraEffectPrivate *priv = (RCPluginKaraEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->kara_element, "level", (gfloat)value, NULL);
    priv->level = value;
}

static void rc_plugin_karaeff_mono_level_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginKaraEffectPrivate *priv = (RCPluginKaraEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->kara_element, "mono-level", (gfloat)value, NULL);
    priv->mono_level = value;
}

static gboolean rc_plugin_karaeff_window_delete_event_cb(
    GtkWidget *widget, GdkEvent *event, gpointer data)
{
    RCPluginKaraEffectPrivate *priv = (RCPluginKaraEffectPrivate *)data;
    if(data==NULL) return TRUE;
    gtk_toggle_action_set_active(priv->action, FALSE);
    return TRUE;
}

static inline void rc_plugin_karaeff_window_init(
    RCPluginKaraEffectPrivate *priv)
{
    GtkWidget *filter_band_label, *filter_width_label;
    GtkWidget *level_label, *mono_level_label;
    GtkWidget *kara_main_grid;
    if(priv->kara_window!=NULL) return;
    priv->kara_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    filter_band_label = gtk_label_new(_("Filter Band"));
    filter_width_label = gtk_label_new(_("Filter Width"));
    level_label = gtk_label_new(_("Level"));
    mono_level_label = gtk_label_new(_("Mono Level"));
    priv->filter_band_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 441.0, 1);
    priv->filter_width_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 100.0, 1);
    priv->level_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    priv->mono_level_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    g_object_set(priv->kara_window, "title", _("Karaoke Audio Effect"),
        "window-position", GTK_WIN_POS_CENTER, "has-resize-grip", FALSE,
        "type-hint", GDK_WINDOW_TYPE_HINT_DIALOG, NULL);
    gtk_range_set_value(GTK_RANGE(priv->filter_band_scale),
        (gdouble)priv->filter_band);
    gtk_range_set_value(GTK_RANGE(priv->filter_width_scale),
        (gdouble)priv->filter_width); 
    gtk_range_set_value(GTK_RANGE(priv->level_scale),
        (gdouble)priv->level);
    gtk_range_set_value(GTK_RANGE(priv->mono_level_scale),
        (gdouble)priv->mono_level);
    gtk_widget_set_hexpand(priv->filter_band_scale, TRUE);
    gtk_widget_set_hexpand(priv->filter_width_scale, TRUE);
    gtk_widget_set_hexpand(priv->level_scale, TRUE);
    gtk_widget_set_hexpand(priv->mono_level_scale, TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->filter_band_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->filter_width_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->level_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->mono_level_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(priv->filter_band_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->filter_width_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->level_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->mono_level_scale),
        GTK_POS_RIGHT);
    kara_main_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(kara_main_grid), 3);
    gtk_grid_set_row_spacing(GTK_GRID(kara_main_grid), 4);
    gtk_grid_attach(GTK_GRID(kara_main_grid), filter_band_label,
        0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(kara_main_grid), priv->filter_band_scale,
        1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(kara_main_grid), filter_width_label,
        0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(kara_main_grid), priv->filter_width_scale,
        1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(kara_main_grid), level_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(kara_main_grid), priv->level_scale, 1, 2,
        1, 1);
    gtk_grid_attach(GTK_GRID(kara_main_grid), mono_level_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(kara_main_grid), priv->mono_level_scale, 1, 3,
        1, 1);
    gtk_container_add(GTK_CONTAINER(priv->kara_window), kara_main_grid);      
    gtk_widget_set_size_request(priv->kara_window, 300, 150);
    gtk_widget_show_all(kara_main_grid);
    
    g_signal_connect(priv->kara_window, "delete-event",
        G_CALLBACK(rc_plugin_karaeff_window_delete_event_cb), priv);
    g_signal_connect(G_OBJECT(priv->filter_band_scale), "value-changed",
        G_CALLBACK(rc_plugin_karaeff_fb_scale_changed_cb), priv);
    g_signal_connect(G_OBJECT(priv->filter_width_scale), "value-changed",
        G_CALLBACK(rc_plugin_karaeff_fw_scale_changed_cb), priv);
    g_signal_connect(G_OBJECT(priv->level_scale), "value-changed",
        G_CALLBACK(rc_plugin_karaeff_level_scale_changed_cb), priv);
    g_signal_connect(G_OBJECT(priv->mono_level_scale), "value-changed",
        G_CALLBACK(rc_plugin_karaeff_mono_level_scale_changed_cb), priv);
}

static void rc_plugin_karaeff_view_menu_toggled(GtkToggleAction *toggle,
    gpointer data)
{
    RCPluginKaraEffectPrivate *priv = (RCPluginKaraEffectPrivate *)data;
    gboolean flag;
    if(data==NULL) return;
    flag = gtk_toggle_action_get_active(toggle);
    if(flag)
        gtk_window_present(GTK_WINDOW(priv->kara_window));
    else
        gtk_widget_hide(priv->kara_window);
}

static void rc_plugin_karaeff_shutdown_cb(RCLibPlugin *plugin, gpointer data)
{
    RCPluginKaraEffectPrivate *priv = (RCPluginKaraEffectPrivate *)data;
    if(data==NULL) return;
    rc_plugin_karaeff_save_conf(priv);
}

static gboolean rc_plugin_karaeff_load(RCLibPluginData *plugin)
{
    RCPluginKaraEffectPrivate *priv = &karaeff_priv;
    gboolean flag;
    priv->kara_element = gst_element_factory_make("audiokaraoke", NULL);
    flag = rclib_core_effect_plugin_add(priv->kara_element);
    if(!flag)
    {
        gst_object_unref(priv->kara_element);
        priv->kara_element = NULL;
    }
    g_object_set(priv->kara_element, "filter-band", priv->filter_band,
        "filter-width", priv->filter_width, "level", priv->level,
        "mono-level", priv->mono_level, NULL);
    rc_plugin_karaeff_window_init(priv);
    priv->action = gtk_toggle_action_new("RC2ViewPluginKaraokeEffect",
        _("Karaoke Effect"), _("Show/hide karaoke audio effect "
        "configuation window"), NULL);
    priv->menu_id = rc_ui_menu_add_menu_action(GTK_ACTION(priv->action),
        "/RC2MenuBar/ViewMenu/ViewSep2", "RC2ViewPluginKaraokeEffect",
        "RC2ViewPluginKaraokeEffect", TRUE);
    g_signal_connect(priv->action, "toggled",
        G_CALLBACK(rc_plugin_karaeff_view_menu_toggled), priv);
    return flag;
}

static gboolean rc_plugin_karaeff_unload(RCLibPluginData *plugin)
{
    RCPluginKaraEffectPrivate *priv = &karaeff_priv;
    if(priv->menu_id>0)
    {
        rc_ui_menu_remove_menu_action(GTK_ACTION(priv->action),
            priv->menu_id);
        g_object_unref(priv->action);
    }
    if(priv->kara_window!=NULL)
    {
        gtk_widget_destroy(priv->kara_window);
        priv->kara_window = NULL;
    }
    if(priv->kara_element!=NULL)
    {
        rclib_core_effect_plugin_remove(priv->kara_element);
        priv->kara_element = NULL;
    }
    return TRUE;
}

static gboolean rc_plugin_karaeff_init(RCLibPluginData *plugin)
{
    RCPluginKaraEffectPrivate *priv = &karaeff_priv;
    priv->filter_band = 220;
    priv->filter_width = 100;
    priv->level = 1;
    priv->mono_level = 1;
    priv->keyfile = rclib_plugin_get_keyfile();
    rc_plugin_karaeff_load_conf(priv);
    priv->shutdown_id = rclib_plugin_signal_connect("shutdown",
        G_CALLBACK(rc_plugin_karaeff_shutdown_cb), priv);
    return TRUE;
}

static void rc_plugin_karaeff_destroy(RCLibPluginData *plugin)
{
    RCPluginKaraEffectPrivate *priv = &karaeff_priv;
    if(priv->shutdown_id>0)
    {
        rclib_plugin_signal_disconnect(priv->shutdown_id);
        priv->shutdown_id = 0;
    }
}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = KARAEFF_ID,
    .name = N_("Karaoke Audio Effect"),
    .version = "0.1",
    .description = N_("Remove voice from the music."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_karaeff_load,
    .unload = rc_plugin_karaeff_unload,
    .configure = NULL,
    .destroy = rc_plugin_karaeff_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_karaeff_init, rcplugin_info);


