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
};

struct _RCUiLibraryPropStorePrivate
{
    RCLibDbLibraryQueryResult *base;
    RCLibDbQueryDataType prop_type;
    gint stamp;
    gint n_columns;
};

static gchar *format_string = NULL;
static gpointer rc_ui_library_list_store_parent_class = NULL;
static gpointer rc_ui_library_prop_store_parent_class = NULL;


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
    if(priv->base==NULL ||
        i-1>=rclib_db_library_query_result_prop_get_length(priv->base,
        priv->prop_type))
    {
        return FALSE;
    }
    iter->stamp = priv->stamp;
    if(i>0)
    {
        iter->user_data = rclib_db_library_query_result_prop_get_iter_at_pos(
            priv->base, priv->prop_type, i);
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
            iter->user_data));
    }
    return path;
}

static void rc_ui_library_list_store_get_value(GtkTreeModel *model,
    GtkTreeIter *iter, gint column, GValue *value)
{
    RCUiLibraryListStore *store;
    RCUiLibraryListStorePrivate *priv;
    RCLibDbLibraryData *library_data;
    RCLibDbLibraryQueryResultIter *seq_iter, *reference;
    gchar *dstr;
    GstState state;
    gint vint;
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
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TYPE, &type,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_STRING);
            if(type==RCLIB_DB_LIBRARY_TYPE_MISSING)
            {
                g_value_set_static_string(value, GTK_STOCK_CANCEL);
                break;
            }
            rclib_core_get_external_reference(NULL, (gpointer *)&reference);
            if(reference!=seq_iter)
            {
                g_value_set_static_string(value, NULL);
                break;
            }
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
            gchar *dalbum = NULL;
            gchar *dartist = NULL;
            gchar *rtitle = NULL;
            gchar *rartist, *ralbum;
            GString *ftitle;
            gsize i, len;
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &duri,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE, &dtitle,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST, &dartist,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM, &dalbum,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
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
            if(dartist!=NULL && strlen(dartist)>0)
                rartist = g_strdup(dartist);
            else
                rartist = g_strdup(_("Unknown Artist"));
            g_free(dartist);
            if(dalbum!=NULL && strlen(dalbum)>0)
                ralbum = g_strdup(dalbum);
            else
                ralbum = g_strdup(_("Unknown Album"));
            g_free(dalbum);
            len = strlen(format_string);
            ftitle = g_string_new(NULL);
            for(i=0;i<len;i++)
            {
                if(format_string[i]!='%')
                    g_string_append_c(ftitle, format_string[i]);
                else
                {
                    if(strncmp(format_string+i, "%TITLE", 6)==0)
                    {
                        g_string_append(ftitle, rtitle);
                        i+=5;
                    }
                    else if(strncmp(format_string+i, "%ARTIST", 7)==0)
                    {
                        g_string_append(ftitle, rartist);
                        i+=6;
                    }
                    else if(strncmp(format_string+i, "%ALBUM", 6)==0)
                    {
                        g_string_append(ftitle, ralbum);
                        i+=5;
                    }
                    else
                        g_string_append_c(ftitle, format_string[i]);
                }
            }
            g_free(rtitle);
            g_free(rartist);
            g_free(ralbum);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, g_string_free(ftitle, FALSE));
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
            library_data = rclib_db_library_query_result_get_data(
                priv->query_result, seq_iter);
            if(library_data!=NULL)
            {
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_TYPE, &type,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
            }
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, FALSE);
            if(type==RCLIB_DB_LIBRARY_TYPE_MISSING) break;
            rclib_core_get_external_reference(NULL, (gpointer *)&reference);
            if(reference!=seq_iter) break;
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
            g_value_take_string(value, dstr);
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
        return rclib_db_library_query_result_prop_get_length(priv->base,
            priv->prop_type) + 1;
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

static void rc_ui_library_list_store_finalize(GObject *object)
{
    RCUiLibraryListStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_LIBRARY_LIST_STORE(object));
    priv = RC_UI_LIBRARY_LIST_STORE(object)->priv;
    RC_UI_LIBRARY_LIST_STORE(object)->priv = NULL;
    if(priv->query_result!=NULL)
    {
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
        g_object_unref(priv->base);
        priv->base = NULL;
    }   
    G_OBJECT_CLASS(rc_ui_library_prop_store_parent_class)->finalize(object);
}

static void rc_ui_library_list_store_class_init(
    RCUiLibraryListStoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_library_prop_store_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_library_list_store_finalize;
    g_type_class_add_private(klass, sizeof(RCUiLibraryListStorePrivate));
}

static void rc_ui_library_prop_store_class_init(
    RCUiLibraryPropStoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_library_prop_store_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_library_prop_store_finalize;
    g_type_class_add_private(klass, sizeof(RCUiLibraryPropStorePrivate));
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
    g_return_if_fail(RC_UI_IS_LIBRARY_PROP_STORE(store));
    priv = G_TYPE_INSTANCE_GET_PRIVATE(store, RC_UI_TYPE_LIBRARY_PROP_STORE,
        RCUiLibraryPropStorePrivate);
    store->priv = priv;
    g_return_if_fail(priv!=NULL);
    priv->stamp = g_random_int();
    priv->n_columns = RC_UI_LIBRARY_PROP_COLUMN_LAST;
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
 * Returns: The new library list store, #NULL if failed.
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
 * Returns: The new library property store, #NULL if failed.
 */

GtkTreeModel *rc_ui_library_prop_store_new(
    RCLibDbLibraryQueryResult *base, RCLibDbQueryDataType prop_type)
{
    GtkTreeModel *model = g_object_new(RC_UI_TYPE_LIBRARY_PROP_STORE,
        "base", base, "property-type", prop_type, NULL);
    return model;
}
