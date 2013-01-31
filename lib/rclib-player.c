/*
 * RhythmCat Library Advanced Player Schedule Module
 * Schedule the player.
 *
 * rclib-player.c
 * This file is part of RhythmCat Library (LibRhythmCat)
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

#include "rclib-player.h"
#include "rclib-common.h"
#include "rclib-db.h"
#include "rclib-core.h"

/**
 * SECTION: rclib-player
 * @Short_description: The player scheduler
 * @Title: RCLibPlayer
 * @Include: rclib-player.h
 *
 * The #RCLibPlayer is a class which schedules the player, like sequential
 * playing, repeat playing, and random playing. The playing mode can be set
 * easily by the given API.
 */

struct _RCLibPlayerPrivate
{
    RCLibPlayerRepeatMode repeat_mode;
    RCLibPlayerRandomMode random_mode;
    gulong eos_handler;
    gboolean limit_state;
    gfloat limit_rating;
    gboolean limit_condition;
};

enum
{
    SIGNAL_REPEAT_MODE_CHANGED,
    SIGNAL_RANDOM_MODE_CHANGED,
    SIGNAL_LAST
};

static GObject *player_instance = NULL;
static gpointer rclib_player_parent_class = NULL;
static gint player_signals[SIGNAL_LAST] = {0};

