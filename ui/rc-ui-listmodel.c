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

#define RC_UI_CATALOG_STORE_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), RC_UI_TYPE_CATALOG_STORE, \
    RCUiCatalogStorePrivate))

#define RC_UI_PLAYLIST_STORE_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), RC_UI_TYPE_PLAYLIST_STORE, \
    RCUiPlaylistStorePrivate))

typedef struct RCUiCatalogStorePrivate {
    GSequence *catalog;
    gint stamp;
    gint n_columns;
}RCUiCatalogStorePrivate; 

typedef struct RCUiPlaylistStorePrivate {
    GSequence *playlist;
    GSequenceIter *catalog_iter;
    gint stamp;
    gint n_columns;
    gchar *format;
}RCUiPlaylistStorePrivate;

static GtkTreeModel *catalog_model = NULL;
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
        case RC_UI_PLAYLIST_COLUMN_RATING:
        case RC_UI_PLAYLIST_COLUMN_YEAR:
            type = G_TYPE_INT;
            break;
        case RC_UI_PLAYLIST_COLUMN_STATE:
        case RC_UI_PLAYLIST_COLUMN_FTITLE:
        case RC_UI_PLAYLIST_COLUMN_TITLE:
        case RC_UI_PLAYLIST_COLUMN_ARTIST:
        case RC_UI_PLAYLIST_COLUMN_ALBUM:
        case RC_UI_PLAYLIST_COLUMN_FTYPE:
        case RC_UI_PLAYLIST_COLUMN_LENGTH:
            type = G_TYPE_STRING;
            break;
        case RC_UI_PLAYLIST_COLUMN_PLAYING_FLAG:
            type = G_TYPE_BOOLEAN;
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
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    i = gtk_tree_path_get_indices(path)[0];
    if(i>=g_sequence_get_length(priv->catalog))
        return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = g_sequence_get_iter_at_pos(priv->catalog, i);
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
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    i = gtk_tree_path_get_indices(path)[0];
    if(i>=g_sequence_get_length(priv->playlist))
        return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = g_sequence_get_iter_at_pos(priv->playlist, i);
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
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, NULL);
    g_return_val_if_fail(iter->stamp==priv->stamp, NULL);
    if(g_sequence_iter_is_end(iter->user_data)) return NULL;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path,
        g_sequence_iter_get_position(iter->user_data));
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
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, NULL);
    g_return_val_if_fail(iter->stamp==priv->stamp, NULL);
    if(g_sequence_iter_is_end(iter->user_data)) return NULL;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path,
        g_sequence_iter_get_position(iter->user_data));
    return path;
}

static void rc_ui_catalog_store_get_value(GtkTreeModel *model,
    GtkTreeIter *iter, gint column, GValue *value)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    RCLibDbCatalogData *cata_data;
    GSequenceIter *seq_iter, *reference_iter;
    GstState state;
    RCLibDbPlaylistData *reference = NULL;
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(model));
    g_return_if_fail(iter!=NULL);
    store = RC_UI_CATALOG_STORE(model);
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    g_return_if_fail(priv!=NULL);
    g_return_if_fail(column<priv->n_columns);
    seq_iter = iter->user_data;
    cata_data = g_sequence_get(seq_iter);
    g_return_if_fail(cata_data!=NULL);
    switch(column)
    {
        case RC_UI_CATALOG_COLUMN_TYPE:
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, cata_data->type);
            break;
        case RC_UI_CATALOG_COLUMN_STATE:
            g_value_init(value, G_TYPE_STRING);
            reference_iter = rclib_core_get_db_reference();
            if(reference_iter!=NULL)
                reference = g_sequence_get(reference_iter);
            if(reference_iter==NULL || reference==NULL ||
                reference->catalog!=seq_iter)
            {
                g_value_set_string(value, NULL);
                break;
            }
            if(!rclib_core_get_state(&state, NULL, 0))
            {
                g_value_set_string(value, NULL);
                break;
            }
            switch(state)
            {
                case GST_STATE_PLAYING:
                    g_value_set_string(value, GTK_STOCK_MEDIA_PLAY);
                    break;
                case GST_STATE_PAUSED:
                    g_value_set_string(value, GTK_STOCK_MEDIA_PAUSE);
                    break;
                default:
                    g_value_set_string(value, NULL);
            }
            break;
        case RC_UI_CATALOG_COLUMN_NAME:
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, cata_data->name);
            break;
        case RC_UI_CATALOG_COLUMN_PLAYING_FLAG:
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, FALSE);
            reference_iter = rclib_core_get_db_reference();
            if(reference_iter!=NULL)
                reference = g_sequence_get(reference_iter);
            if(reference_iter==NULL || reference==NULL ||
                reference->catalog!=seq_iter)
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
        default:
            break;
    }
}

