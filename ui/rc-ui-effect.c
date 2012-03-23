/*
 * RhythmCat UI Audio Effect Module
 * The audio effect setting UI for the player.
 *
 * rc-ui-effect.c
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

#include "rc-ui-effect.h"
#include "rc-common.h"

typedef struct RCUiAudioEffectPrivate
{
    GtkWidget *effect_window;
    GtkWidget *eq_combo_box;
    GtkWidget *eq_scales[10];
    GtkWidget *bal_scale;
    GtkWidget *echo_delay_scale;
    GtkWidget *echo_fb_scale;
    GtkWidget *echo_intensity_scale;
    gulong eq_id;
    gulong balance_id;
    gulong echo_id;
}RCUiAudioEffectPrivate;

static RCUiAudioEffectPrivate effect_priv = {0};

static void rc_ui_effect_window_eq_combo_box_changed_cb(GtkComboBox *widget,
    gpointer data)
{
    gint index = 0;
    index = gtk_combo_box_get_active(widget);
    rclib_core_set_eq(index, NULL);
}

static void rc_ui_effect_window_eq_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    gdouble bands[10] = {0.0};
    gdouble value = 0.0;
    gint i;
    rclib_core_get_eq(NULL, bands);
    i = GPOINTER_TO_INT(data);
    if(i<0 || i>=10) return;
    value = gtk_range_get_value(range);
    bands[i] = value;
    rclib_core_set_eq(RCLIB_CORE_EQ_TYPE_CUSTOM, bands);
}

static void rc_ui_effect_window_balance_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    gdouble value = 0.0;
    value = gtk_range_get_value(range);
    g_signal_handlers_block_by_func(range,
        G_CALLBACK(rc_ui_effect_window_balance_scale_changed_cb), data);
    rclib_core_set_balance(value);
    g_signal_handlers_unblock_by_func(range,
        G_CALLBACK(rc_ui_effect_window_balance_scale_changed_cb), data);
}

static void rc_ui_effect_window_echo_delay_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    gdouble value = 0.0;
    guint64 echo_delay = 0;
    gfloat echo_feedback = 0.0, echo_intensity = 0.0;
    value = gtk_range_get_value(range);
    rclib_core_get_echo(&echo_delay, NULL, &echo_feedback, &echo_intensity);
    echo_delay = (guint64)value * GST_MSECOND;
    if(echo_delay==0) echo_delay = 1;
    g_signal_handlers_block_by_func(range,
        G_CALLBACK(rc_ui_effect_window_echo_delay_scale_changed_cb), data);
    rclib_core_set_echo(echo_delay, echo_feedback, echo_intensity);
    g_signal_handlers_unblock_by_func(range,
        G_CALLBACK(rc_ui_effect_window_echo_delay_scale_changed_cb), data);
}

static void rc_ui_effect_window_echo_fb_scale_changed_cb(GtkRange *range,
    gpointer data)
{
    gdouble value = 0.0;
    guint64 echo_delay = 0;
    gfloat echo_feedback = 0.0, echo_intensity = 0.0;
    value = gtk_range_get_value(range);
    rclib_core_get_echo(&echo_delay, NULL, &echo_feedback, &echo_intensity);
    echo_feedback = value;
    g_signal_handlers_block_by_func(range,
        G_CALLBACK(rc_ui_effect_window_echo_fb_scale_changed_cb), data);
    rclib_core_set_echo(echo_delay, echo_feedback, echo_intensity);
    g_signal_handlers_unblock_by_func(range,
        G_CALLBACK(rc_ui_effect_window_echo_fb_scale_changed_cb), data);
}

static void rc_ui_effect_window_echo_intensity_scale_changed_cb(
    GtkRange *range, gpointer data)
{
    gdouble value = 0.0;
    guint64 echo_delay = 0;
    gfloat echo_feedback = 0.0, echo_intensity = 0.0;
    value = gtk_range_get_value(range);
    rclib_core_get_echo(&echo_delay, NULL, &echo_feedback, &echo_intensity);
    echo_intensity = value;
    g_signal_handlers_block_by_func(range,
        G_CALLBACK(rc_ui_effect_window_echo_intensity_scale_changed_cb),
        data);
    rclib_core_set_echo(echo_delay, echo_feedback, echo_intensity);
    g_signal_handlers_unblock_by_func(range,
        G_CALLBACK(rc_ui_effect_window_echo_intensity_scale_changed_cb),
        data);
}

static void rc_ui_effect_eq_changed_cb(RCLibCore *core, RCLibCoreEQType type,
    gdouble *values, gpointer data)
{
    RCUiAudioEffectPrivate *priv = &effect_priv;
    gint i;
    if(values==NULL) return;
    for(i=0;i<10;i++)
    {
        g_signal_handlers_block_by_func(priv->eq_scales[i],
            G_CALLBACK(rc_ui_effect_window_eq_scale_changed_cb),
            GINT_TO_POINTER(i));
        gtk_range_set_value(GTK_RANGE(priv->eq_scales[i]),
            values[i]);
        g_signal_handlers_unblock_by_func(priv->eq_scales[i],
            G_CALLBACK(rc_ui_effect_window_eq_scale_changed_cb),
            GINT_TO_POINTER(i));
    }
    g_signal_handlers_block_by_func(priv->eq_combo_box,
        G_CALLBACK(rc_ui_effect_window_eq_combo_box_changed_cb), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(priv->eq_combo_box), type);
    g_signal_handlers_unblock_by_func(priv->eq_combo_box,
        G_CALLBACK(rc_ui_effect_window_eq_combo_box_changed_cb), NULL);
}

static void rc_ui_effect_balance_changed_cb(RCLibCore *core, gfloat balance,
    gpointer data)
{
    RCUiAudioEffectPrivate *priv = &effect_priv;
    gtk_range_set_value(GTK_RANGE(priv->bal_scale), balance);
}

static void rc_ui_effect_echo_changed_cb(RCLibCore *core, gpointer data)
{
    RCUiAudioEffectPrivate *priv = &effect_priv;
    guint64 echo_delay = 0;
    gfloat echo_feedback = 0.0, echo_intensity = 0.0;
    if(!rclib_core_get_echo(&echo_delay, NULL, &echo_feedback,
        &echo_intensity)) return;
    gtk_range_set_value(GTK_RANGE(priv->echo_delay_scale),
        echo_delay/GST_MSECOND);
    gtk_range_set_value(GTK_RANGE(priv->echo_fb_scale), echo_feedback);
    gtk_range_set_value(GTK_RANGE(priv->echo_intensity_scale),
        echo_intensity);
}

static void rc_ui_effect_eq_save_setting()
{
    RCUiAudioEffectPrivate *priv = &effect_priv;
    GtkWidget *file_chooser;
    GtkFileFilter *file_filter1;
    int i = 0;
    char eq_file_data[299] = {0};
    gdouble eq_data[10] = {0.0};
    gint result = 0;
    gchar *file_name = NULL;
    const gchar *home_dir;
    g_snprintf(eq_file_data, 64, "Winamp EQ library file v1.1\x1A!--Entry1");
    eq_file_data[298] = 0x1F;
    file_filter1 = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter1,
        _("EQ Setting File (*.EQF)"));
    gtk_file_filter_add_pattern(file_filter1, "*.[E,e][Q,q][F,f]");
    file_chooser = gtk_file_chooser_dialog_new(_("Save Equalizer Setting..."),
        GTK_WINDOW(priv->effect_window), GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,
        GTK_RESPONSE_CANCEL, NULL);
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
            rclib_core_get_eq(NULL, eq_data);
            for(i=0;i<10;i++)
            {
                eq_file_data[288+i] = 
                    (char)((12 - eq_data[i]) / 24 * 0x3F);   
            }
            g_file_set_contents(file_name, eq_file_data, 299, NULL);
            g_free(file_name);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

static void rc_ui_effect_eq_load_setting()
{
    RCUiAudioEffectPrivate *priv = &effect_priv;
    GtkWidget *file_chooser;
    GtkFileFilter *file_filter1;
    gint result = 0;
    char eq_file_data[299];
    FILE *fp;
    int i;
    gchar *file_name = NULL;
    gdouble eq_data[10];
    const gchar *home_dir;
    file_filter1 = gtk_file_filter_new();
    gtk_file_filter_set_name(file_filter1,
        _("EQ Setting File (*.EQF)"));
    gtk_file_filter_add_pattern(file_filter1, "*.[E,e][Q,q][F,f]");
    file_chooser = gtk_file_chooser_dialog_new(_("Load Equalizer Setting..."),
        GTK_WINDOW(priv->effect_window), GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,
        GTK_RESPONSE_CANCEL, NULL);
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
            fp = fopen(file_name, "r");
            if(fp!=NULL)
            {
                if(fread(eq_file_data, 299, 1, fp)<=0) break;
                if(strncmp(eq_file_data, "Winamp EQ library file", 22)==0)
                {
                    for(i=0;i<10;i++)
                    {
                        eq_data[i] = 12 - 
                            (gdouble)eq_file_data[288+i] / 0x3F * 24;
                    }
                    rclib_core_set_eq(RCLIB_CORE_EQ_TYPE_CUSTOM, eq_data);
                }
                fclose(fp);
            }
            g_free(file_name);
            break;
        case GTK_RESPONSE_CANCEL:
            break;
        default: break;
    }
    gtk_widget_destroy(file_chooser);
}

static void rc_ui_effect_window_destroy_cb(GtkWidget *widget, gpointer data)
{
    RCUiAudioEffectPrivate *priv = &effect_priv;
    if(priv->eq_id>0)
        rclib_core_signal_disconnect(priv->eq_id);
    if(priv->balance_id>0)
        rclib_core_signal_disconnect(priv->balance_id);
    if(priv->echo_id>0)
        rclib_core_signal_disconnect(priv->echo_id);
    gtk_widget_destroyed(priv->effect_window, &(priv->effect_window));
}

/**
 * rc_ui_effect_window_init:
 *
 * Initalize the effect window.
 */

