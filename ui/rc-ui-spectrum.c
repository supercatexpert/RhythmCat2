/*
 * RhythmCat UI Spectrum Widget Module
 * A spectrum show widget in the player.
 *
 * rc-ui-spectrum.c
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
 
#include "rc-ui-spectrum.h"

/**
 * SECTION: rc-ui-spectrum
 * @Short_description: A spectrum show widget.
 * @Title: RCUiSpectrumWidget
 * @Include: rc-ui-slabel.h
 *
 * The spectrum show widget. It shows the spectrum gragh in the player.
 */

#define RC_UI_SPECTRUM_WIDGET_GET_PRIVATE(obj)  \
    G_TYPE_INSTANCE_GET_PRIVATE((obj), RC_UI_SPECTRUM_WIDGET_TYPE, \
    RCUiSpectrumWidgetPrivate)

typedef struct RCUiSpectrumWidgetPrivate
{
    guint rate;
    guint bands;
    gfloat threshold;
    gfloat *magnitudes;
}RCUiSpectrumWidgetPrivate;

static gpointer rc_ui_spectrum_widget_parent_class = NULL;

static void rc_ui_spectrum_widget_realize(GtkWidget *widget)
{
    RCUiSpectrumWidget *spectrum;
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    GdkWindow *window, *parent;
    gint attr_mask;
    GtkStyleContext *context;
    g_return_if_fail(widget!=NULL);
    g_return_if_fail(IS_RC_UI_SPECTRUM_WIDGET(widget));
    spectrum = RC_UI_SPECTRUM_WIDGET(widget);
    gtk_widget_set_realized(widget, TRUE);
    gtk_widget_get_allocation(widget, &allocation);
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget);
    attributes.event_mask |= (GDK_EXPOSURE_MASK);
    attributes.visual = gtk_widget_get_visual(widget);
    attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
    gtk_widget_set_has_window(widget, TRUE);
    parent = gtk_widget_get_parent_window(widget);
    window = gdk_window_new(parent, &attributes, attr_mask);
    gtk_widget_set_window(widget, window);
    gdk_window_set_user_data(window, spectrum);
    gdk_window_set_background_pattern(window, NULL);
    context = gtk_widget_get_style_context(widget);
    gtk_style_context_set_background(context, window);
    gdk_window_show(window);
}

static void rc_ui_spectrum_widget_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
    GdkWindow *window;
    g_return_if_fail(widget!=NULL);
    g_return_if_fail(IS_RC_UI_SPECTRUM_WIDGET(widget));
    gtk_widget_set_allocation(widget, allocation);
    window = gtk_widget_get_window(widget);
    if(gtk_widget_get_realized(widget))
    {
        gdk_window_move_resize(window, allocation->x, allocation->y,
            allocation->width, allocation->height);
    }
}

static void rc_ui_spectrum_widget_get_preferred_height(GtkWidget *widget,
    gint *min_height, gint *nat_height)
{
    *min_height = 15;
    *nat_height = 24;
}

static void rc_ui_spectrum_widget_interpolate(gfloat *in_array,
    guint in_size, gfloat *out_array, guint out_size)
{
    guint i;
    gfloat pos = 0.0;
    gdouble error;
    gulong offset;
    gulong index_left, index_right;
    gfloat step = (gfloat)in_size / out_size;
    for(i=0;i<out_size;i++, pos+=step)
    {
        error = pos - floor(pos);
        offset = (gulong)pos;
        index_left = offset + 0;
        if(index_left>=in_size)
            index_left = in_size - 1;
        index_right = offset + 1;
        if(index_right>=in_size)
            index_right = in_size - 1;
        out_array[i] = in_array[index_left] * (1.0 - error) +
            in_array[index_right] * error;
    }
}

static gboolean rc_ui_spectrum_widget_draw(GtkWidget *widget, cairo_t *cr)
{
    RCUiSpectrumWidget *spectrum;
    RCUiSpectrumWidgetPrivate *priv;
    GtkAllocation allocation;
    guint num;
    guint swidth = 4;
    guint sheight;
    guint space = 1;
    gfloat *narray;
    gfloat percent;
    gint i;
    GdkRGBA color;
    GtkStyleContext *style_context;
    g_return_val_if_fail(widget!=NULL || cr!=NULL, FALSE);
    g_return_val_if_fail(IS_RC_UI_SPECTRUM_WIDGET(widget), FALSE);
    spectrum = RC_UI_SPECTRUM_WIDGET(widget);
    priv = RC_UI_SPECTRUM_WIDGET_GET_PRIVATE(spectrum);
    if(priv==NULL) return FALSE;
    gtk_widget_get_allocation(widget, &allocation);
    style_context = gtk_widget_get_style_context(widget);
    gtk_style_context_get_color(style_context, GTK_STATE_FLAG_NORMAL,
        &color);
    num = allocation.width / (swidth + space);
    if(priv->bands>0 && priv->magnitudes!=NULL)
    {
        narray = g_new0(gfloat, num);
        rc_ui_spectrum_widget_interpolate(priv->magnitudes, priv->bands,
            narray, num);
        for(i=0;i<num;i++)
        {
            percent = (narray[i] - priv->threshold)/30;
            if(percent>1) percent = 1.0;
            sheight = (gfloat)allocation.height * percent;
            cairo_set_source_rgba(cr, color.red, color.green, color.blue,
                color.alpha);
            cairo_rectangle(cr, (swidth+space)*i, allocation.height-sheight,
                swidth, sheight);
            cairo_fill(cr);
        }
        g_free(narray);     
    }
    return TRUE;
}