static void rc_ui_playlist_store_get_value(GtkTreeModel *model,
    GtkTreeIter *iter, gint column, GValue *value)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    RCLibDbPlaylistData *list_data;
    GSequenceIter *seq_iter, *reference;
    gchar *string;
    gint sec, min;
    gsize i, len;
    GstState state;
    gchar *rtitle = NULL;
    gchar *rartist, *ralbum;
    GString *ftitle;
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(model));
    g_return_if_fail(iter!=NULL);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    g_return_if_fail(priv!=NULL);
    g_return_if_fail(column<priv->n_columns);
    seq_iter = iter->user_data;
    list_data = g_sequence_get(seq_iter);
    g_return_if_fail(list_data!=NULL);
    switch(column)
    {
        case RC_UI_PLAYLIST_COLUMN_TYPE:
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, list_data->type);
            break;
        case RC_UI_PLAYLIST_COLUMN_STATE:
            g_value_init(value, G_TYPE_STRING);
            if(list_data->type==RCLIB_DB_PLAYLIST_TYPE_MISSING)
            {
                g_value_set_string(value, GTK_STOCK_CANCEL);
                break;
            }
            reference = rclib_core_get_db_reference();
            if(reference!=seq_iter)
            {
                g_value_set_string(value, NULL);
                break;
            }
            if(!rclib_core_get_state(&state, NULL, 0))
            {
                g_value_set_string(value, NULL);
                break;
            }
            switch(state)
            {
                case GST_STATE_PLAYING:
                    g_value_set_string(value, GTK_STOCK_MEDIA_PLAY);
                    break;
                case GST_STATE_PAUSED:
                    g_value_set_string(value, GTK_STOCK_MEDIA_PAUSE);
                    break;
                default:
                    g_value_set_string(value, NULL);
            }
            break;
        case RC_UI_PLAYLIST_COLUMN_FTITLE:
            if(list_data->title!=NULL && strlen(list_data->title)>0)
                rtitle = g_strdup(list_data->title);
            else
            {
                if(list_data->uri!=NULL)
                    rtitle = rclib_tag_get_name_from_uri(list_data->uri);
            }
            if(rtitle==NULL) rtitle = g_strdup(_("Unknown Title"));
            if(list_data->artist!=NULL && strlen(list_data->artist)>0)
                rartist = g_strdup(list_data->artist);
            else
                rartist = g_strdup(_("Unknown Artist"));
            if(list_data->album!=NULL && strlen(list_data->album)>0)
                ralbum = g_strdup(list_data->album);
            else
                ralbum = g_strdup(_("Unknown Album"));
            len = strlen(priv->format);
            ftitle = g_string_new(NULL);
            for(i=0;i<len;i++)
            {
                if(priv->format[i]!='%')
                    g_string_append_c(ftitle, priv->format[i]);
                else
                {
                    if(strncmp(priv->format+i, "%TITLE", 6)==0)
                    {
                        g_string_append(ftitle, rtitle);
                        i+=5;
                    }
                    else if(strncmp(priv->format+i, "%ARTIST", 7)==0)
                    {
                        g_string_append(ftitle, rartist);
                        i+=6;
                    }
                    else if(strncmp(priv->format+i, "%ALBUM", 6)==0)
                    {
                        g_string_append(ftitle, ralbum);
                        i+=5;
                    }
                    else
                        g_string_append_c(ftitle, priv->format[i]);
                }
            }
            g_free(rtitle);
            g_free(rartist);
            g_free(ralbum);
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, ftitle->str);
            g_string_free(ftitle, TRUE);
            break;
        case RC_UI_PLAYLIST_COLUMN_TITLE:
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, list_data->title);
            break;
        case RC_UI_PLAYLIST_COLUMN_ARTIST:
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, list_data->artist);
            break;
        case RC_UI_PLAYLIST_COLUMN_ALBUM:
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, list_data->album);
            break;
        case RC_UI_PLAYLIST_COLUMN_FTYPE:
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, list_data->ftype);
            break;
        case RC_UI_PLAYLIST_COLUMN_LENGTH:
            sec = (gint)(list_data->length / GST_SECOND);
            min = sec / 60;
            sec = sec % 60;
            string = g_strdup_printf("%02d:%02d", min, sec);
            g_value_init(value, G_TYPE_STRING);
            g_value_set_string(value, string);
            g_free(string);
            break;
        case RC_UI_PLAYLIST_COLUMN_TRACK:
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, list_data->tracknum);
            break;
        case RC_UI_PLAYLIST_COLUMN_YEAR:
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, list_data->year);
            break;
        case RC_UI_PLAYLIST_COLUMN_RATING:
            g_value_init(value, G_TYPE_INT);
            g_value_set_int(value, list_data->rating);
            break;
        case RC_UI_PLAYLIST_COLUMN_PLAYING_FLAG:
            g_value_init(value, G_TYPE_BOOLEAN);
            g_value_set_boolean(value, FALSE);
            if(list_data->type==RCLIB_DB_PLAYLIST_TYPE_MISSING) break;
            reference = rclib_core_get_db_reference();
            if(reference!=seq_iter) break;
            if(!rclib_core_get_state(&state, NULL, 0))
                break;
            if(state==GST_STATE_PLAYING || state==GST_STATE_PAUSED)
                g_value_set_boolean(value, TRUE);
            break;
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
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = g_sequence_iter_next(iter->user_data);
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    if(g_sequence_iter_is_end(iter->user_data))
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
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    iter->user_data = g_sequence_iter_next(iter->user_data);
    iter->user_data2 = NULL;
    iter->user_data3 = NULL;
    if(g_sequence_iter_is_end(iter->user_data))
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
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    if(g_sequence_iter_is_begin(iter->user_data))
    {
        iter->stamp = 0;
        return FALSE;
    }
    iter->user_data = g_sequence_iter_prev(iter->user_data);
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
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    g_return_val_if_fail(priv->stamp==iter->stamp, FALSE);
    if(g_sequence_iter_is_begin(iter->user_data))
    {
        iter->stamp = 0;
        return FALSE;
    }
    iter->user_data = g_sequence_iter_prev(iter->user_data);
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
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    if(g_sequence_get_length(priv->catalog)>0)
    {
        iter->stamp = priv->stamp;
        iter->user_data = g_sequence_get_begin_iter(priv->catalog);
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
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL)
    {
        iter->stamp = 0;
        return FALSE;
    }
    if(g_sequence_get_length(priv->playlist)>0)
    {
        iter->stamp = priv->stamp;
        iter->user_data = g_sequence_get_begin_iter(priv->playlist);
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
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(model);
    g_return_val_if_fail(priv!=NULL, -1);
    if(iter==NULL)
        return g_sequence_get_length(priv->catalog);
    g_return_val_if_fail(priv->stamp==iter->stamp, -1);
    return 0;
}

