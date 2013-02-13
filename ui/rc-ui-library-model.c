/*
 * RhythmCat UI Library Models Module
 * Library data and property models.
 *
 * rc-ui-library-model.c
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
 
#include "rc-ui-library-model.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-library-model
 * @Short_description: Library data and property models
 * @Title: Library View Models
 * @Include: rc-ui-library-model.h
 *
 * This module provides 2 classes: #RCUiLibraryListStore and
 * #RCUiLibraryPropStore, which implements interface #GtkTreeModel.
 * They can be used by #GtkTreeView for showing and operating the data
 * inside. #RCUiLibraryListStore contains the query result list obtained 
 * from the library, and #RCUiLibraryPropStore contains the property list
 * of the query result.
 */

struct _RCUiLibraryListStorePrivate
{
    RCLibDbLibraryQueryResult *base;
    RCLibDbLibraryQueryResult *query_result;
    gint stamp;
    gint n_columns;
    gulong query_result_added_id;
    gulong query_result_delete_id;
    gulong query_result_changed_id;
    gulong query_result_reordered_id;
};

struct _RCUiLibraryPropStorePrivate
{
    RCLibDbLibraryQueryResult *base;
    RCLibDbQueryDataType prop_type;
    gint stamp;
    gint n_columns;
    gulong prop_added_id;
    gulong prop_delete_id;
    gulong prop_changed_id;
    gulong prop_reordered_id;
};

enum
{
    LIST_STORE_PROP_O,
    LIST_STORE_PROP_QUERY_RESULT
};

enum
{
    PROP_STORE_PROP_O,
    PROP_STORE_PROP_BASE,
    PROP_STORE_PROP_PROP_TYPE
};

static gpointer rc_ui_library_list_store_parent_class = NULL;
static gpointer rc_ui_library_prop_store_parent_class = NULL;

static void rc_ui_library_list_query_result_added_cb(
    RCLibDbLibraryQueryResult *qr, const gchar *uri, gpointer data)
{
    RCUiLibraryListStorePrivate *priv;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    RCLibDbLibraryQueryResultIter *iter;
    gint pos;
    if(uri==NULL) return;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model));
    priv = RC_UI_LIBRARY_LIST_STORE(model)->priv;
    if(priv==NULL) return;
    iter = rclib_db_library_query_result_get_iter_by_uri(qr, uri);
    if(iter==NULL) return;
    pos = rclib_db_library_query_result_get_position(qr, iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_inserted(model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_library_list_query_result_delete_cb(
    RCLibDbLibraryQueryResult *qr, const gchar *uri, gpointer data)
{
    GtkTreeModel *model;
    GtkTreePath *path;
    gint pos;
    RCLibDbLibraryQueryResultIter *iter;
    if(uri==NULL) return;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model));
    iter = rclib_db_library_query_result_get_iter_by_uri(qr, uri);
    if(iter==NULL) return;
    pos = rclib_db_library_query_result_get_position(qr, iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    gtk_tree_model_row_deleted(model, path);
    gtk_tree_path_free(path);
}

static void rc_ui_library_list_query_result_changed_cb(
    RCLibDbLibraryQueryResult *qr, const gchar *uri, gpointer data)
{
    RCUiLibraryListStorePrivate *priv;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    RCLibDbLibraryQueryResultIter *iter;
    gint pos;
    if(uri==NULL) return;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model));
    priv = RC_UI_LIBRARY_LIST_STORE(model)->priv;
    if(priv==NULL) return;
    iter = rclib_db_library_query_result_get_iter_by_uri(qr, uri);
    if(iter==NULL) return;
    pos = rclib_db_library_query_result_get_position(qr, iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_changed(model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_library_list_query_result_reordered_cb(
    RCLibDbLibraryQueryResult *qr, gint *new_order, gpointer data)
{
    GtkTreeModel *model;
    GtkTreePath *path;
    if(new_order==NULL) return;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model));
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(model, path, NULL, new_order);
    gtk_tree_path_free(path);
}