static inline void rclib_player_repeat_list(RCLibPlayerPrivate *priv)
{
    gpointer reference = NULL;
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    RCLibDbPlaylistIter *iter;
    RCLibDbPlaylistIter *foreach_iter;
    RCLibDbPlaylistIter *iter_new = NULL;
    gfloat rating;
    rclib_core_get_play_source(&source_type, &reference, NULL);
    if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
    {
        if(reference==NULL) return;
        if(!rclib_db_playlist_is_valid_iter((RCLibDbPlaylistIter *)reference))
            return;
        iter = (RCLibDbPlaylistIter *)reference;
        iter = rclib_db_playlist_iter_next(iter);
        if(priv->limit_state)
        {
            for(foreach_iter=iter;foreach_iter!=NULL;
                foreach_iter=rclib_db_playlist_iter_next(foreach_iter))
            {
                rating = -1.0;
                rclib_db_playlist_data_iter_get(foreach_iter,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_RATING, &rating,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                if(rating>-0.1)
                {
                    if(priv->limit_condition)
                    {
                        if(rating<=priv->limit_rating)
                        {
                            rclib_player_play_playlist(foreach_iter);
                            return;
                        }
                    }
                    else
                    {
                        if(rating>=priv->limit_rating)
                        {
                            rclib_player_play_playlist(foreach_iter);
                            return;
                        }
                    }
                }
            }
            for(foreach_iter=rclib_db_playlist_iter_get_begin_iter(
                (RCLibDbPlaylistIter *)reference);foreach_iter!=iter;
                foreach_iter=rclib_db_playlist_iter_next(foreach_iter))
            {
                rating = -1.0;
                rclib_db_playlist_data_iter_get(foreach_iter,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_RATING, &rating,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                if(rating>-0.1)
                {
                    if(priv->limit_condition)
                    {
                        if(rating<=priv->limit_rating)
                        {
                            iter_new = foreach_iter;
                            break;
                        }
                    }
                    else
                    {
                        if(rating>=priv->limit_rating)
                        {
                            iter_new = foreach_iter;
                            break;
                        }
                    }
                }
            }
            if(iter_new!=NULL)
                rclib_player_play_playlist(iter_new);
        }
        else
        {
            if(iter==NULL)
            {
                iter = rclib_db_playlist_iter_get_begin_iter(
                    (RCLibDbPlaylistIter *)reference);
            }
            rclib_player_play_playlist(iter);
        }
    }
}

static inline void rclib_player_play_next_internal(RCLibPlayerPrivate *priv,
    gboolean loop)
{
    gpointer reference = NULL;
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    gfloat rating;
    rclib_core_get_play_source(&source_type, &reference, NULL);
    if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
    {
        RCLibDbPlaylistIter *iter;
        RCLibDbCatalogIter *catalog_iter;
        RCLibDbCatalogIter *cforeach_iter;
        RCLibDbPlaylistIter *pforeach_iter;
        if(reference==NULL) return;
        iter = (RCLibDbPlaylistIter *)reference;
        if(priv->limit_state)
        {
            iter = rclib_db_playlist_iter_next(iter);
            for(pforeach_iter=iter;pforeach_iter!=NULL;
                pforeach_iter=rclib_db_playlist_iter_next(pforeach_iter))
            {
                rating = -1.0;
                rclib_db_playlist_data_iter_get(pforeach_iter,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_RATING, &rating,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                if(rating<-0.1) continue;
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                    {
                        rclib_player_play_playlist(pforeach_iter);
                        return;
                    }
                }
                else
                {
                    if(rating>=priv->limit_rating)
                    {
                        rclib_player_play_playlist(pforeach_iter);
                        return;
                    }
                }
            }
            catalog_iter = NULL;
            rclib_db_playlist_data_iter_get((RCLibDbPlaylistIter *)reference,
                RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            if(catalog_iter!=NULL)
                catalog_iter = rclib_db_catalog_iter_next(catalog_iter);
            for(cforeach_iter=catalog_iter;cforeach_iter!=NULL;
                cforeach_iter=rclib_db_catalog_iter_next(cforeach_iter))
            {
                for(iter = rclib_db_playlist_get_begin_iter(cforeach_iter);
                    iter!=NULL;iter=rclib_db_playlist_iter_next(iter))
                {
                    rating = -1.0;
                    rclib_db_playlist_data_iter_get(iter,
                        RCLIB_DB_PLAYLIST_DATA_TYPE_RATING, &rating,
                        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                    if(rating<-0.1) continue;
                    if(priv->limit_condition)
                    {
                        if(rating<=priv->limit_rating)
                        {
                            rclib_player_play_playlist(iter);
                            return;
                        }
                    }
                    else
                    {
                        if(rating>=priv->limit_rating)
                        {
                            rclib_player_play_playlist(iter);
                            return;
                        }
                    }
                }
            }
            for(cforeach_iter = rclib_db_catalog_get_begin_iter();
                cforeach_iter!=catalog_iter;
                cforeach_iter=rclib_db_catalog_iter_next(cforeach_iter))
            {
                for(iter = rclib_db_playlist_get_begin_iter(cforeach_iter);
                    iter!=NULL;iter=rclib_db_playlist_iter_next(iter))
                {
                    rating = -1.0;
                    rclib_db_playlist_data_iter_get(iter,
                        RCLIB_DB_PLAYLIST_DATA_TYPE_RATING, &rating,
                        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                    if(rating<-0.1) continue;
                    if(priv->limit_condition)
                    {
                        if(rating<=priv->limit_rating)
                        {
                            rclib_player_play_playlist(iter);
                            return;
                        }
                    }
                    else
                    {
                        if(rating>=priv->limit_rating)
                        {
                            rclib_player_play_playlist(iter);
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            iter = rclib_db_playlist_iter_next(iter);
            if(iter==NULL)
            {
                catalog_iter = NULL;
                rclib_db_playlist_data_iter_get(reference,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
                    RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
                if(catalog_iter!=NULL)
                {
                    do
                    {
                        catalog_iter = rclib_db_catalog_iter_next(
                            catalog_iter);
                        if(catalog_iter==NULL) break;
                    }
                    while(rclib_db_playlist_get_length(catalog_iter)==0);
                }
                if(catalog_iter==NULL)
                {
                    if(!loop) return;
                    catalog_iter = rclib_db_catalog_get_begin_iter();
                    while(catalog_iter!=NULL &&
                        rclib_db_playlist_get_length(catalog_iter)==0)
                    {
                        catalog_iter = rclib_db_catalog_iter_next(
                            catalog_iter);
                    }
                    if(catalog_iter==NULL)
                    {
                        catalog_iter = rclib_db_catalog_get_begin_iter();
                    }
                    iter = rclib_db_playlist_get_begin_iter(catalog_iter);
                    if(iter==NULL) return;
                    rclib_player_play_playlist(iter);
                    return;
                }
                iter = rclib_db_playlist_get_begin_iter(catalog_iter);
                if(iter==NULL) return;
                rclib_player_play_playlist(iter);
                return;
            }
            if(iter!=NULL)
                rclib_player_play_playlist(iter);
        }
    }
    else if(source_type==RCLIB_CORE_PLAY_SOURCE_LIBRARY)
    {
        RCLibDbLibraryQueryResultIter *iter, *lforeach_iter;
        GObject *library_query_result;
        RCLibDbLibraryData *library_data;
        gchar *uri;
        if(reference==NULL) return;
        library_query_result = rclib_db_library_get_album_query_result();
        iter = rclib_db_library_query_result_get_iter_by_uri(
            RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
            (const gchar *)reference);
        if(iter==NULL)
        {
            g_object_unref(library_query_result);
            return;
        }
        if(priv->limit_state)
        {
            iter = rclib_db_library_query_result_get_next_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), iter);
            for(lforeach_iter=iter;lforeach_iter!=NULL;
                lforeach_iter=rclib_db_library_query_result_get_next_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                lforeach_iter))
            {
                rating = -1.0;
                uri = NULL;
                library_data = rclib_db_library_query_result_get_data(
                    RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                    lforeach_iter);
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                    RCLIB_DB_LIBRARY_DATA_TYPE_RATING, &rating,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
                if(uri==NULL) continue;
                if(rating<-0.1)
                {
                    g_free(uri);
                    continue;
                }
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                    {
                        rclib_player_play_library(uri);
                        g_free(uri);
                        g_object_unref(library_query_result);
                        return;
                    }
                }
                else
                {
                    if(rating>=priv->limit_rating)
                    {
                        rclib_player_play_library(uri);
                        g_free(uri);
                        g_object_unref(library_query_result);
                        return;
                    }
                }
                g_free(uri);
            }
            if(!loop)
            {
                g_object_unref(library_query_result);
                return;
            }
            for(lforeach_iter=rclib_db_library_query_result_get_begin_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result));
                lforeach_iter!=NULL;
                lforeach_iter=rclib_db_library_query_result_get_next_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                lforeach_iter))
            {
                rating = -1.0;
                uri = NULL;
                library_data = rclib_db_library_query_result_get_data(
                    RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                    lforeach_iter);
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                    RCLIB_DB_LIBRARY_DATA_TYPE_RATING, &rating,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
                if(uri==NULL) continue;
                if(rating<-0.1)
                {
                    g_free(uri);
                    continue;
                }
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                    {
                        rclib_player_play_library(uri);
                        g_free(uri);
                        g_object_unref(library_query_result);
                        return;
                    }
                }
                else
                {
                    if(rating>=priv->limit_rating)
                    {
                        rclib_player_play_library(uri);
                        g_free(uri);
                        g_object_unref(library_query_result);
                        return;
                    }
                }
                g_free(uri);
            }
        }
        else
        {
            iter = rclib_db_library_query_result_get_next_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result), iter);
            for(lforeach_iter=iter;lforeach_iter!=NULL;
                lforeach_iter=rclib_db_library_query_result_get_next_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                lforeach_iter))
            {
                uri = NULL;
                library_data = rclib_db_library_query_result_get_data(
                    RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                    lforeach_iter);
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
                if(uri==NULL) continue;
                rclib_player_play_library(uri);
                g_free(uri);
                g_object_unref(library_query_result);
                return;
            }
            if(!loop)
            {
                g_object_unref(library_query_result);
                return;
            }
            for(lforeach_iter=rclib_db_library_query_result_get_begin_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result));
                lforeach_iter!=NULL;
                lforeach_iter=rclib_db_library_query_result_get_next_iter(
                RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                lforeach_iter))
            {
                uri = NULL;
                library_data = rclib_db_library_query_result_get_data(
                    RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                    lforeach_iter);
                rclib_db_library_data_get(library_data,
                    RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                rclib_db_library_data_unref(library_data);
                if(uri==NULL) continue;
                rclib_player_play_library(uri);
                g_free(uri);
                g_object_unref(library_query_result);
                return;
            }
        }
        g_object_unref(library_query_result);
    }
}

