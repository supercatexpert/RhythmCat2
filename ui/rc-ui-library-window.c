/*
 * RhythmCat UI Library Window Module
 * The library window for the player.
 *
 * rc-ui-library-window.c
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

#include "rc-ui-library-window.h"
#include "rc-ui-library-model.h"
#include "rc-ui-library-view.h"
#include "rc-ui-menu.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-library-window
 * @Short_description: The library window of the player
 * @Title: RCUiLibraryWindow
 * @Include: rc-ui-library-window.h
 *
 * This module implements the main window class #RCUiLibraryWindow,
 * which implements the library window of the player.
 */

struct _RCUiLibraryWindowPrivate
{
    GtkTreeModel *library_genre_model;
    GtkTreeModel *library_album_model;
    GtkTreeModel *library_artist_model;
    GtkTreeModel *library_list_model;
    GtkWidget *library_main_grid;
    GtkWidget *library_view_paned;
    GtkWidget *library_prop_grid;
    GtkWidget *library_prop_genre_view;
    GtkWidget *library_prop_album_view;
    GtkWidget *library_prop_artist_view;
    GtkWidget *library_list_view;
    GtkWidget *search_entry;
    GtkWidget *library_prop_genre_scr_window;
    GtkWidget *library_prop_album_scr_window;
    GtkWidget *library_prop_artist_scr_window;
    GtkWidget *library_list_scr_window;
    guint search_type;
    gulong state_changed_id;
    guint search_timeout;
};

static GObject *ui_library_window_instance = NULL;
static gpointer rc_ui_library_window_parent_class = NULL;