void rc_ui_effect_window_init()
{
    GtkWidget *effect_notebook;
    GtkWidget *eq_label, *bal_label, *echo_label;
    GtkWidget *eq_labels[10];
    GtkWidget *echo_delay_label;
    GtkWidget *echo_fb_label, *echo_intensity_label;
    GtkWidget *eq_scale_grid;
    GtkWidget *eq_save_button, *eq_open_button;
    GtkWidget *eq_button_hbox;
    GtkWidget *eq_head_grid;
    GtkWidget *eq_main_grid;    
    GtkWidget *bal_main_grid;
    GtkWidget *echo_main_grid;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeIter iter;
    RCUiAudioEffectPrivate *priv = &effect_priv;
    gint i;
    RCLibCoreEQType eq_type;
    gdouble eq_bands[10] = {0.0};
    gfloat balance = 0.0;
    guint64 echo_delay = 0, echo_mdelay = 0;
    gfloat echo_feedback = 0.0, echo_intensity = 0.0;
    if(priv->effect_window!=NULL) return;
    rclib_core_get_eq(&eq_type, eq_bands);
    rclib_core_get_balance(&balance);
    rclib_core_get_echo(&echo_delay, &echo_mdelay, &echo_feedback,
        &echo_intensity);
    priv->effect_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    effect_notebook = gtk_notebook_new();
    store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    renderer = gtk_cell_renderer_text_new();
    priv->eq_combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->eq_combo_box),
        renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(priv->eq_combo_box),
        renderer, "text", 0, NULL);
    eq_save_button = gtk_button_new_from_stock(GTK_STOCK_SAVE);
    eq_open_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    eq_label = gtk_label_new(_("Equalizer"));
    bal_label = gtk_label_new(_("Balance"));
    echo_label = gtk_label_new(_("Echo"));
    eq_labels[0] = gtk_label_new("29Hz");
    eq_labels[1] = gtk_label_new("59Hz");
    eq_labels[2] = gtk_label_new("119Hz");
    eq_labels[3] = gtk_label_new("227Hz");
    eq_labels[4] = gtk_label_new("474Hz");
    eq_labels[5] = gtk_label_new("947Hz");
    eq_labels[6] = gtk_label_new("1.9KHz");
    eq_labels[7] = gtk_label_new("3.8KHz");
    eq_labels[8] = gtk_label_new("7.5KHz");
    eq_labels[9] = gtk_label_new("15KHz");
    eq_scale_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(eq_scale_grid), 12);
    gtk_widget_set_vexpand(eq_scale_grid, TRUE);
    for(i=0;i<10;i++)
    {
        gtk_label_set_angle(GTK_LABEL(eq_labels[i]), 90.0);
        priv->eq_scales[i] = gtk_scale_new_with_range(
            GTK_ORIENTATION_VERTICAL, -12.0, 12.0, 0.1);
        gtk_widget_set_vexpand(priv->eq_scales[i], TRUE);
        gtk_range_set_value(GTK_RANGE(priv->eq_scales[i]),
            eq_bands[i]);
        gtk_scale_set_draw_value(GTK_SCALE(priv->eq_scales[i]), TRUE);  
        gtk_scale_set_value_pos(GTK_SCALE(priv->eq_scales[i]),
            GTK_POS_BOTTOM);
        gtk_range_set_inverted(GTK_RANGE(priv->eq_scales[i]), TRUE);
        gtk_widget_set_size_request(priv->eq_scales[i], -1, 120);
        gtk_grid_attach(GTK_GRID(eq_scale_grid), eq_labels[i], i, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(eq_scale_grid), priv->eq_scales[i],
            i, 1, 1, 1);
    }
    priv->bal_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
        -1.0, 1.0, 0.01);
    gtk_range_set_value(GTK_RANGE(priv->bal_scale), balance);
    gtk_scale_set_draw_value(GTK_SCALE(priv->bal_scale), FALSE);
    gtk_scale_add_mark(GTK_SCALE(priv->bal_scale), -1.0, GTK_POS_BOTTOM,
        _("Left"));
    gtk_scale_add_mark(GTK_SCALE(priv->bal_scale), 1.0, GTK_POS_BOTTOM,
        _("Right"));
    gtk_scale_add_mark(GTK_SCALE(priv->bal_scale), 0.0, GTK_POS_BOTTOM,
        NULL);
    g_object_set(priv->bal_scale, "expand", TRUE, NULL);
    echo_delay_label = gtk_label_new(_("Delay"));
    echo_fb_label = gtk_label_new(_("Feedback"));
    echo_intensity_label = gtk_label_new(_("Intensity"));
    priv->echo_delay_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, echo_mdelay/GST_MSECOND, 1);
    priv->echo_fb_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    priv->echo_intensity_scale = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL, 0, 1.0, 0.01);
    gtk_widget_set_hexpand(priv->echo_delay_scale, TRUE);
    gtk_widget_set_hexpand(priv->echo_fb_scale, TRUE);
    gtk_widget_set_hexpand(priv->echo_intensity_scale, TRUE);
    gtk_range_set_value(GTK_RANGE(priv->echo_delay_scale),
        echo_delay/GST_MSECOND);   
    gtk_range_set_value(GTK_RANGE(priv->echo_fb_scale),
        echo_feedback); 
    gtk_range_set_value(GTK_RANGE(priv->echo_intensity_scale),
        echo_intensity);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_delay_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_fb_scale), TRUE);
    gtk_scale_set_draw_value(GTK_SCALE(priv->echo_intensity_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_delay_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_fb_scale),
        GTK_POS_RIGHT);
    gtk_scale_set_value_pos(GTK_SCALE(priv->echo_intensity_scale),
        GTK_POS_RIGHT);
    eq_button_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_set(eq_button_hbox, "layout-style", GTK_BUTTONBOX_END,
        "hexpand-set", TRUE, "hexpand", TRUE, "spacing", 4, NULL);
    for(i=RCLIB_CORE_EQ_TYPE_NONE;i<=RCLIB_CORE_EQ_TYPE_CUSTOM;i++)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, 0, rclib_core_get_eq_name(i), 1,
            i, -1);
    }
    if(eq_type>=RCLIB_CORE_EQ_TYPE_NONE && eq_type<RCLIB_CORE_EQ_TYPE_CUSTOM)
        gtk_combo_box_set_active(GTK_COMBO_BOX(priv->eq_combo_box),
            eq_type);
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(priv->eq_combo_box),
            RCLIB_CORE_EQ_TYPE_CUSTOM);
    eq_head_grid = gtk_grid_new();
    eq_main_grid = gtk_grid_new();
    bal_main_grid = gtk_grid_new();
    echo_main_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(eq_head_grid), 8);
    g_object_set(eq_head_grid, "margin-left", 2, "margin-right", 2,
        "margin-top", 4, "margin-bottom", 4, NULL);
    gtk_grid_set_row_spacing(GTK_GRID(eq_main_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(echo_main_grid), 3);
    gtk_grid_set_row_spacing(GTK_GRID(echo_main_grid), 4);
    g_object_set(priv->effect_window, "title", _("Audio Effects"),
        "window-position", GTK_WIN_POS_CENTER, "has-resize-grip", FALSE,
        "type-hint", GDK_WINDOW_TYPE_HINT_DIALOG, NULL);
    gtk_box_pack_start(GTK_BOX(eq_button_hbox), eq_save_button, FALSE,
        FALSE, 2);
    gtk_box_pack_start(GTK_BOX(eq_button_hbox), eq_open_button, FALSE,
        FALSE, 2);
    gtk_grid_attach(GTK_GRID(eq_head_grid), priv->eq_combo_box, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(eq_head_grid), eq_button_hbox, 1, 0, 1, 1);       
    gtk_grid_attach(GTK_GRID(eq_main_grid), eq_head_grid, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(eq_main_grid), eq_scale_grid, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(bal_main_grid), priv->bal_scale, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_delay_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_delay_scale, 1, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_fb_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_fb_scale, 1, 1,
        1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), echo_intensity_label, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(echo_main_grid), priv->echo_intensity_scale, 1, 2,
        1, 1);
    gtk_notebook_append_page(GTK_NOTEBOOK(effect_notebook), eq_main_grid,
        eq_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(effect_notebook), bal_main_grid,
        bal_label);
    gtk_notebook_append_page(GTK_NOTEBOOK(effect_notebook), echo_main_grid,
        echo_label);
    gtk_container_add(GTK_CONTAINER(priv->effect_window), effect_notebook);
    for(i=0;i<10;i++)
    {
        g_signal_connect(G_OBJECT(priv->eq_scales[i]), "value-changed",
            G_CALLBACK(rc_ui_effect_window_eq_scale_changed_cb),
            GINT_TO_POINTER(i));
    }
    g_signal_connect(G_OBJECT(priv->eq_combo_box), "changed",
        G_CALLBACK(rc_ui_effect_window_eq_combo_box_changed_cb), NULL);
    g_signal_connect(G_OBJECT(priv->bal_scale), "value-changed",
        G_CALLBACK(rc_ui_effect_window_balance_scale_changed_cb), NULL);
    g_signal_connect(G_OBJECT(priv->echo_delay_scale), "value-changed",
        G_CALLBACK(rc_ui_effect_window_echo_delay_scale_changed_cb), NULL);
    g_signal_connect(G_OBJECT(priv->echo_fb_scale), "value-changed",
        G_CALLBACK(rc_ui_effect_window_echo_fb_scale_changed_cb), NULL);
    g_signal_connect(G_OBJECT(priv->echo_intensity_scale), "value-changed",
        G_CALLBACK(rc_ui_effect_window_echo_intensity_scale_changed_cb),
        NULL);
    g_signal_connect(G_OBJECT(eq_save_button), "clicked",   
        G_CALLBACK(rc_ui_effect_eq_save_setting), NULL);  
    g_signal_connect(G_OBJECT(eq_open_button), "clicked",   
        G_CALLBACK(rc_ui_effect_eq_load_setting), NULL);
    g_signal_connect(G_OBJECT(priv->effect_window), "delete-event",
        G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    g_signal_connect(G_OBJECT(priv->effect_window), "destroy",
        G_CALLBACK(rc_ui_effect_window_destroy_cb), NULL);
    effect_priv.eq_id = rclib_core_signal_connect("eq-changed",
        G_CALLBACK(rc_ui_effect_eq_changed_cb), NULL);
    effect_priv.balance_id = rclib_core_signal_connect("balance-changed",
        G_CALLBACK(rc_ui_effect_balance_changed_cb), NULL);
    effect_priv.echo_id = rclib_core_signal_connect("echo-changed",
        G_CALLBACK(rc_ui_effect_echo_changed_cb), NULL);
    gtk_widget_show_all(priv->effect_window);
    gtk_widget_hide(priv->effect_window);
}

/**
 * rc_ui_effect_window_show:
 *
 * Show an audio effect configuration window.
 */

void rc_ui_effect_window_show()
{
    if(effect_priv.effect_window==NULL)
    {
        rc_ui_effect_window_init();
        gtk_window_present(GTK_WINDOW(effect_priv.effect_window));
    }
    else
        gtk_window_present(GTK_WINDOW(effect_priv.effect_window));
}

/**
 * rc_ui_effect_window_hide:
 *
 * Hide the audio effect configuration window.
 */

void rc_ui_effect_window_hide()
{
    if(effect_priv.effect_window!=NULL)
        gtk_widget_hide(effect_priv.effect_window);
}

/**
 * rc_ui_effect_window_destroy:
 *
 * Destroy the audio effect configuration window.
 */

void rc_ui_effect_window_destroy()
{
    if(effect_priv.effect_window!=NULL)
        gtk_widget_destroy(effect_priv.effect_window);
}


