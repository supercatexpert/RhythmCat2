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

#define RCLIB_PLAYER_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RCLIB_TYPE_PLAYER, RCLibPlayerPrivate)

typedef struct RCLibPlayerPrivate
{
    RCLibPlayerRepeatMode repeat_mode;
    RCLibPlayerRandomMode random_mode;
    gulong eos_handler;
    gboolean limit_state;
    gfloat limit_rating;
    gboolean limit_condition;
}RCLibPlayerPrivate;

enum
{
    SIGNAL_REPEAT_MODE_CHANGED,
    SIGNAL_RANDOM_MODE_CHANGED,
    SIGNAL_LAST
};

static GObject *player_instance = NULL;
static gpointer rclib_player_parent_class = NULL;
static gint player_signals[SIGNAL_LAST] = {0};

static inline void rclib_player_random_single(RCLibPlayerPrivate *priv)
{
    GSequenceIter *iter;
    GSequence *seq;
    gint pos, length;
    GSequenceIter *foreach_iter;
    GPtrArray *iter_array;
    gfloat rating;
    iter = rclib_core_get_db_reference();
    if(iter==NULL) return;
    seq = g_sequence_iter_get_sequence(iter);
    if(seq==NULL) return;
    if(priv->limit_state)
    {
        iter_array = g_ptr_array_new();
        for(foreach_iter=g_sequence_get_begin_iter(seq);
            !g_sequence_iter_is_end(foreach_iter);
            foreach_iter=g_sequence_iter_next(foreach_iter))
        {
            if(rclib_db_playlist_get_rating(foreach_iter, &rating))
            {
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                        g_ptr_array_add(iter_array, foreach_iter);
                }
                else
                {
                    if(rating>=priv->limit_rating)
                        g_ptr_array_add(iter_array, foreach_iter);
                }
            }
        }
        if(iter_array->len==0)
        {
            rclib_player_play_db(iter);
            g_ptr_array_free(iter_array, TRUE);
            return;
        }
        pos = g_random_int_range(0, iter_array->len);
        iter = g_ptr_array_index(iter_array, pos);
        g_ptr_array_free(iter_array, TRUE);
        if(iter==NULL) return;
        rclib_player_play_db(iter);
    }
    else
    {
        length = g_sequence_get_length(seq);
        if(length<=0)
        {
            rclib_player_play_db(iter);
            return;
        }
        pos = g_random_int_range(0, length);
        iter = g_sequence_get_iter_at_pos(seq, pos);
        if(iter==NULL) return;
        rclib_player_play_db(iter);
    }
}

static inline void rclib_player_random_all_play(RCLibPlayerPrivate *priv)
{
    GSequence *catalog = NULL;
    GSequenceIter *catalog_iter = NULL, *playlist_iter = NULL;
    gint total_length = 0;
    gint pos, length;
    gfloat rating;
    RCLibDbCatalogData *catalog_data = NULL;
    catalog = rclib_db_get_catalog();
    if(catalog==NULL) return;
    for(catalog_iter = g_sequence_get_begin_iter(catalog);
        !g_sequence_iter_is_end(catalog_iter);
        catalog_iter = g_sequence_iter_next(catalog_iter))
    {
        catalog_data = g_sequence_get(catalog_iter);
        if(catalog_data==NULL) continue;
        if(priv->limit_state)
        {
            for(playlist_iter = g_sequence_get_begin_iter(
                catalog_data->playlist);
                !g_sequence_iter_is_end(playlist_iter);
                playlist_iter = g_sequence_iter_next(playlist_iter))
            {
                if(!rclib_db_playlist_get_rating(playlist_iter, &rating))
                    continue;
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                        total_length++;
                }
                else
                {
                    if(rating>=priv->limit_rating)
                        total_length++;
                }
            }
        }
        else
            total_length += g_sequence_get_length(catalog_data->playlist);
    }
    if(total_length<=0) return;
    pos = g_random_int_range(0, total_length);
    if(priv->limit_state) pos++;
    for(catalog_iter = g_sequence_get_begin_iter(catalog);
        !g_sequence_iter_is_end(catalog_iter);
        catalog_iter = g_sequence_iter_next(catalog_iter))
    {
        catalog_data = g_sequence_get(catalog_iter);
        if(catalog_data==NULL) continue;
        if(priv->limit_state)
        {
            playlist_iter = g_sequence_get_begin_iter(
                catalog_data->playlist);
            for(;!g_sequence_iter_is_end(playlist_iter);
                playlist_iter = g_sequence_iter_next(playlist_iter))
            {
                if(!rclib_db_playlist_get_rating(playlist_iter, &rating))
                    continue;
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                        pos--;
                }
                else
                {
                    if(rating>=priv->limit_rating)
                        pos--;
                }
                if(pos<=0) break;
            }
            if(pos<=0) break;
        }
        else
        {
            length = g_sequence_get_length(catalog_data->playlist);
            if(length==0) continue;
            if(pos>=length)
                pos-=length;
            else
            {
                if(catalog_data==NULL) return;
                playlist_iter = g_sequence_get_iter_at_pos(
                    catalog_data->playlist, pos);
                break;
            }
        }
    }
    if(playlist_iter==NULL) return;
    rclib_player_play_db(playlist_iter);
}

