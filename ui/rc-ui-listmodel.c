/*
 * RhythmCat UI List Model Module
 * Show music catalog & library.
 *
 * rc-ui-listmodel.c
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

#include "rc-ui-listmodel.h"
#include "rc-common.h"

/**
 * SECTION: rc-ui-listmodel
 * @Short_description: Catalog and playlist list view models
 * @Title: Playlist View Models
 * @Include: rc-ui-listmodel.h
 *
 * This module provides 2 classes: #RCUiCatalogStore and #RCUiPlaylistStore,
 * which implements interface #GtkTreeModel. They can be used by #GtkTreeView
 * for showing and operating the data inside. #RCUiCatalogStore contains the
 * catalog list, and #RCUiPlaylistStore contains the playlist.
 */

struct _RCUiCatalogStorePrivate
{
    gint stamp;
    gint n_columns;
};

struct _RCUiPlaylistStorePrivate
{
    RCLibDbCatalogIter *catalog_iter;
    gint stamp;
    gint n_columns;
};

static GtkTreeModel *catalog_model = NULL;
static gchar *format_string = NULL;
static gpointer rc_ui_catalog_store_parent_class = NULL;
static gpointer rc_ui_playlist_store_parent_class = NULL;

static GtkTreeModelFlags rc_ui_catalog_store_get_flags(GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model),
        (GtkTreeModelFlags)0);
    return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static GtkTreeModelFlags rc_ui_playlist_store_get_flags(GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model),
        (GtkTreeModelFlags)0);
    return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint rc_ui_catalog_store_get_n_columns(GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), 0);
    return RC_UI_CATALOG_COLUMN_LAST;
}

static gint rc_ui_playlist_store_get_n_columns(GtkTreeModel *model)
{
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), 0);
    return RC_UI_PLAYLIST_COLUMN_LAST;
}

static GType rc_ui_catalog_store_get_column_type(GtkTreeModel *model,
    gint index)
{
    GType type;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), G_TYPE_INVALID);
    switch(index)
    {
        case RC_UI_CATALOG_COLUMN_TYPE:
            type = G_TYPE_INT;
            break;
        case RC_UI_CATALOG_COLUMN_STATE:
        case RC_UI_CATALOG_COLUMN_NAME:
            type = G_TYPE_STRING;
            break;
        case RC_UI_CATALOG_COLUMN_PLAYING_FLAG:
            type = G_TYPE_BOOLEAN;
            break;
        default:
            type = G_TYPE_INVALID;
            break;
    }
    return type;
}

static GType rc_ui_playlist_store_get_column_type(GtkTreeModel *model,
    gint index)
{
    GType type;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), G_TYPE_INVALID);
    switch(index)
    {
        case RC_UI_PLAYLIST_COLUMN_TYPE:
        case RC_UI_PLAYLIST_COLUMN_TRACK:
        case RC_UI_PLAYLIST_COLUMN_YEAR:
            type = G_TYPE_INT;
            break;
        case RC_UI_PLAYLIST_COLUMN_STATE:
        case RC_UI_PLAYLIST_COLUMN_FTITLE:
        case RC_UI_PLAYLIST_COLUMN_TITLE:
        case RC_UI_PLAYLIST_COLUMN_ARTIST:
        case RC_UI_PLAYLIST_COLUMN_ALBUM:
        case RC_UI_PLAYLIST_COLUMN_FTYPE:
        case RC_UI_PLAYLIST_COLUMN_GENRE:
        case RC_UI_PLAYLIST_COLUMN_LENGTH:
            type = G_TYPE_STRING;
            break;
        case RC_UI_PLAYLIST_COLUMN_PLAYING_FLAG:
            type = G_TYPE_BOOLEAN;
            break;
        case RC_UI_PLAYLIST_COLUMN_RATING:
            type = G_TYPE_FLOAT;
            break;
        default:
            type = G_TYPE_INVALID;
            break;
    }
    return type;
}

static gboolean rc_ui_catalog_store_get_iter(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreePath *path)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    gint i;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), FALSE);
    g_return_val_if_fail(path!=NULL, FALSE);
    store = RC_UI_CATALOG_STORE(model);
    priv = store->priv;
    i = gtk_tree_path_get_indices(path)[0];
    if(i>=rclib_db_catalog_get_length())
        return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = rclib_db_catalog_get_iter_at_pos(i);
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    return TRUE;
}

