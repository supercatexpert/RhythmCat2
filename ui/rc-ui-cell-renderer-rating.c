/*
 * RhythmCat UI Cell Renderer Rating Widget Module
 * The rating cell renderer widget used in list view.
 *
 * rc-ui-cell-renderer-rating.c
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
 
#include "rc-ui-cell-renderer-rating.h"
#include "rc-ui-unset-star-inline.h"
#include "rc-ui-no-star-inline.h"
#include "rc-ui-set-star-inline.h"
#include "rc-marshal.h"

/**
 * SECTION: rc-ui-cell-renderer-rating
 * @Short_description: The rating cell renderer widget.
 * @Title: RCUiCellRendererRating
 * @Include: rc-ui-cell-renderer-rating.h
 *
 * The rating cell renderer widget used in list view.
 */

#define RC_UI_CELL_RENDERER_RATING_GET_PRIVATE(obj)  \
    G_TYPE_INSTANCE_GET_PRIVATE((obj), RC_UI_TYPE_CELL_RENDERER_RATING, \
    RCUiCellRendererRatingPrivate)

typedef struct RCUiCellRendererRatingPrivate
{
    gfloat rating;
    GdkPixbuf *star_pixbuf;
    GdkPixbuf *dot_pixbuf;
    GdkPixbuf *blank_pixbuf;
}RCUiCellRendererRatingPrivate;

enum
{
    PROP_O,
    PROP_RATING
};

enum
{
    SIGNAL_RATED,
    SIGNAL_LAST
};

static gpointer rc_ui_cell_renderer_rating_parent_class = NULL;
static gint ui_cell_renderer_rating_signals[SIGNAL_LAST] = {0};

/* The image colorized function from Rhythmbox. */
static GdkPixbuf *rc_ui_rating_create_colorized_pixbuf(GdkPixbuf *src,
    gint red_value, gint green_value, gint blue_value)
{
    gint i, j;
    gint width, height, has_alpha, src_row_stride, dst_row_stride;
    guchar *target_pixels;
    guchar *original_pixels;
    guchar *pixsrc;
    guchar *pixdest;
    GdkPixbuf *dest;
    g_return_val_if_fail(gdk_pixbuf_get_colorspace(src)==GDK_COLORSPACE_RGB,
        NULL);
    g_return_val_if_fail((!gdk_pixbuf_get_has_alpha(src)
        && gdk_pixbuf_get_n_channels(src)== 3) ||
        (gdk_pixbuf_get_has_alpha(src) &&
        gdk_pixbuf_get_n_channels(src)==4), NULL);
    g_return_val_if_fail(gdk_pixbuf_get_bits_per_sample(src)==8, NULL);
    dest = gdk_pixbuf_new(gdk_pixbuf_get_colorspace(src),
        gdk_pixbuf_get_has_alpha(src), gdk_pixbuf_get_bits_per_sample(src),
        gdk_pixbuf_get_width(src), gdk_pixbuf_get_height(src));

    has_alpha = gdk_pixbuf_get_has_alpha(src);
    width = gdk_pixbuf_get_width(src);
    height = gdk_pixbuf_get_height(src);
    src_row_stride = gdk_pixbuf_get_rowstride(src);
    dst_row_stride = gdk_pixbuf_get_rowstride(dest);
    target_pixels = gdk_pixbuf_get_pixels(dest);
    original_pixels = gdk_pixbuf_get_pixels(src);

    for(i=0;i<height;i++)
    {
        pixdest = target_pixels + i*dst_row_stride;
        pixsrc = original_pixels + i*src_row_stride;
        for(j=0;j<width;j++)
        {
            *pixdest++ = (*pixsrc++ * red_value) >> 8;
            *pixdest++ = (*pixsrc++ * green_value) >> 8;
            *pixdest++ = (*pixsrc++ * blue_value) >> 8;
            if (has_alpha)
                *pixdest++ = *pixsrc++;
        }
    }
    return dest;
}

