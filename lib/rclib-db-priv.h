/*
 * RhythmCat Library Music Database Private Header Declaration
 *
 * rclib-db-priv.h
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

#ifndef HAVE_RC_LIB_DB_PRIVATE_H
#define HAVE_RC_LIB_DB_PRIVATE_H

#include "rclib-tag.h"

#define RCLIB_DB_ERROR rclib_db_error_quark()

typedef struct RCLibDbPlaylistImportData
{
    GSequenceIter *iter;
    GSequenceIter *insert_iter;
    gchar *uri;
    gboolean play_flag;
}RCLibDbPlaylistImportData;

typedef struct RCLibDbPlaylistImportIdleData
{
    GSequenceIter *iter;
    GSequenceIter *insert_iter;
    RCLibTagMetadata *mmd;
    RCLibDbPlaylistType type;
    gboolean play_flag;
}RCLibDbPlaylistImportIdleData;

typedef struct RCLibDbPlaylistRefreshData
{
    GSequenceIter *catalog_iter;
    GSequenceIter *playlist_iter;
    gchar *uri;
}RCLibDbPlaylistRefreshData;

typedef struct RCLibDbPlaylistRefreshIdleData
{
    GSequenceIter *catalog_iter;
    GSequenceIter *playlist_iter;
    RCLibTagMetadata *mmd;
    RCLibDbPlaylistType type;
}RCLibDbPlaylistRefreshIdleData;

struct _RCLibDbPrivate
{
    gchar *filename;
    GSequence *catalog;
    GThread *import_thread;
    GThread *refresh_thread;
    GThread *autosave_thread;
    GAsyncQueue *import_queue;
    GAsyncQueue *refresh_queue;
    gboolean import_work_flag;
    gboolean refresh_work_flag;
    gboolean dirty_flag;
    gboolean work_flag;
    gulong autosave_timeout;
    GMutex autosave_mutex;
    GCond autosave_cond;
    GString *autosave_xml_data;
};

/*< private >*/
gboolean _rclib_db_instance_init_playlist(RCLibDb *db, RCLibDbPrivate *priv);

#endif