static gboolean rc_ui_playlist_store_get_iter(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreePath *path)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    gint i;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), FALSE);
    g_return_val_if_fail(path!=NULL, FALSE);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    i = gtk_tree_path_get_indices(path)[0];
    if(i>=rclib_db_playlist_get_length(priv->catalog_iter))
        return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = rclib_db_playlist_get_iter_at_pos(priv->catalog_iter,
        i);
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    return TRUE;
}

static GtkTreePath *rc_ui_catalog_store_get_path(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    GtkTreePath *path;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), NULL);
    store = RC_UI_CATALOG_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, NULL);
    g_return_val_if_fail(iter->stamp==priv->stamp, NULL);
    if(rclib_db_catalog_iter_is_end((RCLibDbCatalogIter *)iter->user_data))
        return NULL;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path,
        rclib_db_catalog_iter_get_position((RCLibDbCatalogIter *)
        iter->user_data));
    return path;
}

static GtkTreePath *rc_ui_playlist_store_get_path(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    GtkTreePath *path;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), NULL);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, NULL);
    g_return_val_if_fail(iter->stamp==priv->stamp, NULL);
    if(rclib_db_playlist_iter_is_end((RCLibDbPlaylistIter *)iter->user_data))
        return NULL;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path,
        rclib_db_playlist_iter_get_position((RCLibDbPlaylistIter *)
        iter->user_data));
    return path;
}