static void rc_ui_library_prop_prop_added_cb(RCLibDbLibraryQueryResult *qr,
    guint prop_type, const gchar *prop_text, gpointer data)
{
    RCUiLibraryPropStorePrivate *priv;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    RCLibDbLibraryQueryResultPropIter *iter;
    gint pos;
    if(prop_text==NULL) return;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model));
    priv = RC_UI_LIBRARY_PROP_STORE(model)->priv;
    if(priv==NULL) return;
    iter = rclib_db_library_query_result_prop_get_iter_by_prop(qr,
        priv->prop_type, prop_text);
    if(iter==NULL) return;
    pos = rclib_db_library_query_result_prop_get_position(qr,
        priv->prop_type, iter) + 1;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_inserted(model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_library_prop_prop_delete_cb(RCLibDbLibraryQueryResult *qr,
    guint prop_type, const gchar *prop_text, gpointer data)
{
    RCUiLibraryPropStorePrivate *priv;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    gint pos;
    RCLibDbLibraryQueryResultPropIter *iter;
    if(prop_text==NULL) return;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model));
    priv = RC_UI_LIBRARY_PROP_STORE(model)->priv;
    if(priv==NULL) return;
    if(prop_text==NULL) prop_text = "";
    iter = rclib_db_library_query_result_prop_get_iter_by_prop(qr,
        priv->prop_type, prop_text);
    if(iter==NULL) return; 
    pos = rclib_db_library_query_result_prop_get_position(qr, priv->prop_type,
        iter) + 1;
    if(pos==0) return;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    gtk_tree_model_row_deleted(model, path);
    gtk_tree_path_free(path);
    
    path = gtk_tree_path_new_first();
    gtk_tree_model_get_iter_first(model, &tree_iter);
    gtk_tree_model_row_changed(model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_library_prop_prop_changed_cb(RCLibDbLibraryQueryResult *qr,
    guint prop_type, const gchar *prop_text, gpointer data)
{
    RCUiLibraryPropStorePrivate *priv;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    gint pos;
    RCLibDbLibraryQueryResultPropIter *iter;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model));
    priv = RC_UI_LIBRARY_PROP_STORE(model)->priv;
    if(priv==NULL) return;
    if(prop_text==NULL) prop_text = "";
    iter = rclib_db_library_query_result_prop_get_iter_by_prop(qr,
        priv->prop_type, prop_text);
    if(iter==NULL) return; 
    pos = rclib_db_library_query_result_prop_get_position(qr,
        priv->prop_type, iter) + 1;
    if(pos==0) return;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_changed(model, path, &tree_iter);
    gtk_tree_path_free(path);
    
    path = gtk_tree_path_new_first();
    gtk_tree_model_get_iter_first(model, &tree_iter);
    gtk_tree_model_row_changed(model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_library_prop_prop_reordered_cb(RCLibDbLibraryQueryResult *qr,
    guint prop_type, gint *new_order, gpointer data)
{
    GtkTreeModel *model;
    GtkTreePath *path;
    if(new_order==NULL) return;
    if(data==NULL) return;
    model = GTK_TREE_MODEL(data);
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model));
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(model, path, NULL, new_order);
    gtk_tree_path_free(path);
}

static GtkTreeModelFlags rc_ui_library_list_store_get_flags(
    GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model),
        (GtkTreeModelFlags)0);
    return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static GtkTreeModelFlags rc_ui_library_prop_store_get_flags(
    GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model),
        (GtkTreeModelFlags)0);
    return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint rc_ui_library_list_store_get_n_columns(GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), 0);
    return RC_UI_LIBRARY_LIST_COLUMN_LAST;
}

static gint rc_ui_library_prop_store_get_n_columns(GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), 0);
    return RC_UI_LIBRARY_PROP_COLUMN_LAST;
}

static GType rc_ui_library_list_store_get_column_type(GtkTreeModel *model,
    gint index)
{
    GType type;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), G_TYPE_INVALID);
    switch(index)
    {
        case RC_UI_LIBRARY_LIST_COLUMN_TYPE:
        case RC_UI_LIBRARY_LIST_COLUMN_TRACK:
        case RC_UI_LIBRARY_LIST_COLUMN_YEAR:
            type = G_TYPE_INT;
            break;
        case RC_UI_LIBRARY_LIST_COLUMN_STATE:
        case RC_UI_LIBRARY_LIST_COLUMN_FTITLE:
        case RC_UI_LIBRARY_LIST_COLUMN_TITLE:
        case RC_UI_LIBRARY_LIST_COLUMN_ARTIST:
        case RC_UI_LIBRARY_LIST_COLUMN_ALBUM:
        case RC_UI_LIBRARY_LIST_COLUMN_FTYPE:
        case RC_UI_LIBRARY_LIST_COLUMN_GENRE:
        case RC_UI_LIBRARY_LIST_COLUMN_LENGTH:
            type = G_TYPE_STRING;
            break;
        case RC_UI_LIBRARY_LIST_COLUMN_PLAYING_FLAG:
            type = G_TYPE_BOOLEAN;
            break;
        case RC_UI_LIBRARY_LIST_COLUMN_RATING:
            type = G_TYPE_FLOAT;
            break;
        default:
            type = G_TYPE_INVALID;
            break;
    }
    return type;
}

static GType rc_ui_library_prop_store_get_column_type(GtkTreeModel *model,
    gint index)
{
    GType type;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), G_TYPE_INVALID);
    switch(index)
    {
        case RC_UI_LIBRARY_PROP_COLUMN_COUNT:
        case RC_UI_LIBRARY_PROP_COLUMN_FLAG:
            type = G_TYPE_INT;
            break;
        case RC_UI_LIBRARY_PROP_COLUMN_NAME:
            type = G_TYPE_STRING;
            break;
        default:
            type = G_TYPE_INVALID;
            break;
    }
    return type;
}

static gboolean rc_ui_library_list_store_get_iter(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreePath *path)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    gint i;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), FALSE);
    g_return_val_if_fail(path!=NULL, FALSE);
    store = RC_UI_LIBRARY_LIST_STORE(model);
    priv = store->priv;
    i = gtk_tree_path_get_indices(path)[0];
    if(priv->query_result==NULL ||
        i>=rclib_db_library_query_result_get_length(priv->query_result))
    {
        return FALSE;
    }
    iter->stamp = priv->stamp;
    iter->user_data = rclib_db_library_query_result_get_iter_at_pos(
        priv->query_result, i);
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    return TRUE;
}