static void rc_ui_library_prop_genre_row_selected(GtkTreeSelection *selection,
    gpointer data)
{
    GtkTreeIter iter;
    GtkTreePath *path = NULL;
    GtkTreeModel *model = NULL;
    gboolean total_flag = FALSE;
    GList *list, *list_foreach;
    RCLibDbQuery *query = NULL, *query_tmp;
    RCLibDbLibraryQueryResultPropIter *prop_iter;
    GObject *library_query_result;
    gchar *prop_string = NULL;
    guint prop_count = 0;
    list = gtk_tree_selection_get_selected_rows(selection, &model);
    if(list==NULL) return;
    if(model==NULL)
    {
        if(list!=NULL)
        {
            g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
        }
    }
    library_query_result = rclib_db_library_get_base_query_result();
    for(list_foreach=list;list_foreach!=NULL;
        list_foreach=g_list_next(list_foreach))
    {
        prop_string = NULL;
        path = list_foreach->data;
        if(path==NULL) continue;
        if(!gtk_tree_model_get_iter(model, &iter, path))
        {
            continue;
        }
        if(iter.user_data2!=NULL)
        {
            total_flag = TRUE;
            continue;
        }
        prop_iter = iter.user_data;
        rclib_db_library_query_result_prop_get_data(
            RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
            RCLIB_DB_QUERY_DATA_TYPE_GENRE, prop_iter, &prop_string,
            &prop_count);
        if(query!=NULL)
        {
            query_tmp = rclib_db_query_parse(
                RCLIB_DB_QUERY_CONDITION_TYPE_OR,
                RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS,
                RCLIB_DB_QUERY_DATA_TYPE_GENRE, prop_string,
                RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
            rclib_db_query_concatenate(query, query_tmp);
            rclib_db_query_free(query_tmp);
        }
        else
        {
            query = rclib_db_query_parse(
                RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS,
                RCLIB_DB_QUERY_DATA_TYPE_GENRE, prop_string,
                RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
        }
        g_free(prop_string);
    }
    g_object_unref(library_query_result);
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
    if(total_flag)
    {
        g_signal_handlers_block_by_func(selection,
            G_CALLBACK(rc_ui_library_prop_genre_row_selected), data);
        gtk_tree_selection_unselect_all(selection);
        if(gtk_tree_model_get_iter_first(model, &iter))
            gtk_tree_selection_select_iter(selection, &iter);
        g_signal_handlers_unblock_by_func(selection,
            G_CALLBACK(rc_ui_library_prop_genre_row_selected), data);
        if(query!=NULL)
            rclib_db_query_free(query);
        query = NULL;
    }
    if(query==NULL)
    {
        query = rclib_db_query_parse(RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
    }
    if(query==NULL) return;
    library_query_result = rclib_db_library_get_genre_query_result();
    rclib_db_library_query_result_set_query(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), query);
    rclib_db_query_free(query);
    rclib_db_library_query_result_query_start(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), TRUE);
    g_object_unref(library_query_result);
}

static void rc_ui_library_prop_album_row_selected(GtkTreeSelection *selection,
    gpointer data)
{
    GtkTreeIter iter;
    GtkTreePath *path = NULL;
    GtkTreeModel *model = NULL;
    gboolean total_flag = FALSE;
    GList *list, *list_foreach;
    RCLibDbQuery *query = NULL, *query_tmp;
    RCLibDbLibraryQueryResultPropIter *prop_iter;
    GObject *library_query_result;
    gchar *prop_string = NULL;
    guint prop_count = 0;
    list = gtk_tree_selection_get_selected_rows(selection, &model);
    if(list==NULL) return;
    if(model==NULL)
    {
        if(list!=NULL)
        {
            g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
        }
    }
    library_query_result = rclib_db_library_get_artist_query_result();
    for(list_foreach=list;list_foreach!=NULL;
        list_foreach=g_list_next(list_foreach))
    {
        prop_string = NULL;
        path = list_foreach->data;
        if(path==NULL) continue;
        if(!gtk_tree_model_get_iter(model, &iter, path))
        {
            continue;
        }
        if(iter.user_data2!=NULL)
        {
            total_flag = TRUE;
            continue;
        }
        prop_iter = iter.user_data;
        rclib_db_library_query_result_prop_get_data(
            RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
            RCLIB_DB_QUERY_DATA_TYPE_ALBUM, prop_iter, &prop_string,
            &prop_count);
        if(query!=NULL)
        {
            query_tmp = rclib_db_query_parse(
                RCLIB_DB_QUERY_CONDITION_TYPE_OR,
                RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS,
                RCLIB_DB_QUERY_DATA_TYPE_ALBUM, prop_string,
                RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
            rclib_db_query_concatenate(query, query_tmp);
            rclib_db_query_free(query_tmp);
        }
        else
        {
            query = rclib_db_query_parse(
                RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS,
                RCLIB_DB_QUERY_DATA_TYPE_ALBUM, prop_string,
                RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
        }
        g_free(prop_string);
    }
    g_object_unref(library_query_result);
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
    if(total_flag)
    {
        g_signal_handlers_block_by_func(selection,
            G_CALLBACK(rc_ui_library_prop_album_row_selected), data);
        gtk_tree_selection_unselect_all(selection);
        if(gtk_tree_model_get_iter_first(model, &iter))
            gtk_tree_selection_select_iter(selection, &iter);
        g_signal_handlers_unblock_by_func(selection,
            G_CALLBACK(rc_ui_library_prop_album_row_selected), data);
        if(query!=NULL)
            rclib_db_query_free(query);
        query = NULL;
    }
    if(query==NULL)
    {
        query = rclib_db_query_parse(RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
    }
    if(query==NULL) return;
    library_query_result = rclib_db_library_get_album_query_result();
    rclib_db_library_query_result_set_query(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), query);
    rclib_db_query_free(query);
    rclib_db_library_query_result_query_start(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), TRUE);
    g_object_unref(library_query_result);
}
    
static void rc_ui_library_prop_artist_row_selected(GtkTreeSelection *selection,
    gpointer data)
{
    GtkTreeIter iter;
    GtkTreePath *path = NULL;
    GtkTreeModel *model = NULL;
    gboolean total_flag = FALSE;
    GList *list, *list_foreach;
    RCLibDbQuery *query = NULL, *query_tmp;
    RCLibDbLibraryQueryResultPropIter *prop_iter;
    GObject *library_query_result;
    gchar *prop_string = NULL;
    guint prop_count = 0;
    list = gtk_tree_selection_get_selected_rows(selection, &model);
    if(list==NULL) return;
    if(model==NULL)
    {
        if(list!=NULL)
        {
            g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
        }
    }
    library_query_result = rclib_db_library_get_genre_query_result();
    for(list_foreach=list;list_foreach!=NULL;
        list_foreach=g_list_next(list_foreach))
    {
        prop_string = NULL;
        path = list_foreach->data;
        if(path==NULL) continue;
        if(!gtk_tree_model_get_iter(model, &iter, path))
        {
            continue;
        }
        if(iter.user_data2!=NULL)
        {
            total_flag = TRUE;
            continue;
        }
        prop_iter = iter.user_data;
        rclib_db_library_query_result_prop_get_data(
            RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
            RCLIB_DB_QUERY_DATA_TYPE_ARTIST, prop_iter, &prop_string,
            &prop_count);
        if(query!=NULL)
        {
            query_tmp = rclib_db_query_parse(
                RCLIB_DB_QUERY_CONDITION_TYPE_OR,
                RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS,
                RCLIB_DB_QUERY_DATA_TYPE_ARTIST, prop_string,
                RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
            rclib_db_query_concatenate(query, query_tmp);
            rclib_db_query_free(query_tmp);
        }
        else
        {
            query = rclib_db_query_parse(
                RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS,
                RCLIB_DB_QUERY_DATA_TYPE_ARTIST, prop_string,
                RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
        }
        g_free(prop_string);
    }
    g_object_unref(library_query_result);
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
    if(total_flag)
    {
        g_signal_handlers_block_by_func(selection,
            G_CALLBACK(rc_ui_library_prop_artist_row_selected), data);
        gtk_tree_selection_unselect_all(selection);
        if(gtk_tree_model_get_iter_first(model, &iter))
            gtk_tree_selection_select_iter(selection, &iter);
        g_signal_handlers_unblock_by_func(selection,
            G_CALLBACK(rc_ui_library_prop_artist_row_selected), data);
        if(query!=NULL)
            rclib_db_query_free(query);
        query = NULL;
    }
    if(query==NULL)
    {
        query = rclib_db_query_parse(RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
    }
    if(query==NULL) return;
    library_query_result = rclib_db_library_get_artist_query_result();
    rclib_db_library_query_result_set_query(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), query);
    rclib_db_query_free(query);
    rclib_db_library_query_result_query_start(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), TRUE);
    g_object_unref(library_query_result);
}    

static void rc_ui_library_list_row_activated(GtkTreeView *widget,
    GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    GObject *library_query_result;
    gchar *uri = NULL;
    RCLibDbLibraryData *library_data;
    model = gtk_tree_view_get_model(widget);
    if(model==NULL) return;
    if(!gtk_tree_model_get_iter(model, &iter, path)) return;
    library_query_result = rclib_db_library_get_album_query_result();
    library_data = rclib_db_library_query_result_get_data(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
        (RCLibDbLibraryQueryResultIter *)iter.user_data);
    g_object_unref(library_query_result);
    if(library_data==NULL) return;
    rclib_db_library_data_get(library_data, RCLIB_DB_LIBRARY_DATA_TYPE_URI,
        &uri, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
    rclib_db_library_data_unref(library_data);
    if(uri==NULL) return;
    rclib_player_play_library(uri);
    g_free(uri);
    gtk_tree_model_row_changed(model, path, &iter);
}

static void rc_ui_library_window_search_entry_icon_pressed_cb(GtkEntry *entry,
    GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer data)
{
    GtkUIManager *ui_manager;
    GtkWidget *search_menu = NULL;
    RCUiLibraryWindowPrivate *priv = (RCUiLibraryWindowPrivate *)data;
    if(data==NULL) return;
    ui_manager = rc_ui_menu_get_ui_manager();
    if(icon_pos==GTK_ENTRY_ICON_PRIMARY)
    {
        if(ui_manager!=NULL)
        {
            search_menu = gtk_ui_manager_get_widget(ui_manager,
                "/LibrarySearchMenu");
        }
        if(search_menu!=NULL)
        {
            gtk_menu_popup(GTK_MENU(search_menu), NULL, NULL, NULL, NULL,
                event->button.button, event->button.time);
        }
    }
    else if(icon_pos==GTK_ENTRY_ICON_SECONDARY)
    {
        gtk_entry_set_text(GTK_ENTRY(priv->search_entry), "");
    }   
}

static gboolean rc_ui_library_search_timeout_cb(gpointer data)
{
    RCUiLibraryWindowPrivate *priv = (RCUiLibraryWindowPrivate *)data;
    if(data==NULL) return FALSE;
    
    priv->search_timeout = 0;
    return FALSE;
}

static void rc_ui_library_window_search_entry_text_changed_cb(GtkEntry *entry,
    GParamSpec *pspec, gpointer data)
{
    RCUiLibraryWindowPrivate *priv = (RCUiLibraryWindowPrivate *)data;
    gboolean has_text = FALSE;
    if(data==NULL) return;
    has_text = (gtk_entry_get_text_length(entry)>0);
    gtk_entry_set_icon_sensitive(entry, GTK_ENTRY_ICON_SECONDARY, has_text);
    if(priv->search_timeout>0)
    {
        g_source_remove(priv->search_timeout);
    }
    priv->search_timeout = g_timeout_add(500, (GSourceFunc)
        rc_ui_library_search_timeout_cb, priv);
}

static void rc_ui_library_window_core_state_changed_cb(RCLibCore *core,
    GstState state, gpointer data)
{
    RCUiLibraryWindowPrivate *priv = (RCUiLibraryWindowPrivate *)data;
    if(data==NULL) return;
    gtk_widget_queue_draw(priv->library_list_view);
}

static gboolean rc_ui_library_window_delete_event_cb(GtkWidget *widget,
    GdkEvent *event, gpointer data)
{
    rc_ui_library_window_hide();
    return TRUE;
}

static void rc_ui_library_window_finalize(GObject *object)
{
    RCUiLibraryWindowPrivate *priv = RC_UI_LIBRARY_WINDOW(object)->priv;
    RC_UI_LIBRARY_WINDOW(object)->priv = NULL;
    if(priv->state_changed_id>0)
    {
        rclib_core_signal_disconnect(priv->state_changed_id);
        priv->state_changed_id = 0;
    }
    if(priv->search_timeout>0)
    {
        g_source_remove(priv->search_timeout);
    }
    if(priv->library_genre_model!=NULL)
        g_object_unref(priv->library_genre_model);
    if(priv->library_album_model!=NULL)
        g_object_unref(priv->library_album_model);
    if(priv->library_artist_model!=NULL)
        g_object_unref(priv->library_artist_model);
    if(priv->library_list_model!=NULL)
        g_object_unref(priv->library_list_model);
    G_OBJECT_CLASS(rc_ui_library_window_parent_class)->finalize(object);
}

static GObject *rc_ui_library_window_constructor(GType type,
    guint n_construct_params, GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(ui_library_window_instance!=NULL) return ui_library_window_instance;
    retval = G_OBJECT_CLASS(rc_ui_library_window_parent_class)->constructor(
        type, n_construct_params, construct_params);
    ui_library_window_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&ui_library_window_instance);
    return retval;
}

static void rc_ui_library_window_class_init(RCUiLibraryWindowClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_library_window_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_library_window_finalize;
    object_class->constructor = rc_ui_library_window_constructor;
    g_type_class_add_private(klass, sizeof(RCUiLibraryWindowPrivate));
}

static void rc_ui_library_window_instance_init(RCUiLibraryWindow *window)
{
    RCUiLibraryWindowPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE(window,
        RC_UI_TYPE_LIBRARY_WINDOW, RCUiLibraryWindowPrivate);
    GObject *library_query_result;
    GtkTreePath *path;
    GtkTreeSelection *selection;
    GtkUIManager *ui_manager;
    GtkWidget *search_menu;
    window->priv = priv;

    priv->search_type = 0;

    library_query_result = rclib_db_library_get_base_query_result();
    priv->library_genre_model = rc_ui_library_prop_store_new(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
        RCLIB_DB_QUERY_DATA_TYPE_GENRE);
    g_object_unref(library_query_result);
    library_query_result = rclib_db_library_get_genre_query_result();
    priv->library_artist_model = rc_ui_library_prop_store_new(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
        RCLIB_DB_QUERY_DATA_TYPE_ARTIST);
    g_object_unref(library_query_result);
    library_query_result = rclib_db_library_get_artist_query_result();   
    priv->library_album_model = rc_ui_library_prop_store_new(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
        RCLIB_DB_QUERY_DATA_TYPE_ALBUM);
    g_object_unref(library_query_result);
    library_query_result = rclib_db_library_get_album_query_result();
    priv->library_list_model = rc_ui_library_list_store_new(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result));
    g_object_unref(library_query_result);
    
    ui_manager = rc_ui_menu_get_ui_manager();
    priv->library_list_view = rc_ui_library_list_view_new();
    gtk_tree_view_set_model(GTK_TREE_VIEW(priv->library_list_view),
        priv->library_list_model);
    priv->library_prop_genre_view = rc_ui_library_prop_view_new(_("Genre"));
    gtk_tree_view_set_model(GTK_TREE_VIEW(priv->library_prop_genre_view),
        priv->library_genre_model);
    priv->library_prop_album_view = rc_ui_library_prop_view_new(_("Album"));
    gtk_tree_view_set_model(GTK_TREE_VIEW(priv->library_prop_album_view),
        priv->library_album_model);
    priv->library_prop_artist_view = rc_ui_library_prop_view_new(_("Artist"));
    gtk_tree_view_set_model(GTK_TREE_VIEW(priv->library_prop_artist_view),
        priv->library_artist_model);
    path = gtk_tree_path_new_first();
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(priv->library_prop_genre_view),
        path, NULL, FALSE);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(priv->library_prop_album_view),
        path, NULL, FALSE);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(priv->library_prop_artist_view),
        path, NULL, FALSE);
    gtk_tree_path_free(path);
    priv->search_entry = gtk_entry_new();
        
    g_object_set(window, "title", _("Music Library"), "type",
        GTK_WINDOW_TOPLEVEL, "default-width", 600, "default-height", 400,
        "has-resize-grip", FALSE, NULL);
    gtk_entry_set_icon_from_stock(GTK_ENTRY(priv->search_entry),
        GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
    gtk_entry_set_icon_from_stock(GTK_ENTRY(priv->search_entry),
        GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
    gtk_entry_set_icon_sensitive(GTK_ENTRY(priv->search_entry),
        GTK_ENTRY_ICON_SECONDARY, FALSE);
    g_object_set(priv->search_entry, "hexpand-set", TRUE, "hexpand", TRUE,
        "name", "RC2LibrarySearchEntry", NULL);
    priv->library_list_scr_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(priv->library_list_scr_window, "name",
        "RC2LibraryListScrolledWindow", "hscrollbar-policy", 
        GTK_POLICY_AUTOMATIC, "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
        "expand", TRUE, NULL);
    gtk_container_add(GTK_CONTAINER(priv->library_list_scr_window),
        priv->library_list_view);
    priv->library_prop_genre_scr_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(priv->library_prop_genre_scr_window, "name",
        "RC2LibraryPropGenreScrolledWindow", "hscrollbar-policy", 
        GTK_POLICY_NEVER, "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
        "expand", TRUE, NULL);
    gtk_container_add(GTK_CONTAINER(priv->library_prop_genre_scr_window),
        priv->library_prop_genre_view);
    priv->library_prop_artist_scr_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(priv->library_prop_artist_scr_window, "name",
        "RC2LibraryPropArtitstScrolledWindow", "hscrollbar-policy", 
        GTK_POLICY_NEVER, "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
        "expand", TRUE, NULL);
    gtk_container_add(GTK_CONTAINER(priv->library_prop_artist_scr_window),
        priv->library_prop_artist_view);
    priv->library_prop_album_scr_window = gtk_scrolled_window_new(NULL, NULL);
    g_object_set(priv->library_prop_album_scr_window, "name",
        "RC2LibraryPropAlbumScrolledWindow", "hscrollbar-policy", 
        GTK_POLICY_NEVER, "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
        "expand", TRUE, NULL);
    gtk_container_add(GTK_CONTAINER(priv->library_prop_album_scr_window),
        priv->library_prop_album_view);
    priv->library_prop_grid = gtk_grid_new();
    g_object_set(priv->library_prop_grid, "expand", TRUE, "column-homogeneous",
        TRUE, "column-spacing", 5, NULL);
    gtk_grid_attach(GTK_GRID(priv->library_prop_grid),
        priv->library_prop_genre_scr_window, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(priv->library_prop_grid),
        priv->library_prop_artist_scr_window, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(priv->library_prop_grid),
        priv->library_prop_album_scr_window, 2, 0, 1, 1);
    priv->library_view_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    g_object_set(priv->library_view_paned, "name", "RC2LibraryViewPaned",
        "expand", TRUE, "position-set", TRUE, "position", 180, NULL);
    
    if(ui_manager!=NULL)
    {
        search_menu = gtk_ui_manager_get_widget(ui_manager,
            "/LibrarySearchMenu");
        gtk_menu_attach_to_widget(GTK_MENU(search_menu), priv->search_entry,
            NULL);
    }
        
    gtk_paned_add1(GTK_PANED(priv->library_view_paned),
        priv->library_prop_grid);
    gtk_paned_add2(GTK_PANED(priv->library_view_paned),
        priv->library_list_scr_window);
    priv->library_main_grid = gtk_grid_new();
    g_object_set(priv->library_main_grid, "expand", TRUE, NULL);
    gtk_grid_attach(GTK_GRID(priv->library_main_grid), priv->search_entry,
        0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(priv->library_main_grid),
        priv->library_view_paned, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(window), priv->library_main_grid);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->library_prop_genre_view));
    g_signal_connect(selection, "changed",
        G_CALLBACK(rc_ui_library_prop_genre_row_selected), priv);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->library_prop_artist_view));
    g_signal_connect(selection, "changed",
        G_CALLBACK(rc_ui_library_prop_artist_row_selected), priv);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->library_prop_album_view));
    g_signal_connect(selection, "changed",
        G_CALLBACK(rc_ui_library_prop_album_row_selected), priv);
    g_signal_connect(priv->library_list_view, "row-activated",
        G_CALLBACK(rc_ui_library_list_row_activated), priv);
    g_signal_connect(priv->search_entry, "icon-press",
        G_CALLBACK(rc_ui_library_window_search_entry_icon_pressed_cb), priv);
    g_signal_connect(priv->search_entry, "notify::text",
        G_CALLBACK(rc_ui_library_window_search_entry_text_changed_cb), priv);
    g_signal_connect(window, "delete-event",
        G_CALLBACK(rc_ui_library_window_delete_event_cb), priv);
    priv->state_changed_id = rclib_core_signal_connect("state-changed",
        G_CALLBACK(rc_ui_library_window_core_state_changed_cb), priv);
}

