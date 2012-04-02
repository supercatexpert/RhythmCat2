/*
 * RhythmCat Library Album Image Backend Module
 * Process album image files and data.
 *
 * rclib-album.c
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

#include "rclib-album.h"
#include "rclib-common.h"
#include "rclib-core.h"
#include "rclib-db.h"
#include "rclib-util.h"
#include "rclib-marshal.h"

/**
 * SECTION: rclib-album
 * @Short_description: The album image processor
 * @Title: RCLibAlbum
 * @Include: rclib-album.h
 *
 * The #RCLibAlbum is a class which processes the album image. It can read
 * album image from image files, or get the data from the metadata, and then
 * send signals so that anyone who connected to the class can get the data.
 */

#define RCLIB_ALBUM_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RCLIB_ALBUM_TYPE, RCLibAlbumPrivate)

typedef struct RCLibAlbumPrivate
{
    RCLibAlbumType type;
    gpointer album_data;
    gulong tag_found_handler;
    gulong uri_changed_handler;
}RCLibAlbumPrivate;

enum
{
    SIGNAL_ALBUM_FOUND,
    SIGNAL_ALBUM_NONE,
    SIGNAL_LAST
};

static GObject *album_instance = NULL;
static gpointer rclib_album_parent_class = NULL;
static gint album_signals[SIGNAL_LAST] = {0};

static void rclib_album_tag_found_cb(RCLibCore *core,
    const RCLibCoreMetadata *metadata, const gchar *uri, gpointer data)
{
    gchar *filename;
    gboolean flag = TRUE;
    RCLibAlbumPrivate *priv = (RCLibAlbumPrivate *)data;
    if(data==NULL) return;
    if(priv->album_data!=NULL) return; 
    filename = rclib_util_search_cover(uri, metadata->title,
        metadata->artist, metadata->album);
    if(filename!=NULL && g_file_test(filename, G_FILE_TEST_EXISTS))
    {
        g_signal_emit(album_instance, album_signals[SIGNAL_ALBUM_FOUND], 0,
            RCLIB_ALBUM_TYPE_FILENAME, filename, &flag);
        if(flag)
        {
            priv->type = RCLIB_ALBUM_TYPE_FILENAME;
            priv->album_data = (gpointer)g_strdup(filename);
        }
    }
    else if(metadata->image!=NULL)
    {
        g_signal_emit(album_instance, album_signals[SIGNAL_ALBUM_FOUND], 0,
            RCLIB_ALBUM_TYPE_BUFFER, metadata->image, &flag);
        if(flag)
        {
            priv->type = RCLIB_ALBUM_TYPE_BUFFER;
            priv->album_data = gst_buffer_ref(metadata->image);
        }
    }
}

static void rclib_album_uri_changed_cb(RCLibCore *core, const gchar *uri,
    gpointer data)
{
    GSequenceIter *reference;
    gchar *filename = NULL;
    gboolean flag = TRUE;
    RCLibAlbumPrivate *priv = (RCLibAlbumPrivate *)data;
    if(data==NULL) return;
    if(priv->album_data!=NULL)
    {
        if(priv->type==RCLIB_ALBUM_TYPE_BUFFER)
            gst_buffer_unref((GstBuffer *)priv->album_data);
        else
            g_free(priv->album_data);
        priv->album_data = NULL;
    }
    reference = rclib_core_get_db_reference();
    if(reference!=NULL)
        filename = g_strdup(rclib_db_playlist_get_album_bind(reference));
    if(filename==NULL)
        filename = rclib_util_search_cover(uri, NULL, NULL, NULL);
    if(filename!=NULL && g_file_test(filename, G_FILE_TEST_EXISTS))
    {
        g_signal_emit(album_instance, album_signals[SIGNAL_ALBUM_FOUND], 0,
            RCLIB_ALBUM_TYPE_FILENAME, filename, &flag);
        if(flag)
        {
            priv->type = RCLIB_ALBUM_TYPE_FILENAME;
            priv->album_data = (gpointer)g_strdup(filename);
        }
    }
    else
        g_signal_emit(album_instance, album_signals[SIGNAL_ALBUM_NONE], 0);
    g_free(filename);
}

static void rclib_album_finalize(GObject *object)
{
    RCLibAlbumPrivate *priv = RCLIB_ALBUM_GET_PRIVATE(RCLIB_ALBUM(object));
    if(priv->tag_found_handler>0)
        rclib_core_signal_disconnect(priv->tag_found_handler);
    if(priv->uri_changed_handler>0)
        rclib_core_signal_disconnect(priv->uri_changed_handler);
    G_OBJECT_CLASS(rclib_album_parent_class)->finalize(object);
}

static GObject *rclib_album_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(album_instance!=NULL) return album_instance;
    retval = G_OBJECT_CLASS(rclib_album_parent_class)->constructor
        (type, n_construct_params, construct_params);
    album_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&album_instance);
    return retval;
}

