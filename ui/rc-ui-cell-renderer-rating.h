/*
 * RhythmCat UI Cell Renderer Rating Widget Declaration
 * The rating cell renderer widget used in list view.
 *
 * rc-ui-cell-renderer-rating.h
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

#ifndef HAVE_RC_UI_CELL_RENDERER_RATING_H
#define HAVE_RC_UI_CELL_RENDERER_RATING_H

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RC_UI_TYPE_CELL_RENDERER_RATING ( \
    rc_ui_cell_renderer_rating_get_type())
#define RC_UI_CELL_RENDERER_RATING(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_CELL_RENDERER_RATING, RCUiCellRendererRating))
#define RC_UI_CELL_RENDERER_RATING_CLASS(klass) ( \
    G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_TYPE_CELL_RENDERER_RATING, \
    RCUiCellRendererRatingClass))
#define RC_UI_IS_CELL_RENDERER_RATING(obj) ( \
    G_TYPE_CHECK_INSTANCE_TYPE((obj), RC_UI_TYPE_CELL_RENDERER_RATING))
#define RC_UI_IS_CELL_RENDERER_RATING_CLASS(klass) ( \
    G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_CELL_RENDERER_RATING))
#define RC_UI_CELL_RENDERER_RATING_GET_CLASS(obj) ( \
    G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_CELL_RENDERER_RATING, \
    RCUiCellRendererRatingClass))

typedef struct _RCUiCellRendererRating RCUiCellRendererRating;
typedef struct _RCUiCellRendererRatingClass RCUiCellRendererRatingClass;
typedef struct _RCUiCellRendererRatingPrivate RCUiCellRendererRatingPrivate;

struct _RCUiCellRendererRating {
    /*< private >*/
    GtkCellRenderer parent;
    RCUiCellRendererRatingPrivate *priv;
};

struct _RCUiCellRendererRatingClass {
    /*< private >*/
    GtkCellRendererClass parent_class;

    void (*rated)(RCUiCellRendererRating *renderer, const char *path,
        gfloat rating);
};

/*< private >*/
GType rc_ui_cell_renderer_rating_get_type();

/*< public >*/
GtkCellRenderer *rc_ui_cell_renderer_rating_new();

G_END_DECLS

#endif

