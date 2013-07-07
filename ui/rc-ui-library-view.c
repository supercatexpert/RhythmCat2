/*
 * RhythmCat UI Library Views Module
 * Library data and property views.
 *
 * rc-ui-library-view.c
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

#include "rc-ui-library-view.h"
#include "rc-ui-library-model.h"
#include "rc-common.h"
#include "rc-ui-menu.h"
#include "rc-ui-cell-renderer-rating.h"

/**
 * SECTION: rc-ui-library-view
 * @Short_description: Library data and property views
 * @Title: Library Views
 * @Include: rc-ui-library-view.h
 *
 * This module provides the Library List and Property view
 * widgets for the player. They show library item list and
 * property item list in the library window.
 */

struct _RCUiLibraryListViewPrivate
{
    GtkCellRenderer *state_renderer;
    GtkCellRenderer *title_renderer;
    GtkCellRenderer *artist_renderer;
    GtkCellRenderer *album_renderer;
    GtkCellRenderer *tracknum_renderer;
    GtkCellRenderer *year_renderer;
    GtkCellRenderer *ftype_renderer;
    GtkCellRenderer *genre_renderer;
    GtkCellRenderer *length_renderer;
    GtkCellRenderer *rating_renderer;
    GtkTreeViewColumn *state_column;
    GtkTreeViewColumn *title_column;
    GtkTreeViewColumn *artist_column;
    GtkTreeViewColumn *album_column;  
    GtkTreeViewColumn *tracknum_column;
    GtkTreeViewColumn *year_column;
    GtkTreeViewColumn *ftype_column;
    GtkTreeViewColumn *genre_column;
    GtkTreeViewColumn *length_column;
    GtkTreeViewColumn *rating_column;
    gboolean display_mode;
};

struct _RCUiLibraryPropViewPrivate
{
    GtkCellRenderer *name_renderer;
    GtkCellRenderer *count_renderer;
    GtkTreeViewColumn *name_column;
    GtkTreeViewColumn *count_column;
};

static gpointer rc_ui_library_list_view_parent_class = NULL;
static gpointer rc_ui_library_prop_view_parent_class = NULL;

static void rc_ui_library_list_text_call_data_func(
    GtkTreeViewColumn *column, GtkCellRenderer *renderer,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gboolean flag;
    gtk_tree_model_get(model, iter, RC_UI_LIBRARY_LIST_COLUMN_PLAYING_FLAG,
        &flag, -1);
    if(flag)
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_BOLD, NULL);
    else
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_NORMAL, NULL);
}

static void rc_ui_library_prop_text_call_data_func(
    GtkTreeViewColumn *column, GtkCellRenderer *renderer,
    GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    gboolean flag;
    gtk_tree_model_get(model, iter, RC_UI_LIBRARY_PROP_COLUMN_FLAG,
        &flag, -1);
    if(flag)
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_BOLD, NULL);
    else
        g_object_set(G_OBJECT(renderer), "weight", PANGO_WEIGHT_NORMAL, NULL);
}

static void rc_ui_library_view_rated_cb(RCUiCellRendererRating *renderer,
    const char *path, gfloat rating, gpointer data)
{
    GtkTreeModel *model;
    GtkTreeIter iter = {0};
    GObject *library_query_result = NULL;
    RCLibDbLibraryData *library_data;
    if(data==NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(data));
    if(model==NULL) return;
    if(!gtk_tree_model_get_iter_from_string(model, &iter, path))
        return;
    if(iter.user_data==NULL) return;
    g_object_get(model, "query-result", &library_query_result, NULL);
    if(library_query_result==NULL) return;
    library_data = rclib_db_library_query_result_get_data(
        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
        (RCLibDbLibraryQueryResultIter *)iter.user_data);
    if(library_data!=NULL)
    {
        rclib_db_library_data_set(library_data,
            RCLIB_DB_LIBRARY_DATA_TYPE_RATING, rating,
            RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
        rclib_db_library_data_unref(library_data);
    }
    g_object_unref(library_query_result);
}

static gboolean rc_ui_library_list_view_multidrag_selection_block(
    GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
    gboolean pcs, gpointer data)
{
    return *(const gboolean *)data;
}

static void rc_ui_library_list_view_block_selection(GtkWidget *widget,
    gboolean block, gint x, gint y)
{
    GtkTreeSelection *selection;
    gint *where;
    static const gboolean which[] = {FALSE, TRUE};
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    gtk_tree_selection_set_select_function(selection,
        rc_ui_library_list_view_multidrag_selection_block,
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

static gboolean rc_ui_library_list_view_button_pressed_event(
    GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    GtkTreePath *path = NULL;
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
    if(selection==NULL) return FALSE;
    rc_ui_library_list_view_block_selection(widget, TRUE, -1, -1);
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
            rc_ui_library_list_view_block_selection(widget, FALSE, event->x,
                event->y);
        }
        if(path!=NULL) gtk_tree_path_free(path);
        return FALSE;
    }
    if(gtk_tree_selection_count_selected_rows(selection)>1)
        return TRUE;
    return FALSE;
}

static gboolean rc_ui_library_list_view_button_release_event(
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
        rc_ui_library_list_view_block_selection(widget, TRUE, -1, -1);
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
            rc_ui_menu_get_ui_manager(), "/LibraryPopupMenu")),
            NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
    }
    return FALSE;
}