static gboolean rc_ui_library_prop_store_get_iter(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreePath *path)
{
    RCUiLibraryPropStore *store;
    RCUiLibraryPropStorePrivate *priv;
    gint i;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), FALSE);
    g_return_val_if_fail(path!=NULL, FALSE);
    store = RC_UI_LIBRARY_PROP_STORE(model);
    priv = store->priv;
    i = gtk_tree_path_get_indices(path)[0];
    if(priv->base!=NULL && i>0 &&
        i-1>=rclib_db_library_query_result_prop_get_length(priv->base,
        priv->prop_type))
    {
        return FALSE;
    }
    iter->stamp = priv->stamp;
    if(i>0 && priv->base!=NULL)
    {
        iter->user_data = rclib_db_library_query_result_prop_get_iter_at_pos(
            priv->base, priv->prop_type, i-1);
        iter->user_data2 = NULL;
    }
    else
    {
        iter->user_data = GUINT_TO_POINTER(1);
        iter->user_data2 = GUINT_TO_POINTER(1);
    }
    iter->user_data3 = NULL;
    return TRUE;
}

static GtkTreePath *rc_ui_library_list_store_get_path(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    GtkTreePath *path;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), NULL);
    store = RC_UI_LIBRARY_LIST_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, NULL);
    g_return_val_if_fail(iter->stamp==priv->stamp, NULL);
    if(iter->user_data==NULL)
        return NULL;
    if(rclib_db_library_query_result_iter_is_end(priv->query_result,
        (RCLibDbLibraryQueryResultIter *)iter->user_data))
    {
        return NULL;
    }
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path,
        rclib_db_library_query_result_get_position(priv->query_result,
        (RCLibDbLibraryQueryResultIter *)iter->user_data));
    return path;
}

static GtkTreePath *rc_ui_library_prop_store_get_path(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryPropStore *store;
    RCUiLibraryPropStorePrivate *priv;
    GtkTreePath *path;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), NULL);
    store = RC_UI_LIBRARY_PROP_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, NULL);
    g_return_val_if_fail(iter->stamp==priv->stamp, NULL);
    if(iter->user_data==NULL)
        return NULL;
    if(iter->user_data2==NULL &&
        rclib_db_library_query_result_prop_iter_is_end(priv->base,
        priv->prop_type, (RCLibDbLibraryQueryResultPropIter *)iter->user_data))
    {
        return NULL;
    }
    path = gtk_tree_path_new();
    if(iter->user_data2!=NULL)
    {
        gtk_tree_path_append_index(path, 0);
    }
    else
    {
        gtk_tree_path_append_index(path,
            rclib_db_library_query_result_prop_get_position(priv->base,
            priv->prop_type, (RCLibDbLibraryQueryResultPropIter *)
            iter->user_data)+1);
    }
    return path;
}