/* The rating rendering function from Rhythmbox. */
static gboolean rc_ui_rating_render_stars(GtkWidget *widget, cairo_t *cr,
    RCUiCellRendererRatingPrivate *priv, gint x, gint y, gint x_offset,
    gint y_offset, gboolean selected)
{
    gint i, icon_width;
    gboolean rtl;
    GdkPixbuf *buf;
    GtkStateType state;
    gint star_offset;
    gint offset;
    GdkRGBA color;
    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(priv!= NULL, FALSE);

    rtl = (gtk_widget_get_direction(widget)==GTK_TEXT_DIR_RTL);
    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &icon_width, NULL);

    for(i=0;i<5;i++)
    {
        if(selected)
        {
            offset = 0;
            if(gtk_widget_has_focus(widget))
                state = GTK_STATE_SELECTED;
            else
                state = GTK_STATE_ACTIVE;
        }
        else
        {
            offset = 120;
            if(gtk_widget_get_state(widget)==GTK_STATE_INSENSITIVE)
                state = GTK_STATE_INSENSITIVE;
            else
                state = GTK_STATE_NORMAL;
        }
        if(i<priv->rating)
            buf = priv->star_pixbuf;
        else if(i>=priv->rating && i<5)
            buf = priv->dot_pixbuf;
        else
            buf = priv->blank_pixbuf;
        if(buf==NULL)
        {
            return FALSE;
        }
        gtk_style_context_get_color(gtk_widget_get_style_context(widget),
            state, &color);
        buf = rc_ui_rating_create_colorized_pixbuf(buf,
            ((guint16)(color.red * G_MAXUINT16) + offset) >> 8,
            ((guint16)(color.green * G_MAXUINT16) + offset) >> 8,
            ((guint16)(color.blue * G_MAXUINT16) + offset) >> 8);
        if (buf == NULL)
            return FALSE;

        if(rtl)
            star_offset = (5 - i - 1) * icon_width;
        else
            star_offset = i * icon_width;
        gdk_cairo_set_source_pixbuf(cr, buf, x_offset + star_offset,
            y_offset);
        cairo_paint(cr);
        g_object_unref(buf);
    }
    return TRUE;
}

static gdouble rc_ui_rating_get_rating_from_widget(GtkWidget *widget,
    gint widget_x, gint widget_width, gdouble current_rating)
{
    gint icon_width;
    gdouble rating = -1.0;
    gboolean rtl;
    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &icon_width, NULL);

    if(widget_x>=0 && widget_x<=widget_width)
    {
        rating = (int) (widget_x / icon_width) + 1;
        rtl =(gtk_widget_get_direction(widget)==GTK_TEXT_DIR_RTL);
        if(rtl)
        {
            rating = 5 - rating + 1;
        }
        if(rating<0)
            rating = 0;
        if(rating>5)
            rating = 5;
        if(rating==current_rating)
        {
            rating--;
        }
    }
    return rating;
}

static void rc_ui_cell_renderer_rating_get_property(GObject *object,
    guint param_id, GValue *value, GParamSpec *pspec)
{
    RCUiCellRendererRatingPrivate *priv = 
        RC_UI_CELL_RENDERER_RATING_GET_PRIVATE(object);
    switch(param_id)
    {
        case PROP_RATING:
            g_value_set_float(value, priv->rating);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
            break;
    }
}

static void rc_ui_cell_renderer_rating_set_property(GObject *object,
    guint param_id, const GValue *value, GParamSpec *pspec)
{
    RCUiCellRendererRatingPrivate *priv = 
        RC_UI_CELL_RENDERER_RATING_GET_PRIVATE(object);
    switch(param_id)
    {
        case PROP_RATING:
            priv->rating = g_value_get_float(value);
            if(priv->rating<0)
                priv->rating = 0;
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
            break;
    }
}

static void rc_ui_cell_renderer_rating_get_size(GtkCellRenderer *cell,
    GtkWidget *widget, const GdkRectangle *cell_area, gint *x_offset,
    gint *y_offset, gint *width, gint *height)
{
    gint icon_width;
    gint xpad, ypad;
    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &icon_width, NULL);
    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
    if(x_offset!=NULL)
        *x_offset = 0;
    if(y_offset!=NULL)
        *y_offset = 0;
    if(width!=NULL)
        *width = xpad*2 + icon_width * 5.0;
    if(height!=NULL)
        *height = ypad*2 + icon_width;
}