static void rc_ui_library_list_view_finalize(GObject *object)
{
    RC_UI_LIBRARY_LIST_VIEW(object)->priv = NULL;
    G_OBJECT_CLASS(rc_ui_library_list_view_parent_class)->finalize(object);
}

static void rc_ui_library_prop_view_finalize(GObject *object)
{
    RC_UI_LIBRARY_PROP_VIEW(object)->priv = NULL;
    G_OBJECT_CLASS(rc_ui_library_prop_view_parent_class)->finalize(object);
}

static void rc_ui_library_list_view_class_init(RCUiLibraryListViewClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_library_list_view_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_library_list_view_finalize;
    g_type_class_add_private(klass, sizeof(RCUiLibraryListViewPrivate));
}

static void rc_ui_library_prop_view_class_init(RCUiLibraryPropViewClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_library_prop_view_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_library_prop_view_finalize;
    g_type_class_add_private(klass, sizeof(RCUiLibraryPropViewPrivate));
}

static void rc_ui_library_list_view_instance_init(RCUiLibraryListView *view)
{
    RCUiLibraryListViewPrivate *priv = NULL;
    GtkTreeSelection *selection;
    priv = G_TYPE_INSTANCE_GET_PRIVATE(view, RC_UI_TYPE_LIBRARY_LIST_VIEW,
        RCUiLibraryListViewPrivate);
    view->priv = priv;
    g_object_set(view, "name", "RC2LibraryListView", "headers-visible",
        TRUE, "reorderable", FALSE, "rules-hint", TRUE, "enable-search",
        TRUE, NULL);
    priv->state_renderer = gtk_cell_renderer_pixbuf_new();
    priv->title_renderer = gtk_cell_renderer_text_new();
    priv->artist_renderer = gtk_cell_renderer_text_new();
    priv->album_renderer = gtk_cell_renderer_text_new();
    priv->tracknum_renderer = gtk_cell_renderer_text_new();
    priv->year_renderer = gtk_cell_renderer_text_new();
    priv->ftype_renderer = gtk_cell_renderer_text_new();
    priv->genre_renderer = gtk_cell_renderer_text_new();
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
    gtk_cell_renderer_set_fixed_size(priv->genre_renderer, 60, -1);
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
    g_object_set(priv->genre_renderer, "ellipsize", 
        PANGO_ELLIPSIZE_END, "ellipsize-set", TRUE, "weight",
        PANGO_WEIGHT_NORMAL, "weight-set", TRUE, NULL);
    g_object_set(priv->length_renderer, "xalign", 1.0,
        "width-chars", 6, NULL);
    priv->state_column = gtk_tree_view_column_new_with_attributes(
        "#", priv->state_renderer, "stock-id",
        RC_UI_LIBRARY_LIST_COLUMN_STATE, NULL);
    priv->title_column = gtk_tree_view_column_new_with_attributes(
        _("Title"), priv->title_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_FTITLE, NULL);
    priv->artist_column = gtk_tree_view_column_new_with_attributes(
        _("Artist"), priv->artist_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_ARTIST, NULL);
    priv->album_column = gtk_tree_view_column_new_with_attributes(
        _("Album"), priv->artist_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_ALBUM, NULL);
    priv->tracknum_column = gtk_tree_view_column_new_with_attributes(
        _("Track"), priv->tracknum_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_TRACK, NULL);
    priv->year_column = gtk_tree_view_column_new_with_attributes(
        _("Year"), priv->year_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_YEAR, NULL);
    priv->ftype_column = gtk_tree_view_column_new_with_attributes(
        _("Format"), priv->ftype_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_FTYPE, NULL);
    priv->genre_column = gtk_tree_view_column_new_with_attributes(
        _("Genre"), priv->genre_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_GENRE, NULL);
    priv->length_column = gtk_tree_view_column_new_with_attributes(
        _("Length"), priv->length_renderer, "text",
        RC_UI_LIBRARY_LIST_COLUMN_LENGTH, NULL);
    priv->rating_column = gtk_tree_view_column_new_with_attributes(
        _("Rating"), priv->rating_renderer, "rating",
        RC_UI_LIBRARY_LIST_COLUMN_RATING, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->title_column,
        priv->title_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->artist_column,
        priv->artist_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->album_column,
        priv->album_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->tracknum_column,
        priv->tracknum_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->year_column,
        priv->year_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->ftype_column,
        priv->ftype_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->genre_column,
        priv->genre_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->length_column,
        priv->length_renderer, rc_ui_library_list_text_call_data_func,
        NULL, NULL);
    g_object_set(priv->state_column, "sizing",
        GTK_TREE_VIEW_COLUMN_FIXED, "fixed-width", 30, "min-width", 30,
        "max-width", 30, NULL);        
    g_object_set(priv->title_column, "expand", TRUE, "resizable", TRUE,
        "sort-column-id", 1, NULL);
    g_object_set(priv->artist_column, "expand", TRUE, "resizable", TRUE,
        "sort-column-id", 2, NULL);
    g_object_set(priv->album_column, "expand", TRUE, "resizable", TRUE,
        "sort-column-id", 3, NULL);
    g_object_set(priv->tracknum_column, "expand", TRUE, "visible", FALSE,
        "resizable", TRUE, "sort-column-id", 4, NULL);
    g_object_set(priv->year_column, "expand", TRUE, "visible", FALSE,
        "resizable", TRUE, "sort-column-id", 5, NULL);
    g_object_set(priv->ftype_column, "expand", TRUE, "visible", FALSE, 
        "resizable", TRUE, "sort-column-id", 6, NULL);
    g_object_set(priv->genre_column, "expand", TRUE, "visible", FALSE,
        "resizable", TRUE, "sort-column-id", 7, NULL);
    g_object_set(priv->length_column, "sizing",
        GTK_TREE_VIEW_COLUMN_FIXED, "fixed-width", 55, "alignment",
        1.0, "sort-column-id", 8, NULL);
    g_object_set(priv->rating_column, "sizing",
        GTK_TREE_VIEW_COLUMN_FIXED, "fixed-width", 80, "alignment",
        1.0, "visible", FALSE, "sort-column-id", 9, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->state_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->title_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->artist_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->album_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->tracknum_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->year_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->ftype_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->genre_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->length_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->rating_column);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
    g_signal_connect(priv->rating_renderer, "rated",
        G_CALLBACK(rc_ui_library_view_rated_cb), view);
    g_signal_connect(view, "button-press-event",
        G_CALLBACK(rc_ui_library_list_view_button_pressed_event), NULL);
    g_signal_connect(view, "button-release-event",
        G_CALLBACK(rc_ui_library_list_view_button_release_event), NULL);
}

