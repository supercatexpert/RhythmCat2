/*
 * RhythmCat UI List View Module
 * Show music catalog & library in list views.
 *
 * rc-ui-listview.c
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

#include "rc-ui-listview.h"
#include "rc-ui-listmodel.h"
#include "rc-ui-cell-renderer-rating.h"
#include "rc-ui-menu.h"
#include "rc-ui-window.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-listview
 * @Short_description: Catalog and playlist list views
 * @Title: List Views
 * @Include: rc-ui-listview.h
 *
 * This module provides the catalog and playlist list view
 * widgets for the player. They show catalog and playlist
 * in the main window.
 */

enum
{
    RC_UI_LISTVIEW_TARGET_INFO_CATALOG,
    RC_UI_LISTVIEW_TARGET_INFO_PLAYLIST,
    RC_UI_LISTVIEW_TARGET_INFO_FILE,
    RC_UI_LISTVIEW_TARGET_INFO_URILIST
};

struct _RCUiCatalogViewPrivate
{
    GtkCellRenderer *state_renderer;
    GtkCellRenderer *name_renderer;
    GtkTreeViewColumn *state_column;
    GtkTreeViewColumn *name_column;
    gulong catalog_delete_id;
};

struct _RCUiPlaylistViewPrivate
{
    GtkCellRenderer *state_renderer;
    GtkCellRenderer *title_renderer;
    GtkCellRenderer *artist_renderer;
    GtkCellRenderer *album_renderer;
    GtkCellRenderer *tracknum_renderer;
    GtkCellRenderer *year_renderer;
    GtkCellRenderer *ftype_renderer;
    GtkCellRenderer *length_renderer;
    GtkCellRenderer *rating_renderer; /* Not available now */
    GtkTreeViewColumn *state_column;
    GtkTreeViewColumn *title_column;
    GtkTreeViewColumn *artist_column;
    GtkTreeViewColumn *album_column;  
    GtkTreeViewColumn *tracknum_column;
    GtkTreeViewColumn *year_column;
    GtkTreeViewColumn *ftype_column;
    GtkTreeViewColumn *length_column;
    GtkTreeViewColumn *rating_column; /* Not available now */
    gboolean display_mode;
};

static GObject *ui_catalog_view_instance = NULL;
static GObject *ui_playlist_view_instance = NULL;
static gpointer rc_ui_catalog_view_parent_class = NULL;
static gpointer rc_ui_playlist_view_parent_class = NULL;

static gboolean rc_ui_listview_multidrag_selection_block(
    GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
    gboolean pcs, gpointer data)
{
    return *(const gboolean *)data;
}

static void rc_ui_listview_block_selection(GtkWidget *widget,
    gboolean block, gint x, gint y)
{
    GtkTreeSelection *selection;
    gint *where;
    static const gboolean which[] = {FALSE, TRUE};
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    gtk_tree_selection_set_select_function(selection,
        rc_ui_listview_multidrag_selection_block,
        (gboolean *)&which[!!block], NULL);
    where = g_object_get_data(G_OBJECT(widget), "multidrag-where");
    if(where==NULL)
    {
        where = g_new(gint, 2);
        g_object_set_data_full(G_OBJECT(widget), "multidrag-where",
            where, g_free);
    }
    where[0] = x;
    where[1] = y; 
}

static void rc_ui_listview_dnd_data_get(GtkWidget *widget,
    GdkDragContext *context, GtkSelectionData *sdata, guint info,
    guint time, gpointer data)
{
    GtkTreeSelection *selection;
    static GList *path_list = NULL;
    if(path_list!=NULL)
    {
        g_list_foreach(path_list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(path_list);
        path_list = NULL;
    }
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    if(selection==NULL) return;
    path_list = gtk_tree_selection_get_selected_rows(selection, NULL);
    if(path_list==NULL) return;
    gtk_selection_data_set(sdata, gdk_atom_intern("Playlist index array",
        FALSE), 8, (gpointer)&path_list, sizeof(GList *));
}

static void rc_ui_listview_dnd_motion(GtkWidget *widget, GdkDragContext *context,
    gint x, gint y, guint time, gpointer data)
{
    GtkTreeViewDropPosition pos;
    GtkTreePath *path_drop = NULL;
    gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y,
        &path_drop, &pos);
    if(pos==GTK_TREE_VIEW_DROP_INTO_OR_BEFORE)
        pos = GTK_TREE_VIEW_DROP_BEFORE;
    if(pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER)
        pos = GTK_TREE_VIEW_DROP_AFTER;
    if(path_drop!=NULL)
    {
        gtk_tree_view_set_drag_dest_row(GTK_TREE_VIEW(widget),
            path_drop, pos);
        if(pos==GTK_TREE_VIEW_DROP_AFTER)
            gtk_tree_path_next(path_drop);
        else
            gtk_tree_path_prev(path_drop);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(widget), path_drop, NULL,
            FALSE, 0.0, 0.0);
        gtk_tree_path_free(path_drop);
    }
}

static void rc_ui_listview_dnd_delete(GtkWidget *widget, GdkDragContext *context,
    gpointer data)
{
    g_signal_stop_emission_by_name(widget, "drag-data-delete");
}

static gint rc_ui_listview_sort_comp_func(const gint a, const gint b,
    gpointer data)
{
    return a - b;
}

static inline gint *rc_ui_listview_reorder_by_selection(GtkTreeModel *model,
    GtkSelectionData *seldata, gint target, GtkTreeViewDropPosition pos)
{
    GList *path_list_foreach = NULL;
    GList *path_list = NULL;
    GtkTreePath *path_start = NULL;
    guint length = 0;
    gint i = 0, j = 0, k = 0, count = 0;
    gint list_length = 0;
    gint *indices = NULL;
    gint *index = NULL;
    gint *reorder_array = NULL;
    gboolean insert_flag = FALSE;
    if(model==NULL || seldata==NULL) return NULL;
    memcpy(&path_list, gtk_selection_data_get_data(seldata),
        sizeof(path_list));
    if(path_list==NULL) return NULL;
    length = g_list_length(path_list);
    if(length<1) return NULL;
    indices = g_new(gint, length);
    for(path_list_foreach=path_list;path_list_foreach!=NULL;
        path_list_foreach=g_list_next(path_list_foreach))
    {
        path_start = path_list_foreach->data;
        index = gtk_tree_path_get_indices(path_start);
        indices[count] = index[0];
        count++;
    }
    g_qsort_with_data(indices, length, sizeof(gint),
        (GCompareDataFunc)rc_ui_listview_sort_comp_func, NULL);
    if(pos==GTK_TREE_VIEW_DROP_AFTER ||
        pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER) target++;
    list_length = gtk_tree_model_iter_n_children(model, NULL);
    if(target<0) target = list_length;
    reorder_array = g_new0(gint, list_length);
    count = 0;
    while(i<list_length)
    {
        if((j>=length || count!=indices[j]) && count!=target)
        {
            reorder_array[i] = count;
            count++;
            i++;
        }
        else if(count==target && !insert_flag)
        {
            for(k=0;k<length;k++)
            {
                if(target==indices[k])
                {
                    target++;
                    count++;
                }
                reorder_array[i] = indices[k];
                i++;
            }
            if(i<list_length) reorder_array[i] = target;
            i++;
            count++;
            insert_flag = TRUE;
        }
        else if(j<length && count==indices[j])
        {
            count++;             
            j++;
        }
        else break;
    }
    g_free(indices);
    return reorder_array;
}

