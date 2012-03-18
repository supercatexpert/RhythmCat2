/*
 * RhythmCat UI Scrollable Label Widget Declaration
 * A scrollable label widget in the player.
 *
 * rc-ui-slabel.h
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

#ifndef HAVE_RC_UI_SCROLLABLE_LABEL_H
#define HAVE_RC_UI_SCROLLABLE_LABEL_H

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define RC_UI_SCROLLABLE_LABEL_TYPE (rc_ui_scrollable_label_get_type())
#define RC_UI_SCROLLABLE_LABEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
    RC_UI_SCROLLABLE_LABEL_TYPE, RCUiScrollableLabel))

#define RC_UI_SCROLLABLE_LABEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_SCROLLABLE_LABEL_TYPE, \
    RCUiScrollableLabelClass))
#define IS_RC_UI_SCROLLABLE_LABEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_SCROLLABLE_LABEL_TYPE))
#define IS_RC_UI_SCROLLABLE_LABEL_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_SCROLLABLE_LABEL_TYPE))

typedef struct _RCUiScrollableLabel RCUiScrollableLabel;
typedef struct _RCUiScrollableLabelClass RCUiScrollableLabelClass;

/**
 * RCUiScrollableLabel:
 *
 * The structure used in object.
 */

struct _RCUiScrollableLabel {
    /*< private >*/
    GtkWidget widget;
};

/**
 * RCUiScrollableLabelClass:
 *
 * The class data.
 */

struct _RCUiScrollableLabelClass {
    /*< private >*/
    GtkWidgetClass parent_class;
};

/*< private >*/
GType rc_ui_scrollable_label_get_type();

/*< public >*/
GtkWidget *rc_ui_scrollable_label_new();
void rc_ui_scrollable_label_set_text(RCUiScrollableLabel *widget,
    const gchar *text);
const gchar *rc_ui_scrollable_label_get_text(RCUiScrollableLabel *widget);
void rc_ui_scrollable_label_set_attributes(RCUiScrollableLabel *widget,
    PangoAttrList *attrs);
PangoAttrList *rc_ui_scrollable_label_get_attributes(
    RCUiScrollableLabel *widget);
void rc_ui_scrollable_label_set_percent(RCUiScrollableLabel *widget,
    gdouble percent);
gdouble rc_ui_scrollable_label_get_percent(RCUiScrollableLabel *widget);
gint rc_ui_scrollable_label_get_width(RCUiScrollableLabel *widget);

G_END_DECLS

#endif