static void rc_ui_library_list_store_get_value(GtkTreeModel *model,
    GtkTreeIter *iter, gint column, GValue *value)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    RCLibDbLibraryData *library_data;
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    RCLibDbLibraryQueryResultIter *seq_iter;
    gpointer reference = NULL;
    gchar *dstr;
    GstState state;
    gint vint;
    gchar *uri;
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model));
    g_return_if_fail(iter!=NULL);
    store = RC_UI_LIBRARY_LIST_STORE(model);
    priv = RC_UI_LIBRARY_LIST_STORE(store)->priv;
    g_return_if_fail(priv!=NULL);
    g_return_if_fail(column<priv->n_columns);
    seq_iter = iter->user_data;
    switch(column)
    {
        case RC_UI_LIBRARY_LIST_COLUMN_TYPE:
        {
            RCLibDbLibraryType type = 0;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TYPE, &type,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, type);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_STATE:
        {
            RCLibDbLibraryType type = 0;
            uri = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TYPE, &type,
                    RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_STRING);
            if(type==RCLIB_DB_LIBRARY_TYPE_MISSING)
            {
                g_value_set_static_string(value, GTK_STOCK_CANCEL);
                g_free(uri);
                break;
            }
            if(!rclib_core_get_play_source(&source_type, &reference, NULL))
            {
                g_value_set_static_string(value, NULL);
                g_free(uri);
                break;
            }
            if(source_type!=RCLIB_CORE_PLAY_SOURCE_LIBRARY ||
                g_strcmp0(uri, (const gchar *)reference)!=0)
            {
                g_value_set_static_string(value, NULL);
                g_free(uri);
                break;
            }
            g_free(uri);
            if(!rclib_core_get_state(&state, NULL, 0))
            {
                g_value_set_static_string(value, NULL);
                break;
            }
            switch(state)
            {
                case GST_STATE_PLAYING:
                    g_value_set_static_string(value, GTK_STOCK_MEDIA_PLAY);
                    break;
                case GST_STATE_PAUSED:
                    g_value_set_static_string(value, GTK_STOCK_MEDIA_PAUSE);
                    break;
                default:
                    g_value_set_static_string(value, NULL);
            }
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_FTITLE:
        {
            gchar *duri = NULL;
            gchar *dtitle = NULL;
            gchar *rtitle = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_URI, &duri,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TITLE, &dtitle,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            if(dtitle!=NULL && strlen(dtitle)>0)
                rtitle = g_strdup(dtitle);
            else
            {
                if(duri!=NULL)
                    rtitle = rclib_tag_get_name_from_uri(duri);
            }
            g_free(duri);
            g_free(dtitle);
            if(rtitle==NULL) rtitle = g_strdup(_("Unknown Title"));
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, rtitle);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_TITLE:
        {
            dstr = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TITLE, &dstr,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_ARTIST:
        {
            dstr = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST, &dstr,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_ALBUM:
        {
            dstr = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM, &dstr,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_FTYPE:
        {
            dstr = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE, &dstr,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_GENRE:
        {
            dstr = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_GENRE, &dstr,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_LENGTH:
        {
            gint sec, min;
            gchar *string;
            gint64 tlength = 0;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH, &tlength,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            sec = (gint)(tlength / GST_SECOND);
            min = sec / 60;
            sec = sec % 60;
            string = g_strdup_printf("%02d:%02d", min, sec);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, string);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_TRACK:
        {
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM, &vint,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, vint);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_YEAR:
        {
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_YEAR, &vint,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, vint);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_RATING:
        {
            gfloat rating = 0.0;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_RATING, &rating,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_FLOAT);
            g_value_set_float(value, rating);
            break;
        }
        case RC_UI_LIBRARY_LIST_COLUMN_PLAYING_FLAG:
        {
            RCLibDbLibraryType type = 0;
            uri = NULL;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TYPE, &type,
                    RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, FALSE);
            if(type==RCLIB_DB_LIBRARY_TYPE_MISSING)
            {
                g_free(uri);
                break;
            }
            if(!rclib_core_get_play_source(&source_type, &reference, NULL))
            {
                g_free(uri);
                break;
            }
            if(source_type!=RCLIB_CORE_PLAY_SOURCE_LIBRARY ||
                g_strcmp0(uri, (const gchar *)reference)!=0)
            {
                g_free(uri);
                break;
            }
            g_free(uri);
            if(!rclib_core_get_state(&state, NULL, 0))
                break;
            if(state==GST_STATE_PLAYING || state==GST_STATE_PAUSED)
                g_value_set_boolean(value, TRUE);
            break;
        }
        default:
            break;
    }
}

static void rc_ui_library_prop_store_get_value(GtkTreeModel *model,
    GtkTreeIter *iter, gint column, GValue *value)
{
    RCUiLibraryPropStore *store;
    RCUiLibraryPropStorePrivate *priv;
    RCLibDbLibraryQueryResultPropIter *seq_iter = NULL;
    gchar *dstr;
    guint vuint;
    gboolean vflag;
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model));
    g_return_if_fail(iter!=NULL);
    store = RC_UI_LIBRARY_PROP_STORE(model);
    priv = RC_UI_LIBRARY_PROP_STORE(store)->priv;
    g_return_if_fail(priv!=NULL);
    g_return_if_fail(column<priv->n_columns);
    seq_iter = iter->user_data;
    switch(column)
    {
        case RC_UI_LIBRARY_PROP_COLUMN_NAME:
        {
            dstr = NULL;
            if(iter->user_data2!=NULL)
            {
                dstr = g_strdup(_("All"));
            }
            else
            {
                rclib_db_library_query_result_prop_get_data(priv->base, 
                    priv->prop_type, seq_iter, &dstr, NULL);
            }
            g_value_init(value, G_TYPE_STRING);
            if(strlen(dstr)>0)
            {
                g_value_take_string(value, dstr);
            }
            else
            {
                g_value_set_string(value, _("(Unknown)"));
                g_free(dstr);
            }
            break;
        }
        case RC_UI_LIBRARY_PROP_COLUMN_COUNT:
        {
            vuint = 0;
            if(iter->user_data2!=NULL)
            {
                rclib_db_library_query_result_prop_get_total_count(priv->base, 
                    priv->prop_type, &vuint);
            }
            else
            {
                rclib_db_library_query_result_prop_get_data(priv->base, 
                    priv->prop_type, seq_iter, NULL, &vuint);
            }
            g_value_init(value, G_TYPE_UINT);
            g_value_set_uint(value, vuint);
            break;
        }
        case RC_UI_LIBRARY_PROP_COLUMN_FLAG:
        {
            vflag = FALSE;
            vflag = (iter->user_data2!=NULL);
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, vflag);
            break;
        }
        default:
            break;
    }
}