static inline void rclib_player_repeat_list(RCLibPlayerPrivate *priv)
{
    GSequenceIter *iter;
    GSequence *seq;
    GSequenceIter *foreach_iter;
    GSequenceIter *iter_new = NULL;
    gfloat rating;
    iter = rclib_core_get_db_reference();
    if(iter==NULL) return;
    seq = g_sequence_iter_get_sequence(iter);
    if(seq==NULL) return;
    iter = g_sequence_iter_next(iter);
    if(iter==NULL) return;
    if(priv->limit_state)
    {
        for(foreach_iter=iter;!g_sequence_iter_is_end(foreach_iter);
            foreach_iter=g_sequence_iter_next(foreach_iter))
        {
            if(rclib_db_playlist_get_rating(foreach_iter, &rating))
            {
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                    {
                        rclib_player_play_db(foreach_iter);
                        return;
                    }
                }
                else
                {
                    if(rating>=priv->limit_rating)
                    {
                        rclib_player_play_db(foreach_iter);
                        return;
                    }
                }
            }
        }
        for(foreach_iter=g_sequence_get_begin_iter(seq);foreach_iter!=iter;
            foreach_iter=g_sequence_iter_next(foreach_iter))
        {
            if(rclib_db_playlist_get_rating(foreach_iter, &rating))
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
            rclib_player_play_db(iter_new);
    }
    else
    {
        if(g_sequence_iter_is_end(iter))
            iter = g_sequence_get_begin_iter(seq);
        rclib_player_play_db(iter);
    }
}