static inline void rc_ui_listview_playlist_move_to_another_catalog(
    GtkSelectionData *seldata, GtkTreeIter *target_iter)
{
    GList *path_list = NULL;
    RCLibDbPlaylistIter **iters;
    GtkTreeIter iter;
    GtkTreePath *path;
    GList *list_foreach;
    GtkTreeModel *model;
    guint length = 0;
    gint i;
    if(seldata==NULL || target_iter==NULL) return;
    if(target_iter->user_data==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    if(model==NULL) return;
    memcpy(&path_list, gtk_selection_data_get_data(seldata),
        sizeof(path_list));
    if(path_list==NULL) return;
    length = g_list_length(path_list);
    path_list = g_list_sort_with_data(path_list, (GCompareDataFunc)
        gtk_tree_path_compare, NULL);
    iters = g_new0(RCLibDbPlaylistIter *, length);
    for(list_foreach=path_list, i=0;list_foreach!=NULL;
        list_foreach=g_list_next(list_foreach), i++)
    {
        path = list_foreach->data;
        if(path==NULL) continue; 
        if(!gtk_tree_model_get_iter(model, &iter, path)) continue;
        iters[i] = iter.user_data;
    }
    rclib_db_playlist_move_to_another_catalog(iters, length,
        (RCLibDbCatalogIter *)target_iter->user_data);
}

static inline void rc_ui_listview_playlist_add_uris_by_selection(
    GtkSelectionData *seldata, RCLibDbCatalogIter *catalog_iter,
    GtkTreeIter *iter, GtkTreeViewDropPosition pos)
{
    GtkTreeModel *model; 
    gchar *uris;
    gchar **uri_array;
    gint i;
    gchar *filename;
    if(seldata==NULL || catalog_iter==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    if(model==NULL) return;
    uris = gdk_utf8_to_string_target((gchar *)
        gtk_selection_data_get_data(seldata));
    if((iter!=NULL && iter->user_data!=NULL) &&
        (pos==GTK_TREE_VIEW_DROP_AFTER ||
        pos==GTK_TREE_VIEW_DROP_INTO_OR_AFTER))
    {
        if(!gtk_tree_model_iter_next(model, iter))
        {
            iter->user_data = NULL;
            iter->stamp = 0;
        }
    }
    uri_array = g_uri_list_extract_uris(uris);
    g_free(uris);
    for(i=0;uri_array[i]!=NULL;i++)
    {
        if(uri_array[i]!=NULL)
        {
            if(rclib_util_is_supported_media(uri_array[i]))
            {
                if(iter!=NULL && iter->user_data!=NULL)
                    rclib_db_playlist_add_music(catalog_iter,
                        (RCLibDbPlaylistIter *)iter->user_data, uri_array[i]);
                else
                    rclib_db_playlist_add_music(catalog_iter, NULL, uri_array[i]);
            }
            else if(rclib_util_is_supported_list(uri_array[i]))
            {
                filename = g_filename_from_uri(uri_array[i], NULL, NULL);
                if(filename!=NULL)
                {
                    if(iter!=NULL && iter->user_data!=NULL)
                        rclib_db_playlist_add_m3u_file(catalog_iter,
                            (RCLibDbPlaylistIter *)iter->user_data, filename);
                    else
                        rclib_db_playlist_add_m3u_file(catalog_iter, NULL,
                            filename);
                    g_free(filename);
                }
            }
        }
    }
    g_strfreev(uri_array);
}

static void rc_ui_listview_catalog_dnd_data_received(GtkWidget *widget,
    GdkDragContext *context, gint x, gint y, GtkSelectionData *seldata,
    guint info, guint time, gpointer data)
{
    gint *reorder_array = NULL;
    gint *index = NULL;
    gint target = 0;
    GtkTreeViewDropPosition pos;
    GtkTreePath *path_drop = NULL;
    GtkTreeModel *model;
    GtkTreeIter target_iter;
    g_signal_stop_emission_by_name(G_OBJECT(widget), "drag-data-received");
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y,
        &path_drop, &pos);
    if(path_drop!=NULL)
    {
        if(!gtk_tree_model_get_iter(model, &target_iter, path_drop))
        {
            target_iter.user_data = NULL;
            target_iter.stamp = 0;
        }
        index = gtk_tree_path_get_indices(path_drop);
        target = index[0];
        gtk_tree_path_free(path_drop);
    }
    else target = -2;
    switch(info)
    {
        case RC_UI_LISTVIEW_TARGET_INFO_CATALOG:
            reorder_array = rc_ui_listview_reorder_by_selection(model,
                seldata, target, pos);
            if(reorder_array==NULL) break;
            rclib_db_catalog_reorder(reorder_array);
            g_free(reorder_array);
            break;
        case RC_UI_LISTVIEW_TARGET_INFO_PLAYLIST:
            rc_ui_listview_playlist_move_to_another_catalog(seldata,
                &target_iter);
            break;
        default:
            break;
    }
}

static void rc_ui_listview_playlist_dnd_data_received(GtkWidget *widget,
    GdkDragContext *context, gint x, gint y, GtkSelectionData *seldata,
    guint info, guint time, gpointer data)
{
    gint *reorder_array = NULL;
    gint *index = NULL;
    gint target = 0;
    GtkTreeViewDropPosition pos;
    GtkTreePath *path_drop = NULL;
    GtkTreeModel *model;
    RCLibDbCatalogIter *seq_iter;
    GtkTreeIter target_iter = {0};
    g_signal_stop_emission_by_name(G_OBJECT(widget), "drag-data-received");
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));
    gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y,
        &path_drop, &pos);
    if(path_drop!=NULL)
    {
        if(!gtk_tree_model_get_iter(model, &target_iter, path_drop))
        {
            target_iter.user_data = NULL;
            target_iter.stamp = 0;
        }
        index = gtk_tree_path_get_indices(path_drop);
        target = index[0];
        gtk_tree_path_free(path_drop);
    }
    else target = -2;
    switch(info)
    {
        case RC_UI_LISTVIEW_TARGET_INFO_PLAYLIST:
            reorder_array = rc_ui_listview_reorder_by_selection(model,
                seldata, target, pos);
            if(reorder_array==NULL) break;
            seq_iter = rc_ui_list_model_get_catalog_by_model(model);
            rclib_db_playlist_reorder(seq_iter, reorder_array);
            g_free(reorder_array);
            break;
        case RC_UI_LISTVIEW_TARGET_INFO_FILE:
            seq_iter = rc_ui_list_model_get_catalog_by_model(model);
            rc_ui_listview_playlist_add_uris_by_selection(seldata,
                seq_iter, &target_iter, pos);
            break;
        default:
            break;
    }
}