static void rc_ui_catalog_store_get_value(GtkTreeModel *model,
    GtkTreeIter *iter, gint column, GValue *value)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    RCLibDbCatalogIter *seq_iter, *ref_catalog_iter = NULL;
    RCLibDbPlaylistIter *reference_iter = NULL;
    GstState state;
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(model));
    g_return_if_fail(iter!=NULL);
    store = RC_UI_CATALOG_STORE(model);
    priv = store->priv;
    g_return_if_fail(priv!=NULL);
    g_return_if_fail(column<priv->n_columns);
    seq_iter = iter->user_data;
    switch(column)
    {
        case RC_UI_CATALOG_COLUMN_TYPE:
        {
            RCLibDbCatalogType type = 0;
            rclib_db_catalog_data_iter_get(seq_iter,
                RCLIB_DB_CATALOG_DATA_TYPE_TYPE, &type,
                RCLIB_DB_CATALOG_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, type);
            break;
        }
        case RC_UI_CATALOG_COLUMN_STATE:
        {
            g_value_init(value, G_TYPE_STRING);
            reference_iter = (RCLibDbPlaylistIter *)
                rclib_core_get_db_reference();
            if(reference_iter==NULL ||
                !rclib_db_playlist_is_valid_iter(reference_iter))
            {
                g_value_set_static_string(value, NULL);
                break;
            }
            rclib_db_playlist_data_iter_get(reference_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &ref_catalog_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            if(ref_catalog_iter==NULL || ref_catalog_iter!=seq_iter)
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
        case RC_UI_CATALOG_COLUMN_NAME:
        {
            gchar *str = NULL;
            rclib_db_catalog_data_iter_get(seq_iter,
                RCLIB_DB_CATALOG_DATA_TYPE_NAME, &str,
                RCLIB_DB_CATALOG_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, str);
            break;
        }
        case RC_UI_CATALOG_COLUMN_PLAYING_FLAG:
        {
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, FALSE);
            reference_iter = (RCLibDbPlaylistIter *)
                rclib_core_get_db_reference();
            if(reference_iter==NULL ||
                !rclib_db_playlist_is_valid_iter(reference_iter))
            {
                break;
            }
            rclib_db_playlist_data_iter_get(reference_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &ref_catalog_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            if(ref_catalog_iter==NULL || ref_catalog_iter!=seq_iter)
            {
                break;
            }
            if(!rclib_core_get_state(&state, NULL, 0))
                break;
            if(state==GST_STATE_PLAYING || state==GST_STATE_PAUSED)
            {
                g_value_set_boolean(value, TRUE);
            }
            break;
        }
        default:
            break;
    }
}

static void rc_ui_playlist_store_get_value(GtkTreeModel *model,
    GtkTreeIter *iter, gint column, GValue *value)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    RCLibDbPlaylistIter *seq_iter, *reference;
    gchar *dstr;
    GstState state;
    gint vint;
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(model));
    g_return_if_fail(iter!=NULL);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    g_return_if_fail(priv!=NULL);
    g_return_if_fail(column<priv->n_columns);
    seq_iter = iter->user_data;
    switch(column)
    {
        case RC_UI_PLAYLIST_COLUMN_TYPE:
        {
            RCLibDbPlaylistType type = 0;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE, &type,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, type);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_STATE:
        {
            RCLibDbPlaylistType type = 0;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE, &type,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_STRING);
            if(type==RCLIB_DB_PLAYLIST_TYPE_MISSING)
            {
                g_value_set_static_string(value, GTK_STOCK_CANCEL);
                break;
            }
            reference = (RCLibDbPlaylistIter *)rclib_core_get_db_reference();
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
        case RC_UI_PLAYLIST_COLUMN_FTITLE:
        {
            gchar *duri = NULL;
            gchar *dtitle = NULL;
            gchar *dalbum = NULL;
            gchar *dartist = NULL;
            gchar *rtitle = NULL;
            gchar *rartist, *ralbum;
            GString *ftitle;
            gsize i, len;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &duri,
                RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE, &dtitle,
                RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST, &dartist,
                RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM, &dalbum,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
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
        case RC_UI_PLAYLIST_COLUMN_TITLE:
        {
            dstr = NULL;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE, &dstr,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_ARTIST:
        {
            dstr = NULL;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST, &dstr,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_ALBUM:
        {
            dstr = NULL;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM, &dstr,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_FTYPE:
        {
            dstr = NULL;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE, &dstr,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_GENRE:
        {
            dstr = NULL;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE, &dstr,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, dstr);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_LENGTH:
        {
            gint sec, min;
            gchar *string;
            gint64 tlength = 0;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH, &tlength,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            sec = (gint)(tlength / GST_SECOND);
            min = sec / 60;
            sec = sec % 60;
            string = g_strdup_printf("%02d:%02d", min, sec);
            g_value_init(value, G_TYPE_STRING);
            g_value_take_string(value, string);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_TRACK:
        {
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM, &vint,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, vint);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_YEAR:
        {
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR, &vint,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, vint);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_RATING:
        {
            gfloat rating = 0.0;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_RATING, &rating,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_FLOAT);
            g_value_set_float(value, rating);
            break;
        }
        case RC_UI_PLAYLIST_COLUMN_PLAYING_FLAG:
        {
            RCLibDbPlaylistType type = 0;
            rclib_db_playlist_data_iter_get(seq_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE, &type,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, FALSE);
            if(type==RCLIB_DB_PLAYLIST_TYPE_MISSING) break;
            reference = (RCLibDbPlaylistIter *)rclib_core_get_db_reference();
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

static gboolean rc_ui_catalog_store_iter_next(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_CATALOG_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = rclib_db_catalog_iter_next((RCLibDbCatalogIter *)
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

static gboolean rc_ui_playlist_store_iter_next(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = rclib_db_playlist_iter_next((RCLibDbPlaylistIter *)
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

static gboolean rc_ui_catalog_store_iter_prev(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_CATALOG_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = rclib_db_catalog_iter_prev((RCLibDbCatalogIter *)
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

static gboolean rc_ui_playlist_store_iter_prev(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), FALSE);
    g_return_val_if_fail(iter!=NULL, FALSE);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = rclib_db_playlist_iter_prev((RCLibDbPlaylistIter *)
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

static gboolean rc_ui_catalog_store_iter_children(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), FALSE);
    store = RC_UI_CATALOG_STORE(model);
    priv = RC_UI_CATALOG_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    if(rclib_db_catalog_get_length()>0)
    {
        iter->stamp = priv->stamp;
        iter->user_data = rclib_db_catalog_get_begin_iter();
        return TRUE;
    }
    else
    {
        iter->stamp = 0;
        return FALSE;
    }
}

static gboolean rc_ui_playlist_store_iter_children(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), FALSE);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    if(rclib_db_playlist_get_length(priv->catalog_iter)>0)
    {
        iter->stamp = priv->stamp;
        iter->user_data = rclib_db_playlist_get_begin_iter(
            priv->catalog_iter);
        return TRUE;
    }
    else
    {
        iter->stamp = 0;
        return FALSE;
    }
}

static gboolean rc_ui_catalog_store_iter_has_child(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    return FALSE;
}

static gboolean rc_ui_playlist_store_iter_has_child(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    return FALSE;
}

static gint rc_ui_catalog_store_iter_n_children(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiCatalogStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), -1);
    priv = RC_UI_CATALOG_STORE(model)->priv;
    g_return_val_if_fail(priv!=NULL, -1);
    if(iter==NULL)
        return rclib_db_catalog_get_length();
    g_return_val_if_fail(priv->stamp==iter->stamp, -1);
    return 0;
}

static gint rc_ui_playlist_store_iter_n_children(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiPlaylistStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), -1);
    priv = RC_UI_PLAYLIST_STORE(model)->priv;
    g_return_val_if_fail(priv!=NULL, -1);
    if(iter==NULL)
        return rclib_db_playlist_get_length(priv->catalog_iter);
    g_return_val_if_fail(priv->stamp==iter->stamp, -1);
    return 0;
}

static gboolean rc_ui_catalog_store_iter_nth_child (GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    RCLibDbCatalogIter *child;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), FALSE);
    store = RC_UI_CATALOG_STORE(model);
    priv = store->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL) return FALSE;
    child = rclib_db_catalog_get_iter_at_pos(n);
    if(rclib_db_catalog_iter_is_end(child)) return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = child;
    return TRUE;
}

static gboolean rc_ui_playlist_store_iter_nth_child (GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    RCLibDbPlaylistIter *child;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), FALSE);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL) return FALSE;
    child = rclib_db_playlist_get_iter_at_pos(priv->catalog_iter, n);
    if(rclib_db_playlist_iter_is_end(child)) return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = child;
    return TRUE;
}

