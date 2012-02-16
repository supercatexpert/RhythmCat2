/*
 * RhythmCat Library Advanced Player Schedule Header Declaration
 *
 * rclib-player.h
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

#ifndef HAVE_RCLIB_PLAYER_H
#define HAVE_RCLIB_PLAYER_H

#include <glib.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define RCLIB_PLAYER_TYPE (rclib_player_get_type())
#define RCLIB_PLAYER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RCLIB_PLAYER_TYPE, RCLibPlayer))
#define RCLIB_PLAYER_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RCLIB_PLAYER_TYPE, RCLibPlayerClass))
#define RCLIB_IS_PLAYER(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RCLIB_PLAYER_TYPE))
#define RCLIB_IS_PLAYER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RCLIB_PLAYER_TYPE))
#define RCLIB_PLAYER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_PLAYER_TYPE, RCLibPlayerClass))

/**
 * RCLibPlayerRepeatMode:
 * @RCLIB_PLAYER_REPEAT_NONE: no repeat
 * @RCLIB_PLAYER_REPEAT_SINGLE: repeat playing current music
 * @RCLIB_PLAYER_REPEAT_LIST: repeat playing music in current playlist
 * @RCLIB_PLAYER_REPEAT_ALL: repeat playing music in all playlists
 *
 * The enum type for repeat playing mode type.
 */

typedef enum {
    RCLIB_PLAYER_REPEAT_NONE,
    RCLIB_PLAYER_REPEAT_SINGLE,
    RCLIB_PLAYER_REPEAT_LIST,
    RCLIB_PLAYER_REPEAT_ALL
}RCLibPlayerRepeatMode;

/**
 * RCLibPlayerRandomMode:
 * @RCLIB_PLAYER_RANDOM_NONE: no random playing 
 * @RCLIB_PLAYER_RANDOM_SINGLE: random playing in current playlist
 * @RCLIB_PlAYER_RANDOM_ALL: random playing in all playlists
 *
 * The enum type for random playing mode type.
 */

typedef enum {
    RCLIB_PLAYER_RANDOM_NONE,
    RCLIB_PLAYER_RANDOM_SINGLE,
    RCLIB_PlAYER_RANDOM_ALL
}RCLibPlayerRandomMode;

typedef struct _RCLibPlayer RCLibPlayer;
typedef struct _RCLibPlayerClass RCLibPlayerClass;

/**
 * RCLibPlayer:
 *
 * The advanced player scheduler. The contents of the #RCLibPlayer structure
 * are private and should only be accessed via the provided API.
 */

struct _RCLibPlayer {
    /*< private >*/
    GObject parent;
};

/**
 * RCLibPlayerClass:
 *
 * #RCLibPlayer class.
 */

struct _RCLibPlayerClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*repeat_mode_changed)(RCLibPlayer *player,
        RCLibPlayerRepeatMode mode);
    void (*random_mode_changed)(RCLibPlayer *player,
        RCLibPlayerRandomMode mode);
};

/*< private >*/
GType rclib_player_get_type();

/*< public >*/
void rclib_player_init();
void rclib_player_exit();
GObject *rclib_player_get_instance();
gulong rclib_player_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_player_signal_disconnect(gulong handler_id);
void rclib_player_play_db(GSequenceIter *iter);
void rclib_player_play_uri(const gchar *uri);
void rclib_player_play_prev(gboolean jump, gboolean repeat, gboolean loop);
void rclib_player_play_next(gboolean jump, gboolean repeat, gboolean loop);
void rclib_player_set_repeat_mode(RCLibPlayerRepeatMode mode);
void rclib_player_set_random_mode(RCLibPlayerRandomMode mode);
RCLibPlayerRepeatMode rclib_player_get_repeat_mode();
RCLibPlayerRandomMode rclib_player_get_random_mode();

G_END_DECLS

#endif

