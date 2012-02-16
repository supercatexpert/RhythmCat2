/*
 * RhythmCat UI List View Module
 * Show music catalog & library in list views.
 *
 * ui-listview.c
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

#include "ui-listview.h"
#include "ui-listmodel.h"
#include "ui-menu.h"
#include "common.h"

enum
{
    RC_UI_LISTVIEW_TARGET_INFO_CATALOG,
    RC_UI_LISTVIEW_TARGET_INFO_PLAYLIST,
    RC_UI_LISTVIEW_TARGET_INFO_FILE,
    RC_UI_LISTVIEW_TARGET_INFO_URILIST
};

static GtkWidget *catalog_listview;
static GtkWidget *playlist_listview;
static GtkCellRenderer *catalog_state_renderer;
static GtkCellRenderer *catalog_name_renderer;
static GtkCellRenderer *playlist_state_renderer;
static GtkCellRenderer *playlist_title_renderer;
static GtkCellRenderer *playlist_artist_renderer;
static GtkCellRenderer *playlist_album_renderer;
static GtkCellRenderer *playlist_length_renderer;
static GtkCellRenderer *playlist_rating_renderer; /* Not available now */
static GtkTreeViewColumn *catalog_state_column;
static GtkTreeViewColumn *catalog_name_column;
static GtkTreeViewColumn *playlist_state_column;
static GtkTreeViewColumn *playlist_title_column;
static GtkTreeViewColumn *playlist_artist_column;
static GtkTreeViewColumn *playlist_album_column;
static GtkTreeViewColumn *playlist_length_column;
static GtkTreeViewColumn *playlist_rating_column; /* not available now */

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

void rc_ui_listview_dnd_delete(GtkWidget *widget, GdkDragContext *context,
    gpointer data)
{
    g_signal_stop_emission_by_name(G_OBJECT(widget), "drag-data-delete");
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
    GSequenceIter **iters;
    GtkTreeIter iter;
    GtkTreePath *path;
    GList *list_foreach;
    GtkTreeModel *model;
    guint length = 0;
    gint i;
    if(seldata==NULL || target_iter==NULL) return;
    if(target_iter->user_data==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_listview));
    if(model==NULL) return;
    memcpy(&path_list, gtk_selection_data_get_data(seldata),
        sizeof(path_list));
    if(path_list==NULL) return;
    length = g_list_length(path_list);
    path_list = g_list_sort_with_data(path_list, (GCompareDataFunc)
        gtk_tree_path_compare, NULL);
    iters = g_new0(GSequenceIter *, length);
    for(list_foreach=path_list, i=0;list_foreach!=NULL;
        list_foreach=g_list_next(list_foreach), i++)
    {
        path = list_foreach->data;
        if(path==NULL) continue; 
        if(!gtk_tree_model_get_iter(model, &iter, path)) continue;
        iters[i] = iter.user_data;
    }
    rclib_db_playlist_move_to_another_catalog(iters, length,
        target_iter->user_data);
}

static inline void rc_ui_listview_playlist_add_uris_by_selection(
    GtkSelectionData *seldata, GSequenceIter *catalog_iter,
    GtkTreeIter *iter, GtkTreeViewDropPosition pos)
{
    GtkTreeModel *model; 
    gchar *uris;
    gchar **uri_array;
    gint i;
    gchar *filename;
    if(seldata==NULL || catalog_iter==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_listview));
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
                        (GSequenceIter *)iter->user_data, uri_array[i]);
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
                            (GSequenceIter *)iter->user_data, filename);
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
    GSequenceIter *seq_iter;
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

static void rc_ui_listview_catalog_set_drag()
{
    GtkTargetEntry entry[2];   
    entry[0].target = "RhythmCat2/CatalogItem";
    entry[0].flags = GTK_TARGET_SAME_WIDGET;
    entry[0].info = RC_UI_LISTVIEW_TARGET_INFO_CATALOG;
    entry[1].target = "RhythmCat2/PlaylistItem";
    entry[1].flags = GTK_TARGET_SAME_APP;
    entry[1].info = RC_UI_LISTVIEW_TARGET_INFO_PLAYLIST;
    gtk_drag_source_set(catalog_listview, GDK_BUTTON1_MASK, entry,
        1, GDK_ACTION_MOVE);
    gtk_drag_dest_set(catalog_listview, GTK_DEST_DEFAULT_ALL, entry,
        2, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
}

static void rc_ui_listview_playlist_set_drag()
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
    gtk_drag_source_set(playlist_listview, GDK_BUTTON1_MASK, entry,
        1, GDK_ACTION_MOVE);
    gtk_drag_dest_set(playlist_listview, GTK_DEST_DEFAULT_ALL, entry,
        4, GDK_ACTION_COPY | GDK_ACTION_MOVE | GDK_ACTION_LINK);
}