static void rc_ui_cell_renderer_rating_render(GtkCellRenderer *cell,
    cairo_t *cr, GtkWidget *widget, const GdkRectangle *background_area,
    const GdkRectangle *cell_area, GtkCellRendererState flags)

{
    RCUiCellRendererRating *rating = (RCUiCellRendererRating *)cell;
    RCUiCellRendererRatingPrivate *priv = 
        RC_UI_CELL_RENDERER_RATING_GET_PRIVATE(rating);
    gint xpad, ypad;
    gboolean selected;
    GdkRectangle pix_rect, draw_rect;
    rc_ui_cell_renderer_rating_get_size(cell, widget, cell_area, &pix_rect.x,
        &pix_rect.y, &pix_rect.width, &pix_rect.height);
    pix_rect.x += cell_area->x;
    pix_rect.y += cell_area->y;
    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
    pix_rect.width -= xpad * 2;
    pix_rect.height -= ypad * 2;
    if(!gdk_rectangle_intersect(cell_area, &pix_rect, &draw_rect))
        return;
    selected = (flags & GTK_CELL_RENDERER_SELECTED);
    rc_ui_rating_render_stars(widget, cr, priv,
        draw_rect.x - pix_rect.x, draw_rect.y - pix_rect.y,
        draw_rect.x, draw_rect.y, selected);
}

static gboolean rc_ui_cell_renderer_rating_activate(GtkCellRenderer *cell,
    GdkEvent *event, GtkWidget *widget, const gchar *path,
    const GdkRectangle *background_area, const GdkRectangle *cell_area,
    GtkCellRendererState flags)
{
    RCUiCellRendererRating *cell_rating = (RCUiCellRendererRating *)cell;
    RCUiCellRendererRatingPrivate *priv = 
        RC_UI_CELL_RENDERER_RATING_GET_PRIVATE(cell_rating);
    gint mouse_x, mouse_y;
    gfloat rating;
    g_return_val_if_fail(RC_UI_IS_CELL_RENDERER_RATING(cell_rating), FALSE);
    mouse_x = event->button.x;
    mouse_y = event->button.y;
    gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(widget),
        mouse_x, mouse_y, &mouse_x, &mouse_y);
    rating = rc_ui_rating_get_rating_from_widget(widget,
        mouse_x - cell_area->x, cell_area->width, priv->rating);
    if(rating!=-1.0)
    {
        g_signal_emit(cell_rating,
            ui_cell_renderer_rating_signals[SIGNAL_RATED], 0, path, rating);
    }
    return TRUE;
}

static void rc_ui_cell_renderer_rating_finalize(GObject *object)
{
    RCUiCellRendererRatingPrivate *priv = 
        RC_UI_CELL_RENDERER_RATING_GET_PRIVATE(object);
    if(priv->star_pixbuf!=NULL) g_object_unref(priv->star_pixbuf);
    if(priv->dot_pixbuf!=NULL) g_object_unref(priv->dot_pixbuf);
    if(priv->blank_pixbuf!=NULL) g_object_unref(priv->blank_pixbuf);
    G_OBJECT_CLASS(rc_ui_cell_renderer_rating_parent_class)->finalize(
        object);
}