static void rc_ui_spectrum_widget_init(RCUiSpectrumWidget *object)
{
    RCUiSpectrumWidgetPrivate *priv;
    priv = RC_UI_SPECTRUM_WIDGET_GET_PRIVATE(object);
    priv->bands = 0;
    priv->magnitudes = NULL;
}

static void rc_ui_spectrum_widget_finalize(GObject *object)
{
    RCUiSpectrumWidget *spectrum = RC_UI_SPECTRUM_WIDGET(object);
    RCUiSpectrumWidgetPrivate *priv;
    priv = RC_UI_SPECTRUM_WIDGET_GET_PRIVATE(spectrum);
    g_free(priv->magnitudes);
    G_OBJECT_CLASS(rc_ui_spectrum_widget_parent_class)->finalize(object);
}

static void rc_ui_spectrum_widget_class_init(RCUiSpectrumWidgetClass *klass)
{
    GObjectClass *object_class;
    GtkWidgetClass *widget_class;
    rc_ui_spectrum_widget_parent_class = g_type_class_peek_parent(klass);
    object_class = (GObjectClass *)klass;
    widget_class = (GtkWidgetClass *)klass;
    object_class->set_property = NULL;
    object_class->get_property = NULL;
    object_class->finalize = rc_ui_spectrum_widget_finalize;
    widget_class->realize = rc_ui_spectrum_widget_realize;
    widget_class->size_allocate = rc_ui_spectrum_widget_size_allocate;
    widget_class->draw = rc_ui_spectrum_widget_draw;
    widget_class->get_preferred_height =
        rc_ui_spectrum_widget_get_preferred_height;
    g_type_class_add_private(klass, sizeof(RCUiSpectrumWidgetPrivate));
}

GType rc_ui_spectrum_widget_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo object_info = {
        .class_size = sizeof(RCUiSpectrumWidgetClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_spectrum_widget_class_init,
        .class_finalize = NULL, 
        .class_data = NULL,
        .instance_size = sizeof(RCUiSpectrumWidget),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_spectrum_widget_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_WIDGET,
            g_intern_static_string("RCUiSpectrumWidget"), &object_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_ui_spectrum_widget_new:
 *
 * Create a new #RCUiSpectrumWidget widget.
 *
 * Returns: A new #RCUiSpectrumWidget widget.
 */

GtkWidget *rc_ui_spectrum_widget_new()
{
    return GTK_WIDGET(g_object_new(rc_ui_spectrum_widget_get_type(),
        NULL));
}

/**
 * rc_ui_spectrum_widget_set_magnitudes:
 * @spectrum: the #RCUiSpectrumWidget widget
 * @rate: the sample rate
 * @bands: the band number
 * @threshold: the threshold value
 * @magnitudes: a magniude array
 *
 * Set magnitude values for the #RCUiSpectrumWidget widget.
 */

void rc_ui_spectrum_widget_set_magnitudes(RCUiSpectrumWidget *spectrum,
    guint rate, guint bands, gfloat threshold, const GValue *magnitudes)
{
    RCUiSpectrumWidgetPrivate *priv;
    const GValue *mag;
    gfloat fvalue;
    gint i;
    guint bands_copy;
    if(spectrum==NULL || magnitudes==NULL || bands==0) return;
    priv = RC_UI_SPECTRUM_WIDGET_GET_PRIVATE(spectrum);
    if(priv==NULL) return;
    bands_copy = 5 * bands / 16;
    priv->rate = rate;
    priv->threshold = threshold;
    if(priv->bands!=bands_copy)
    {
        g_free(priv->magnitudes);
        priv->magnitudes = g_new0(gfloat, bands_copy);
        priv->bands = bands_copy;
    }
    for(i=0;i<bands;i++)
    {
        mag = gst_value_array_get_value(magnitudes, i);
        if(mag==NULL) continue;
        fvalue = g_value_get_float(mag);
        if(i>=0 && i<bands/16)
        {
            /* Range: 0 - bands/16 */
            priv->magnitudes[i] = fvalue;
        }
        else if(i>=bands/16 && i<bands/8)
        {
            /* Range: bands/16 - bands/8 */
            priv->magnitudes[bands/16+i/2] += fvalue / 2; 
        }
        else if(i>=bands/8 && i<bands/4)
        {
            /* Range: bands/8 - 3*bands/16 */
            priv->magnitudes[2*bands/16+i/4] += fvalue / 4;
        }
        else if(i>=bands/4 && i<bands/2)
        {
            /* Range: 3*bands/16 - bands/4 */
            priv->magnitudes[3*bands/16+i/8] += fvalue / 8;
        }
        else if(i>=bands/2)
        {
            /* Range: bands/4 - 5*bands/16 */
            priv->magnitudes[4*bands/16+i/16] += fvalue / 16;
        }
    }
    for(i=0;i<bands_copy;i++)
    {
        mag = gst_value_array_get_value(magnitudes, i);
        if(mag==NULL) continue;
        priv->magnitudes[i] = g_value_get_float(mag);
    }
    gtk_widget_queue_draw(GTK_WIDGET(spectrum));
}

/**
 * rc_ui_spectrum_widget_clean:
 * @spectrum: the #RCUiSpectrumWidget widget
 *
 * Clean the spectrum widget.
 */

void rc_ui_spectrum_widget_clean(RCUiSpectrumWidget *spectrum)
{
    RCUiSpectrumWidgetPrivate *priv;
    if(spectrum==NULL) return;
    priv = RC_UI_SPECTRUM_WIDGET_GET_PRIVATE(spectrum);
    if(priv==NULL) return;
    g_free(priv->magnitudes);
    priv->magnitudes = NULL;
    priv->bands = 0;
    gtk_widget_queue_draw(GTK_WIDGET(spectrum));
}