static inline void rclib_player_play_next_internal(RCLibPlayerPrivate *priv,
    gboolean loop)
{
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *iter, *catalog_iter;
    GSequenceIter *foreach_iter;
    gfloat rating;
    iter = rclib_core_get_db_reference();
    if(iter==NULL) return;
    if(g_sequence_iter_is_end(iter)) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    if(priv->limit_state)
    {
        iter = g_sequence_iter_next(iter);
        for(foreach_iter=iter;!g_sequence_iter_is_end(foreach_iter);
            foreach_iter=g_sequence_iter_next(foreach_iter))
        {
            if(!rclib_db_playlist_get_rating(foreach_iter, &rating))
                continue;
            if(priv->limit_condition)
            {
                if(rating<=priv->limit_rating)
                {
                    rclib_player_play_db(foreach_iter);
                    return;
                }
            }
            else
            {
                if(rating>=priv->limit_rating)
                {
                    rclib_player_play_db(foreach_iter);
                    return;
                }
            }
        }
        catalog_iter = playlist_data->catalog;
        catalog_iter = g_sequence_iter_next(catalog_iter);
        for(foreach_iter=catalog_iter;!g_sequence_iter_is_end(foreach_iter);
            foreach_iter=g_sequence_iter_next(foreach_iter))
        {
            catalog_data = g_sequence_get(foreach_iter);
            iter = g_sequence_get_begin_iter(catalog_data->playlist);
            for(;!g_sequence_iter_is_end(iter);
                iter=g_sequence_iter_next(iter))
            {
                if(!rclib_db_playlist_get_rating(iter, &rating))
                    continue;
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                    {
                        rclib_player_play_db(iter);
                        return;
                    }
                }
                else
                {
                    if(rating>=priv->limit_rating)
                    {
                        rclib_player_play_db(iter);
                        return;
                    }
                }
            }
        }
        foreach_iter = g_sequence_get_begin_iter(rclib_db_get_catalog());
        for(;foreach_iter!=catalog_iter;
            foreach_iter=g_sequence_iter_next(foreach_iter))
        {
            catalog_data = g_sequence_get(foreach_iter);
            iter = g_sequence_get_begin_iter(catalog_data->playlist);
            for(;!g_sequence_iter_is_end(iter);
                iter=g_sequence_iter_next(iter))
            {
                if(!rclib_db_playlist_get_rating(iter, &rating))
                    continue;
                if(priv->limit_condition)
                {
                    if(rating<=priv->limit_rating)
                    {
                        rclib_player_play_db(iter);
                        return;
                    }
                }
                else
                {
                    if(rating>=priv->limit_rating)
                    {
                        rclib_player_play_db(iter);
                        return;
                    }
                }
            }
        }
    }
    else
    {
        iter = g_sequence_iter_next(iter);
        if(g_sequence_iter_is_end(iter))
        {
            catalog_iter = playlist_data->catalog;
            catalog_iter = g_sequence_iter_next(catalog_iter);
            if(g_sequence_iter_is_end(catalog_iter))
            {
                if(!loop) return;
                catalog_iter = g_sequence_get_begin_iter(
                    rclib_db_get_catalog());
                catalog_data = g_sequence_get(catalog_iter);
                if(catalog_data==NULL) return;           
                while(!g_sequence_iter_is_end(catalog_iter) &&
                    g_sequence_get_length(catalog_data->playlist)<=0)
                {
                    catalog_iter = g_sequence_iter_next(catalog_iter);
                    catalog_data = g_sequence_get(catalog_iter);
                    if(catalog_data==NULL) break;
                }
                if(g_sequence_iter_is_end(catalog_iter))
                {
                    catalog_iter = g_sequence_get_begin_iter(
                        rclib_db_get_catalog());
                    catalog_data = g_sequence_get(catalog_iter);
                }
                if(catalog_data==NULL) return;
                iter = g_sequence_get_begin_iter(catalog_data->playlist);
                rclib_player_play_db(iter);
                return;
            }
            catalog_data = g_sequence_get(catalog_iter);
            if(catalog_data==NULL) return;
            iter = g_sequence_get_begin_iter(catalog_data->playlist);
            playlist_data = g_sequence_get(iter);
            rclib_player_play_db(iter);
            return;
        }
        rclib_player_play_db(iter);
    }
}