static gboolean rc_ui_catalog_store_iter_parent(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *child)
{
    iter->stamp = 0;
    return FALSE;
}

static gboolean rc_ui_playlist_store_iter_parent(GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *child)
{
    iter->stamp = 0;
    return FALSE;
}

static void rc_ui_catalog_store_tree_model_init(GtkTreeModelIface *iface)
{
    iface->get_flags = rc_ui_catalog_store_get_flags;
    iface->get_n_columns = rc_ui_catalog_store_get_n_columns;
    iface->get_column_type = rc_ui_catalog_store_get_column_type;
    iface->get_iter = rc_ui_catalog_store_get_iter;
    iface->get_path = rc_ui_catalog_store_get_path;
    iface->get_value = rc_ui_catalog_store_get_value;
    iface->iter_next = rc_ui_catalog_store_iter_next;
    iface->iter_previous = rc_ui_catalog_store_iter_prev;
    iface->iter_children  = rc_ui_catalog_store_iter_children;
    iface->iter_has_child = rc_ui_catalog_store_iter_has_child;
    iface->iter_n_children = rc_ui_catalog_store_iter_n_children;
    iface->iter_nth_child = rc_ui_catalog_store_iter_nth_child;
    iface->iter_parent = rc_ui_catalog_store_iter_parent;
}

static void rc_ui_playlist_store_tree_model_init(GtkTreeModelIface *iface)
{
    iface->get_flags = rc_ui_playlist_store_get_flags;
    iface->get_n_columns = rc_ui_playlist_store_get_n_columns;
    iface->get_column_type = rc_ui_playlist_store_get_column_type;
    iface->get_iter = rc_ui_playlist_store_get_iter;
    iface->get_path = rc_ui_playlist_store_get_path;
    iface->get_value = rc_ui_playlist_store_get_value;
    iface->iter_next = rc_ui_playlist_store_iter_next;
    iface->iter_previous = rc_ui_playlist_store_iter_prev;
    iface->iter_children  = rc_ui_playlist_store_iter_children;
    iface->iter_has_child = rc_ui_playlist_store_iter_has_child;
    iface->iter_n_children = rc_ui_playlist_store_iter_n_children;
    iface->iter_nth_child = rc_ui_playlist_store_iter_nth_child;
    iface->iter_parent = rc_ui_playlist_store_iter_parent;
}

static void rc_ui_catalog_store_finalize(GObject *object)
{
    RC_UI_CATALOG_STORE(object)->priv = NULL;
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(object));
    G_OBJECT_CLASS(rc_ui_catalog_store_parent_class)->finalize(object);
}

static void rc_ui_playlist_store_finalize(GObject *object)
{
    RC_UI_PLAYLIST_STORE(object)->priv = NULL;
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(object));
    G_OBJECT_CLASS(rc_ui_playlist_store_parent_class)->finalize(object);
}