static void rclib_player_eos_cb(RCLibCore *core, gpointer data)
{
    RCLibDbPlaylistIter *iter;
    gpointer reference = NULL;
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    RCLibPlayer *player;
    RCLibPlayerPrivate *priv;
    if(data==NULL) return;
    player = RCLIB_PLAYER(data);
    priv = player->priv;
    rclib_core_get_play_source(&source_type, &reference, NULL);
    if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
    {
        if(priv->random_mode!=RCLIB_PLAYER_RANDOM_NONE)
        {
            switch(priv->random_mode)
            {
                case RCLIB_PLAYER_RANDOM_SINGLE:
                    iter = rclib_db_playlist_iter_get_random_iter(
                        (RCLibDbPlaylistIter *)reference, priv->limit_state,
                        priv->limit_condition, priv->limit_rating);
                    if(iter!=NULL) rclib_player_play_playlist(iter);
                    break;
                case RCLIB_PLAYER_RANDOM_ALL:
                    iter = rclib_db_playlist_iter_get_random_iter(NULL,
                        priv->limit_state, priv->limit_condition,
                        priv->limit_rating);
                    if(iter!=NULL) rclib_player_play_playlist(iter);
                        break;
                    break;
                default:
                    rclib_player_play_next_internal(priv, FALSE);
                    break;
            }
            return;
        }
        switch(priv->repeat_mode)
        {
            case RCLIB_PLAYER_REPEAT_SINGLE:
                if(reference!=NULL) rclib_player_play_playlist(reference);
                break;
            case RCLIB_PLAYER_REPEAT_LIST:
                rclib_player_repeat_list(priv);
                break;
            case RCLIB_PLAYER_REPEAT_ALL:
                rclib_player_play_next_internal(priv, TRUE);
                break;
            default:
                rclib_player_play_next_internal(priv, FALSE);
                break;
        }
    }
    else if(source_type==RCLIB_CORE_PLAY_SOURCE_LIBRARY)
    {
        if(priv->random_mode!=RCLIB_PLAYER_RANDOM_NONE)
        {
            switch(priv->random_mode)
            {
                /*
                 * No difference between the two random modes in library
                 * playing. So merge them.
                 */
                case RCLIB_PLAYER_RANDOM_SINGLE: 
                case RCLIB_PLAYER_RANDOM_ALL:
                    /* TBD */
                    break;
                default:
                    rclib_player_play_next_internal(priv, FALSE);
                    break;
            }
            return;
        }
        switch(priv->repeat_mode)
        {
            case RCLIB_PLAYER_REPEAT_SINGLE:
            {
                if(reference!=NULL)
                {
                    gchar *uri = NULL;
                    GObject *library_query_result;
                    RCLibDbLibraryData *library_data;
                    if(reference==NULL) break;
                    library_query_result =
                        rclib_db_library_get_album_query_result();
                    if(library_query_result==NULL) break;
                    library_data = rclib_db_library_query_result_get_data(
                        RCLIB_DB_LIBRARY_QUERY_RESULT(library_query_result),
                        (RCLibDbLibraryQueryResultIter *)reference);
                    g_object_unref(library_query_result);
                    if(library_data==NULL) break;
                    rclib_db_library_data_get(library_data,
                        RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                        RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                    rclib_db_library_data_unref(library_data);
                    rclib_player_play_library(uri);
                    g_free(uri);
                }
                break;
            }
            
            /*
             * No difference between the two repeat modes in library
             * playing. So merge them.
             */
            case RCLIB_PLAYER_REPEAT_LIST:
            case RCLIB_PLAYER_REPEAT_ALL:
                rclib_player_play_next_internal(priv, TRUE);
                break;
            default:
                rclib_player_play_next_internal(priv, FALSE);
                break;
        }  
    }
}

static void rclib_player_finalize(GObject *object)
{
    RCLibPlayerPrivate *priv = RCLIB_PLAYER(object)->priv;
    RCLIB_PLAYER(object)->priv = NULL;
    rclib_core_signal_disconnect(priv->eos_handler);
    G_OBJECT_CLASS(rclib_player_parent_class)->finalize(object);
}

static GObject *rclib_player_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(player_instance!=NULL) return player_instance;
    retval = G_OBJECT_CLASS(rclib_player_parent_class)->constructor
        (type, n_construct_params, construct_params);
    player_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&player_instance);
    return retval;
}