static void rc_ui_library_prop_view_instance_init(RCUiLibraryPropView *view)
{
    RCUiLibraryPropViewPrivate *priv = NULL;
    GtkTreeSelection *selection;
    GtkTreePath *path;
    priv = G_TYPE_INSTANCE_GET_PRIVATE(view, RC_UI_TYPE_LIBRARY_PROP_VIEW,
        RCUiLibraryPropViewPrivate);
    view->priv = priv;
    g_object_set(view, "name", "RC2LibraryPropView", "headers-visible",
        TRUE, "reorderable", FALSE, "rules-hint", TRUE, "enable-search",
        TRUE, NULL);
    priv->name_renderer = gtk_cell_renderer_text_new();
    priv->count_renderer = gtk_cell_renderer_text_new();
    g_object_set(priv->name_renderer, "ellipsize", PANGO_ELLIPSIZE_END,
        "ellipsize-set", TRUE, "weight", PANGO_WEIGHT_NORMAL,
        "weight-set", TRUE, NULL);
    g_object_set(priv->count_renderer, "weight", PANGO_WEIGHT_NORMAL,
        "weight-set", TRUE, "xalign", 1.0, NULL);
    priv->name_column = gtk_tree_view_column_new_with_attributes(
        _("Property"), priv->name_renderer, "text",
        RC_UI_LIBRARY_PROP_COLUMN_NAME, NULL);
    gtk_tree_view_column_pack_end(priv->name_column, priv->count_renderer,
        FALSE);
    gtk_tree_view_column_add_attribute(priv->name_column,
        priv->count_renderer, "text", RC_UI_LIBRARY_PROP_COLUMN_COUNT);
    gtk_tree_view_column_set_cell_data_func(priv->name_column,
        priv->name_renderer, rc_ui_library_prop_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->name_column,
        priv->count_renderer, rc_ui_library_prop_text_call_data_func,
        NULL, NULL);
    g_object_set(priv->name_column, "expand", TRUE, "resizable", TRUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->name_column);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
    path = gtk_tree_path_new_first();
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(view), path, NULL, FALSE);
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_path_free(path);
}