static void rclib_album_class_init(RCLibAlbumClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_album_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_album_finalize;
    object_class->constructor = rclib_album_constructor;
    g_type_class_add_private(klass, sizeof(RCLibAlbumPrivate));
    
    /**
     * RCLibAlbum::album-found:
     * @album: the #RCLibAlbum that received the signal
     * @type: the data type
     * @album_data: the album data (filename or #GstBuffer)
     *
     * The ::album-found signal is emitted when the album file or data
     * is found.
     *
     * Returns: Whether the album data can be used, if not, the album data
     * will not be saved in the player.
     */
    album_signals[SIGNAL_ALBUM_FOUND] = g_signal_new("album-found",
        RCLIB_ALBUM_TYPE, G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(RCLibAlbumClass, album_found), NULL, NULL,
        rclib_marshal_BOOLEAN__UINT_POINTER, G_TYPE_BOOLEAN, 2, G_TYPE_UINT,
        G_TYPE_POINTER, NULL);

    /**
     * RCLibAlbum::album-none:
     * @album: the #RCLibAlbum that received the signal
     *
     * The ::album-found signal is emitted if no album image file or data
     * is found.
     */
    album_signals[SIGNAL_ALBUM_NONE] = g_signal_new("album-none",
        RCLIB_ALBUM_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibAlbumClass, album_none), NULL, NULL,
        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE, NULL);
}

static void rclib_album_instance_init(RCLibAlbum *album)
{
    RCLibAlbumPrivate *priv = RCLIB_ALBUM_GET_PRIVATE(album);
    bzero(priv, sizeof(RCLibAlbumPrivate));
    priv->tag_found_handler = rclib_core_signal_connect("tag-found",
        G_CALLBACK(rclib_album_tag_found_cb), priv);
    priv->uri_changed_handler = rclib_core_signal_connect("uri-changed",
        G_CALLBACK(rclib_album_uri_changed_cb), priv);
}

GType rclib_album_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo album_info = {
        .class_size = sizeof(RCLibAlbumClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_album_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibAlbum),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_album_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibAlbum"), &album_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_album_init:
 *
 * Initialize the album process instance.
 */

void rclib_album_init()
{
    g_message("Loading album image processor....");
    album_instance = g_object_new(RCLIB_ALBUM_TYPE, NULL);
    g_message("Album processor loaded.");
}

/**
 * rclib_album_exit:
 *
 * Unload the album process instance.
 */

void rclib_album_exit()
{
    if(album_instance!=NULL) g_object_unref(album_instance);
    album_instance = NULL;
    g_message("Album processor exited.");
}

/**
 * rclib_album_get_instance:
 *
 * Get the running #RCLibAlbum instance.
 *
 * Returns: The running instance.
 */

GObject *rclib_album_get_instance()
{
    return album_instance;
}

/**
 * rclib_album_signal_connect:
 * @name: the name of the signal
 * @callback: the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibAlbum object.
 *
 * Returns: The handler ID.
 */

gulong rclib_album_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(album_instance==NULL) return 0;
    return g_signal_connect(album_instance, name, callback, data);
}

/**
 * rclib_album_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running #RCLibAlbum instance so it
 * will not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_album_signal_disconnect(gulong handler_id)
{
    if(album_instance==NULL) return;
    g_signal_handler_disconnect(album_instance, handler_id);
}

/**
 * rclib_album_get_album_data:
 * @type: (out) (allow-none) the album data type
 * @data: (out) (allow-none) the album data
 *
 * Get the album data.
 *
 * Returns: Whether the album data is set.
 */

gboolean rclib_album_get_album_data(RCLibAlbumType *type, gpointer *data)
{
    RCLibAlbumPrivate *priv;
    if(album_instance==NULL) return FALSE;
    priv = RCLIB_ALBUM_GET_PRIVATE(album_instance);
    if(priv==NULL) return FALSE;
    if(priv->album_data==NULL) return FALSE;
    if(type!=NULL) *type = priv->type;
    if(data!=NULL) *data = priv->album_data;
    return TRUE;
}

/**
 * rclib_album_save_file:
 * @filename: the filename for the new album image
 *
 * Save the album image to a new image file.
 *
 * Returns: Whether the operation is successful.
 */

gboolean rclib_album_save_file(const gchar *filename)
{
    RCLibAlbumPrivate *priv;
    GError *error = NULL;
    GstBuffer *buffer;
    GFile *file_src, *file_dst;
    gboolean flag;
    if(album_instance==NULL) return FALSE;
    priv = RCLIB_ALBUM_GET_PRIVATE(album_instance);
    if(priv==NULL) return FALSE;
    if(priv->album_data==NULL) return FALSE;
    if(priv->type==RCLIB_ALBUM_TYPE_BUFFER)
    {
        buffer = (GstBuffer *)priv->album_data;
        flag =g_file_set_contents(filename, (const gchar *)buffer->data,
            buffer->size, &error);
        if(!flag)
        {
            g_warning("Cannot save album data: %s", error->message);
            g_error_free(error);
        }
        return flag;
    }
    else if(priv->type==RCLIB_ALBUM_TYPE_FILENAME)
    {
        file_src = g_file_new_for_path((const gchar *)priv->album_data);
        file_dst = g_file_new_for_path((const gchar *)filename);
        if(file_src==NULL || file_dst==NULL)
        {
            if(file_src!=NULL) g_object_unref(file_src);
            if(file_dst!=NULL) g_object_unref(file_dst);
            return FALSE;
        }
        flag = g_file_copy(file_src, file_dst, G_FILE_COPY_OVERWRITE, NULL,
            NULL, NULL, &error);
        if(!flag)
        {
            g_warning("Cannot save album file: %s", error->message);
            g_error_free(error);
        }
        return flag;
    }
    return FALSE;
}