static void rc_ui_catalog_store_class_init(RCUiCatalogStoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_catalog_store_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_catalog_store_finalize;
    g_type_class_add_private(klass, sizeof(RCUiCatalogStorePrivate));
}

static void rc_ui_playlist_store_class_init(RCUiPlaylistStoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rc_ui_playlist_store_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rc_ui_playlist_store_finalize;
    g_type_class_add_private(klass, sizeof(RCUiPlaylistStorePrivate));
}


static void rc_ui_catalog_store_init(RCUiCatalogStore *store)
{
    RCUiCatalogStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(store));
    priv = G_TYPE_INSTANCE_GET_PRIVATE(store, RC_UI_TYPE_CATALOG_STORE,
        RCUiCatalogStorePrivate);
    store->priv = priv;
    g_return_if_fail(priv!=NULL);
	priv->stamp = g_random_int();
    priv->n_columns = RC_UI_CATALOG_COLUMN_LAST;
}

static void rc_ui_playlist_store_init(RCUiPlaylistStore *store)
{
    RCUiPlaylistStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(store));
    priv = G_TYPE_INSTANCE_GET_PRIVATE(store, RC_UI_TYPE_PLAYLIST_STORE,
        RCUiPlaylistStorePrivate);
    store->priv = priv;
    g_return_if_fail(priv!=NULL);
	priv->stamp = g_random_int();
    priv->catalog_iter = NULL;
    priv->n_columns = RC_UI_PLAYLIST_COLUMN_LAST;
}