static void rc_ui_listview_catalog_row_selected(GtkTreeView *view,
    gpointer data)
{
    GtkTreeIter iter;
    GSequenceIter *catalog_iter;
    RCLibDbCatalogData *catalog_data;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    catalog_iter = iter.user_data;
    if(catalog_iter==NULL) return;
    catalog_data = g_sequence_get(catalog_iter);
    if(!RC_UI_IS_PLAYLIST_STORE(catalog_data->store)) return;
    gtk_tree_view_set_model(GTK_TREE_VIEW(playlist_listview),
        GTK_TREE_MODEL(catalog_data->store));
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
    rclib_db_catalog_set_name((GSequenceIter *)iter.user_data, new_text);
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
    GSequenceIter *iter, gpointer data)
{
    GtkTreeModel *model;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_listview));
    if(model==NULL) return;
    if(iter==rc_ui_list_model_get_catalog_by_model(model))
        gtk_tree_view_set_model(GTK_TREE_VIEW(playlist_listview), NULL);
}

/**
 * rc_ui_listview_init:
 * 
 * Initialize the catalog list view & playlist list view.
 */

void rc_ui_listview_init()
{
    GtkTreeModel *catalog_model;
    GtkTreeModel *playlist_model;
    GtkTreeSelection *catalog_selection;
    GtkTreeIter iter;
    catalog_listview = gtk_tree_view_new();
    playlist_listview = gtk_tree_view_new();
    rc_ui_list_model_init();
    catalog_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        catalog_listview));
    catalog_model = rc_ui_list_model_get_catalog_store();
    if(catalog_model!=NULL)
    {
        gtk_tree_view_set_model(GTK_TREE_VIEW(catalog_listview),
            catalog_model);
        if(gtk_tree_model_get_iter_first(catalog_model, &iter))
        {
            playlist_model = rc_ui_list_model_get_playlist_store(&iter);
            if(playlist_model!=NULL)
            {
                gtk_tree_view_set_model(GTK_TREE_VIEW(playlist_listview),
                    playlist_model);
                gtk_tree_selection_select_iter(catalog_selection, &iter);
            }
        }
    }
    gtk_widget_set_name(catalog_listview, "RCUiCatalogListview");
    gtk_widget_set_name(playlist_listview, "RCUiPlaylistListview");
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(catalog_listview));
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(playlist_listview));
    catalog_state_renderer = gtk_cell_renderer_pixbuf_new();
    catalog_name_renderer = gtk_cell_renderer_text_new();
    playlist_state_renderer = gtk_cell_renderer_pixbuf_new();
    playlist_title_renderer = gtk_cell_renderer_text_new();
    playlist_artist_renderer = gtk_cell_renderer_text_new();
    playlist_album_renderer = gtk_cell_renderer_text_new();
    playlist_length_renderer = gtk_cell_renderer_text_new();
    playlist_rating_renderer = NULL;
    gtk_cell_renderer_set_fixed_size(catalog_state_renderer, 16, -1);
    gtk_cell_renderer_set_fixed_size(catalog_name_renderer, 80, -1);
    gtk_cell_renderer_set_fixed_size(playlist_state_renderer, 16, -1);
    gtk_cell_renderer_set_fixed_size(playlist_title_renderer, 120, -1);
    gtk_cell_renderer_set_fixed_size(playlist_artist_renderer, 60, -1);
    gtk_cell_renderer_set_fixed_size(playlist_album_renderer, 60, -1);
    gtk_cell_renderer_set_fixed_size(playlist_length_renderer, 55, -1);
    g_object_set(G_OBJECT(catalog_name_renderer), "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(G_OBJECT(playlist_title_renderer), "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(G_OBJECT(playlist_artist_renderer), "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(G_OBJECT(playlist_album_renderer), "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(G_OBJECT(playlist_length_renderer), "xalign", 1.0,
        "width-chars", 6, NULL);
    catalog_state_column = gtk_tree_view_column_new_with_attributes(
        "#", catalog_state_renderer, "stock-id", 
        RC_UI_CATALOG_COLUMN_STATE, NULL);
    catalog_name_column = gtk_tree_view_column_new_with_attributes(
        _("Name"), catalog_name_renderer, "text",
        RC_UI_CATALOG_COLUMN_NAME, NULL);
    playlist_state_column = gtk_tree_view_column_new_with_attributes(
        "#", playlist_state_renderer, "stock-id",
        RC_UI_PLAYLIST_COLUMN_STATE, NULL);
    playlist_title_column = gtk_tree_view_column_new_with_attributes(
        _("Title"), playlist_title_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_FTITLE, NULL);
    playlist_artist_column = gtk_tree_view_column_new_with_attributes(
        _("Artist"), playlist_artist_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_ARTIST, NULL);
    playlist_album_column = gtk_tree_view_column_new_with_attributes(
        _("Album"), playlist_artist_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_ALBUM, NULL);
    playlist_length_column = gtk_tree_view_column_new_with_attributes(
        _("Length"), playlist_length_renderer, "text",
        RC_UI_PLAYLIST_COLUMN_LENGTH, NULL);
    playlist_rating_column = NULL;
    gtk_tree_view_column_set_cell_data_func(catalog_name_column,
        catalog_name_renderer, rc_ui_listview_catalog_name_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(playlist_title_column,
        playlist_title_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(playlist_artist_column,
        playlist_artist_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(playlist_album_column,
        playlist_album_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(playlist_length_column,
        playlist_length_renderer, rc_ui_listview_playlist_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_sizing(catalog_state_column,
        GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_expand(catalog_name_column, TRUE);
    gtk_tree_view_column_set_sizing(playlist_state_column,
        GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_expand(playlist_title_column, TRUE);
    gtk_tree_view_column_set_expand(playlist_artist_column, TRUE);
    gtk_tree_view_column_set_expand(playlist_album_column, TRUE);
    gtk_tree_view_column_set_sizing(playlist_length_column,
        GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(catalog_state_column ,30);
    gtk_tree_view_column_set_min_width(catalog_state_column, 30);
    gtk_tree_view_column_set_max_width(catalog_state_column, 30);
    gtk_tree_view_column_set_fixed_width(playlist_state_column ,30);
    gtk_tree_view_column_set_min_width(playlist_state_column, 30);
    gtk_tree_view_column_set_max_width(playlist_state_column, 30);
    gtk_tree_view_column_set_fixed_width(playlist_length_column, 55);
    gtk_tree_view_column_set_alignment(playlist_length_column, 1.0);
    gtk_tree_view_column_set_visible(playlist_artist_column, FALSE);
    gtk_tree_view_column_set_visible(playlist_album_column, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(catalog_listview),
        catalog_state_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(catalog_listview),
        catalog_name_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_listview),
        playlist_state_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_listview),
        playlist_title_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_listview),
        playlist_artist_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_listview),
        playlist_album_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(playlist_listview),
        playlist_length_column);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(
        GTK_TREE_VIEW(catalog_listview)), GTK_SELECTION_MULTIPLE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(
        GTK_TREE_VIEW(playlist_listview)), GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(catalog_listview),
        FALSE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(playlist_listview),
        FALSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(catalog_listview),
        FALSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(playlist_listview),
        FALSE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(catalog_listview),
        TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(playlist_listview),
        TRUE);
    rc_ui_listview_catalog_set_drag();
    rc_ui_listview_playlist_set_drag();
    g_signal_connect(G_OBJECT(catalog_name_renderer), "edited",
        G_CALLBACK(rc_ui_listview_catalog_row_edited), NULL);
    g_signal_connect(G_OBJECT(catalog_listview), "drag-motion",
        G_CALLBACK(rc_ui_listview_dnd_motion), NULL);
    g_signal_connect(G_OBJECT(playlist_listview), "drag-motion",
        G_CALLBACK(rc_ui_listview_dnd_motion), NULL);
    g_signal_connect(G_OBJECT(catalog_listview), "drag-data-get",
        G_CALLBACK(rc_ui_listview_dnd_data_get), NULL);
    g_signal_connect(G_OBJECT(playlist_listview), "drag-data-get",
        G_CALLBACK(rc_ui_listview_dnd_data_get), NULL);
    g_signal_connect(G_OBJECT(catalog_listview), "drag-data-delete",
        G_CALLBACK(rc_ui_listview_dnd_delete), NULL);
    g_signal_connect(G_OBJECT(playlist_listview), "drag-data-delete",
        G_CALLBACK(rc_ui_listview_dnd_delete), NULL);
    g_signal_connect(G_OBJECT(catalog_listview), "drag-data-received",
        G_CALLBACK(rc_ui_listview_catalog_dnd_data_received), NULL);
    g_signal_connect(G_OBJECT(playlist_listview), "drag-data-received",
        G_CALLBACK(rc_ui_listview_playlist_dnd_data_received), NULL);
    g_signal_connect(G_OBJECT(catalog_listview), "button-press-event",
        G_CALLBACK(rc_ui_listview_button_pressed_event), NULL);
    g_signal_connect(G_OBJECT(playlist_listview), "button-press-event",
        G_CALLBACK(rc_ui_listview_button_pressed_event), NULL);
    g_signal_connect(G_OBJECT(catalog_listview), "button-release-event",
        G_CALLBACK(rc_ui_listview_catalog_button_release_event), NULL);
    g_signal_connect(G_OBJECT(playlist_listview), "button-release-event",
        G_CALLBACK(rc_ui_listview_playlist_button_release_event), NULL);
    g_signal_connect(G_OBJECT(catalog_listview), "cursor-changed",
        G_CALLBACK(rc_ui_listview_catalog_row_selected), NULL);
    g_signal_connect(G_OBJECT(playlist_listview), "row-activated",
        G_CALLBACK(rc_ui_listview_playlist_row_activated), NULL);
    rclib_db_signal_connect("catalog-delete",
        G_CALLBACK(rc_ui_listview_catalog_delete_cb), NULL);
}

/**
 * rc_ui_listview_get_catalog_widget:
 *
 * Get the catalog list view widget.
 */

GtkWidget *rc_ui_listview_get_catalog_widget()
{
    return catalog_listview;
}

/**
 * rc_ui_listview_get_playlist_widget:
 *
 * Get the playlist list view widget.
 */

GtkWidget *rc_ui_listview_get_playlist_widget()
{
    return playlist_listview;
}

/**
 * rc_ui_listview_catalog_set_pango_attributes:
 * @list: the pango attribute list
 *
 * Set the pango attribute for the catalog list view.
 */

void rc_ui_listview_catalog_set_pango_attributes(const PangoAttrList *list)
{
    g_object_set(G_OBJECT(catalog_name_renderer), "attributes", list, NULL);
}

/**
 * rc_ui_listview_playlist_set_pango_attributes:
 * @list: the pango attribute list
 *
 * Set the pango attribute for the playlist list view.
 */

void rc_ui_listview_playlist_set_pango_attributes(const PangoAttrList *list)
{
    g_object_set(G_OBJECT(playlist_title_renderer), "attributes", list,
        NULL);
    g_object_set(G_OBJECT(playlist_artist_renderer), "attributes", list,
        NULL);
    g_object_set(G_OBJECT(playlist_album_renderer), "attributes", list,
        NULL);
    g_object_set(G_OBJECT(playlist_length_renderer), "attributes", list,
        NULL);
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
    if(iter==NULL);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(catalog_listview));
    if(model==NULL) return;
    path = gtk_tree_model_get_path(model, iter);
    if(path==NULL) return;
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(catalog_listview), path, 
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
    if(iter==NULL);
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_listview));
    if(model==NULL) return;
    path = gtk_tree_model_get_path(model, iter);
    if(path==NULL) return;
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(playlist_listview), path, 
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
    if(iter==NULL) return FALSE;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(catalog_listview));
    if(model==NULL) return FALSE;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(catalog_listview), &path,
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
    if(iter==NULL) return FALSE;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_listview));
    if(model==NULL) return FALSE;
    gtk_tree_view_get_cursor(GTK_TREE_VIEW(playlist_listview), &path,
        NULL);
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
 * Returns: The #GtkTreeModel.
 */