static void rclib_player_class_init(RCLibPlayerClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_player_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_player_finalize;
    object_class->constructor = rclib_player_constructor;
    g_type_class_add_private(klass, sizeof(RCLibPlayerPrivate));
    
    /**
     * RCLibPlayer::repeat-mode-changed:
     * @player: the #RCLibPlayer that received the signal
     * @mode: the new repeat mode
     * 
     * The ::repeat-mode-changed signal is emitted when a new repeat playing
     * mode has been set.
     */
    player_signals[SIGNAL_REPEAT_MODE_CHANGED] = g_signal_new(
        "repeat-mode-changed", RCLIB_TYPE_PLAYER, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPlayerClass, repeat_mode_changed), NULL, NULL,
        g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT, NULL);
        
    /**
     * RCLibPlayer::random-mode-changed:
     * @player: the #RCLibPlayer that received the signal
     * @mode: the new random mode
     * 
     * The ::random-mode-changed signal is emitted when a new random playing
     * mode has been set.
     */
    player_signals[SIGNAL_RANDOM_MODE_CHANGED] = g_signal_new(
        "random-mode-changed", RCLIB_TYPE_PLAYER, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPlayerClass, random_mode_changed), NULL, NULL,
        g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT, NULL);
}

static void rclib_player_instance_init(RCLibPlayer *player)
{
    RCLibPlayerPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE(player,
        RCLIB_TYPE_PLAYER, RCLibPlayerPrivate);
    player->priv = priv;
    priv->repeat_mode = RCLIB_PLAYER_REPEAT_NONE;
    priv->random_mode = RCLIB_PLAYER_RANDOM_NONE;
    priv->eos_handler = rclib_core_signal_connect("eos",
        G_CALLBACK(rclib_player_eos_cb), player);
}

