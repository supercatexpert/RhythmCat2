/*
 * RhythmCat UI Scrollable Label Widget Module
 * A scrollable label widget in the player.
 *
 * rc-ui-slabel.c
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
 
#include "rc-ui-slabel.h" 

/**
 * SECTION: rc-ui-slabel
 * @Short_description: A scrollable label widget.
 * @Title: RCUiScrollableLabel
 * @Include: rc-ui-slabel.h
 *
 * An scrolledate label widget.
 */

#define RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(obj)  \
    G_TYPE_INSTANCE_GET_PRIVATE((obj), RC_UI_TYPE_SCROLLABLE_LABEL, \
    RCUiScrollableLabelPrivate)

typedef struct RCUiScrollableLabelPrivate
{
    gchar *text;
    PangoAttrList *attrs;
    gdouble percent;
    PangoLayout *layout;
    gint current_x;
}RCUiScrollableLabelPrivate;

enum
{
    PROP_O,
    PROP_TEXT,
    PROP_ATTRS,
    PROP_PERCENT
};

static gpointer rc_ui_scrollable_label_parent_class = NULL;

static void rc_ui_scrollable_label_set_property(GObject *object,
    guint prop_id, const GValue *value, GParamSpec *pspec)
{
    RCUiScrollableLabel *text = RC_UI_SCROLLABLE_LABEL(object);
    switch(prop_id)
    {
        case PROP_TEXT:
            rc_ui_scrollable_label_set_text(text,
                g_value_get_string(value));
            break;
        case PROP_ATTRS:
            rc_ui_scrollable_label_set_attributes(text,
                g_value_get_boxed(value));
            break;
        case PROP_PERCENT:
            rc_ui_scrollable_label_set_percent(text,
                g_value_get_double(value));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void rc_ui_scrollable_label_get_property(GObject *object,
    guint prop_id, GValue *value, GParamSpec *pspec)
{
    RCUiScrollableLabel *text = RC_UI_SCROLLABLE_LABEL(object);
    RCUiScrollableLabelPrivate *priv =
        RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(text);
    switch(prop_id)
    {
        case PROP_TEXT:
            g_value_set_string(value, priv->text);
            break;
        case PROP_ATTRS:
            g_value_set_boxed(value, priv->attrs);
            break;
        case PROP_PERCENT:
            g_value_set_double(value, priv->percent);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void rc_ui_scrollable_label_realize(GtkWidget *widget)
{
    RCUiScrollableLabel *label;
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    GdkWindow *window, *parent;
    gint attr_mask;
    GtkStyleContext *context;
    g_return_if_fail(widget!=NULL);
    g_return_if_fail(RC_UI_IS_SCROLLABLE_LABEL(widget));
    label = RC_UI_SCROLLABLE_LABEL(widget);
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
    gdk_window_set_user_data(window, label);
    gdk_window_set_background_pattern(window, NULL);
    context = gtk_widget_get_style_context(widget);
    gtk_style_context_set_background(context, window);
    gdk_window_show(window);
}

static void rc_ui_scrollable_label_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
    GdkWindow *window;
    g_return_if_fail(widget!=NULL);
    g_return_if_fail(RC_UI_IS_SCROLLABLE_LABEL(widget));
    gtk_widget_set_allocation(widget, allocation);
    window = gtk_widget_get_window(widget);
    if(gtk_widget_get_realized(widget))
    {
        gdk_window_move_resize(window, allocation->x, allocation->y,
            allocation->width, allocation->height);
    }
}

static void rc_ui_scrollable_label_get_preferred_height(GtkWidget *widget,
    gint *min_height, gint *nat_height)
{
    RCUiScrollableLabelPrivate *priv;
    gint height;
    if(widget==NULL) return;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL) return;
    pango_layout_get_pixel_size(priv->layout, NULL, &height);
    *min_height = height+2;
    *nat_height = height+2;
}

static gboolean rc_ui_scrollable_label_draw(GtkWidget *widget, cairo_t *cr)
{
    RCUiScrollableLabel *label;
    RCUiScrollableLabelPrivate *priv;
    GtkAllocation allocation;
    gint width, height;
    GtkStyleContext *style_context;
    g_return_val_if_fail(widget!=NULL || cr!=NULL, FALSE);
    g_return_val_if_fail(RC_UI_IS_SCROLLABLE_LABEL(widget), FALSE);
    label = RC_UI_SCROLLABLE_LABEL(widget);
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(label);
    style_context = gtk_widget_get_style_context(widget);
    pango_layout_get_pixel_size(priv->layout, &width, &height);
    gtk_widget_get_allocation(widget, &allocation);
    if(width > allocation.width)
        priv->current_x = (gint)((gdouble)(allocation.width -
            width) * priv->percent);
    else
        priv->current_x = 0;
    gtk_render_layout(style_context, cr, priv->current_x,
        (allocation.height - height) / 2, priv->layout);
    return TRUE;
}

static void rc_ui_scrollable_label_init(RCUiScrollableLabel *object)
{
    RCUiScrollableLabelPrivate *priv;
    const PangoFontDescription *fd;
    GtkStyleContext *style_context;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(object);
    priv->percent = 0.0;
    priv->text = NULL;
    priv->attrs = NULL;
    priv->layout = gtk_widget_create_pango_layout(GTK_WIDGET(object), NULL);
    priv->current_x = 0;
    style_context = gtk_widget_get_style_context(GTK_WIDGET(object));
    fd = gtk_style_context_get_font(style_context, GTK_STATE_FLAG_NORMAL);
    pango_layout_set_font_description(priv->layout, fd);
}

static void rc_ui_scrollable_label_finalize(GObject *object)
{
    RCUiScrollableLabel *label = RC_UI_SCROLLABLE_LABEL(object);
    RCUiScrollableLabelPrivate *priv;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(label);
    if(priv->text!=NULL) g_free(priv->text);
    if(priv->layout!=NULL) g_object_unref(priv->layout);
    if(priv->attrs!=NULL) pango_attr_list_unref(priv->attrs);
    G_OBJECT_CLASS(rc_ui_scrollable_label_parent_class)->finalize(object);
}

static void rc_ui_scrollable_label_class_init(RCUiScrollableLabelClass *klass)
{
    GObjectClass *object_class;
    GtkWidgetClass *widget_class;
    rc_ui_scrollable_label_parent_class = g_type_class_peek_parent(klass);
    object_class = (GObjectClass *)klass;
    widget_class = (GtkWidgetClass *)klass;
    object_class->set_property = rc_ui_scrollable_label_set_property;
    object_class->get_property = rc_ui_scrollable_label_get_property;
    object_class->finalize = rc_ui_scrollable_label_finalize;
    widget_class->realize = rc_ui_scrollable_label_realize;
    widget_class->size_allocate = rc_ui_scrollable_label_size_allocate;
    widget_class->draw = rc_ui_scrollable_label_draw;
    widget_class->get_preferred_height =
        rc_ui_scrollable_label_get_preferred_height;
    g_type_class_add_private(klass, sizeof(RCUiScrollableLabelPrivate));

    /**
     * RCUiScrollableLabel:text:
     *
     * Sets the text of the widget to show.
     *
     */
    g_object_class_install_property(object_class, PROP_TEXT,
        g_param_spec_string("text", "Scrollable Text",
        "The text to show in widget", NULL, G_PARAM_READWRITE));


    /**
     * RCUiScrollableLabel:attributes:
     *
     * Sets the text of the widget to show.
     */
    g_object_class_install_property(object_class, PROP_ATTRS,
        g_param_spec_boxed("attributes", "Text attributes",
        "A list of style attributes to apply to the text",
        PANGO_TYPE_ATTR_LIST, G_PARAM_READWRITE));

    /**
     * RCUiScrollableLabel:percent:
     *
     * Sets the percentage of text movement.
     */
    g_object_class_install_property(object_class, PROP_PERCENT,
        g_param_spec_double("percent", "Percentage of movement",
        "The percentage of text movement", 0.0, 1.0, 0.0, G_PARAM_READWRITE));
}

/**
 * rc_ui_scrollable_label_get_type:
 *
 * Return the #GType of the #RCUiScrollableLabel class.
 *
 * Returns: The #GType of the #RCUiScrollableLabel class.
 */

GType rc_ui_scrollable_label_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo object_info = {
        .class_size = sizeof(RCUiScrollableLabelClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_scrollable_label_class_init,
        .class_finalize = NULL, 
        .class_data = NULL,
        .instance_size = sizeof(RCUiScrollableLabel),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_scrollable_label_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_WIDGET,
            g_intern_static_string("RCUiScrollableLabel"), &object_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_ui_scrollable_label_new:
 *
 * Create a new #RCUiScrollableLabel widget.
 *
 * Returns: A new #RCUiScrollableLabel widget.
 */

GtkWidget *rc_ui_scrollable_label_new()
{
    return GTK_WIDGET(g_object_new(rc_ui_scrollable_label_get_type(),
        NULL));
}

/**
 * rc_ui_scrollable_label_set_text:
 * @widget: the #RCUiScrollableLabel widget to set
 * @text: the text to set
 *
 * Set the text to show in the widget.
 */

void rc_ui_scrollable_label_set_text(RCUiScrollableLabel *widget,
    const gchar *text)
{
    RCUiScrollableLabelPrivate *priv;
    if(widget==NULL) return;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL) return;
    if(priv->text!=NULL)
    {
        if(g_strcmp0(priv->text, text)==0) return;
        g_free(priv->text);
        priv->text = NULL;
    }
    if(text!=NULL)
    {
        priv->text = g_strdup(text);
        pango_layout_set_text(priv->layout, text, -1);
    }
    else
        pango_layout_set_text(priv->layout, "", -1);
    gtk_widget_queue_draw(GTK_WIDGET(widget));
}

/**
 * rc_ui_scrollable_label_get_text:
 * @widget: the #RCUiScrollableLabel widget
 *
 * Return the text in the widget.
 *
 * Returns: The text in the widget, do not modify or free it.
 */

const gchar *rc_ui_scrollable_label_get_text(RCUiScrollableLabel *widget)
{
    RCUiScrollableLabelPrivate *priv;
    if(widget==NULL) return NULL;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL) return NULL;
    return priv->text;
}

/**
 * rc_ui_scrollable_label_set_attributes:
 * @widget: the #RCUiScrollableLabel widget
 * @attrs: the #PangoAttrList to set
 *
 * Sets a PangoAttrList; the attributes in the list are applied to the
 * text in the widget.
 */

void rc_ui_scrollable_label_set_attributes(RCUiScrollableLabel *widget,
    PangoAttrList *attrs)
{
    RCUiScrollableLabelPrivate *priv;
    if(widget==NULL) return;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL) return;
    if(priv->attrs!=NULL)
        pango_attr_list_unref(priv->attrs);
    priv->attrs = NULL;
    if(attrs!=NULL)
    {
        priv->attrs = attrs;
        pango_attr_list_ref(priv->attrs);
    }
    pango_layout_set_attributes(priv->layout, attrs);
    gtk_widget_queue_draw(GTK_WIDGET(widget));
}

/**
 * rc_ui_scrollable_label_get_attributes:
 * @widget: the #RCUiScrollableLabel widget
 *
 * Get the attribute list that was set on the widget.
 *
 * Returns: The attribute list, or NULL if none was set.
 */


PangoAttrList *rc_ui_scrollable_label_get_attributes(
    RCUiScrollableLabel *widget)
{
    RCUiScrollableLabelPrivate *priv;
    if(widget==NULL) return NULL;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL) return NULL;
    return priv->attrs;
}