static gboolean rc_ui_listview_button_pressed_event(
    GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkTreePath *path = NULL;
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    if(selection==NULL) return FALSE;
    rc_ui_listview_block_selection(widget, TRUE, -1, -1);
    if(event->button!=3 && event->button!=1) return FALSE;
    if(event->button==1)
    {
        if(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK))
            return FALSE;
        if(!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), event->x,
            event->y, &path, NULL, NULL, NULL))
            return FALSE;

        if(gtk_tree_selection_path_is_selected(selection, path))
        {
            rc_ui_listview_block_selection(widget, FALSE, event->x,
                event->y);
        }
        if(path!=NULL) gtk_tree_path_free(path);
        return FALSE;
    }
    if(gtk_tree_selection_count_selected_rows(selection)>1)
        return TRUE;
    return FALSE;
}

static gboolean rc_ui_listview_catalog_button_release_event(
    GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkTreePath *path = NULL;
    GtkTreeViewColumn *col;
    gint x, y;
    gint *where = g_object_get_data(G_OBJECT(widget), "multidrag-where");
    if(where && where[0] != -1)
    {
        x = where[0];
        y = where[1];
        rc_ui_listview_block_selection(widget, TRUE, -1, -1);
        if(x==event->x && y==event->y)
        {

            if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                event->x, event->y, &path, &col, NULL, NULL))
            {
                gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path,
                    col, FALSE);
            }
            if(path) gtk_tree_path_free(path);
        }
    }
    if(event->button==3)
    {
        gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(
            rc_ui_menu_get_ui_manager(), "/CatalogPopupMenu")),
            NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
    }
    return FALSE;
}

static gboolean rc_ui_listview_playlist_button_release_event(
    GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkTreePath *path = NULL;
    GtkTreeViewColumn *col;
    gint x, y;
    gint *where = g_object_get_data(G_OBJECT(widget), "multidrag-where");
    if(where && where[0] != -1)
    {
        x = where[0];
        y = where[1];
        rc_ui_listview_block_selection(widget, TRUE, -1, -1);
        if(x==event->x && y==event->y)
        {

            if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                event->x, event->y, &path, &col, NULL, NULL))
            {
                gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path,
                    col, FALSE);
            }
            if(path) gtk_tree_path_free(path);
        }
    }
    if(event->button==3)
    {
        gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(
            rc_ui_menu_get_ui_manager(), "/PlaylistPopupMenu")),
            NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
    }
    return FALSE;
}

static void rc_ui_listview_catalog_set_drag(GtkWidget *widget)
{
    GtkTargetEntry entry[2];   
    entry[0].target = "RhythmCat2/CatalogItem";
    entry[0].flags = GTK_TARGET_SAME_WIDGET;
    entry[0].info = RC_UI_LISTVIEW_TARGET_INFO_CATALOG;
    entry[1].target = "RhythmCat2/PlaylistItem";
    entry[1].flags = GTK_TARGET_SAME_APP;
    entry[1].info = RC_UI_LISTVIEW_TARGET_INFO_PLAYLIST;
    gtk_drag_source_set(widget, GDK_BUTTON1_MASK, entry, 1,
        GDK_ACTION_MOVE);
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, entry, 2,
        GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
}

static void rc_ui_listview_playlist_set_drag(GtkWidget *widget)
{
    GtkTargetEntry entry[4];
    entry[0].target = "RhythmCat2/PlaylistItem";
    entry[0].flags = GTK_TARGET_SAME_APP;
    entry[0].info = RC_UI_LISTVIEW_TARGET_INFO_PLAYLIST;
    entry[1].target = "STRING";
    entry[1].flags = 0;
    entry[1].info = RC_UI_LISTVIEW_TARGET_INFO_FILE;
    entry[2].target = "text/plain";
    entry[2].flags = 0;
    entry[2].info = RC_UI_LISTVIEW_TARGET_INFO_FILE;
    entry[3].target = "text/uri-list";
    entry[3].flags = 0;
    entry[3].info = RC_UI_LISTVIEW_TARGET_INFO_URILIST;
    gtk_drag_source_set(widget, GDK_BUTTON1_MASK, entry, 1,
        GDK_ACTION_MOVE);
    gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, entry, 4,
        GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
}

static void rc_ui_listview_catalog_row_selected(GtkTreeView *view,
    gpointer data)
{
    GtkTreeIter iter;
    RCLibDbCatalogIter *catalog_iter;
    gpointer store = NULL;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    catalog_iter = iter.user_data;
    if(catalog_iter==NULL) return;
    rclib_db_catalog_data_iter_get(catalog_iter,
        RCLIB_DB_CATALOG_DATA_TYPE_STORE, &store,
        RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    if(!RC_UI_IS_PLAYLIST_STORE(store)) return;
    gtk_tree_view_set_model(GTK_TREE_VIEW(ui_playlist_view_instance),
        GTK_TREE_MODEL(store));
}

static void rc_ui_listview_playlist_row_activated(GtkTreeView *widget,
    GtkTreePath *path, GtkTreeViewColumn *column, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    model = gtk_tree_view_get_model(widget);
    if(model==NULL) return;
    if(!gtk_tree_model_get_iter(model, &iter, path)) return;
    rclib_player_play_db((GSequenceIter *)iter.user_data);
}

static void rc_ui_listview_catalog_row_edited(GtkCellRendererText *renderer,
    gchar *path_str, gchar *new_text, gpointer data)
{
    GtkTreeIter iter = {0};
    if(new_text==NULL || strlen(new_text)<=0) return;
    if(!gtk_tree_model_get_iter_from_string(
        rc_ui_list_model_get_catalog_store(), &iter, path_str))
        return;
    if(iter.user_data==NULL) return;
    rclib_db_catalog_set_name((RCLibDbCatalogIter *)iter.user_data, new_text);
}

static void rc_ui_listview_catalog_name_call_data_func(
    GtkTreeViewColumn *column, GtkCellRenderer *renderer,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gboolean flag;
    gtk_tree_model_get(model, iter, RC_UI_CATALOG_COLUMN_PLAYING_FLAG,
        &flag, -1);
    if(flag)
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_BOLD, NULL);
    else
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_NORMAL, NULL);
}