static gint rc_ui_playlist_store_iter_n_children(GtkTreeModel *model,
    GtkTreeIter *iter)
{
    RCUiPlaylistStorePrivate *priv;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), -1);
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(model);
    g_return_val_if_fail(priv!=NULL, -1);
    if(iter==NULL)
        return g_sequence_get_length(priv->playlist);
    g_return_val_if_fail(priv->stamp==iter->stamp, -1);
    return 0;
}

static gboolean rc_ui_catalog_store_iter_nth_child (GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
    RCUiCatalogStore *store;
    RCUiCatalogStorePrivate *priv;
    GSequenceIter *child;
    g_return_val_if_fail(RC_UI_IS_CATALOG_STORE(model), FALSE);
    store = RC_UI_CATALOG_STORE(model);
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL) return FALSE;
    child = g_sequence_get_iter_at_pos(priv->catalog, n);
    if(g_sequence_iter_is_end(child)) return FALSE;
    iter->stamp = priv->stamp;
    iter->user_data = child;
    return TRUE;
}

static gboolean rc_ui_playlist_store_iter_nth_child (GtkTreeModel *model,
    GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    GSequenceIter *child;
    g_return_val_if_fail(RC_UI_IS_PLAYLIST_STORE(model), FALSE);
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(parent!=NULL) return FALSE;
    child = g_sequence_get_iter_at_pos(priv->playlist, n);
    if(g_sequence_iter_is_end(child)) return FALSE;
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
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(object));
    G_OBJECT_CLASS(rc_ui_catalog_store_parent_class)->finalize(object);
}