/**
 * rc_ui_scrollable_label_set_percent:
 * @widget: the #RCUiScrollableLabel widget to set
 * @percent: the horizon percentage of the text show in the widget, must
 * be between 0.0 and 1.0
 *
 * Set the horizon percentage of the text show in the widget, if the text in
 * the widget is longer than the width of the widget.
 */

void rc_ui_scrollable_label_set_percent(RCUiScrollableLabel *widget,
    gdouble percent)
{
    RCUiScrollableLabelPrivate *priv;
    if(widget==NULL) return;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL) return;
    if(percent>=0.0 && percent<=1.0)
        priv->percent = percent;
    else if(percent>1.0)
        priv->percent = 1.0;
    else
        priv->percent = 0.0;
    gtk_widget_queue_draw(GTK_WIDGET(widget));
}

/**
 * rc_ui_scrollable_label_get_percent:
 * @widget: the #RCUiScrollableLabel widget
 *
 * Get the horizon percentage of the text show in the widget.
 *
 * Returns: The horizon percentage of the text show in the widget.
 */

gdouble rc_ui_scrollable_label_get_percent(RCUiScrollableLabel *widget)
{
    RCUiScrollableLabelPrivate *priv;
    if(widget==NULL) return 0.0;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL) return 0.0;
    return priv->percent;
}

/**
 * rc_ui_scrollable_label_get_width:
 * @widget: the #RCUiScrollableLabel widget
 *
 * Get the text width in the widget.
 *
 * Returns: The text width in the widget.
 */

gint rc_ui_scrollable_label_get_width(RCUiScrollableLabel *widget)
{
    RCUiScrollableLabelPrivate *priv;
    gint width = 0;
    if(widget==NULL) return 0;
    priv = RC_UI_SCROLLABLE_LABEL_GET_PRIVATE(widget);
    if(priv==NULL || priv->layout==NULL) return 0;
    pango_layout_get_pixel_size(priv->layout, &width, NULL);
    return width;
}