GType rc_ui_catalog_store_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo type_info = {
        .class_size = sizeof(RCUiCatalogStoreClass),
        .base_init =  NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_catalog_store_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiCatalogStore),
        .n_preallocs = 1,
        .instance_init = (GInstanceInitFunc)rc_ui_catalog_store_init
    };
    static const GInterfaceInfo tree_model_info = {
        .interface_init = (GInterfaceInitFunc)
            rc_ui_catalog_store_tree_model_init,
        .interface_finalize = NULL,
        .interface_data = NULL
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCUiCatalogStore"), &type_info, 0);
        g_type_add_interface_static(g_define_type_id, GTK_TYPE_TREE_MODEL,
            &tree_model_info);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

GType rc_ui_playlist_store_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo type_info = {
        .class_size = sizeof(RCUiPlaylistStoreClass),
        .base_init =  NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_playlist_store_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiPlaylistStore),
        .n_preallocs = 1,
        .instance_init = (GInstanceInitFunc)rc_ui_playlist_store_init
    };
    static const GInterfaceInfo tree_model_info = {
        .interface_init = (GInterfaceInitFunc)
            rc_ui_playlist_store_tree_model_init,
        .interface_finalize = NULL,
        .interface_data = NULL
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCUiPlaylistStore"), &type_info, 0);
        g_type_add_interface_static(g_define_type_id, GTK_TYPE_TREE_MODEL,
            &tree_model_info);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

static void rc_ui_list_model_catalog_added_cb(RCLibDb *db,
    RCLibDbCatalogIter *iter, gpointer data)
{
    RCUiCatalogStorePrivate *priv;
    RCUiPlaylistStorePrivate *playlist_priv;
    GtkTreeModel *playlist_model;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(catalog_model));
    priv = RC_UI_CATALOG_STORE(catalog_model)->priv;
    g_return_if_fail(priv!=NULL);
    playlist_model = GTK_TREE_MODEL(g_object_new(
        RC_UI_TYPE_PLAYLIST_STORE, NULL));    
    playlist_priv = RC_UI_PLAYLIST_STORE(playlist_model)->priv;
    playlist_priv->catalog_iter = iter;
    rclib_db_catalog_data_iter_set(iter, RCLIB_DB_CATALOG_DATA_TYPE_STORE,
        playlist_model, RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    pos = rclib_db_catalog_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_inserted(catalog_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_catalog_changed_cb(RCLibDb *db,
    RCLibDbCatalogIter *iter, gpointer data)
{
    RCUiCatalogStorePrivate *priv;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(catalog_model));
    priv = RC_UI_CATALOG_STORE(catalog_model)->priv;
    g_return_if_fail(priv!=NULL);
    pos = rclib_db_catalog_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_changed(catalog_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_catalog_delete_cb(RCLibDb *db,
    RCLibDbCatalogIter *iter, gpointer data)
{
    gpointer store = NULL;
    GtkTreePath *path;
    gint pos;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(catalog_model));
    rclib_db_catalog_data_iter_get(iter, RCLIB_DB_CATALOG_DATA_TYPE_STORE,
        &store, RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    if(store!=NULL) g_object_unref(G_OBJECT(store));
    pos = rclib_db_catalog_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    gtk_tree_model_row_deleted(catalog_model, path);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_catalog_reordered_cb(RCLibDb *db,
    gint *new_order, gpointer data)
{
    GtkTreePath *path;
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(catalog_model));
    g_return_if_fail(new_order!=NULL);
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(catalog_model, path, NULL, new_order);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_playlist_added_cb(RCLibDb *db,
    RCLibDbPlaylistIter *iter, gpointer data)
{
    RCUiPlaylistStorePrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GtkTreePath *path;
    GtkTreeModel *playlist_model = NULL;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    rclib_db_playlist_data_iter_get(iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    rclib_db_catalog_data_iter_get(catalog_iter,
        RCLIB_DB_CATALOG_DATA_TYPE_STORE, &playlist_model,
        RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    if(playlist_model==NULL) return;
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(playlist_model));
    priv = RC_UI_PLAYLIST_STORE(playlist_model)->priv;
    g_return_if_fail(priv!=NULL);
    pos = rclib_db_playlist_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_inserted(playlist_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_playlist_changed_cb(RCLibDb *db,
    RCLibDbPlaylistIter *iter, gpointer data)
{
    RCUiPlaylistStorePrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GtkTreePath *path;
    GtkTreeModel *playlist_model = NULL;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    rclib_db_playlist_data_iter_get(iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    rclib_db_catalog_data_iter_get(catalog_iter,
        RCLIB_DB_CATALOG_DATA_TYPE_STORE, &playlist_model,
        RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(playlist_model));
    priv = RC_UI_PLAYLIST_STORE(playlist_model)->priv;
    g_return_if_fail(priv!=NULL);
    pos = rclib_db_playlist_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_changed(playlist_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_playlist_delete_cb(RCLibDb *db,
    RCLibDbPlaylistIter *iter, gpointer data)
{
    RCUiPlaylistStorePrivate *priv;
    RCLibDbCatalogIter *catalog_iter = NULL;
    GtkTreePath *path;
    GtkTreeModel *playlist_model = NULL;
    gint pos;
    g_return_if_fail(iter!=NULL);
    rclib_db_playlist_data_iter_get(iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    rclib_db_catalog_data_iter_get(catalog_iter,
        RCLIB_DB_CATALOG_DATA_TYPE_STORE, &playlist_model,
        RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(playlist_model));
    priv = RC_UI_PLAYLIST_STORE(playlist_model)->priv;
    g_return_if_fail(priv!=NULL);
    pos = rclib_db_playlist_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    gtk_tree_model_row_deleted(playlist_model, path);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_playlist_reordered_cb(RCLibDb *db,
    RCLibDbCatalogIter *iter, gint *new_order, gpointer data)
{
    GtkTreePath *path;
    GtkTreeModel *playlist_model = NULL;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(new_order!=NULL);
    rclib_db_catalog_data_iter_get(iter, RCLIB_DB_CATALOG_DATA_TYPE_STORE,
        &playlist_model, RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    g_return_if_fail(playlist_model!=NULL);
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(playlist_model, path, NULL, new_order);
    gtk_tree_path_free(path);
}

static gboolean rc_ui_list_model_init()
{
    RCUiPlaylistStorePrivate *playlist_priv;
    RCLibDbCatalogIter *catalog_iter;
    GtkTreeModel *playlist_model;
    if(catalog_model!=NULL) return FALSE;
    if(format_string==NULL)
        format_string = g_strdup("%TITLE");
    catalog_model = GTK_TREE_MODEL(g_object_new(
        RC_UI_TYPE_CATALOG_STORE, NULL));
    for(catalog_iter = rclib_db_catalog_get_begin_iter();catalog_iter!=NULL;
        catalog_iter = rclib_db_catalog_iter_next(catalog_iter))
    {
        playlist_model = GTK_TREE_MODEL(g_object_new(
            RC_UI_TYPE_PLAYLIST_STORE, NULL));
        playlist_priv = RC_UI_PLAYLIST_STORE(playlist_model)->priv;
        rclib_db_catalog_data_iter_set(catalog_iter,
            RCLIB_DB_CATALOG_DATA_TYPE_STORE, playlist_model,
            RCLIB_DB_CATALOG_DATA_TYPE_NONE);         
        playlist_priv->catalog_iter = catalog_iter;
    }
    rclib_db_signal_connect("catalog-added",
        G_CALLBACK(rc_ui_list_model_catalog_added_cb), NULL);
    rclib_db_signal_connect("catalog-changed",
        G_CALLBACK(rc_ui_list_model_catalog_changed_cb), NULL);
    rclib_db_signal_connect("catalog-delete",
        G_CALLBACK(rc_ui_list_model_catalog_delete_cb), NULL);
    rclib_db_signal_connect("catalog-reordered",
        G_CALLBACK(rc_ui_list_model_catalog_reordered_cb), NULL);
    rclib_db_signal_connect("playlist-added",
        G_CALLBACK(rc_ui_list_model_playlist_added_cb), NULL);
    rclib_db_signal_connect("playlist-changed",
        G_CALLBACK(rc_ui_list_model_playlist_changed_cb), NULL);
    rclib_db_signal_connect("playlist-delete",
        G_CALLBACK(rc_ui_list_model_playlist_delete_cb), NULL);
    rclib_db_signal_connect("playlist-reordered",
        G_CALLBACK(rc_ui_list_model_playlist_reordered_cb), NULL);
    return TRUE;
}

/**
 * rc_ui_list_model_get_catalog_store:
 * 
 * Get the catalog store. If the catalog store is not initialize,
 * it will be intialized first.
 *
 * Returns: (transfer none): The catalog store.
 */

GtkTreeModel *rc_ui_list_model_get_catalog_store()
{
    if(catalog_model==NULL)
        if(!rc_ui_list_model_init()) return NULL;
    return catalog_model;
}

/**
 * rc_ui_list_model_get_playlist_store:
 * @iter: the iter for the playlist in the catalog
 *
 * Get the playlist store by the iter in the catalog. If the store is
 * not initialized, it will be initialzed first.
 *
 * Returns: (transfer none): The playlist store, NULL if the iter is invalid.
 */

GtkTreeModel *rc_ui_list_model_get_playlist_store(GtkTreeIter *iter)
{
    RCLibDbCatalogIter *seq_iter;
    gpointer store = NULL;
    if(iter==NULL || iter->user_data==NULL) return NULL;
    if(catalog_model==NULL)
        if(!rc_ui_list_model_init()) return NULL;
    seq_iter = iter->user_data;
    rclib_db_catalog_data_iter_get(seq_iter,
        RCLIB_DB_CATALOG_DATA_TYPE_STORE, &store,
        RCLIB_DB_CATALOG_DATA_TYPE_NONE);
    return GTK_TREE_MODEL(store);
}


/**
 * rc_ui_list_model_get_catalog_by_model:
 * @model: the playlist store model
 *
 * Get the catalog iter by given playlist store model.
 *
 * Returns: (transfer none): (skip): The catalog iter.
 */

RCLibDbCatalogIter *rc_ui_list_model_get_catalog_by_model(GtkTreeModel *model)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    if(model==NULL) return NULL;
    if(!RC_UI_IS_PLAYLIST_STORE(model)) return NULL;
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE(store)->priv;
    if(priv==NULL) return NULL;
    return priv->catalog_iter;
}

/**
 * rc_ui_list_model_set_playlist_title_format:
 * @format: the format string
 *
 * Set the format string of the title column in the playlist, using
 * \%TITLE as title string, \%ARTIST as artist string, \%ALBUM as album string.
 * Notice that \%TITLE should be always included in the string.
 */

void rc_ui_list_model_set_playlist_title_format(const gchar *format)
{
    if(format==NULL || g_strstr_len(format, -1, "%TITLE")==NULL)
        return;
    g_free(format_string);
    format_string = g_strdup(format);
}

/**
 * rc_ui_list_model_get_playlist_title_format:
 *
 * Get the format string of the title column in the playlist.
 *
 * Returns: The format string, do not free or modify it.
 */

const gchar *rc_ui_list_model_get_playlist_title_format()
{
    return format_string;
}

