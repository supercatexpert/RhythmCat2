/*
 * RhythmCat UI Plug-in Module
 * The plug-in manager for the player.
 *
 * rc-ui-plugin.c
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

#include "rc-ui-plugin.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-plugin
 * @Short_description: Plug-in Configuration UI
 * @Title: Plug-in Configuration UI
 * @Include: rclib-db.h
 *
 * The plug-in configuation UI module, show plug-in configuration panel.
 */

typedef struct RCUiPluginPrivate
{
    GtkWidget *plugin_window;
    GtkWidget *plugin_listview;
    GtkWidget *about_button;
    GtkWidget *config_button;
    GtkListStore *plugin_store;
    gulong registered_id;
    gulong unregistered_id;
    gulong loaded_id;
    gulong unloaded_id;
}RCUiPluginPrivate;

enum
{
    RC_UI_PLUGIN_STORE_ACTIVATABLE,
    RC_UI_PLUGIN_STORE_ENABLED,
    RC_UI_PLUGIN_STORE_TYPE,
    RC_UI_PLUGIN_STORE_ID,
    RC_UI_PLUGIN_STORE_TEXT,
    RC_UI_PLUGIN_STORE_LAST
};

static RCUiPluginPrivate plugin_priv = {0};

static void rc_ui_plugin_window_destroy_cb(GtkWidget *widget, gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    if(priv->registered_id>0)
        rclib_plugin_signal_disconnect(priv->registered_id);
    if(priv->loaded_id>0)
        rclib_plugin_signal_disconnect(priv->loaded_id);
    if(priv->unloaded_id>0)
        rclib_plugin_signal_disconnect(priv->unloaded_id);
    if(priv->unregistered_id>0)
        rclib_plugin_signal_disconnect(priv->unregistered_id);
    gtk_widget_destroyed(priv->plugin_window, &(priv->plugin_window));
}

static void rc_ui_plugin_toggled(GtkCellRendererToggle *renderer,
    gchar *path_str, gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkTreeIter iter;
    GtkTreePath *path;
    RCLibPluginData *plugin_data = NULL;
    gchar *id;
    if(path_str==NULL) return;
    path = gtk_tree_path_new_from_string(path_str);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(priv->plugin_store), &iter, path);
    gtk_tree_path_free(path);
    gtk_tree_model_get(GTK_TREE_MODEL(priv->plugin_store), &iter,
        RC_UI_PLUGIN_STORE_ID, &id, -1);
    if(id!=NULL)
        plugin_data = rclib_plugin_lookup(id);
    g_free(id);
    if(plugin_data==NULL) return;
    if(!plugin_data->loaded)
        rclib_plugin_load(plugin_data);
    else
        rclib_plugin_unload(plugin_data);
}

static void rc_ui_plugin_selection_changed(GtkTreeSelection *selection,
    gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    gboolean flag;
    gchar *id;
    GtkTreeIter iter;
    RCLibPluginData *plugin_data = NULL;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->plugin_listview));
    flag = gtk_tree_selection_get_selected(selection, NULL, &iter);
    if(flag)
    {
        g_object_set(priv->about_button, "sensitive", TRUE, NULL);
        gtk_tree_model_get(GTK_TREE_MODEL(priv->plugin_store), &iter,
            RC_UI_PLUGIN_STORE_ID, &id, -1);
        if(id!=NULL)
            plugin_data = rclib_plugin_lookup(id);
        g_free(id);
        if(plugin_data!=NULL && plugin_data->info!=NULL &&
            plugin_data->info->configure)
            g_object_set(priv->config_button, "sensitive", TRUE, NULL);
        else
            g_object_set(priv->config_button, "sensitive", FALSE, NULL);
    }
    else
    {
        g_object_set(priv->about_button, "sensitive", FALSE, NULL);
        g_object_set(priv->config_button, "sensitive", FALSE, NULL);
    }
}

static void rc_ui_plugin_about_button_clicked(GtkButton *button,
    gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    gchar *id;
    GtkTreeIter iter;
    GtkWidget *about_dialog;
    GtkTreeSelection *selection;
    gchar *authors[] = {NULL, NULL};
    RCLibPluginData *plugin_data = NULL;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->plugin_listview));
    if(selection==NULL) return;
    if(!gtk_tree_selection_get_selected(selection, NULL, &iter)) return;
    gtk_tree_model_get(GTK_TREE_MODEL(priv->plugin_store), &iter,
        RC_UI_PLUGIN_STORE_ID, &id, -1);
    if(id!=NULL)
        plugin_data = rclib_plugin_lookup(id);
    g_free(id);
    if(plugin_data==NULL || plugin_data->info==NULL) return;
    about_dialog = gtk_about_dialog_new();
    authors[0] = plugin_data->info->author;
    g_object_set(about_dialog, "program-name", plugin_data->info->name,
        "authors", authors, "comments", plugin_data->info->description,
        "website", plugin_data->info->homepage, "version",
        plugin_data->info->version, NULL);
    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_destroy(about_dialog);
}