static void rc_ui_cell_renderer_rating_class_init(
    RCUiCellRendererRatingClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS(klass);
    
    rc_ui_cell_renderer_rating_parent_class = g_type_class_peek_parent(klass);
    
    object_class->finalize = rc_ui_cell_renderer_rating_finalize;
    object_class->get_property = rc_ui_cell_renderer_rating_get_property;
    object_class->set_property = rc_ui_cell_renderer_rating_set_property;

    cell_class->get_size = rc_ui_cell_renderer_rating_get_size;
    cell_class->render = rc_ui_cell_renderer_rating_render;
    cell_class->activate = rc_ui_cell_renderer_rating_activate;
    
    /**
     * RCUiCellRendererRating:rating:
     *
     * The rating displayed by the renderer, as a floating point value
     * between 0.0 and 5.0.
     */
    g_object_class_install_property(object_class, PROP_RATING,
        g_param_spec_float("rating", "Rating Value", "Rating Value",
        0.0, 5.0, 3.0, G_PARAM_READWRITE));
        
    /**
     * RCUiCellRendererRating::rated:
     * @renderer: the #RCUiCellRendererRating
     * @score: the new rating
     * @path: string form of the #GtkTreePath to the row that was changed
     *
     * Emitted when the user changes the rating.
     */
    ui_cell_renderer_rating_signals[SIGNAL_RATED] = g_signal_new("rated",
        G_OBJECT_CLASS_TYPE(object_class), G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(RCUiCellRendererRatingClass, rated), NULL, NULL,
        rc_marshal_VOID__STRING_FLOAT, G_TYPE_NONE, 2, G_TYPE_STRING,
        G_TYPE_FLOAT);

    g_type_class_add_private(klass, sizeof(RCUiCellRendererRatingPrivate));
}


static void rc_ui_cell_renderer_rating_init(RCUiCellRendererRating *cell)
{
    RCUiCellRendererRatingPrivate *priv = 
        RC_UI_CELL_RENDERER_RATING_GET_PRIVATE(cell);
    GtkIconTheme *theme;
    gint width;
    GdkPixbuf *pixbuf;
    theme = gtk_icon_theme_get_default();
    gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, NULL, &width);
    
    priv->star_pixbuf = gtk_icon_theme_load_icon(theme, "RCSetStar", width,
        0, NULL);
    if(priv->star_pixbuf==NULL)
    {
        pixbuf = gdk_pixbuf_new_from_inline(-1, rc_ui_set_star_inline,
            FALSE, NULL);
        if(pixbuf!=NULL)
        {
            priv->star_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, width,
                GDK_INTERP_HYPER);
            g_object_unref(pixbuf);
        }    
    }
    
    priv->dot_pixbuf = gtk_icon_theme_load_icon(theme, "RCUnsetStar", width,
        0, NULL);
    if(priv->dot_pixbuf==NULL)
    {
        pixbuf = gdk_pixbuf_new_from_inline(-1, rc_ui_unset_star_inline,
            FALSE, NULL);
        if(pixbuf!=NULL)
        {
            priv->dot_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, width,
                GDK_INTERP_HYPER);
            g_object_unref(pixbuf);
        }    
    }
    
    priv->blank_pixbuf = gtk_icon_theme_load_icon(theme, "RCNoStar", width,
        0, NULL);
    if(priv->blank_pixbuf==NULL)
    {
        pixbuf = gdk_pixbuf_new_from_inline(-1, rc_ui_no_star_inline,
            FALSE, NULL);
        if(pixbuf!=NULL)
        {
            priv->blank_pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, width,
                GDK_INTERP_HYPER);
            g_object_unref(pixbuf);
        }    
    }
    
    g_object_set(cell, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

GType rc_ui_cell_renderer_rating_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo object_info = {
        .class_size = sizeof(RCUiCellRendererRatingClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_cell_renderer_rating_class_init,
        .class_finalize = NULL, 
        .class_data = NULL,
        .instance_size = sizeof(RCUiCellRendererRating),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_cell_renderer_rating_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_CELL_RENDERER,
            g_intern_static_string("RCUiCellRendererRating"), &object_info,
            0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_ui_cell_renderer_rating_new:
 *
 * Create a cell renderer that will display some pixbufs for representing
 * the rating of a song. It is also able to update the rating.
 * This widget is copied form Rhythmbox.
 *
 * Return value: the new cell renderer
 *
 */

GtkCellRenderer *rc_ui_cell_renderer_rating_new()
{
    return GTK_CELL_RENDERER(g_object_new(RC_UI_TYPE_CELL_RENDERER_RATING, NULL));
}