static void rc_ui_listview_playlist_text_call_data_func(
    GtkTreeViewColumn *column, GtkCellRenderer *renderer,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gboolean flag;
    gtk_tree_model_get(model, iter, RC_UI_PLAYLIST_COLUMN_PLAYING_FLAG,
        &flag, -1);
    if(flag)
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_BOLD, NULL);
    else
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_NORMAL, NULL);
}

static void rc_ui_listview_catalog_delete_cb(RCLibDb *db,
    RCLibDbCatalogIter *iter, gpointer data)
{
    GtkTreeModel *model;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(ui_playlist_view_instance));
    if(model==NULL) return;
    if(iter==rc_ui_list_model_get_catalog_by_model(model))
    {
        gtk_tree_view_set_model(GTK_TREE_VIEW(ui_playlist_view_instance),
            NULL);
    }
}

static gboolean rc_ui_listview_catalog_search_comparison_func(
    GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter,
    gpointer search_data)
{
    gchar *name = NULL;
    gchar *name_down, *key_down;
    gboolean flag;
    gtk_tree_model_get(model, iter, RC_UI_CATALOG_COLUMN_NAME, &name, -1);
    if(name==NULL) return TRUE;
    name_down = g_utf8_strdown(name, -1);
    key_down = g_utf8_strdown(key, -1);
    g_free(name);
    if(g_strstr_len(name_down, -1, key_down)!=NULL)
        flag = FALSE;
    else
        flag = TRUE;
    g_free(name_down);
    g_free(key_down);
    return flag;
}

static gboolean rc_ui_listview_playlist_search_comparison_func(
    GtkTreeModel *model, gint column, const gchar *key, GtkTreeIter *iter,
    gpointer search_data)
{
    gchar *name = NULL;
    gchar *name_down, *key_down;
    gboolean flag;
    gtk_tree_model_get(model, iter, RC_UI_PLAYLIST_COLUMN_FTITLE, &name, -1);
    if(name==NULL) return TRUE;
    name_down = g_utf8_strdown(name, -1);
    key_down = g_utf8_strdown(key, -1);
    g_free(name);
    if(g_strstr_len(name_down, -1, key_down)!=NULL)
        flag = FALSE;
    else
        flag = TRUE;
    g_free(name_down);
    g_free(key_down);
    return flag;
}

static void rc_ui_playlist_view_rated_cb(RCUiCellRendererRating *renderer,
    const char *path, gfloat rating, gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter = {0};
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    if(model==NULL) return;
    if(!gtk_tree_model_get_iter_from_string(model, &iter, path))
        return;
    rclib_db_playlist_set_rating((RCLibDbPlaylistIter *)iter.user_data,
        rating);
}

static void rc_ui_catalog_view_finalize(GObject *object)
{
    RCUiCatalogViewPrivate *priv = NULL;
    priv = RC_UI_CATALOG_VIEW(object)->priv;
    if(priv->catalog_delete_id>0)
        rclib_db_signal_disconnect(priv->catalog_delete_id);
    G_OBJECT_CLASS(rc_ui_catalog_view_parent_class)->finalize(object);
}

static void rc_ui_playlist_view_finalize(GObject *object)
{
    G_OBJECT_CLASS(rc_ui_playlist_view_parent_class)->finalize(object);
}

static GObject *rc_ui_catalog_view_constructor(GType type,
    guint n_construct_params, GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(ui_catalog_view_instance!=NULL) return ui_catalog_view_instance;
    retval = G_OBJECT_CLASS(rc_ui_catalog_view_parent_class)->constructor(
        type, n_construct_params, construct_params);
    ui_catalog_view_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&ui_catalog_view_instance);
    return retval;
}

static GObject *rc_ui_playlist_view_constructor(GType type,
    guint n_construct_params, GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(ui_playlist_view_instance!=NULL) return ui_playlist_view_instance;
    retval = G_OBJECT_CLASS(rc_ui_playlist_view_parent_class)->constructor(
        type, n_construct_params, construct_params);
    ui_playlist_view_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&ui_playlist_view_instance);
    return retval;
}

static void rc_ui_catalog_view_class_init(RCUiCatalogViewClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_catalog_view_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_catalog_view_finalize;
    object_class->constructor = rc_ui_catalog_view_constructor;
    g_type_class_add_private(klass, sizeof(RCUiCatalogViewPrivate));
}

static void rc_ui_playlist_view_class_init(RCUiPlaylistViewClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_playlist_view_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_playlist_view_finalize;
    object_class->constructor = rc_ui_playlist_view_constructor;
    g_type_class_add_private(klass, sizeof(RCUiPlaylistViewPrivate));
}

static void rc_ui_catalog_view_instance_init(RCUiCatalogView *view)
{
    RCUiCatalogViewPrivate *priv = NULL;
    GtkTreeSelection *selection;
    priv = G_TYPE_INSTANCE_GET_PRIVATE(view, RC_UI_TYPE_CATALOG_VIEW,
        RCUiCatalogViewPrivate);
    view->priv = priv;
    g_object_set(view, "name", "RC2CatalogListView", "headers-visible",
        FALSE, "reorderable", FALSE, "rules-hint", TRUE, "enable-search",
        TRUE, NULL);
    priv->state_renderer = gtk_cell_renderer_pixbuf_new();
    priv->name_renderer = gtk_cell_renderer_text_new();
    gtk_cell_renderer_set_fixed_size(priv->state_renderer, 16, -1);
    gtk_cell_renderer_set_fixed_size(priv->name_renderer, 80, -1);
    g_object_set(priv->name_renderer, "ellipsize", PANGO_ELLIPSIZE_END,
        "ellipsize-set", TRUE, "weight", PANGO_WEIGHT_NORMAL, "weight-set",
        TRUE, NULL);
    priv->state_column = gtk_tree_view_column_new_with_attributes("#",
        priv->state_renderer, "stock-id", RC_UI_CATALOG_COLUMN_STATE, NULL);
    priv->name_column = gtk_tree_view_column_new_with_attributes(_("Name"),
        priv->name_renderer, "text", RC_UI_CATALOG_COLUMN_NAME, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->name_column,
        priv->name_renderer,
        rc_ui_listview_catalog_name_call_data_func, NULL, NULL);
    g_object_set(priv->state_column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED,
        "fixed-width", 30, "min-width", 30, "max-width", 30, NULL);
    g_object_set(priv->name_column, "expand", TRUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->state_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->name_column);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
    gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(view),
        rc_ui_listview_catalog_search_comparison_func, NULL, NULL);
    rc_ui_listview_catalog_set_drag(GTK_WIDGET(view));
    g_signal_connect(priv->name_renderer, "edited",
        G_CALLBACK(rc_ui_listview_catalog_row_edited), NULL);
    g_signal_connect(view, "drag-motion",
        G_CALLBACK(rc_ui_listview_dnd_motion), NULL);
    g_signal_connect(view, "drag-data-get",
        G_CALLBACK(rc_ui_listview_dnd_data_get), NULL);
    g_signal_connect(view, "drag-data-delete",
        G_CALLBACK(rc_ui_listview_dnd_delete), NULL);
    g_signal_connect(view, "drag-data-received",
        G_CALLBACK(rc_ui_listview_catalog_dnd_data_received), NULL);
    g_signal_connect(view, "button-press-event",
        G_CALLBACK(rc_ui_listview_button_pressed_event), NULL);
    g_signal_connect(view, "button-release-event",
        G_CALLBACK(rc_ui_listview_catalog_button_release_event), NULL);
    g_signal_connect(view, "cursor-changed",
        G_CALLBACK(rc_ui_listview_catalog_row_selected), NULL);
    priv->catalog_delete_id = rclib_db_signal_connect("catalog-delete",
        G_CALLBACK(rc_ui_listview_catalog_delete_cb), NULL);
}