static void rc_ui_plugin_config_button_clicked(GtkButton *button,
    gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    gchar *id;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    RCLibPluginData *plugin_data = NULL;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->plugin_listview));
    if(selection==NULL) return;
    if(!gtk_tree_selection_get_selected(selection, NULL, &iter)) return;
    gtk_tree_model_get(GTK_TREE_MODEL(priv->plugin_store), &iter,
        RC_UI_PLUGIN_STORE_ID, &id, -1);
    if(id!=NULL)
        plugin_data = rclib_plugin_lookup(id);
    g_free(id);
    if(plugin_data==NULL || plugin_data->info==NULL ||
        plugin_data->info->configure==NULL) return;
    plugin_data->info->configure(plugin_data);
}   

static void rc_ui_plugin_close_button_clicked(GtkButton *button,
    gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    gtk_widget_destroy(priv->plugin_window);
}

static gboolean rc_ui_plugin_get_iter_by_id(const gchar *id,
    GtkTreeIter *iter)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkTreeIter iter_foreach;
    gchar *data_id;
    gboolean flag = FALSE;
    if(id==NULL) return FALSE;
    if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->plugin_store),
        &iter_foreach)) return FALSE;
    do
    {
        gtk_tree_model_get(GTK_TREE_MODEL(priv->plugin_store), &iter_foreach,
            RC_UI_PLUGIN_STORE_ID, &data_id, -1);
        if(g_strcmp0(data_id, id)==0) flag = TRUE;
        g_free(data_id);
        if(flag)
        {
            *iter = iter_foreach;
            return TRUE;
        }
    }
    while(gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->plugin_store),
        &iter_foreach));
    return FALSE;
}

static void rc_ui_plugin_registered_cb(RCLibPlugin *plugin,
    const RCLibPluginData *plugin_data, gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkTreeIter iter;
    gchar *text;
    const gchar *type_str = GTK_STOCK_EXECUTE;
    gboolean activatable = TRUE;
    gboolean loaded;
    if(plugin_data==NULL || plugin_data->info==NULL) return;
    gtk_list_store_append(priv->plugin_store, &iter);
    loaded = plugin_data->loaded;
    if(plugin_data->info->type==RCLIB_PLUGIN_TYPE_LOADER)
    {
        type_str = GTK_STOCK_PREFERENCES;
        activatable = FALSE;
        loaded = TRUE;
    }
    text = g_markup_printf_escaped("<b>%s</b>\n%s", plugin_data->info->name,
        plugin_data->info->description);
    gtk_list_store_set(priv->plugin_store, &iter,
        RC_UI_PLUGIN_STORE_ACTIVATABLE, activatable,
        RC_UI_PLUGIN_STORE_ENABLED, loaded, RC_UI_PLUGIN_STORE_TYPE,
        type_str, RC_UI_PLUGIN_STORE_ID, plugin_data->info->id,
        RC_UI_PLUGIN_STORE_TEXT, text, -1);
    g_free(text);
}

static void rc_ui_plugin_loaded_cb(RCLibPlugin *plugin,
    const RCLibPluginData *plugin_data, gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkTreeIter iter;
    if(plugin_data==NULL || plugin_data->info==NULL) return;
    if(!rc_ui_plugin_get_iter_by_id(plugin_data->info->id, &iter))
        return;
    gtk_list_store_set(priv->plugin_store, &iter, RC_UI_PLUGIN_STORE_ENABLED,
        plugin_data->loaded, -1);
}

static void rc_ui_plugin_unloaded_cb(RCLibPlugin *plugin,
    const RCLibPluginData *plugin_data, gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkTreeIter iter;
    if(plugin_data==NULL || plugin_data->info==NULL) return;
    if(!rc_ui_plugin_get_iter_by_id(plugin_data->info->id, &iter))
        return;
    gtk_list_store_set(priv->plugin_store, &iter, RC_UI_PLUGIN_STORE_ENABLED,
        plugin_data->loaded, -1);
}