static gboolean rc_ui_library_list_store_iter_next(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_LIBRARY_LIST_STORE(model);
    priv = RC_UI_LIBRARY_LIST_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = rclib_db_library_query_result_get_next_iter(
        priv->query_result, (RCLibDbLibraryQueryResultIter *)
        iter->user_data);
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    if(iter->user_data==NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    return TRUE;
}

static gboolean rc_ui_library_prop_store_iter_next(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryPropStore *store;
    RCUiLibraryPropStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_LIBRARY_PROP_STORE(model);
    priv = RC_UI_LIBRARY_PROP_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    if(iter->user_data2!=NULL)
    {
        iter->user_data = rclib_db_library_query_result_prop_get_begin_iter(
            priv->base, priv->prop_type);
    }
    else
    {
        iter->user_data = rclib_db_library_query_result_prop_get_next_iter(
            priv->base, priv->prop_type, (RCLibDbLibraryQueryResultPropIter *)
            iter->user_data);
    }
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    if(iter->user_data==NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    return TRUE;
}

static gboolean rc_ui_library_list_store_iter_prev(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_LIBRARY_LIST_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = rclib_db_library_query_result_get_prev_iter(
        priv->query_result, (RCLibDbLibraryQueryResultIter *)
        iter->user_data);
    if(iter->user_data==NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    return TRUE;
}

static gboolean rc_ui_library_prop_store_iter_prev(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryPropStore *store;
    RCUiLibraryPropStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_LIBRARY_PROP_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    if(iter->user_data2!=NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    if(rclib_db_library_query_result_prop_get_begin_iter(priv->base,
        priv->prop_type)==iter->user_data)
    {
        iter->user_data = GUINT_TO_POINTER(1);
        iter->user_data2 = GUINT_TO_POINTER(1);
        return TRUE;
    }
    iter->user_data = rclib_db_library_query_result_prop_get_prev_iter(
        priv->base, priv->prop_type, (RCLibDbLibraryQueryResultPropIter *)
        iter->user_data);
    if(iter->user_data==NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    return TRUE;
}

static gboolean rc_ui_library_list_store_iter_children(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), FALSE);
    store = RC_UI_LIBRARY_LIST_STORE(model);
    priv = RC_UI_LIBRARY_LIST_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    if(rclib_db_library_query_result_get_length(priv->query_result)>0)
    {
        iter->stamp = priv->stamp;
        iter->user_data = rclib_db_library_query_result_get_begin_iter(
            priv->query_result);
        return TRUE;
    }
    else
    {
        iter->stamp = 0;
        return FALSE;
    }
}

static gboolean rc_ui_library_prop_store_iter_children(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent)
{
    RCUiLibraryPropStore *store;
    RCUiLibraryPropStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), FALSE);
    store = RC_UI_LIBRARY_PROP_STORE(model);
    priv = RC_UI_LIBRARY_PROP_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    iter->user_data = GUINT_TO_POINTER(1);
    iter->user_data2 = GUINT_TO_POINTER(1);
    iter->user_data3 = NULL;
    return TRUE;
}

static gboolean rc_ui_library_list_store_iter_has_child(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    return FALSE;
}

static gboolean rc_ui_library_prop_store_iter_has_child(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    return FALSE;
}

static gint rc_ui_library_list_store_iter_n_children(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryListStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), -1);
    priv = RC_UI_LIBRARY_LIST_STORE(model)->priv;
    g_return_val_if_fail(priv!=NULL, -1);
    if(iter==NULL)
        return rclib_db_library_query_result_get_length(priv->query_result);
    g_return_val_if_fail(priv->stamp==iter->stamp, -1);
    return 0;
}

static gint rc_ui_library_prop_store_iter_n_children(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiLibraryPropStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), -1);
    priv = RC_UI_LIBRARY_PROP_STORE(model)->priv;
    g_return_val_if_fail(priv!=NULL, -1);
    if(iter==NULL)
    {
        if(priv->base!=NULL)
        {
            return rclib_db_library_query_result_prop_get_length(priv->base,
                priv->prop_type) + 1;
        }
        else
        {
            return 1;
        }
    }
    g_return_val_if_fail(priv->stamp==iter->stamp, -1);
    return 0;
}

static gboolean rc_ui_library_list_store_iter_nth_child(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    RCLibDbLibraryQueryResultIter *child;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(model), FALSE);
    store = RC_UI_LIBRARY_LIST_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL) return FALSE;
    child = rclib_db_library_query_result_get_iter_at_pos(priv->query_result,
        n);
    if(rclib_db_library_query_result_iter_is_end(priv->query_result, child))
        return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = child;
    return TRUE;
}

static gboolean rc_ui_library_prop_store_iter_nth_child(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
    RCUiLibraryPropStore *store;
    RCUiLibraryPropStorePrivate *priv;
    RCLibDbLibraryQueryResultPropIter *child;
    g_return_val_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(model), FALSE);
    store = RC_UI_LIBRARY_PROP_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL) return FALSE;
    if(n==0)
    {
        iter->user_data = GUINT_TO_POINTER(1);
        iter->user_data2 = GUINT_TO_POINTER(2);
    }
    else
    {
        child = rclib_db_library_query_result_prop_get_iter_at_pos(priv->base,
            priv->prop_type, n-1);
        if(rclib_db_library_query_result_prop_iter_is_end(priv->base,
            priv->prop_type, child))
        {
            return FALSE;
        }
        iter->user_data = child;
        iter->user_data2 = NULL;
    }
    iter->stamp = priv->stamp;
    return TRUE;
}

static gboolean rc_ui_library_list_store_iter_parent(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *child)
{
    iter->stamp = 0;
    return FALSE;
}

static gboolean rc_ui_library_prop_store_iter_parent(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *child)
{
    iter->stamp = 0;
    return FALSE;
}