static void rc_ui_playlist_view_instance_init(RCUiPlaylistView *view)
{
    RCUiPlaylistViewPrivate *priv = NULL;
    GtkTreeSelection *selection;
    priv = G_TYPE_INSTANCE_GET_PRIVATE(view, RC_UI_TYPE_PLAYLIST_VIEW,
        RCUiPlaylistViewPrivate);
    view->priv = priv;
    g_object_set(view, "name", "RC2PlaylistListView", "headers-visible",
        FALSE, "reorderable", FALSE, "rules-hint", TRUE, "enable-search",
        TRUE, NULL);
    priv->state_renderer = gtk_cell_renderer_pixbuf_new();
    priv->title_renderer = gtk_cell_renderer_text_new();
    priv->artist_renderer = gtk_cell_renderer_text_new();
    priv->album_renderer = gtk_cell_renderer_text_new();
    priv->tracknum_renderer = gtk_cell_renderer_text_new();
    priv->year_renderer = gtk_cell_renderer_text_new();
    priv->ftype_renderer = gtk_cell_renderer_text_new();
    priv->length_renderer = gtk_cell_renderer_text_new();
    priv->rating_renderer = rc_ui_cell_renderer_rating_new();
    gtk_cell_renderer_set_fixed_size(priv->state_renderer, 16, -1);
    gtk_cell_renderer_set_fixed_size(priv->title_renderer, 120, -1);
    gtk_cell_renderer_set_fixed_size(priv->artist_renderer, 60, -1);
    gtk_cell_renderer_set_fixed_size(priv->album_renderer, 60, -1);
    gtk_cell_renderer_set_fixed_size(priv->tracknum_renderer,
        50, -1);
    gtk_cell_renderer_set_fixed_size(priv->year_renderer, 50, -1);
    gtk_cell_renderer_set_fixed_size(priv->ftype_renderer, 60, -1);
    gtk_cell_renderer_set_fixed_size(priv->length_renderer, 55, -1);
    g_object_set(priv->title_renderer, "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(priv->artist_renderer, "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(priv->album_renderer, "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(priv->tracknum_renderer, "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(priv->year_renderer, "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(priv->ftype_renderer, "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(priv->length_renderer, "xalign", 1.0,
        "width-chars", 6, NULL);
    priv->state_column = gtk_tree_view_column_new_with_attributes(
        "#", priv->state_renderer, "stock-id",
        RC_UI_PLAYLIST_COLUMN_STATE, NULL);
    priv->title_column = gtk_tree_view_column_new_with_attributes(
        _("Title"), priv->title_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_FTITLE, NULL);
    priv->artist_column = gtk_tree_view_column_new_with_attributes(
        _("Artist"), priv->artist_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_ARTIST, NULL);
    priv->album_column = gtk_tree_view_column_new_with_attributes(
        _("Album"), priv->artist_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_ALBUM, NULL);
    priv->tracknum_column = gtk_tree_view_column_new_with_attributes(
        _("Track"), priv->tracknum_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_TRACK, NULL);
    priv->year_column = gtk_tree_view_column_new_with_attributes(
        _("Year"), priv->year_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_YEAR, NULL);
    priv->ftype_column = gtk_tree_view_column_new_with_attributes(
        _("Format"), priv->ftype_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_FTYPE, NULL);
    priv->length_column = gtk_tree_view_column_new_with_attributes(
        _("Length"), priv->length_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_LENGTH, NULL);
    priv->rating_column = gtk_tree_view_column_new_with_attributes(
        _("Rating"), priv->rating_renderer, "rating",
        RC_UI_PLAYLIST_COLUMN_RATING, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->title_column,
        priv->title_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->artist_column,
        priv->artist_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->album_column,
        priv->album_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->tracknum_column,
        priv->tracknum_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->year_column,
        priv->year_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->ftype_column,
        priv->ftype_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->length_column,
        priv->length_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    g_object_set(priv->state_column, "sizing",
        GTK_TREE_VIEW_COLUMN_FIXED, "fixed-width", 30, "min-width", 30,
        "max-width", 30, NULL);        
    g_object_set(priv->title_column, "expand", TRUE, NULL);
    g_object_set(priv->artist_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->album_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->tracknum_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->year_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->ftype_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->length_column, "sizing",
        GTK_TREE_VIEW_COLUMN_FIXED, "fixed-width", 55, "alignment",
        1.0, NULL);
    g_object_set(priv->rating_column, "sizing",
        GTK_TREE_VIEW_COLUMN_FIXED, "fixed-width", 80, "alignment",
        1.0, "visible", FALSE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->state_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->title_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->artist_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->album_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->tracknum_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->year_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->ftype_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->length_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->rating_column);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
    gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(view),
        rc_ui_listview_playlist_search_comparison_func, NULL, NULL);
    rc_ui_listview_playlist_set_drag(GTK_WIDGET(view));
    g_signal_connect(view, "drag-motion",
        G_CALLBACK(rc_ui_listview_dnd_motion), NULL);
    g_signal_connect(view, "drag-data-get",
        G_CALLBACK(rc_ui_listview_dnd_data_get), NULL);
    g_signal_connect(view, "drag-data-delete",
        G_CALLBACK(rc_ui_listview_dnd_delete), NULL);
    g_signal_connect(view, "drag-data-received",
        G_CALLBACK(rc_ui_listview_playlist_dnd_data_received), NULL);
    g_signal_connect(view, "button-press-event",
        G_CALLBACK(rc_ui_listview_button_pressed_event), NULL);
    g_signal_connect(view, "button-release-event",
        G_CALLBACK(rc_ui_listview_playlist_button_release_event), NULL);
    g_signal_connect(view, "row-activated",
        G_CALLBACK(rc_ui_listview_playlist_row_activated), NULL);
    g_signal_connect(priv->rating_renderer, "rated",
        G_CALLBACK(rc_ui_playlist_view_rated_cb), priv);
}

GType rc_ui_catalog_view_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo view_info = {
        .class_size = sizeof(RCUiCatalogViewClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_catalog_view_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiCatalogView),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_catalog_view_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_TREE_VIEW,
            g_intern_static_string("RCUiCatalogView"), &view_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rc_ui_playlist_view_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo view_info = {
        .class_size = sizeof(RCUiPlaylistViewClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_playlist_view_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiPlaylistView),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_playlist_view_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_TREE_VIEW,
            g_intern_static_string("RCUiPlaylistView"), &view_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

static void rc_ui_listview_init()
{
    GtkTreeModel *catalog_model;
    GtkTreeModel *playlist_model;
    GtkTreeSelection *catalog_selection;
    GtkTreeSelection *playlist_selection;
    GtkTreeIter catalog_iter, playlist_iter;
    ui_catalog_view_instance = g_object_new(RC_UI_TYPE_CATALOG_VIEW, NULL);
    ui_playlist_view_instance = g_object_new(RC_UI_TYPE_PLAYLIST_VIEW, NULL);
    catalog_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        ui_catalog_view_instance));
    catalog_model = rc_ui_list_model_get_catalog_store();
    if(catalog_model!=NULL)
    {
        gtk_tree_view_set_model(GTK_TREE_VIEW(ui_catalog_view_instance),
            catalog_model);
        if(gtk_tree_model_get_iter_first(catalog_model, &catalog_iter))
        {
            playlist_model = rc_ui_list_model_get_playlist_store(
                &catalog_iter);
            gtk_tree_selection_select_iter(catalog_selection, &catalog_iter);
            if(playlist_model!=NULL)
            {
                gtk_tree_view_set_model(GTK_TREE_VIEW(
                    ui_playlist_view_instance), playlist_model);
                playlist_selection = gtk_tree_view_get_selection(
                    GTK_TREE_VIEW(ui_playlist_view_instance));
                if(gtk_tree_model_get_iter_first(playlist_model,
                    &playlist_iter))
                {
                    gtk_tree_selection_select_iter(playlist_selection,
                        &playlist_iter);
                }
            }
        }
    }
}

/**
 * rc_ui_listview_get_catalog_widget:
 *
 * Get the catalog list view widget. If the widget is not initialized
 * yet, it will be initialized.
 *
 * Returns: (transfer none): the catalog widget.
 */

GtkWidget *rc_ui_listview_get_catalog_widget()
{
    if(ui_catalog_view_instance==NULL)
        rc_ui_listview_init();
    return GTK_WIDGET(ui_catalog_view_instance);
}

/**
 * rc_ui_listview_get_playlist_widget:
 *
 * Get the playlist list view widget. If the widget is not initialized
 * yet, it will be initialized.
 *
 * Returns: (transfer none): the playlist widget.
 */

GtkWidget *rc_ui_listview_get_playlist_widget()
{
    if(ui_playlist_view_instance==NULL)
        rc_ui_listview_init();
    return GTK_WIDGET(ui_playlist_view_instance);
}

/**
 * rc_ui_listview_catalog_set_pango_attributes:
 * @list: the pango attribute list
 *
 * Set the pango attribute for the catalog list view.
 */

void rc_ui_listview_catalog_set_pango_attributes(const PangoAttrList *list)
{
    RCUiCatalogViewPrivate *priv = NULL;
    if(ui_catalog_view_instance==NULL) return;
    priv = RC_UI_CATALOG_VIEW(ui_catalog_view_instance)->priv;
    if(priv==NULL) return;
    g_object_set(priv->name_renderer, "attributes", list, NULL);
}

/**
 * rc_ui_listview_playlist_set_pango_attributes:
 * @list: the pango attribute list
 *
 * Set the pango attribute for the playlist list view.
 */

void rc_ui_listview_playlist_set_pango_attributes(const PangoAttrList *list)
{
    RCUiPlaylistViewPrivate *priv = NULL;
    if(ui_playlist_view_instance==NULL) return;
    priv = RC_UI_PLAYLIST_VIEW(ui_playlist_view_instance)->priv;
    if(priv==NULL) return;
    g_object_set(priv->title_renderer, "attributes", list, NULL);
    g_object_set(priv->artist_renderer, "attributes", list, NULL);
    g_object_set(priv->album_renderer, "attributes", list, NULL);
    g_object_set(priv->tracknum_renderer, "attributes", list, NULL);
    g_object_set(priv->year_renderer, "attributes", list, NULL);
    g_object_set(priv->ftype_renderer, "attributes", list, NULL);
    g_object_set(priv->length_renderer, "attributes", list, NULL);
}

/**
 * rc_ui_listview_catalog_select:
 * @iter: the iter to the playlist in the catalog
 *
 * Select the playlist in the catalog.
 */

void rc_ui_listview_catalog_select(GtkTreeIter *iter)
{
    GtkTreePath *path;
    GtkTreeModel *model;
    if(iter==NULL || ui_catalog_view_instance==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(ui_catalog_view_instance));
    if(model==NULL) return;
    path = gtk_tree_model_get_path(model, iter);
    if(path==NULL) return;
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(ui_catalog_view_instance), path, 
        NULL, FALSE);
    gtk_tree_path_free(path);
}

/**
 * rc_ui_listview_playlist_select:
 * @iter: the iter to the playlist in the catalog
 *
 * Select the music in the playlist.
 */

void rc_ui_listview_playlist_select(GtkTreeIter *iter)
{
    GtkTreePath *path;
    GtkTreeModel *model;
    if(iter==NULL || ui_playlist_view_instance==NULL);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    if(model==NULL) return;
    path = gtk_tree_model_get_path(model, iter);
    if(path==NULL) return;
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(ui_playlist_view_instance), path,
        NULL, FALSE);
    gtk_tree_path_free(path);
}

/**
 * rc_ui_listview_catalog_get_cursor:
 * @iter: the uninitialized GtkTreeIter
 *
 * Get the GtkTreeIter of the selected playlist item in the catalog.
 *
 * Returns: TRUE, if iter was set.
 */

gboolean rc_ui_listview_catalog_get_cursor(GtkTreeIter *iter)
{
    GtkTreePath *path;
    GtkTreeModel *model;
    gboolean flag;
    if(iter==NULL || ui_catalog_view_instance==NULL) return FALSE;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(
        ui_catalog_view_instance));
    if(model==NULL) return FALSE;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ui_catalog_view_instance), &path,
        NULL);
    if(path==NULL) return FALSE;
    flag = gtk_tree_model_get_iter(model, iter, path);
    gtk_tree_path_free(path);
    return flag;
}