static void rc_ui_plugin_unregistered_cb(RCLibPlugin *plugin,
    const gchar *id, gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkTreeIter iter;
    if(id==NULL) return;
    if(!rc_ui_plugin_get_iter_by_id(id, &iter))
        return;
    gtk_list_store_remove(priv->plugin_store, &iter);
}

static void rc_ui_plugin_data_foreach(gpointer key, gpointer value,
    gpointer data)
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkTreeIter iter;
    gchar *text;
    const gchar *type_str = GTK_STOCK_EXECUTE;
    gboolean activatable = TRUE;
    gboolean loaded;
    RCLibPluginData *plugin_data = (RCLibPluginData *)value;
    if(value==NULL || plugin_data->info==NULL) return;
    gtk_list_store_append(priv->plugin_store, &iter);
    loaded = plugin_data->loaded;
    if(plugin_data->info->type==RCLIB_PLUGIN_TYPE_LOADER)
    {
        type_str = GTK_STOCK_PREFERENCES;
        activatable = FALSE;
        loaded = TRUE;
    }
    text = g_markup_printf_escaped("<b>%s</b>\n%s", plugin_data->info->name,
        plugin_data->info->description);
    gtk_list_store_set(priv->plugin_store, &iter,
        RC_UI_PLUGIN_STORE_ACTIVATABLE, activatable,
        RC_UI_PLUGIN_STORE_ENABLED, loaded, RC_UI_PLUGIN_STORE_TYPE,
        type_str, RC_UI_PLUGIN_STORE_ID, plugin_data->info->id,
        RC_UI_PLUGIN_STORE_TEXT, text, -1);
    g_free(text);
}

static gboolean rc_ui_plugin_window_key_press_cb(GtkWidget *widget,
    GdkEvent *event, gpointer data)
{
    guint keyval = event->key.keyval;
    RCUiPluginPrivate *priv = &plugin_priv;
    if(keyval==GDK_KEY_Escape)
        gtk_widget_destroy(priv->plugin_window);
    return FALSE;
}