static void rc_ui_library_list_store_tree_model_init(GtkTreeModelIface *iface)
{
    iface->get_flags = rc_ui_library_list_store_get_flags;
    iface->get_n_columns = rc_ui_library_list_store_get_n_columns;
    iface->get_column_type = rc_ui_library_list_store_get_column_type;
    iface->get_iter = rc_ui_library_list_store_get_iter;
    iface->get_path = rc_ui_library_list_store_get_path;
    iface->get_value = rc_ui_library_list_store_get_value;
    iface->iter_next = rc_ui_library_list_store_iter_next;
    iface->iter_previous = rc_ui_library_list_store_iter_prev;
    iface->iter_children = rc_ui_library_list_store_iter_children;
    iface->iter_has_child = rc_ui_library_list_store_iter_has_child;
    iface->iter_n_children = rc_ui_library_list_store_iter_n_children;
    iface->iter_nth_child = rc_ui_library_list_store_iter_nth_child;
    iface->iter_parent = rc_ui_library_list_store_iter_parent;
}

static void rc_ui_library_prop_store_tree_model_init(GtkTreeModelIface *iface)
{
    iface->get_flags = rc_ui_library_prop_store_get_flags;
    iface->get_n_columns = rc_ui_library_prop_store_get_n_columns;
    iface->get_column_type = rc_ui_library_prop_store_get_column_type;
    iface->get_iter = rc_ui_library_prop_store_get_iter;
    iface->get_path = rc_ui_library_prop_store_get_path;
    iface->get_value = rc_ui_library_prop_store_get_value;
    iface->iter_next = rc_ui_library_prop_store_iter_next;
    iface->iter_previous = rc_ui_library_prop_store_iter_prev;
    iface->iter_children  = rc_ui_library_prop_store_iter_children;
    iface->iter_has_child = rc_ui_library_prop_store_iter_has_child;
    iface->iter_n_children = rc_ui_library_prop_store_iter_n_children;
    iface->iter_nth_child = rc_ui_library_prop_store_iter_nth_child;
    iface->iter_parent = rc_ui_library_prop_store_iter_parent;
}

static void rc_ui_library_list_set_property(GObject *object,
    guint prop_id, const GValue *value, GParamSpec *pspec)
{
    RCUiLibraryListStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(object));
    priv = RC_UI_LIBRARY_LIST_STORE(object)->priv;
    if(priv==NULL) return;
    switch(prop_id)
    {
        case LIST_STORE_PROP_QUERY_RESULT:
        {
            if(priv->query_result!=NULL)
            {
                if(priv->query_result_added_id>0)
                {
                    g_signal_handler_disconnect(priv->query_result,
                        priv->query_result_added_id);
                    priv->query_result_added_id = 0;
                }
                if(priv->query_result_delete_id>0)
                {
                    g_signal_handler_disconnect(priv->query_result,
                        priv->query_result_delete_id);
                    priv->query_result_delete_id = 0;
                }
                if(priv->query_result_changed_id>0)
                {
                    g_signal_handler_disconnect(priv->query_result,
                        priv->query_result_changed_id);
                    priv->query_result_changed_id = 0;
                }
                if(priv->query_result_reordered_id>0)
                {
                    g_signal_handler_disconnect(priv->query_result,
                        priv->query_result_reordered_id);
                    priv->query_result_reordered_id = 0;
                }
                g_object_unref(priv->query_result);
                priv->query_result = NULL;
            }
            priv->query_result = g_value_dup_object(value);
            priv->query_result_added_id = g_signal_connect(priv->query_result,
                "query-result-added",
                G_CALLBACK(rc_ui_library_list_query_result_added_cb),
                object);
            priv->query_result_delete_id = g_signal_connect(priv->query_result,
                "query-result-delete",
                G_CALLBACK(rc_ui_library_list_query_result_delete_cb),
                object);
            priv->query_result_changed_id = g_signal_connect(priv->query_result,
                "query-result-changed",
                G_CALLBACK(rc_ui_library_list_query_result_changed_cb),
                object);
            priv->query_result_reordered_id = g_signal_connect(
                priv->query_result, "query-result-reordered",
                G_CALLBACK(rc_ui_library_list_query_result_reordered_cb),
                object);
            break;
        }
        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
        }
    }
}

static void rc_ui_library_prop_set_property(GObject *object,
    guint prop_id, const GValue *value, GParamSpec *pspec)
{
    RCUiLibraryPropStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(object));
    priv = RC_UI_LIBRARY_PROP_STORE(object)->priv;
    if(priv==NULL) return;
    switch(prop_id)
    {
        case PROP_STORE_PROP_BASE:
        {
            if(priv->base!=NULL)
            {
                if(priv->prop_added_id>0)
                {
                    g_signal_handler_disconnect(priv->base,
                        priv->prop_added_id);
                    priv->prop_added_id = 0;
                }
                if(priv->prop_delete_id>0)
                {
                    g_signal_handler_disconnect(priv->base,
                        priv->prop_delete_id);
                    priv->prop_delete_id = 0;
                }
                if(priv->prop_changed_id>0)
                {
                    g_signal_handler_disconnect(priv->base,
                        priv->prop_changed_id);
                    priv->prop_changed_id = 0;
                }
                if(priv->prop_reordered_id>0)
                {
                    g_signal_handler_disconnect(priv->base,
                        priv->prop_reordered_id);
                    priv->prop_reordered_id = 0;
                }
                g_object_unref(priv->base);
                priv->base = NULL;
            }
            priv->base = g_value_dup_object(value);
            priv->prop_added_id = g_signal_connect(priv->base, "prop-added",
                G_CALLBACK(rc_ui_library_prop_prop_added_cb), object);
            priv->prop_delete_id = g_signal_connect(priv->base, "prop-delete",
                G_CALLBACK(rc_ui_library_prop_prop_delete_cb), object);
            priv->prop_changed_id = g_signal_connect(priv->base,
                "prop-changed", G_CALLBACK(rc_ui_library_prop_prop_changed_cb),
                object);
            priv->prop_reordered_id = g_signal_connect(priv->base,
                "prop-reordered",
                G_CALLBACK(rc_ui_library_prop_prop_reordered_cb), object);
            break;
        }
        case PROP_STORE_PROP_PROP_TYPE:
        {
            priv->prop_type = g_value_get_uint(value);
            break;
        }
        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
        }
    }
}