static void rclib_player_eos_cb(RCLibCore *core, gpointer data)
{
    GSequenceIter *iter;
    RCLibPlayer *player;
    RCLibPlayerPrivate *priv;
    if(data==NULL) return;
    player = RCLIB_PLAYER(data);
    priv = RCLIB_PLAYER_GET_PRIVATE(player);
    if(priv->random_mode!=RCLIB_PLAYER_RANDOM_NONE)
    {
        switch(priv->random_mode)
        {
            case RCLIB_PLAYER_RANDOM_SINGLE:
                rclib_player_random_single(priv);
                break;
            case RCLIB_PLAYER_RANDOM_ALL:
                rclib_player_random_all_play(priv);
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
            iter = rclib_core_get_db_reference();
            if(iter!=NULL)
                rclib_player_play_db(iter);
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

static void rclib_player_finalize(GObject *object)
{
    RCLibPlayerPrivate *priv = RCLIB_PLAYER_GET_PRIVATE(RCLIB_PLAYER(object));
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
    RCLibPlayerPrivate *priv = RCLIB_PLAYER_GET_PRIVATE(player);
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
 * rclib_player_play_db:
 * @iter: the iter to the playlist
 *
 * Play the iter to the playlist in the music database.
 */

void rclib_player_play_db(GSequenceIter *iter)
{
    RCLibDbPlaylistData *playlist_data;
    if(iter==NULL) return;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return;
    rclib_core_set_uri_with_db_ref(playlist_data->uri, iter);
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
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *iter, *catalog_iter;
    iter = rclib_core_get_db_reference();
    if(iter==NULL) return FALSE;
    if(g_sequence_iter_is_begin(iter))
    {
        if(!jump)
        {
            if(repeat)
                rclib_player_play_db(iter);
            return TRUE;
        }
        playlist_data = g_sequence_get(iter);
        if(playlist_data==NULL) return FALSE;
        catalog_iter = playlist_data->catalog;
        if(g_sequence_iter_is_begin(catalog_iter))
        {
            if(!loop)
            {
                if(repeat)
                    rclib_player_play_db(iter);
                return TRUE;
            }
            catalog_iter = g_sequence_get_end_iter(rclib_db_get_catalog());
            do
            {
                catalog_iter = g_sequence_iter_prev(catalog_iter);
                catalog_data = g_sequence_get(catalog_iter);
                if(catalog_data==NULL) break;
            }
            while(!g_sequence_iter_is_begin(catalog_iter) &&
                g_sequence_get_length(catalog_data->playlist)<=0);
            if(catalog_data==NULL) return FALSE;
            iter = g_sequence_iter_prev(g_sequence_get_end_iter(
                catalog_data->playlist));
            rclib_player_play_db(iter);
            return TRUE;
        }
        do
        {
            if(g_sequence_iter_is_begin(catalog_iter))
                catalog_iter = g_sequence_get_end_iter(
                    rclib_db_get_catalog());
            catalog_iter = g_sequence_iter_prev(catalog_iter);
            catalog_data = g_sequence_get(catalog_iter);
            if(catalog_data==NULL) break;
        }
        while(g_sequence_get_length(catalog_data->playlist)<=0);
        if(catalog_data==NULL) return FALSE;
        iter = g_sequence_iter_prev(g_sequence_get_end_iter(
            catalog_data->playlist));
        playlist_data = g_sequence_get(iter);
        rclib_player_play_db(iter);
        return TRUE;
    }
    iter = g_sequence_iter_prev(iter);
    rclib_player_play_db(iter);
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
    RCLibDbCatalogData *catalog_data;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *iter, *catalog_iter;
    iter = rclib_core_get_db_reference();
    if(iter==NULL) return FALSE;
    playlist_data = g_sequence_get(iter);
    if(playlist_data==NULL) return FALSE;
    iter = g_sequence_iter_next(iter);
    if(g_sequence_iter_is_end(iter))
    {
        if(!jump)
        {
            if(repeat)
                rclib_player_play_db(rclib_core_get_db_reference());
            return TRUE;
        }
        catalog_iter = playlist_data->catalog;
        catalog_iter = g_sequence_iter_next(catalog_iter);
        if(g_sequence_iter_is_end(catalog_iter))
        {
            if(!loop)
            {
                if(repeat)
                    rclib_player_play_db(rclib_core_get_db_reference());
                return TRUE;
            }
            catalog_iter = g_sequence_get_begin_iter(
                rclib_db_get_catalog());
            catalog_data = g_sequence_get(catalog_iter);
            if(catalog_data==NULL) return FALSE;           
            while(!g_sequence_iter_is_end(catalog_iter) &&
                g_sequence_get_length(catalog_data->playlist)<=0)
            {
                catalog_iter = g_sequence_iter_next(catalog_iter);
                catalog_data = g_sequence_get(catalog_iter);
                if(catalog_data==NULL) break;
            }
            if(g_sequence_iter_is_end(catalog_iter))
            {
                catalog_iter = g_sequence_get_begin_iter(
                    rclib_db_get_catalog());
                catalog_data = g_sequence_get(catalog_iter);
            }
            if(catalog_data==NULL) return FALSE;
            iter = g_sequence_get_begin_iter(catalog_data->playlist);
            rclib_player_play_db(iter);
            return TRUE;
        }
        catalog_data = g_sequence_get(catalog_iter);
        if(catalog_data==NULL) return FALSE;
        iter = g_sequence_get_begin_iter(catalog_data->playlist);
        playlist_data = g_sequence_get(iter);
        rclib_player_play_db(iter);
        return TRUE;
    }
    rclib_player_play_db(iter);
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
    priv = RCLIB_PLAYER_GET_PRIVATE(player_instance);
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
    priv = RCLIB_PLAYER_GET_PRIVATE(player_instance);
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
    priv = RCLIB_PLAYER_GET_PRIVATE(player_instance);
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
    priv = RCLIB_PLAYER_GET_PRIVATE(player_instance);
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
    priv = RCLIB_PLAYER_GET_PRIVATE(player_instance);
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
    priv = RCLIB_PLAYER_GET_PRIVATE(player_instance);
    if(priv==NULL) return FALSE;
    if(rating!=NULL) *rating = priv->limit_rating;
    if(condition!=NULL) *condition = priv->limit_condition;
    return priv->limit_state;
}