static void rc_ui_playlist_store_finalize(GObject *object)
{
    RCUiPlaylistStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(object));
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(object);
    g_free(priv->format);
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
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(store);
    g_return_if_fail(priv!=NULL);
    priv->catalog = NULL;
	priv->stamp = g_random_int();
    priv->n_columns = RC_UI_CATALOG_COLUMN_LAST;
}

static void rc_ui_playlist_store_init(RCUiPlaylistStore *store)
{
    RCUiPlaylistStorePrivate *priv;
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(store));
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    g_return_if_fail(priv!=NULL);
	priv->stamp = g_random_int();
    priv->playlist = NULL;
    priv->n_columns = RC_UI_PLAYLIST_COLUMN_LAST;
    priv->format = g_strdup("%TITLE");
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
    GSequenceIter *iter, gpointer data)
{
    RCUiCatalogStorePrivate *priv;
    RCUiPlaylistStorePrivate *playlist_priv;
    RCLibDbCatalogData *catalog_data;
    GtkTreeModel *playlist_model;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(catalog_model));
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(catalog_model);
    g_return_if_fail(priv!=NULL);
    catalog_data = g_sequence_get(iter);
    playlist_model = GTK_TREE_MODEL(g_object_new(
        RC_UI_TYPE_PLAYLIST_STORE, NULL));
    playlist_priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(playlist_model);
    playlist_priv->playlist = (GSequence *)catalog_data->playlist;
    playlist_priv->catalog_iter = iter;
    catalog_data->store = playlist_model;
    pos = g_sequence_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_inserted(catalog_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_catalog_changed_cb(RCLibDb *db,
    GSequenceIter *iter, gpointer data)
{
    RCUiCatalogStorePrivate *priv;
    GtkTreePath *path;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(catalog_model));
    priv = RC_UI_CATALOG_STORE_GET_PRIVATE(catalog_model);
    g_return_if_fail(priv!=NULL);
    pos = g_sequence_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_changed(catalog_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_catalog_delete_cb(RCLibDb *db,
    GSequenceIter *iter, gpointer data)
{
    RCLibDbCatalogData *catalog_data;
    GtkTreePath *path;
    gint pos;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(RC_UI_IS_CATALOG_STORE(catalog_model));
    catalog_data = g_sequence_get(iter);
    g_object_unref(G_OBJECT(catalog_data->store));
    pos = g_sequence_iter_get_position(iter);
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
    GSequenceIter *iter, gpointer data)
{
    RCUiPlaylistStorePrivate *priv;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GtkTreePath *path;
    GtkTreeModel *playlist_model;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    playlist_data = g_sequence_get(iter);
    g_return_if_fail(playlist_data!=NULL);
    catalog_data = g_sequence_get(playlist_data->catalog);
    playlist_model = GTK_TREE_MODEL(catalog_data->store);
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(playlist_model));
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(playlist_model);
    g_return_if_fail(priv!=NULL);
    pos = g_sequence_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_inserted(playlist_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_playlist_changed_cb(RCLibDb *db,
    GSequenceIter *iter, gpointer data)
{
    RCUiPlaylistStorePrivate *priv;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GtkTreePath *path;
    GtkTreeModel *playlist_model;
    GtkTreeIter tree_iter;
    gint pos;
    g_return_if_fail(iter!=NULL);
    playlist_data = g_sequence_get(iter);
    g_return_if_fail(playlist_data!=NULL);
    catalog_data = g_sequence_get(playlist_data->catalog);
    playlist_model = GTK_TREE_MODEL(catalog_data->store);
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(playlist_model));
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(playlist_model);
    g_return_if_fail(priv!=NULL);
    pos = g_sequence_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    tree_iter.user_data = iter;
    tree_iter.stamp = priv->stamp;
    gtk_tree_model_row_changed(playlist_model, path, &tree_iter);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_playlist_delete_cb(RCLibDb *db,
    GSequenceIter *iter, gpointer data)
{
    RCUiPlaylistStorePrivate *priv;
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GtkTreePath *path;
    GtkTreeModel *playlist_model;
    gint pos;
    g_return_if_fail(iter!=NULL);
    playlist_data = g_sequence_get(iter);
    g_return_if_fail(playlist_data!=NULL);
    catalog_data = g_sequence_get(playlist_data->catalog);
    playlist_model = GTK_TREE_MODEL(catalog_data->store);
    g_return_if_fail(RC_UI_IS_PLAYLIST_STORE(playlist_model));
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(playlist_model);
    g_return_if_fail(priv!=NULL);
    pos = g_sequence_iter_get_position(iter);
    path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, pos);
    gtk_tree_model_row_deleted(playlist_model, path);
    gtk_tree_path_free(path);
}

static void rc_ui_list_model_playlist_reordered_cb(RCLibDb *db,
    GSequenceIter *iter, gint *new_order, gpointer data)
{
    RCLibDbCatalogData *catalog_data;
    GtkTreePath *path;
    GtkTreeModel *playlist_model;
    g_return_if_fail(iter!=NULL);
    g_return_if_fail(new_order!=NULL);
    catalog_data = g_sequence_get(iter);
    g_return_if_fail(catalog_data!=NULL);
    playlist_model = GTK_TREE_MODEL(catalog_data->store);
    g_return_if_fail(playlist_model!=NULL);
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(playlist_model, path, NULL, new_order);
    gtk_tree_path_free(path);
}

/**
 * rc_ui_list_model_init:
 * 
 * Initialize the list model (catalog model & playlist model).
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rc_ui_list_model_init()
{
    RCUiCatalogStorePrivate *catalog_priv;
    RCUiPlaylistStorePrivate *playlist_priv;
    RCLibDbCatalogData *catalog_data;
    GSequence *catalog_seq;
    GSequenceIter *catalog_iter;
    GtkTreeModel *playlist_model;
    if(catalog_model!=NULL) return FALSE;
    catalog_seq = rclib_db_get_catalog();
    if(catalog_seq==NULL)
    {
        g_warning("Cannot load catalog from music database!");
        return FALSE;
    }
    catalog_model = GTK_TREE_MODEL(g_object_new(
        RC_UI_TYPE_CATALOG_STORE, NULL));
    catalog_priv = RC_UI_CATALOG_STORE_GET_PRIVATE(catalog_model);
    catalog_priv->catalog = catalog_seq;
    for(catalog_iter = g_sequence_get_begin_iter(catalog_seq);
        !g_sequence_iter_is_end(catalog_iter);
        catalog_iter = g_sequence_iter_next(catalog_iter))
    {
        playlist_model = GTK_TREE_MODEL(g_object_new(
            RC_UI_TYPE_PLAYLIST_STORE, NULL));
        playlist_priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(playlist_model);
        catalog_data = g_sequence_get(catalog_iter);
        catalog_data->store = playlist_model;
        playlist_priv->playlist = catalog_data->playlist;
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
 * rc_ui_list_model_exit:
 * 
 * Unload the list model.
 */

void rc_ui_list_model_exit()
{
    g_object_unref(catalog_model);
}

/**
 * rc_ui_list_model_get_catalog_store:
 * 
 * Get the catalog store.
 *
 * Returns: The catalog store.
 */

GtkTreeModel *rc_ui_list_model_get_catalog_store()
{
    return catalog_model;
}

/**
 * rc_ui_list_model_get_playlist_store:
 * @iter: the iter for the playlist in the catalog
 *
 * Get the playlist store by the iter in the catalog.
 *
 * Returns: The playlist store, NULL if the iter is invalid.
 */

GtkTreeModel *rc_ui_list_model_get_playlist_store(GtkTreeIter *iter)
{
    RCLibDbCatalogData *catalog_data;
    GSequenceIter *seq_iter;
    if(iter==NULL || iter->user_data==NULL) return NULL;
    seq_iter = iter->user_data;
    catalog_data = g_sequence_get(seq_iter);
    if(catalog_data==NULL) return NULL;
    return GTK_TREE_MODEL(catalog_data->store);
}


/**
 * rc_ui_list_model_get_catalog_by_model:
 * @model: the playlist store model
 *
 * Get the catalog iter by given playlist store model.
 *
 * Returns: The catalog iter.
 */

GSequenceIter *rc_ui_list_model_get_catalog_by_model(GtkTreeModel *model)
{
    RCUiPlaylistStore *store;
    RCUiPlaylistStorePrivate *priv;
    if(model==NULL) return NULL;
    if(!RC_UI_IS_PLAYLIST_STORE(model)) return NULL;
    store = RC_UI_PLAYLIST_STORE(model);
    priv = RC_UI_PLAYLIST_STORE_GET_PRIVATE(store);
    if(priv==NULL) return NULL;
    return priv->catalog_iter;
}