GType rclib_player_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo player_info = {
        .class_size = sizeof(RCLibPlayerClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_player_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibPlayer),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_player_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibPlayer"), &player_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_player_init:
 *
 * Initialize the player scheduler.
 */

void rclib_player_init()
{
    g_message("Loading player scheduler....");
    if(player_instance!=NULL)
    {
        g_warning("Player scheduler is already loaded!");
        return;
    }
    player_instance = g_object_new(RCLIB_TYPE_PLAYER, NULL);
    g_message("Player scheduler loaded.");
}

/**
 * rclib_player_exit:
 *
 * Unload the running player scheduler instance.
 */

void rclib_player_exit()
{
    if(player_instance!=NULL) g_object_unref(player_instance);
    player_instance = NULL;
    g_message("Player scheduler exited.");
}

/**
 * rclib_player_get_instance:
 *
 * Get the running #RCLibPlayer instance.
 *
 * Returns: (transfer none): The running instance.
 */

GObject *rclib_player_get_instance()
{
    return player_instance;
}

/**
 * rclib_player_signal_connect:
 * @name: the name of the signal
 * @callback: (scope call): the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibPlayer object.
 *
 * Returns: The handler ID.
 */

gulong rclib_player_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(player_instance==NULL) return 0;
    return g_signal_connect(player_instance, name, callback, data);
}

/**
 * rclib_player_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running #RCLibPlayer instance so it
 * will not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_player_signal_disconnect(gulong handler_id)
{
    if(player_instance==NULL) return;
    g_signal_handler_disconnect(player_instance, handler_id);
}

/**
 * rclib_player_play_playlist:
 * @iter: the iter to the playlist
 *
 * Play the iter to the playlist in the music database.
 */