GType rc_ui_library_list_view_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo view_info = {
        .class_size = sizeof(RCUiLibraryListViewClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_library_list_view_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiLibraryListView),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)
            rc_ui_library_list_view_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_TREE_VIEW,
            g_intern_static_string("RCUiLibraryListView"), &view_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rc_ui_library_prop_view_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo view_info = {
        .class_size = sizeof(RCUiLibraryPropViewClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_library_prop_view_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiLibraryPropView),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)
            rc_ui_library_prop_view_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_TREE_VIEW,
            g_intern_static_string("RCUiLibraryPropView"), &view_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_ui_library_list_view_new:
 * 
 * Create a new library list view widget.
 * 
 * Returns: The new library list view widget, #NULL if any error occurs.
 */

GtkWidget *rc_ui_library_list_view_new()
{
    return GTK_WIDGET(g_object_new(RC_UI_TYPE_LIBRARY_LIST_VIEW, NULL));
}

/**
 * rc_ui_library_prop_view_new:
 * @property_text: the property text used in the column header
 * 
 * Create a new library property view widget.
 * 
 * Returns: The new library property view widget, #NULL if any error occurs.
 */

GtkWidget *rc_ui_library_prop_view_new(const gchar *property_text)
{
    RCUiLibraryPropView *view;
    RCUiLibraryPropViewPrivate *priv;
    view = g_object_new(RC_UI_TYPE_LIBRARY_PROP_VIEW, NULL);
    priv = view->priv;
    gtk_tree_view_column_set_title(priv->name_column, property_text);
    return GTK_WIDGET(view);
}

/**
 * rc_ui_library_list_select_all:
 * @list_view: the #RCUiLibraryListView
 * 
 * Select all items in the #RCUiLibraryListView
 */

void rc_ui_library_list_select_all(RCUiLibraryListView *list_view)
{
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_view));
    if(selection==NULL) return;
    gtk_tree_selection_select_all(selection);
}

/**
 * rc_ui_library_list_delete_items:
 * @list_view: the #RCUiLibraryListView
 * 
 * Delete all selected items in the #RCUiLibraryListView.
 */

void rc_ui_library_list_delete_items(RCUiLibraryListView *list_view)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GList *list, *list_foreach;
    GtkTreePath *path;
    GtkTreeIter iter;
    GPtrArray *uri_array;
    GObject *library_query_result = NULL;
    RCLibDbLibraryData *library_data;
    gchar *uri;
    guint i;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list_view));
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(list_view));
    if(model==NULL || selection==NULL) return;
    list = gtk_tree_selection_get_selected_rows(selection, NULL);
    if(list==NULL) return;
    g_object_get(model, "query-result", &library_query_result, NULL);
    uri_array = g_ptr_array_new_with_free_func(g_free);
    if(library_query_result!=NULL)
    {
        for(list_foreach=list;list_foreach!=NULL;
            list_foreach=g_list_next(list_foreach))
        {
            uri = NULL;
            path = list_foreach->data;
            if(path==NULL) continue;
            if(!gtk_tree_model_get_iter(model, &iter, path))
                continue;
            if(iter.user_data==NULL) continue;
            library_data = rclib_db_library_query_result_get_data(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                (RCLibDbLibraryQueryResultIter *)iter.user_data);
            rclib_db_library_data_get(library_data,
                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            rclib_db_library_data_unref(library_data);
            if(uri==NULL) continue;
            g_ptr_array_add(uri_array, uri);
        }
        g_object_unref(library_query_result);
    }
    g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
    g_list_free(list);
    for(i=0;i<uri_array->len;i++)
    {
        uri = g_ptr_array_index(uri_array, i);
        if(uri==NULL) continue;
        rclib_db_library_delete(uri);
    }
    g_ptr_array_free(uri_array, TRUE); 
}

/**
 * rc_ui_library_list_refresh:
 * @list_view: the #RCUiLibraryListView
 * 
 * Refresh all selected items in the #RCUiLibraryListView.
 */

void rc_ui_library_list_refresh(RCUiLibraryListView *list_view)
{
    
}