/**
 * rc_ui_listview_playlist_get_cursor:
 * @iter: the uninitialized GtkTreeIter
 *
 * Get the GtkTreeIter of the selected music item in the playlist.
 *
 * Returns: TRUE, if iter was set.
 */

gboolean rc_ui_listview_playlist_get_cursor(GtkTreeIter *iter)
{
    GtkTreePath *path;
    GtkTreeModel *model;
    gboolean flag;
    if(iter==NULL || ui_playlist_view_instance==NULL) return FALSE;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    if(model==NULL) return FALSE;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(ui_playlist_view_instance),
        &path, NULL);
    if(path==NULL) return FALSE;
    flag = gtk_tree_model_get_iter(model, iter, path);
    gtk_tree_path_free(path);
    return flag;
}

/**
 * rc_ui_listview_catalog_get_model:
 *
 * Get the #GtkTreeModel used in the current catalog list view.
 *
 * Returns: (transfer none): The #GtkTreeModel.
 */

GtkTreeModel *rc_ui_listview_catalog_get_model()
{
    if(ui_catalog_view_instance==NULL) return NULL;
    return gtk_tree_view_get_model(GTK_TREE_VIEW(ui_catalog_view_instance));
}

/**
 * rc_ui_listview_playlist_get_model:
 *
 * Get the #GtkTreeModel used in the current playlist list view.
 *
 * Returns: (transfer none): The #GtkTreeModel.
 */


