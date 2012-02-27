/*
 * RhythmCat Library Album Image Backend Header Declaration
 *
 * rclib-album.h
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

#ifndef HAVE_RCLIB_ALBUM_H
#define HAVE_RCLIB_ALBUM_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define RCLIB_ALBUM_TYPE (rclib_album_get_type())
#define RCLIB_ALBUM(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RCLIB_ALBUM_TYPE, RCLibAlbum))
#define RCLIB_ALBUM_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RCLIB_ALBUM_TYPE, RCLibAlbumClass))
#define RCLIB_IS_ALBUM(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RCLIB_ALBUM_TYPE))
#define RCLIB_IS_ALBUM_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RCLIB_ALBUM_TYPE))
#define RCLIB_ALBUM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_ALBUM_TYPE, RCLibAlbumClass))

/**
 * RCLibAlbumType:
 * @RCLIB_ALBUM_TYPE_FILENAME: the data is a filename (string)
 * @RCLIB_ALBUM_TYPE_BUFFER: the data is in #GstBuffer type
 *
 * The enum type for the album data type.
 */

typedef enum {
    RCLIB_ALBUM_TYPE_FILENAME,
    RCLIB_ALBUM_TYPE_BUFFER
}RCLibAlbumType;

typedef struct _RCLibAlbum RCLibAlbum;
typedef struct _RCLibAlbumClass RCLibAlbumClass;

/**
 * RCLibAlbum:
 *
 * The album processor. The contents of the #RCLibAlbum structure are
 * private and should only be accessed via the provided API.
 */

struct _RCLibAlbum {
    /*< private >*/
    GObject parent;
};

/**
 * RCLibAlbumClass:
 *
 * #RCLibAlbum class.
 */

struct _RCLibAlbumClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*album_found)(RCLibAlbum *album, guint type, gpointer album_data);
    void (*album_none)(RCLibAlbum *album);
};

/*< private >*/
GType rclib_album_get_type();

/*< public >*/
void rclib_album_init();
void rclib_album_exit();
GObject *rclib_album_get_instance();
gulong rclib_album_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_album_signal_disconnect(gulong handler_id);
gboolean rclib_album_get_album_data(RCLibAlbumType *type, gpointer *data);
gboolean rclib_album_save_file(const gchar *filename); //Not completed!

G_END_DECLS

#endif

