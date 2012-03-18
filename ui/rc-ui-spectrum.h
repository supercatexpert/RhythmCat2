/*
 * RhythmCat UI Spectrum Widget Header Declaration
 *
 * rc-ui-spectrum.h
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

#ifndef HAVE_RC_UI_SPECTRUM_H
#define HAVE_RC_UI_SPECTRUM_H

#include <math.h>
#include <glib.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RC_UI_SPECTRUM_WIDGET_TYPE (rc_ui_spectrum_widget_get_type())
#define RC_UI_SPECTRUM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
    RC_UI_SPECTRUM_WIDGET_TYPE, RCUiSpectrumWidget))

#define RC_UI_SPECTRUM_WIDGET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_SPECTRUM_WIDGET_TYPE, \
    RCUiSpectrumWidgetClass))
#define IS_RC_UI_SPECTRUM_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_SPECTRUM_WIDGET_TYPE))
#define IS_RC_UI_SPECTRUM_WIDGET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_SPECTRUM_WIDGET_TYPE))
    
typedef struct _RCUiSpectrumWidget RCUiSpectrumWidget;
typedef struct _RCUiSpectrumWidgetClass RCUiSpectrumWidgetClass;
    
/**
 * RCUiSpectrumWidget:
 *
 * The structure used in object.
 */

struct _RCUiSpectrumWidget {
    /*< private >*/
    GtkWidget widget;
};

/**
 * RCUiSpectrumWidgetClass:
 *
 * The class data.
 */

struct _RCUiSpectrumWidgetClass {
    /*< private >*/
    GtkWidgetClass parent_class;
};


/*< private >*/
GType rc_ui_spectrum_widget_get_type();

/*< public >*/
GtkWidget *rc_ui_spectrum_widget_new();
void rc_ui_spectrum_widget_set_magnitudes(RCUiSpectrumWidget *spectrum,
    guint rate, guint bands, gfloat threshold, const GValue *magnitudes);
void rc_ui_spectrum_widget_clean(RCUiSpectrumWidget *spectrum);

G_END_DECLS

#endif