static inline void rc_ui_plugin_window_init()
{
    RCUiPluginPrivate *priv = &plugin_priv;
    GtkWidget *main_grid;
    GtkWidget *list_scrolledwin;
    GtkWidget *plugin_button_hbox;
    GtkWidget *close_button_hbox;
    GtkWidget *close_button;
    GtkCellRenderer *enable_renderer;
    GtkCellRenderer *text_renderer;
    GtkCellRenderer *type_renderer;
    GtkTreeViewColumn *enable_column;
    GtkTreeViewColumn *type_column;
    GtkTreeViewColumn *text_column;
    GtkTreeSelection *selection;
    GdkGeometry window_hints =
    {
        .min_width = 300,
        .min_height = 200,
        .base_width = 350,
        .base_height = 300
    };
    if(priv->plugin_window!=NULL) return;    
    priv->plugin_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    main_grid = gtk_grid_new();
    priv->about_button = gtk_button_new_from_stock(GTK_STOCK_ABOUT);
    priv->config_button = gtk_button_new_from_stock(GTK_STOCK_PREFERENCES);
    close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    list_scrolledwin = gtk_scrolled_window_new(NULL, NULL);
    priv->plugin_store = gtk_list_store_new(RC_UI_PLUGIN_STORE_LAST,
        G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING);
    priv->plugin_listview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
        priv->plugin_store));
    g_object_unref(priv->plugin_store);
    enable_renderer = gtk_cell_renderer_toggle_new();
    text_renderer = gtk_cell_renderer_text_new();
    type_renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(text_renderer, "ellipsize",  PANGO_ELLIPSIZE_END,
        "ellipsize-set", TRUE, NULL);
    enable_column = gtk_tree_view_column_new_with_attributes(
        "Enabled", enable_renderer, "active", RC_UI_PLUGIN_STORE_ENABLED,
        "activatable", RC_UI_PLUGIN_STORE_ACTIVATABLE, NULL);
    type_column = gtk_tree_view_column_new_with_attributes("Type",
        type_renderer, "stock-id", RC_UI_PLUGIN_STORE_TYPE, NULL);
    text_column = gtk_tree_view_column_new_with_attributes("Information",
        text_renderer, "markup", RC_UI_PLUGIN_STORE_TEXT, NULL);
    gtk_tree_view_column_set_expand(text_column, TRUE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(priv->plugin_listview),
        FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(priv->plugin_listview),
        enable_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(priv->plugin_listview),
        type_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(priv->plugin_listview),
        text_column);
    g_object_set(type_column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED,
        "fixed-width", 30, "min-width", 30, "max-width", 30, NULL);
    g_object_set(priv->plugin_window, "title", _("Plug-in Configuration"),
        "window-position", GTK_WIN_POS_CENTER, "has-resize-grip", FALSE,
        "default-width", 350, "default-height", 300, "icon-name",
        GTK_STOCK_EXECUTE, "type-hint", GDK_WINDOW_TYPE_HINT_DIALOG,
        NULL);
    gtk_window_set_geometry_hints(GTK_WINDOW(priv->plugin_window), 
        GTK_WIDGET(priv->plugin_window), &window_hints,
        GDK_HINT_MIN_SIZE);
    g_object_set(priv->plugin_listview, "expand", TRUE, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scrolledwin),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    rclib_plugin_foreach(rc_ui_plugin_data_foreach, priv);
    plugin_button_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_set(plugin_button_hbox, "layout-style", GTK_BUTTONBOX_END,
        "hexpand-set", TRUE, "hexpand", TRUE, "spacing", 4,
        "margin-left", 4, "margin-right", 4, "margin-top", 4,
        "margin-bottom", 4, NULL);
    close_button_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    g_object_set(close_button_hbox, "layout-style", GTK_BUTTONBOX_END,
        "hexpand-set", TRUE, "hexpand", TRUE, "spacing", 4,
        "margin-left", 4, "margin-right", 4, "margin-top", 4,
        "margin-bottom", 4, NULL);
    g_object_set(priv->about_button, "sensitive", FALSE, NULL);
    g_object_set(priv->config_button, "sensitive", FALSE, NULL);
    gtk_box_pack_start(GTK_BOX(plugin_button_hbox), priv->about_button,
        FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(plugin_button_hbox), priv->config_button,
        FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(close_button_hbox), close_button, FALSE,
        FALSE, 2);
    gtk_container_add(GTK_CONTAINER(list_scrolledwin), priv->plugin_listview);
    gtk_grid_attach(GTK_GRID(main_grid), list_scrolledwin, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), plugin_button_hbox, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), close_button_hbox, 0, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(priv->plugin_window), main_grid);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->plugin_listview));
    g_signal_connect(enable_renderer, "toggled",
        G_CALLBACK(rc_ui_plugin_toggled), NULL);
    g_signal_connect(selection, "changed",
        G_CALLBACK(rc_ui_plugin_selection_changed), NULL);
    g_signal_connect(priv->about_button, "clicked",
        G_CALLBACK(rc_ui_plugin_about_button_clicked), NULL);
    g_signal_connect(priv->config_button, "clicked",
        G_CALLBACK(rc_ui_plugin_config_button_clicked), NULL);      
    g_signal_connect(close_button, "clicked",
        G_CALLBACK(rc_ui_plugin_close_button_clicked), NULL);  
    g_signal_connect(priv->plugin_window, "key-press-event",
        G_CALLBACK(rc_ui_plugin_window_key_press_cb), NULL);  
    g_signal_connect(G_OBJECT(priv->plugin_window), "destroy",
        G_CALLBACK(rc_ui_plugin_window_destroy_cb), NULL);
    priv->registered_id = rclib_plugin_signal_connect("registered",    
        G_CALLBACK(rc_ui_plugin_registered_cb), NULL);
    priv->loaded_id = rclib_plugin_signal_connect("loaded",    
        G_CALLBACK(rc_ui_plugin_loaded_cb), NULL);
    priv->unloaded_id = rclib_plugin_signal_connect("unloaded",
        G_CALLBACK(rc_ui_plugin_unloaded_cb), NULL);
    priv->unregistered_id = rclib_plugin_signal_connect("unregistered",
        G_CALLBACK(rc_ui_plugin_unregistered_cb), NULL);
    gtk_widget_show_all(priv->plugin_window);
}


/**
 * rc_ui_plugin_window_show:
 *
 * Show a plugin configuration window.
 */

void rc_ui_plugin_window_show()
{
    RCUiPluginPrivate *priv = &plugin_priv;
    if(priv->plugin_window==NULL)
        rc_ui_plugin_window_init();
    else
        gtk_window_present(GTK_WINDOW(priv->plugin_window));
}

/**
 * rc_ui_plugin_window_destroy:
 *
 * Destroy the plugin configuration window.
 */

void rc_ui_plugin_window_destroy()
{
    RCUiPluginPrivate *priv = &plugin_priv;
    if(priv->plugin_window!=NULL)
        gtk_widget_destroy(priv->plugin_window);
}

