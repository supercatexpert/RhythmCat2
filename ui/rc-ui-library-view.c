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

static void rc_ui_library_list_view_finalize(GObject *object)
{
    //RCUiLibraryListViewPrivate *priv = NULL;
    //priv = RC_UI_LIBRARY_LIST_VIEW(object)->priv;
    RC_UI_LIBRARY_LIST_VIEW(object)->priv = NULL;
    G_OBJECT_CLASS(rc_ui_library_list_view_parent_class)->finalize(object);
}

static void rc_ui_library_prop_view_finalize(GObject *object)
{
    //RCUiLibraryPropViewPrivate *priv = NULL;
    //priv = RC_UI_LIBRARY_PROP_VIEW(object)->priv;
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
    g_object_set(priv->title_column, "expand", TRUE, "resizable", TRUE, NULL);
    g_object_set(priv->artist_column, "expand", TRUE, "resizable", TRUE, NULL);
    g_object_set(priv->album_column, "expand", TRUE, "resizable", TRUE, NULL);
    g_object_set(priv->tracknum_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->year_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->ftype_column, "expand", TRUE, "visible",
        FALSE, "resizable", TRUE, NULL);
    g_object_set(priv->genre_column, "expand", TRUE, "visible",
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
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->genre_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->length_column);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->rating_column);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    gtk_tree_view_columns_autosize(GTK_TREE_VIEW(view));
    //gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(view),
    //    rc_ui_listview_playlist_search_comparison_func, NULL, NULL);
    //g_signal_connect(view, "row-activated",
    //    G_CALLBACK(rc_ui_listview_playlist_row_activated), NULL);
    //g_signal_connect(priv->rating_renderer, "rated",
    //    G_CALLBACK(rc_ui_playlist_view_rated_cb), priv);
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
    gtk_cell_renderer_set_fixed_size(priv->count_renderer,
        50, -1);
    g_object_set(priv->name_renderer, "ellipsize", PANGO_ELLIPSIZE_END,
        "ellipsize-set", TRUE, "weight", PANGO_WEIGHT_NORMAL,
        "weight-set", TRUE, NULL);
    g_object_set(priv->count_renderer, "weight", PANGO_WEIGHT_NORMAL,
        "weight-set", TRUE, "xalign", 1.0, "width-chars", 6, NULL);
    priv->name_column = gtk_tree_view_column_new_with_attributes(
        _("Property"), priv->name_renderer, "text",
        RC_UI_LIBRARY_PROP_COLUMN_NAME, NULL);
    gtk_tree_view_column_pack_end(priv->name_column, priv->count_renderer,
        FALSE);
    gtk_tree_view_column_add_attribute(priv->name_column,
        priv->count_renderer, "text", RC_UI_LIBRARY_PROP_COLUMN_COUNT);
    /*
    priv->count_column = gtk_tree_view_column_new_with_attributes(
        _("Number"), priv->count_renderer, "text",
        RC_UI_LIBRARY_PROP_COLUMN_COUNT, NULL);
    */
    gtk_tree_view_column_set_cell_data_func(priv->name_column,
        priv->name_renderer, rc_ui_library_prop_text_call_data_func,
        NULL, NULL);
    gtk_tree_view_column_set_cell_data_func(priv->name_column,
        priv->count_renderer, rc_ui_library_prop_text_call_data_func,
        NULL, NULL);
    g_object_set(priv->name_column, "expand", TRUE, "resizable", TRUE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->name_column);
    //gtk_tree_view_append_column(GTK_TREE_VIEW(view), priv->count_column);
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