GtkTreeModel *rc_ui_listview_catalog_get_model()
{
    return gtk_tree_view_get_model(GTK_TREE_VIEW(catalog_listview));
}

/**
 * rc_ui_listview_playlist_get_model:
 *
 * Get the #GtkTreeModel used in the current playlist list view.
 *
 * Returns: The #GtkTreeModel.
 */


GtkTreeModel *rc_ui_listview_playlist_get_model()
{
    return gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_listview));
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
    GSequenceIter *catalog_iter;
    gchar *new_name;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(catalog_listview));
    if(model==NULL) return;
    new_name = g_strdup_printf(_("Playlist %u"), i);
    i++;
    if(!rc_ui_listview_catalog_get_cursor(&iter))
    {
        iter.user_data = NULL;
    }
    catalog_iter = rclib_db_catalog_add(new_name, (GSequenceIter *)
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
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(catalog_listview));
    if(model==NULL) return;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    path = gtk_tree_model_get_path(model, &iter);
    if(path==NULL) return;
    g_object_set(G_OBJECT(catalog_name_renderer), "editable", TRUE,
        "editable-set", TRUE, NULL);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(catalog_listview), path,
        catalog_name_column, TRUE);
    g_object_set(G_OBJECT(catalog_name_renderer), "editable", FALSE,
        "editable-set", FALSE, NULL);
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
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(catalog_listview));
    if(model==NULL) return;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        catalog_listview));
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
        rclib_db_catalog_delete((GSequenceIter *)iter.user_data);
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
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        playlist_listview));
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
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_listview));
    if(model==NULL) return;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        playlist_listview));
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
        rclib_db_playlist_delete((GSequenceIter *)iter.user_data);
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
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(catalog_listview));
    if(model==NULL) return;
    if(!rc_ui_listview_catalog_get_cursor(&iter)) return;
    if(iter.user_data==NULL) return;
    rclib_db_playlist_refresh((GSequenceIter *)iter.user_data);
}