void rclib_player_play_playlist(gpointer iter)
{
    RCLibDbPlaylistIter *playlist_iter;
    gchar *uri = NULL;
    if(iter==NULL) return;
    playlist_iter = (RCLibDbPlaylistIter *)iter;
    if(!rclib_db_playlist_is_valid_iter(playlist_iter)) return;
    rclib_db_playlist_data_iter_get((RCLibDbPlaylistIter *)iter,
        RCLIB_DB_PLAYLIST_DATA_TYPE_URI, &uri,
        RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
    if(uri==NULL) return;
    rclib_core_set_uri_with_play_source(uri, RCLIB_CORE_PLAY_SOURCE_PLAYLIST,
        iter, NULL, NULL);
    g_free(uri);
    rclib_core_play();
}

/**
 * rclib_player_play_library:
 * @uri: the URI of the music item in the library
 *
 * Play the URI of the music in the music library.
 */

void rclib_player_play_library(const gchar *uri)
{
    if(uri==NULL) return;
    rclib_core_set_uri_with_play_source(uri, RCLIB_CORE_PLAY_SOURCE_LIBRARY,
        g_strdup(uri), g_free, NULL);
    rclib_core_play();
}

/**
 * rclib_player_play_prev:
 * @jump: whether the player should be jump to the previous playlist
 *   if the playing music is the first one in current playlist
 * @repeat: whether repeat playing the current music if there is no
 *   previous one
 * @loop: whether the player should be jump to the last playlist
 *   if the playing one is the first
 *
 * Play the previous music.
 *
 * Returns: Whether the player is set to play.
 */

gboolean rclib_player_play_prev(gboolean jump, gboolean repeat,
    gboolean loop)
{
    gpointer reference = NULL;
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    RCLibDbPlaylistIter *iter;
    RCLibDbCatalogIter *catalog_iter, *catalog_prev_iter;
    gint playlist_length;
    rclib_core_get_play_source(&source_type, &reference, NULL);
    if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
    {
        if(reference==NULL) return FALSE;
        if(!rclib_db_playlist_is_valid_iter((RCLibDbPlaylistIter *)reference))
            return FALSE;
        iter = rclib_db_playlist_iter_prev((RCLibDbPlaylistIter *)reference);
        if(iter==NULL)
        {
            if(!jump)
            {
                if(repeat)
                    rclib_player_play_playlist(reference);
                return TRUE;
            }
            catalog_iter = NULL;
            rclib_db_playlist_data_iter_get((RCLibDbPlaylistIter *)reference,
                RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            if(catalog_iter==NULL) return FALSE;
            catalog_prev_iter = rclib_db_catalog_iter_prev(catalog_iter);
            if(catalog_prev_iter==NULL)
            {
                if(!loop)
                {
                    if(repeat)
                        rclib_player_play_playlist(iter);
                    return TRUE;
                }
                catalog_iter = rclib_db_catalog_get_last_iter();
                playlist_length = rclib_db_playlist_get_length(catalog_iter);
                while(playlist_length==0)
                {
                    catalog_prev_iter = rclib_db_catalog_iter_prev(
                        catalog_iter);
                    if(catalog_prev_iter==NULL) break;
                    catalog_iter = catalog_prev_iter;
                    playlist_length = rclib_db_playlist_get_length(
                        catalog_iter);
                }
                if(playlist_length<0) return FALSE;
                iter = rclib_db_playlist_get_last_iter(catalog_iter);
                if(iter!=NULL)
                    rclib_player_play_playlist(iter);
                return TRUE;
            }
            do
            {
                catalog_iter = rclib_db_catalog_iter_prev(catalog_iter);
                if(catalog_iter==NULL)
                    catalog_iter = rclib_db_catalog_get_last_iter();
            }
            while(catalog_iter!=NULL &&
                rclib_db_playlist_get_length(catalog_iter)==0);
            iter = rclib_db_playlist_get_last_iter(catalog_iter);
            if(iter==NULL) return FALSE;
            rclib_player_play_playlist(iter);
            return TRUE;
        }
        else
            rclib_player_play_playlist(iter);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * rclib_player_play_next:
 * @jump: whether the player should be jump to the next playlist
 *   if the playing music is the last one in current playlist
 * @repeat: whether repeat playing the current music if there is no
 *   next one
 * @loop: whether the player should be jump to the first playlist
 *   if the playing one is the last
 *
 * Play the next music.
 *
 * Returns: Whether the player is set to play.
 */

gboolean rclib_player_play_next(gboolean jump, gboolean repeat,
    gboolean loop)
{
    gpointer reference = NULL;
    RCLibCorePlaySource source_type = RCLIB_CORE_PLAY_SOURCE_NONE;
    RCLibDbPlaylistIter *iter;
    RCLibDbCatalogIter *catalog_iter;
    rclib_core_get_play_source(&source_type, &reference, NULL);
    if(source_type==RCLIB_CORE_PLAY_SOURCE_PLAYLIST)
    {
        if(reference==NULL) return FALSE;
        if(!rclib_db_playlist_is_valid_iter((RCLibDbPlaylistIter *)reference))
            return FALSE;
        iter = rclib_db_playlist_iter_next((RCLibDbPlaylistIter *)reference);
        if(iter==NULL)
        {
            if(!jump)
            {
                if(repeat)
                    rclib_player_play_playlist(reference);
                return TRUE;
            }
            catalog_iter = NULL;
            rclib_db_playlist_data_iter_get(iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG, &catalog_iter,
                RCLIB_DB_PLAYLIST_DATA_TYPE_NONE);
            if(catalog_iter==NULL) return FALSE;
            catalog_iter = rclib_db_catalog_iter_next(catalog_iter);
            if(catalog_iter==NULL)
            {
                if(!loop)
                {
                    if(repeat)
                        rclib_player_play_playlist(reference);
                    return TRUE;
                }
                catalog_iter = rclib_db_catalog_get_begin_iter();               
                while(catalog_iter!=NULL &&
                    rclib_db_playlist_get_length(catalog_iter)==0)
                {
                    catalog_iter = rclib_db_catalog_iter_next(catalog_iter);
                }
                if(catalog_iter==NULL)
                {
                    catalog_iter = rclib_db_catalog_get_begin_iter();
                }
                iter = rclib_db_playlist_get_begin_iter(catalog_iter);
                if(iter!=NULL)
                    rclib_player_play_playlist(iter);
                return TRUE;
            }
            iter = rclib_db_playlist_get_begin_iter(catalog_iter);
            if(iter==NULL) return FALSE;
            rclib_player_play_playlist(iter);
            return TRUE;
        }
        rclib_player_play_playlist(iter);
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * rclib_player_set_repeat_mode:
 * @mode: the new repeat mode to set
 *
 * Set the repeat mode for the player.
 */

void rclib_player_set_repeat_mode(RCLibPlayerRepeatMode mode)
{
    RCLibPlayerPrivate *priv;
    if(player_instance==NULL) return;
    priv = RCLIB_PLAYER(player_instance)->priv;
    if(priv==NULL) return;
    priv->repeat_mode = mode;
    g_signal_emit(player_instance,
        player_signals[SIGNAL_REPEAT_MODE_CHANGED], 0, mode);
}

/**
 * rclib_player_set_random_mode:
 * @mode: the new random mode to set
 *
 * Set the random mode for the player.
 */

void rclib_player_set_random_mode(RCLibPlayerRandomMode mode)
{
    RCLibPlayerPrivate *priv;
    if(player_instance==NULL) return;
    priv = RCLIB_PLAYER(player_instance)->priv;
    if(priv==NULL) return;
    priv->random_mode = mode;
    g_signal_emit(player_instance,
        player_signals[SIGNAL_RANDOM_MODE_CHANGED], 0, mode);
}

/**
 * rclib_player_get_repeat_mode:
 *
 * Get the repeat mode using now in the player.
 *
 * Returns: The repeat mode.
 */

RCLibPlayerRepeatMode rclib_player_get_repeat_mode()
{
    RCLibPlayerPrivate *priv;
    if(player_instance==NULL) return 0;
    priv = RCLIB_PLAYER(player_instance)->priv;
    if(priv==NULL) return 0;
    return priv->repeat_mode;
}

/**
 * rclib_player_get_random_mode:
 *
 * Get the random mode using now in the player.
 *
 * Returns: The random mode.
 */

RCLibPlayerRandomMode rclib_player_get_random_mode()
{
    RCLibPlayerPrivate *priv;
    if(player_instance==NULL) return 0;
    priv = RCLIB_PLAYER(player_instance)->priv;
    if(priv==NULL) return 0;
    return priv->random_mode;
}

/**
 * rclib_player_set_rating_limit:
 * @state: enable or disable the rating limit playing mode
 * @rating: the rating limit value
 * @condition: the condition
 *
 * Set the rating limit playing mode. if @condition is set to #TRUE,
 * the player will play the music whose rating is less or equal than
 * the rating limit value, if it is #FALSE, the player will play the
 * the music whose rating is more or equal than the rating limit.
 */

void rclib_player_set_rating_limit(gboolean state, gfloat rating,
    gboolean condition)
{
    RCLibPlayerPrivate *priv;
    if(player_instance==NULL) return;
    priv = RCLIB_PLAYER(player_instance)->priv;
    if(priv==NULL) return;
    priv->limit_state = state;
    if(rating>5.0) rating = 5.0;
    if(rating<0.0) rating = 0.0;
    priv->limit_rating = rating;
    priv->limit_condition = condition;
}

/**
 * rclib_player_get_rating_limit:
 * @rating: (out) (allow-none): the rating value to return
 * @condition: (out) (allow-none): the condition to return
 *
 * Get the rating limit playing mode configuration data.
 *
 * Returns: Whether rating limit playing mode is enabled.
 */

gboolean rclib_player_get_rating_limit(gfloat *rating, gboolean *condition)
{
    RCLibPlayerPrivate *priv;
    if(player_instance==NULL) return FALSE;
    priv = RCLIB_PLAYER(player_instance)->priv;
    if(priv==NULL) return FALSE;
    if(rating!=NULL) *rating = priv->limit_rating;
    if(condition!=NULL) *condition = priv->limit_condition;
    return priv->limit_state;
}







