/*
 * Echo Effect Plug-in
 * Add echo effect in the music playback.
 *
 * echoeffect.c
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

#define ECHOEFF_ID "rc2-native-echo-effect"

typedef struct RCPluginEchoEffectPrivate
{
    GstElement *echo_element;
    GtkToggleAction *action;
    guint menu_id;
    GtkWidget *echo_window;
    GtkWidget *echo_delay_scale;
    GtkWidget *echo_fb_scale;
    GtkWidget *echo_intensity_scale;
    guint64 delay;
    gfloat feedback;
    gfloat intensity;
    gulong shutdown_id;
    GKeyFile *keyfile;
}RCPluginEchoEffectPrivate;

static RCPluginEchoEffectPrivate echoeff_priv = {0};

static void rc_plugin_echoeff_load_conf(RCPluginEchoEffectPrivate *priv)
{
    gint ivalue;
    gdouble dvalue;
    if(priv==NULL || priv->keyfile==NULL) return;
    ivalue = g_key_file_get_integer(priv->keyfile, ECHOEFF_ID, "Delay",
        NULL);
    if(ivalue>=0 && ivalue<=1000)
        priv->delay = ivalue * GST_MSECOND;
    if(priv->delay==0) priv->delay = 1;
    dvalue = g_key_file_get_double(priv->keyfile, ECHOEFF_ID, "Feedback",
        NULL);
    if(dvalue>=0.0 && dvalue<=1)
        priv->feedback = dvalue;
    dvalue = g_key_file_get_double(priv->keyfile, ECHOEFF_ID, "Intensity",
        NULL);
    if(dvalue>=0.0 && dvalue<=1)
        priv->intensity = dvalue;
}

static void rc_plugin_echoeff_save_conf(RCPluginEchoEffectPrivate *priv)
{
    if(priv==NULL || priv->keyfile==NULL) return;
    g_key_file_set_integer(priv->keyfile, ECHOEFF_ID, "Delay",
        (gint)(priv->delay / GST_MSECOND));
    g_key_file_set_double(priv->keyfile, ECHOEFF_ID, "Feedback",
        (gdouble)priv->feedback);
    g_key_file_set_double(priv->keyfile, ECHOEFF_ID, "Intensity",
        (gdouble)priv->intensity);    
}

static void rc_plugin_echoeff_delay_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gdouble value = 0.0;
    gint64 delay;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    delay = value * GST_MSECOND;
    if(delay==0) delay = 1;
    g_object_set(priv->echo_element, "delay", delay, NULL);
    priv->delay = delay;
}

static void rc_plugin_echoeff_fb_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->echo_element, "feedback", (gfloat)value, NULL);
    priv->feedback = value;
}

static void rc_plugin_echoeff_intensity_scale_changed_cb(
    GtkRange *range, gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gdouble value = 0.0;
    if(data==NULL) return;
    value = gtk_range_get_value(range);
    g_object_set(priv->echo_element, "intensity", (gfloat)value, NULL);
    priv->intensity = value;
}

static gboolean rc_plugin_echoeff_window_delete_event_cb(
    GtkWidget *widget, GdkEvent *event, gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    if(data==NULL) return TRUE;
    gtk_toggle_action_set_active(priv->action, FALSE);
    return TRUE;
}

static inline void rc_plugin_echoeff_window_init(
    RCPluginEchoEffectPrivate *priv)
{
    GtkWidget *echo_delay_label;
    GtkWidget *echo_fb_label, *echo_intensity_label;
    GtkWidget *echo_main_grid;
    if(priv->echo_window!=NULL) return;
    priv->echo_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    echo_delay_label = gtk_label_new(_("Delay"));
    echo_fb_label = gtk_label_new(_("Feedback"));
    echo_intensity_label = gtk_label_new(_("Intensity"));
    priv->echo_delay_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1000.0, 1);
    priv->echo_fb_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    priv->echo_intensity_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    g_object_set(priv->echo_window, "title", _("Echo Effect"),
        "window-position", GTK_WIN_POS_CENTER, "has-resize-grip", FALSE,
        "type-hint", GDK_WINDOW_TYPE_HINT_DIALOG, NULL);
    gtk_range_set_value(GTK_RANGE(priv->echo_delay_scale),
        priv->delay / GST_MSECOND);
    gtk_range_set_value(GTK_RANGE(priv->echo_fb_scale),
        (gdouble)priv->feedback); 
    gtk_range_set_value(GTK_RANGE(priv->echo_intensity_scale),
        (gdouble)priv->intensity);
    gtk_widget_set_hexpand(priv->echo_delay_scale, TRUE);
    gtk_widget_set_hexpand(priv->echo_fb_scale, TRUE);
    gtk_widget_set_hexpand(priv->echo_intensity_scale, TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_delay_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_fb_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_intensity_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_delay_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_fb_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_intensity_scale),
        GTK_POS_RIGHT);
    echo_main_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(echo_main_grid), 3);
    gtk_grid_set_row_spacing(GTK_GRID(echo_main_grid), 4);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_delay_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_delay_scale, 1, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_fb_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_fb_scale, 1, 1,
        1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_intensity_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_intensity_scale, 1, 2,
        1, 1);
    gtk_container_add(GTK_CONTAINER(priv->echo_window), echo_main_grid);
    gtk_widget_set_size_request(priv->echo_window, 300, 120);
    gtk_widget_show_all(echo_main_grid);
    g_signal_connect(priv->echo_window, "delete-event",
        G_CALLBACK(rc_plugin_echoeff_window_delete_event_cb), priv);
    g_signal_connect(G_OBJECT(priv->echo_delay_scale), "value-changed",
        G_CALLBACK(rc_plugin_echoeff_delay_scale_changed_cb), priv);
    g_signal_connect(G_OBJECT(priv->echo_fb_scale), "value-changed",
        G_CALLBACK(rc_plugin_echoeff_fb_scale_changed_cb), priv);
    g_signal_connect(G_OBJECT(priv->echo_intensity_scale), "value-changed",
        G_CALLBACK(rc_plugin_echoeff_intensity_scale_changed_cb), priv);
}

static void rc_plugin_echoeff_view_menu_toggled(GtkToggleAction *toggle,
    gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    gboolean flag;
    if(data==NULL) return;
    flag = gtk_toggle_action_get_active(toggle);
    if(flag)
        gtk_window_present(GTK_WINDOW(priv->echo_window));
    else
        gtk_widget_hide(priv->echo_window);
}

static void rc_plugin_echoeff_shutdown_cb(RCLibPlugin *plugin, gpointer data)
{
    RCPluginEchoEffectPrivate *priv = (RCPluginEchoEffectPrivate *)data;
    if(data==NULL) return;
    rc_plugin_echoeff_save_conf(priv);
}

static gboolean rc_plugin_echoeff_load(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
    gboolean flag;
    priv->echo_element = gst_element_factory_make("audioecho", NULL);
    flag = rclib_core_effect_plugin_add(priv->echo_element);
    if(!flag)
    {
        gst_object_unref(priv->echo_element);
        priv->echo_element = NULL;
    }
    g_object_set(priv->echo_element, "max-delay", 1 * GST_SECOND,
        "delay", priv->delay, "feedback", priv->feedback, "intensity",
        priv->intensity, NULL);
    rc_plugin_echoeff_window_init(priv);
    priv->action = gtk_toggle_action_new("RC2ViewPluginEchoEffect",
        _("Echo Effect"), _("Show/hide echo audio effect "
        "configuation window"), NULL);
    priv->menu_id = rc_ui_menu_add_menu_action(GTK_ACTION(priv->action),
        "/RC2MenuBar/ViewMenu/ViewSep2", "RC2ViewPluginEchoEffect",
        "RC2ViewPluginEchoEffect", TRUE);
    g_signal_connect(priv->action, "toggled",
        G_CALLBACK(rc_plugin_echoeff_view_menu_toggled), priv);
    return flag;
}

static gboolean rc_plugin_echoeff_unload(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
    if(priv->menu_id>0)
    {
        rc_ui_menu_remove_menu_action(GTK_ACTION(priv->action),
            priv->menu_id);
        g_object_unref(priv->action);
    }
    if(priv->echo_window!=NULL)
    {
        gtk_widget_destroy(priv->echo_window);
        priv->echo_window = NULL;
    }
    if(priv->echo_element!=NULL)
    {
        rclib_core_effect_plugin_remove(priv->echo_element);
        priv->echo_element = NULL;
    }
    return TRUE;
}

static gboolean rc_plugin_echoeff_init(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
    priv->delay = 1;
    priv->keyfile = rclib_plugin_get_keyfile();
    rc_plugin_echoeff_load_conf(priv);
    priv->shutdown_id = rclib_plugin_signal_connect("shutdown",
        G_CALLBACK(rc_plugin_echoeff_shutdown_cb), priv);
    return TRUE;
}

static void rc_plugin_echoeff_destroy(RCLibPluginData *plugin)
{
    RCPluginEchoEffectPrivate *priv = &echoeff_priv;
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
    .id = ECHOEFF_ID,
    .name = N_("Echo Audio Effect"),
    .version = "0.1",
    .description = N_("Add echo audio effect in the music playback."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_echoeff_load,
    .unload = rc_plugin_echoeff_unload,
    .configure = NULL,
    .destroy = rc_plugin_echoeff_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_echoeff_init, rcplugin_info);