GtkTreeModel *rc_ui_listview_playlist_get_model()
{
    if(ui_playlist_view_instance==NULL) return NULL;
    return gtk_tree_view_get_model(GTK_TREE_VIEW(ui_playlist_view_instance));
}

/**
 * rc_ui_listview_catalog_new_playlist:
 *
 * Create a new playlist in the catalog.
 */

void rc_ui_listview_catalog_new_playlist()
{
    static guint i = 1;
    GtkTreeModel *model;
    GtkTreeIter iter, new_iter;
    RCLibDbCatalogIter *catalog_iter;
    gchar *new_name;
    if(ui_catalog_view_instance==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(ui_catalog_view_instance));
    if(model==NULL) return;
    new_name = g_strdup_printf(_("Playlist %u"), i);
    i++;
    if(!rc_ui_listview_catalog_get_cursor(&iter))
    {
        iter.user_data = NULL;
    }
    catalog_iter = rclib_db_catalog_add(new_name, (RCLibDbCatalogIter *)
        iter.user_data, RCLIB_DB_CATALOG_TYPE_PLAYLIST);
    g_free(new_name);
    gtk_tree_model_get_iter_first(model, &iter);
    new_iter.stamp = iter.stamp;
    new_iter.user_data = catalog_iter;
    rc_ui_listview_catalog_select(&new_iter);
    rc_ui_listview_catalog_rename_playlist();
}

/**
 * rc_ui_listview_catalog_rename_playlist:
 *
 * Rename a list (make the name of the selected playlist editable).
 */

void rc_ui_listview_catalog_rename_playlist()
{
    RCUiCatalogViewPrivate *priv = NULL;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    if(ui_catalog_view_instance==NULL) return;
    priv = RC_UI_CATALOG_VIEW(ui_catalog_view_instance)->priv;
    if(priv==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(ui_catalog_view_instance));
    if(model==NULL) return;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    path = gtk_tree_model_get_path(model, &iter);
    if(path==NULL) return;
    g_object_set(priv->name_renderer, "editable", TRUE, "editable-set",
        TRUE, NULL);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(ui_catalog_view_instance), path,
        priv->name_column, TRUE);
    g_object_set(priv->name_renderer, "editable", FALSE, "editable-set",
        FALSE, NULL);
    gtk_tree_path_free(path);
}

/**
 * rc_ui_listview_catalog_delete_items:
 *
 * Delete the selected playlist(s) in the catalog.
 */

void rc_ui_listview_catalog_delete_items()
{
    GtkTreeSelection *selection;
    GList *path_list = NULL;
    GList *list_foreach;
    GtkTreeIter iter;
    GtkTreeModel *model;
    if(ui_catalog_view_instance==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(ui_catalog_view_instance));
    if(model==NULL) return;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        ui_catalog_view_instance));
    if(selection==NULL) return;
    path_list = gtk_tree_selection_get_selected_rows(selection, NULL);
    if(path_list==NULL) return;
    path_list = g_list_sort(path_list, (GCompareFunc)gtk_tree_path_compare);
    for(list_foreach=g_list_last(path_list);list_foreach!=NULL;
        list_foreach=g_list_previous(list_foreach))
    {
        if(!gtk_tree_model_get_iter(model, &iter, list_foreach->data))
            continue;
        if(iter.user_data==NULL) continue;
        rclib_db_catalog_delete((RCLibDbCatalogIter *)iter.user_data);
    }
    g_list_foreach(path_list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(path_list);
}

/**
 * rc_ui_listview_playlist_select_all:
 *
 * Select all items in the selected playlist.
 */

void rc_ui_listview_playlist_select_all()
{
    GtkTreeSelection *selection;
    if(ui_playlist_view_instance==NULL) return;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    gtk_tree_selection_select_all(selection);
}

/**
 * rc_ui_listview_playlist_delete_items:
 *
 * Delete the selected item(s) in the selected playlist.
 */

void rc_ui_listview_playlist_delete_items()
{
    GtkTreeSelection *selection;
    GList *path_list = NULL;
    GList *list_foreach;
    GtkTreeIter iter;
    GtkTreeModel *model;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    if(model==NULL) return;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        ui_playlist_view_instance));
    if(selection==NULL) return;
    path_list = gtk_tree_selection_get_selected_rows(selection, NULL);
    if(path_list==NULL) return;
    path_list = g_list_sort(path_list, (GCompareFunc)gtk_tree_path_compare);
    for(list_foreach=g_list_last(path_list);list_foreach!=NULL;
        list_foreach=g_list_previous(list_foreach))
    {
        if(!gtk_tree_model_get_iter(model, &iter, list_foreach->data))
            continue;
        if(iter.user_data==NULL) continue;
        rclib_db_playlist_delete((RCLibDbPlaylistIter *)iter.user_data);
    }
    g_list_foreach(path_list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(path_list);
}