GType rc_ui_library_window_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo window_info = {
        .class_size = sizeof(RCUiLibraryWindowClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_library_window_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiLibraryWindow),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_library_window_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_WINDOW,
            g_intern_static_string("RCUiLibraryWindow"), &window_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

static void rc_ui_library_window_init()
{
    ui_library_window_instance = g_object_new(RC_UI_TYPE_LIBRARY_WINDOW, NULL);
}

/**
 * rc_ui_library_window_get_widget:
 *
 * Get the library window widget. If the widget is not initialized
 * yet, it will be initialized.
 *
 * Returns: (transfer none): The library window widget.
 */

GtkWidget *rc_ui_library_window_get_widget()
{
    if(ui_library_window_instance==NULL)
        rc_ui_library_window_init();
    return GTK_WIDGET(ui_library_window_instance);
}

/**
 * rc_ui_library_window_show:
 *
 * Show the library window of the player.
 */

void rc_ui_library_window_show()
{
    GtkUIManager *ui_manager;
    if(ui_library_window_instance==NULL) return;
    ui_manager = rc_ui_menu_get_ui_manager();
    if(ui_manager!=NULL)
    {
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(
            gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ViewMenu/ViewLibrary")), TRUE);
    }
    gtk_widget_show_all(GTK_WIDGET(ui_library_window_instance));
}

/**
 * rc_ui_library_window_hide:
 *
 * Hide the library window of the player.
 */

void rc_ui_library_window_hide()
{
    GtkUIManager *ui_manager;
    if(ui_library_window_instance==NULL) return;
    ui_manager = rc_ui_menu_get_ui_manager();
    if(ui_manager!=NULL)
    {
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(
            gtk_ui_manager_get_action(ui_manager,
            "/RC2MenuBar/ViewMenu/ViewLibrary")), FALSE);
    }
    gtk_widget_hide(GTK_WIDGET(ui_library_window_instance));
}

/**
 * rc_ui_library_window_get_library_list_widget:
 * 
 * Get the library list widget in the library window.
 * 
 * Returns: (transfer none): The library list widget.
 */

GtkWidget *rc_ui_library_window_get_library_list_widget()
{
    RCUiLibraryWindowPrivate *priv;
    if(ui_library_window_instance==NULL) return NULL;
    priv = RC_UI_LIBRARY_WINDOW(ui_library_window_instance)->priv;
    if(priv==NULL) return NULL;
    return priv->library_list_view;
}