static void rc_ui_library_list_get_property(GObject *object,
    guint prop_id, GValue *value, GParamSpec *pspec)
{
    RCUiLibraryListStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(object));
    priv = RC_UI_LIBRARY_LIST_STORE(object)->priv;
    if(priv==NULL) return;
    switch(prop_id)
    {
        case LIST_STORE_PROP_QUERY_RESULT:
        {
            g_value_set_object(value, priv->query_result);
            break;
        }
        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
        }
    }
}

static void rc_ui_library_prop_get_property(GObject *object,
    guint prop_id, GValue *value, GParamSpec *pspec)
{
    RCUiLibraryPropStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(object));
    priv = RC_UI_LIBRARY_PROP_STORE(object)->priv;
    if(priv==NULL) return;
    switch(prop_id)
    {
        case PROP_STORE_PROP_BASE:
        {
            g_value_set_object(value, priv->base);
            break;
        }
        case PROP_STORE_PROP_PROP_TYPE:
        {
            g_value_set_uint(value, priv->prop_type);
        }
        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
        }
    }
}

static void rc_ui_library_list_store_finalize(GObject *object)
{
    RCUiLibraryListStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(object));
    priv = RC_UI_LIBRARY_LIST_STORE(object)->priv;
    RC_UI_LIBRARY_LIST_STORE(object)->priv = NULL;
    if(priv->query_result!=NULL)
    {
        if(priv->query_result_added_id>0)
        {
            g_signal_handler_disconnect(priv->query_result,
                priv->query_result_added_id);
            priv->query_result_added_id = 0;
        }
        if(priv->query_result_delete_id>0)
        {
            g_signal_handler_disconnect(priv->query_result,
                priv->query_result_delete_id);
            priv->query_result_delete_id = 0;
        }
        if(priv->query_result_changed_id>0)
        {
            g_signal_handler_disconnect(priv->query_result,
                priv->query_result_changed_id);
            priv->query_result_changed_id = 0;
        }
        if(priv->query_result_reordered_id>0)
        {
            g_signal_handler_disconnect(priv->query_result,
                priv->query_result_reordered_id);
            priv->query_result_reordered_id = 0;
        }
        g_object_unref(priv->query_result);
        priv->query_result = NULL;
    }
    if(priv->base!=NULL)
    {
        g_object_unref(priv->base);
        priv->base = NULL;
    }
    G_OBJECT_CLASS(rc_ui_library_list_store_parent_class)->finalize(object);
}

static void rc_ui_library_prop_store_finalize(GObject *object)
{
    RCUiLibraryPropStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(object));
    priv = RC_UI_LIBRARY_PROP_STORE(object)->priv;
    RC_UI_LIBRARY_PROP_STORE(object)->priv = NULL;
    if(priv->base!=NULL)
    {
        if(priv->prop_added_id>0)
        {
            g_signal_handler_disconnect(priv->base, priv->prop_added_id);
            priv->prop_added_id = 0;
        }   
        if(priv->prop_delete_id>0)
        {
            g_signal_handler_disconnect(priv->base, priv->prop_delete_id);
            priv->prop_delete_id = 0;
        }
        if(priv->prop_changed_id>0)
        {
            g_signal_handler_disconnect(priv->base, priv->prop_changed_id);
            priv->prop_changed_id = 0;
        }
        if(priv->prop_reordered_id>0)
        {
            g_signal_handler_disconnect(priv->base, priv->prop_reordered_id);
            priv->prop_reordered_id = 0;
        }
        g_object_unref(priv->base);
        priv->base = NULL;
    }   
    G_OBJECT_CLASS(rc_ui_library_prop_store_parent_class)->finalize(object);
}

static void rc_ui_library_list_store_class_init(
    RCUiLibraryListStoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_library_list_store_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_library_list_store_finalize;
    object_class->set_property = rc_ui_library_list_set_property;
    object_class->get_property = rc_ui_library_list_get_property;
    g_type_class_add_private(klass, sizeof(RCUiLibraryListStorePrivate));
    
    /**
     * RCUiLibraryListStore:query-result:
     *
     * Sets the query result object of the library list store.
     *
     */
    g_object_class_install_property(object_class, LIST_STORE_PROP_QUERY_RESULT,
        g_param_spec_object("query-result", "Query Result",
        "The query result of the library list store",
        RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT, G_PARAM_READWRITE |
        G_PARAM_CONSTRUCT_ONLY));
}