/**
 * rc_ui_listview_playlist_refresh:
 *
 * Refresh the metadata of all music in the selected playlist.
 */

void rc_ui_listview_playlist_refresh()
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(ui_catalog_view_instance));
    if(model==NULL) return;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    rclib_db_playlist_refresh((RCLibDbCatalogIter *)iter.user_data);
}

/**
 * rc_ui_listview_playlist_set_column_display_mode:
 * @mode: the column display mode
 *
 * Set the column display mode, set it to #FALSE to use single title column
 * mode, set it to #TRUE to show metadata in more than one columns and
 * the list header.
 */

void rc_ui_listview_playlist_set_column_display_mode(gboolean mode)
{
    RCUiPlaylistViewPrivate *priv = NULL;
    if(ui_playlist_view_instance==NULL) return;
    priv = RC_UI_PLAYLIST_VIEW(ui_playlist_view_instance)->priv;
    if(priv==NULL) return;
    priv->display_mode = mode;
    if(mode)
    {
        g_object_set(ui_playlist_view_instance, "headers-visible",
            TRUE, NULL);
        rc_ui_listview_playlist_set_title_format("%TITLE");
    }
    else
    {
        g_object_set(ui_playlist_view_instance, "headers-visible",
            FALSE, NULL);
        g_object_set(priv->artist_column, "visible", FALSE, NULL);
        g_object_set(priv->album_column, "visible", FALSE, NULL);
        g_object_set(priv->tracknum_column, "visible", FALSE, NULL);
        g_object_set(priv->year_column, "visible", FALSE, NULL);
        g_object_set(priv->ftype_column, "visible", FALSE, NULL);
        g_object_set(priv->rating_column, "visible", FALSE, NULL);
    }
    rc_ui_main_window_playlist_scrolled_window_set_horizontal_policy(mode);
}

/**
 * rc_ui_listview_playlist_get_column_display_mode:
 *
 * Get the column display mode.
 *
 * Returns: The column display mode.
 */

gboolean rc_ui_listview_playlist_get_column_display_mode()
{
    RCUiPlaylistViewPrivate *priv = NULL;
    if(ui_playlist_view_instance==NULL) return FALSE;
    priv = RC_UI_PLAYLIST_VIEW(ui_playlist_view_instance)->priv;
    if(priv==NULL) return FALSE;
    return priv->display_mode;
}

/**
 * rc_ui_listview_playlist_set_title_format:
 * @format: the format string
 *
 * Set the format string of the title column in the playlist, using
 * \%TITLE as title string, \%ARTIST as artist string, \%ALBUM as album
 * string.
 * Notice that \%TITLE should be always included in the string.
 */

void rc_ui_listview_playlist_set_title_format(const gchar *format)
{
    if(ui_playlist_view_instance==NULL) return;
    if(format==NULL || g_strstr_len(format, -1, "%TITLE")==NULL)
        rc_ui_list_model_set_playlist_title_format("%TITLE");
    else
        rc_ui_list_model_set_playlist_title_format(format);
    gtk_widget_queue_draw(GTK_WIDGET(ui_playlist_view_instance));
}

/**
 * rc_ui_listview_playlist_set_enabled_columns:
 * @column_flags: the columns to set
 * @enable_flags: set the columns state
 *
 * Enable or disable (set the visibility of) some columns by the given flags.
 * Notice that this function will only take effects if the display mode is
 * set to #TRUE.
 */

void rc_ui_listview_playlist_set_enabled_columns(guint column_flags,
    guint enable_flags)
{
    RCUiPlaylistViewPrivate *priv = NULL;
    if(ui_playlist_view_instance==NULL) return;
    priv = RC_UI_PLAYLIST_VIEW(ui_playlist_view_instance)->priv;
    if(priv==NULL) return;
    if(!priv->display_mode) return;
    if(column_flags==0) return;
    if(column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST)
    {
        g_object_set(priv->artist_column, "visible",
            enable_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST ?
            TRUE: FALSE, NULL);
    }
    if(column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM)
    {
        g_object_set(priv->album_column, "visible",
            enable_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM ?
            TRUE: FALSE, NULL);
    }
    if(column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK)
    {
        g_object_set(priv->tracknum_column, "visible",
            enable_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK ?
            TRUE: FALSE, NULL);
    }
    if(column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR)
    {
        g_object_set(priv->year_column, "visible",
            enable_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR ?
            TRUE: FALSE, NULL);
    }
    if(column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE)
    {
        g_object_set(priv->ftype_column, "visible",
            enable_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE ?
            TRUE: FALSE, NULL);
    }
    if(column_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING)
    {
        g_object_set(priv->rating_column, "visible",
            enable_flags & RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING ?
            TRUE: FALSE, NULL);
    }
}

/**
 * rc_ui_listview_playlist_get_enabled_columns:
 *
 * Get the visibility of the artist column and album column.
 *
 * Returns: The flags of the columns.
 */

guint rc_ui_listview_playlist_get_enabled_columns()
{
    RCUiPlaylistViewPrivate *priv = NULL;
    gboolean state;
    guint flags = 0;
    if(ui_playlist_view_instance==NULL) return 0;
    priv = RC_UI_PLAYLIST_VIEW(ui_playlist_view_instance)->priv;
    if(priv==NULL) return 0;
    g_object_get(ui_playlist_view_instance, "headers-visible", &state, NULL);
    if(state)
        flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST;
    g_object_get(priv->artist_column, "visible", &state, NULL);
    if(state)
        flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM;
    g_object_get(priv->tracknum_column, "visible", &state, NULL);
    if(state)
        flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK;
    g_object_get(priv->year_column, "visible", &state, NULL);
    if(state)
        flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR;
    g_object_get(priv->ftype_column, "visible", &state, NULL);
    if(state)
        flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE;
    g_object_get(priv->rating_column, "visible", &state, NULL);
    if(state)
        flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING;
    return flags;
}

/**
 * rc_ui_listview_refresh:
 *
 * Refresh the catalog and playlist list view.
 */

void rc_ui_listview_refresh()
{
    if(ui_catalog_view_instance!=NULL)
        gtk_widget_queue_draw(GTK_WIDGET(ui_catalog_view_instance));
    if(ui_playlist_view_instance!=NULL)
        gtk_widget_queue_draw(GTK_WIDGET(ui_playlist_view_instance));
}