static void rc_ui_library_prop_store_class_init(
    RCUiLibraryPropStoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_library_prop_store_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_library_prop_store_finalize;
    object_class->set_property = rc_ui_library_prop_set_property;
    object_class->get_property = rc_ui_library_prop_get_property;
    g_type_class_add_private(klass, sizeof(RCUiLibraryPropStorePrivate));
    
    /**
     * RCUiLibraryPropStore:base:
     *
     * Sets the base query result object of the library property store.
     *
     */
    g_object_class_install_property(object_class, PROP_STORE_PROP_BASE,
        g_param_spec_object("base", "Base Query Result",
        "The base query result of the library property store",
        RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT, G_PARAM_READWRITE |
        G_PARAM_CONSTRUCT_ONLY));
        
    /**
     * RCUiLibraryPropStore:property-type:
     *
     * Sets the property type for the library property store.
     *
     */
    g_object_class_install_property(object_class, PROP_STORE_PROP_PROP_TYPE,
        g_param_spec_uint("property-type", "Property Type",
        "The property type of the library property store",
        RCLIB_DB_QUERY_DATA_TYPE_NONE, RCLIB_DB_QUERY_DATA_TYPE_GENRE,
        RCLIB_DB_QUERY_DATA_TYPE_ARTIST, G_PARAM_READWRITE |
        G_PARAM_CONSTRUCT_ONLY));
}

static void rc_ui_library_list_store_init(RCUiLibraryListStore *store)
{
    RCUiLibraryListStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(store));
    priv = G_TYPE_INSTANCE_GET_PRIVATE(store, RC_UI_TYPE_LIBRARY_LIST_STORE,
        RCUiLibraryListStorePrivate);
    store->priv = priv;
    g_return_if_fail(priv!=NULL);
    priv->stamp = g_random_int();
    priv->n_columns = RC_UI_LIBRARY_LIST_COLUMN_LAST;
}

static void rc_ui_library_prop_store_init(RCUiLibraryPropStore *store)
{
    RCUiLibraryPropStorePrivate *priv;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(store));
    priv = G_TYPE_INSTANCE_GET_PRIVATE(store, RC_UI_TYPE_LIBRARY_PROP_STORE,
        RCUiLibraryPropStorePrivate);
    store->priv = priv;
    g_return_if_fail(priv!=NULL);
    priv->stamp = g_random_int();
    priv->n_columns = RC_UI_LIBRARY_PROP_COLUMN_LAST;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, 0);
    tree_iter.user_data = GUINT_TO_POINTER(1);
    tree_iter.user_data2 = GUINT_TO_POINTER(1);
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_inserted(GTK_TREE_MODEL(store), path, &tree_iter);
    gtk_tree_path_free(path);
}

GType rc_ui_library_list_store_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo type_info = {
        .class_size = sizeof(RCUiLibraryListStoreClass),
        .base_init =  NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_library_list_store_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiLibraryListStore),
        .n_preallocs = 1,
        .instance_init = (GInstanceInitFunc)rc_ui_library_list_store_init
    };
    static const GInterfaceInfo tree_model_info = {
        .interface_init = (GInterfaceInitFunc)
            rc_ui_library_list_store_tree_model_init,
        .interface_finalize = NULL,
        .interface_data = NULL
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCUiLibraryListStore"), &type_info, 0);
        g_type_add_interface_static(g_define_type_id, GTK_TYPE_TREE_MODEL,
            &tree_model_info);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rc_ui_library_prop_store_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo type_info = {
        .class_size = sizeof(RCUiLibraryPropStoreClass),
        .base_init =  NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_library_prop_store_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiLibraryPropStore),
        .n_preallocs = 1,
        .instance_init = (GInstanceInitFunc)rc_ui_library_prop_store_init
    };
    static const GInterfaceInfo tree_model_info = {
        .interface_init = (GInterfaceInitFunc)
            rc_ui_library_prop_store_tree_model_init,
        .interface_finalize = NULL,
        .interface_data = NULL
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCUiLibraryPropStore"), &type_info, 0);
        g_type_add_interface_static(g_define_type_id, GTK_TYPE_TREE_MODEL,
            &tree_model_info);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_ui_library_list_store_new:
 * @query_result: the query result
 * 
 * Create a new library list store.
 * 
 * Returns: (transfer full): The new library list store, #NULL if failed.
 */

GtkTreeModel *rc_ui_library_list_store_new(
    RCLibDbLibraryQueryResult *query_result)
{
    GtkTreeModel *model = g_object_new(RC_UI_TYPE_LIBRARY_LIST_STORE,
        "query-result", query_result, NULL);
    return model;
}

/**
 * rc_ui_library_prop_store_new:
 * @base: the base query result
 * @prop_type: the property type 
 * 
 * Create a new library property store.
 * 
 * Returns: (transfer full): The new library property store, #NULL if failed.
 */

GtkTreeModel *rc_ui_library_prop_store_new(
    RCLibDbLibraryQueryResult *base, RCLibDbQueryDataType prop_type)
{
    GtkTreeModel *model = g_object_new(RC_UI_TYPE_LIBRARY_PROP_STORE,
        "base", base, "property-type", prop_type, NULL);
    return model;
}
