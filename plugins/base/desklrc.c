/*
 * Desktop Lyric Plugin
 * Show lyric on the desktop.
 *
 * desklrc.c
 * This file is part of RhythmCat
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

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <rclib.h>

#define DESKLRC_ID "rc2-desktop-lyric"
#define DESKLRC_OSD_WINDOW_MIN_WIDTH 320

typedef struct RCPluginDesklrcPrivate
{
    GtkWidget *window;
    PangoLayout *layout;
    GdkRGBA bg_color1;
    GdkRGBA bg_color2;
    GdkRGBA fg_color1;
    GdkRGBA fg_color2;
    gchar *font_string;
    gboolean movable;
    gboolean two_line;
    gboolean draw_stroke;
    gint osd_window_width;
    gint osd_window_pos_x;
    gint osd_window_pos_y;
    gboolean notify_flag;
    gulong timeout_id;
    GKeyFile *keyfile;
}RCPluginDesklrcPrivate;

static RCPluginDesklrcPrivate desklrc_priv = {0};

static inline void rc_plugin_desklrc_draw_drag_shadow(cairo_t *cr, gint width,
    gint height)
{
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.4);
    cairo_move_to(cr, 0 + 5, 0);
    cairo_line_to(cr, 0 + width - 5, 0);
    cairo_move_to(cr, 0 + width, 0 + 5);
    cairo_line_to(cr, 0 + width, 0 + height - 5);
    cairo_move_to(cr, 0 + width - 5, 0 + height);
    cairo_line_to(cr, 0 + 5, 0 + height);
    cairo_move_to(cr, 0, 0 + height - 5);
    cairo_line_to(cr, 0, 0 + 5);
    cairo_arc(cr, 0 + 5, 0 + 5, 5, G_PI, 3 * G_PI / 2.0);
    cairo_arc(cr, 0 + width - 5, 0 + 5, 5, 3 * G_PI / 2, 2 * G_PI);
    cairo_arc(cr, 0 + width - 5, 0 + height - 5, 5, 0, G_PI / 2);
    cairo_arc(cr, 0 + 5, 0 + height - 5, 5, G_PI / 2, G_PI);
    cairo_fill(cr);
}

static void rc_plugin_desklrc_show(GtkWidget *widget,
    RCPluginDesklrcPrivate *priv, cairo_t *cr, gint64 pos, gint64 offset1,
    GSequenceIter *iter1, GSequenceIter *iter_begin1, gint64 offset2,
    GSequenceIter *iter2, GSequenceIter *iter_begin2)
{
    GtkAllocation allocation;
    cairo_pattern_t *pat;
    const RCLibLyricData *lrc_data1 = NULL;
    const RCLibLyricData *lrc_data2 = NULL;
    GSequenceIter *iter_now1 = NULL;
    GSequenceIter *iter_now2 = NULL;
    GSequenceIter *iter_next1 = NULL;
    GSequenceIter *iter_next2 = NULL;
    gint index1 = 0, index2 = 0;
    gboolean two_track = FALSE;
    gboolean two_line = FALSE;
    gfloat percent1 = 0.0, percent2 = 0.0;
    gint64 time_passed1 = 0, time_length1 = 0;
    gint64 time_passed2 = 0, time_length2 = 0;
    gint text_width1 = 0, text_height1 = 0;
    gint text_next_width1 = 0, text_next_height1 = 0;
    gint text_width2 = 0, text_height2 = 0;
    gint text_next_width2 = 0, text_next_height2 = 0;
    gint window_height;
    gfloat start_x, start_y, fill_width;
    if(widget==NULL || priv==NULL || priv->layout==NULL || cr==NULL)
        return;
    if(iter1!=NULL)
        iter_now1 = iter1;
    else
        iter_now1 = iter_begin1;
    if(iter2!=NULL)
        iter_now2 = iter2;
    else
        iter_now2 = iter_begin2;
    if(iter_now1==NULL && iter_now2==NULL)
    {
        if(priv->notify_flag)
        {
            gtk_widget_get_allocation(widget, &allocation);
            rc_plugin_desklrc_draw_drag_shadow(cr, allocation.width,
                allocation.height);
        }
        return;
    }
    if(iter_now1==NULL && iter_now2!=NULL)
    {
        iter_now1 = iter_now2;
        iter1 = iter2;
        iter_begin1 = iter_begin2;
        iter_now2 = NULL;
        iter2 = NULL;
        iter_begin2 = NULL;
    }
    if(iter_now1!=NULL && iter_now2!=NULL) two_track = TRUE;
    two_line = priv->two_line;
    if(iter1!=NULL)
    {
        lrc_data1 = g_sequence_get(iter1);
        if(lrc_data1!=NULL)
        {
            time_passed1 = pos - (lrc_data1->time+offset1);
            if(lrc_data1->length>0)
                time_length1 = lrc_data1->length;
            else
                time_length1 = rclib_core_query_duration() -
                    (lrc_data1->time+offset1);
            if(time_passed1>0)
                percent1 = (gdouble)time_passed1 / time_length1;
        }
    }
    if(iter2!=NULL)
    {
        lrc_data2 = g_sequence_get(iter2);
        if(lrc_data2!=NULL)
        {
            time_passed2 = pos - (lrc_data2->time+offset2);
            if(lrc_data2->length>0)
                time_length2 = lrc_data2->length;
            else
                time_length2 = rclib_core_query_duration() -
                    (lrc_data2->time+offset2);
            if(time_passed2>0)
                percent2 = (gdouble)time_passed2 / time_length2;
        }
    }
    if(two_line)
    {
        if(!g_sequence_iter_is_begin(iter_now1))
        {
            if(percent1>=0.5)
                iter_next1 = g_sequence_iter_next(iter_now1);
            else
                iter_next1 = g_sequence_iter_prev(iter_now1);
        }
        else
        {
            if(percent1>=0.5)
                iter_next1 = g_sequence_iter_next(iter_now1);
            else
                iter_next1 = NULL;
        }
        if(iter_now2!=NULL)
        {
            if(!g_sequence_iter_is_begin(iter_now2))
            {
                if(percent2>=0.5)
                    iter_next2 = g_sequence_iter_next(iter_now2);
                else
                    iter_next2 = g_sequence_iter_prev(iter_now2);
            }
            else
            {
                if(percent2>=0.5)
                    iter_next2 = g_sequence_iter_next(iter_now2);
                else
                    iter_next2 = NULL;
            }
        }
    }
    if(iter_now1!=NULL)
    {
        lrc_data1 = g_sequence_get(iter_now1);
        if(lrc_data1!=NULL)
        {
            pango_layout_set_text(priv->layout, lrc_data1->text, -1);
            pango_layout_get_pixel_size(priv->layout, &text_width1,
                &text_height1);
        }
    }
    if(two_line)
    {
        if(iter_next1!=NULL)
            lrc_data1 = g_sequence_get(iter_next1);
        else
            lrc_data1 = NULL;
        if(lrc_data1!=NULL)
            pango_layout_set_text(priv->layout, lrc_data1->text, -1);
        else
            pango_layout_set_text(priv->layout, "", -1);
        pango_layout_get_pixel_size(priv->layout, &text_next_width1,
            &text_next_height1);
    }
    if(iter_now2!=NULL)
    {
        lrc_data2 = g_sequence_get(iter_now2);
        if(lrc_data2!=NULL)
        {
            pango_layout_set_text(priv->layout, lrc_data2->text, -1);
            pango_layout_get_pixel_size(priv->layout, &text_width2,
                &text_height2);
        }
    }
    if(two_line && two_track)
    {
        if(iter_next2!=NULL)
            lrc_data2 = g_sequence_get(iter_next2);
        else
            lrc_data2 = NULL;
        if(lrc_data2!=NULL)
            pango_layout_set_text(priv->layout, lrc_data2->text, -1);
        else
            pango_layout_set_text(priv->layout, "", -1);
        pango_layout_get_pixel_size(priv->layout, &text_next_width2,
            &text_next_height2);
    }
    window_height = ((text_height1 + text_next_height1 + text_height2 +
        text_next_height2) / 10) * 10 + 20;
    if(two_line) window_height+=5;
    if(two_track) window_height+=5;
    if(two_line && two_track) window_height+=5;
    gtk_widget_get_allocation(widget, &allocation);
    if(allocation.height!=window_height ||
        allocation.width!=priv->osd_window_width)
    {
        gtk_window_resize(GTK_WINDOW(widget), priv->osd_window_width,
            window_height);  
    }
    gtk_widget_get_allocation(widget, &allocation);
    if(priv->notify_flag)
    {
        rc_plugin_desklrc_draw_drag_shadow(cr, allocation.width,
            allocation.height);
    }
    if(iter_now1!=NULL)
    {
        cairo_save(cr);
        index1 = g_sequence_iter_get_position(iter_now1);
        lrc_data1 = g_sequence_get(iter_now1);
        if(lrc_data1!=NULL)
        {
            pango_layout_set_text(priv->layout, lrc_data1->text, -1);
            pango_layout_get_pixel_size(priv->layout, &text_width1,
                &text_height1);
        }
        if(two_line)
            start_x = 10 + (index1%2)*allocation.width/3;
        else
            start_x = 10;
        if(start_x+text_width1>allocation.width)
        {
            start_x += (gfloat)(allocation.width-text_width1-start_x-20) *
                percent1;
        }
        if(two_line && two_track)
        {
            if(index1%2==0)
                start_y = 10;
            else
                start_y = 5 + window_height /2;
        }
        else if(two_line && !two_track)
        {
            if(index1%2==0)
                start_y = 10;
            else
                start_y = window_height - 10 - text_height1;
        }
        else
        {
            start_y = 10;
        }
        cairo_move_to(cr, start_x, start_y);
        pango_cairo_update_layout(cr, priv->layout);
        pango_cairo_layout_path(cr, priv->layout);
        if(priv->draw_stroke)
        {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
            cairo_stroke_preserve(cr);
        } 
        cairo_clip(cr);
        pat = cairo_pattern_create_linear(start_x, start_y, start_x,
            start_y+text_height1);
        cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->bg_color2.red,
            priv->bg_color2.green, priv->bg_color2.blue);
        cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->bg_color1.red,
            priv->bg_color1.green, priv->bg_color1.blue);
        cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->bg_color2.red,
            priv->bg_color2.green, priv->bg_color2.blue);
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, start_x, start_y, text_width1, text_height1);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        fill_width = percent1 * (gfloat)text_width1;
        pat = cairo_pattern_create_linear(start_x, start_y, start_x,
            start_y+text_height1);
        cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->fg_color2.red,
            priv->fg_color2.green, priv->fg_color2.blue);
        cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->fg_color1.red,
            priv->fg_color1.green, priv->fg_color1.blue);
        cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->fg_color2.red,
            priv->fg_color2.green, priv->fg_color2.blue);
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, start_x, start_y, fill_width, text_height1);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        cairo_restore(cr);
    }
    if(iter_next1!=NULL)
    {
        cairo_save(cr);
        index1 = g_sequence_iter_get_position(iter_next1);
        lrc_data1 = g_sequence_get(iter_next1);
        if(lrc_data1!=NULL)
        {
            pango_layout_set_text(priv->layout, lrc_data1->text, -1);
            pango_layout_get_pixel_size(priv->layout, &text_next_width1,
                &text_next_height1);
        }
        start_x = 10 + (index1%2)*allocation.width/3;
        if(percent1<0.5 && start_x+text_next_width1>allocation.width)
        {
            start_x += (gfloat)
                (allocation.width-text_next_width1-start_x-20);
        }
        if(two_line && two_track)
        {
            if(index1%2==0)
                start_y = 10;
            else
                start_y = 5 + window_height /2;
        }
        else if(two_line && !two_track)
        {
            if(index1%2==0)
                start_y = 10;
            else
                start_y = window_height - 10 - text_next_height1;
        }
        else
        {
            start_y = 10;
        }
        cairo_move_to(cr, start_x, start_y);
        pango_cairo_update_layout(cr, priv->layout);
        pango_cairo_layout_path(cr, priv->layout);
        if(priv->draw_stroke)
        {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
            cairo_stroke_preserve(cr);
        }
        cairo_clip(cr);
        pat = cairo_pattern_create_linear(start_x, start_y, start_x,
            start_y+text_next_height1);
        if(percent1<0.5)
        {
            cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->fg_color2.red,
                priv->fg_color2.green, priv->fg_color2.blue);
            cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->fg_color1.red,
                priv->fg_color1.green, priv->fg_color1.blue);
            cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->fg_color2.red,
                priv->fg_color2.green, priv->fg_color2.blue);
        }
        else
        {
            cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->bg_color2.red,
                priv->bg_color2.green, priv->bg_color2.blue);
            cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->bg_color1.red,
                priv->bg_color1.green, priv->bg_color1.blue);
            cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->bg_color2.red,
                priv->bg_color2.green, priv->bg_color2.blue);
        }
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, start_x, start_y, text_next_width1,
            text_next_height1);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        cairo_restore(cr);
    }
    if(iter_now2!=NULL)
    {
        cairo_save(cr);
        index2 = g_sequence_iter_get_position(iter_now2);
        lrc_data2 = g_sequence_get(iter_now2);
        if(lrc_data2!=NULL)
        {
            pango_layout_set_text(priv->layout, lrc_data2->text, -1);
            pango_layout_get_pixel_size(priv->layout, &text_width2,
                &text_height2);
        }
        if(two_line)
            start_x = 10 + (index2%2)*allocation.width/3;
        else
            start_x = 10;
        if(start_x+text_width2>allocation.width)
        {
            start_x += (gfloat)(allocation.width-text_width2-start_x-20) *
                percent2;
        }
        if(two_line)
        {
            if(index2%2==0)
                start_y = 15 + text_height1;
            else
                start_y = window_height - 10 - text_height2;
        }
        else
        {
            start_y = 15 + text_height1;
        }
        cairo_move_to(cr, start_x, start_y);
        pango_cairo_update_layout(cr, priv->layout);
        pango_cairo_layout_path(cr, priv->layout);
        if(priv->draw_stroke)
        {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
            cairo_stroke_preserve(cr);
        }
        cairo_clip(cr);
        pat = cairo_pattern_create_linear(start_x, start_y, start_x,
            start_y+text_height2);
        cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->bg_color2.red,
            priv->bg_color2.green, priv->bg_color2.blue);
        cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->bg_color1.red,
            priv->bg_color1.green, priv->bg_color1.blue);
        cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->bg_color2.red,
            priv->bg_color2.green, priv->bg_color2.blue);
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, start_x, start_y, text_width2, text_height2);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        fill_width = percent2 * (gfloat)text_width2;
        pat = cairo_pattern_create_linear(start_x, start_y, start_x,
            start_y+text_height2);
        cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->fg_color2.red,
            priv->fg_color2.green, priv->fg_color2.blue);
        cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->fg_color1.red,
            priv->fg_color1.green, priv->fg_color1.blue);
        cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->fg_color2.red,
            priv->fg_color2.green, priv->fg_color2.blue);
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, start_x, start_y, fill_width, text_height2);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        cairo_restore(cr);
    }
    if(iter_next2!=NULL)
    {
        cairo_save(cr);
        index2 = g_sequence_iter_get_position(iter_next2);
        lrc_data2 = g_sequence_get(iter_next2);
        if(lrc_data2!=NULL)
        {
            pango_layout_set_text(priv->layout, lrc_data2->text, -1);
            pango_layout_get_pixel_size(priv->layout, &text_next_width2,
                &text_next_height2);
        }
        start_x = 10 + (index2%2)*allocation.width/3;
        if(percent2<0.5 && start_x+text_next_width2>allocation.width)
        {
            start_x += (gfloat)
                (allocation.width-text_next_width2-start_x-20);
        }
        if(two_line)
        {
            if(index2%2==0)
                start_y = 15 + text_height1;
            else
                start_y = window_height - 15 - text_height2;
        }
        else
        {
            start_y = 20 + text_height1;
        }
        cairo_move_to(cr, start_x, start_y);
        pango_cairo_update_layout(cr, priv->layout);
        pango_cairo_layout_path(cr, priv->layout);
        if(priv->draw_stroke)
        {
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
            cairo_stroke_preserve(cr);
        }
        cairo_clip(cr);
        pat = cairo_pattern_create_linear(start_x, start_y, start_x,
            start_y+text_next_height2);
        if(percent2<0.5)
        {
            cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->fg_color2.red,
                priv->fg_color2.green, priv->fg_color2.blue);
            cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->fg_color1.red,
                priv->fg_color1.green, priv->fg_color1.blue);
            cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->fg_color2.red,
                priv->fg_color2.green, priv->fg_color2.blue);
        }
        else
        {
            cairo_pattern_add_color_stop_rgb(pat, 0.0, priv->bg_color2.red,
                priv->bg_color2.green, priv->bg_color2.blue);
            cairo_pattern_add_color_stop_rgb(pat, 0.5, priv->bg_color1.red,
                priv->bg_color1.green, priv->bg_color1.blue);
            cairo_pattern_add_color_stop_rgb(pat, 1.0, priv->bg_color2.red,
                priv->bg_color2.green, priv->bg_color2.blue);
        }
        cairo_set_source(cr, pat);
        cairo_rectangle(cr, start_x, start_y, text_next_width2,
            text_next_height2);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        cairo_restore(cr);
    }
}

static gboolean rc_plugin_desklrc_drag(GtkWidget *widget, GdkEvent *event,
    gpointer data)
{
    static gint move_x = 0;
    static gint move_y = 0;
    static gboolean drag_flag = FALSE;
    static gboolean resize_flag = FALSE;
    GdkWindow *window;
    GtkAllocation allocation;
    GdkCursor *cursor = NULL;
    gint x, y;
    gint width = 0;
    RCPluginDesklrcPrivate *priv = (RCPluginDesklrcPrivate *)data;
    if(data==NULL) return FALSE;
    if(!priv->movable) return FALSE;
    window = gtk_widget_get_window(widget);
    gtk_widget_get_allocation(widget, &allocation);
    if(event->button.button==1)
    {
        switch(event->type)
        {
            case GDK_BUTTON_PRESS:
            {
                move_x = event->button.x;
                move_y = event->button.y;
                if(move_x>allocation.width-10)
                {
                    resize_flag = TRUE;
                    cursor = gdk_cursor_new(GDK_RIGHT_SIDE);
                }
                else
                {
                    drag_flag = TRUE;
                    cursor = gdk_cursor_new(GDK_HAND1);
                }
                gdk_window_set_cursor(window, cursor);
                g_object_unref(cursor);
                break;
            }
            case GDK_BUTTON_RELEASE:
            {
                drag_flag = FALSE;
                resize_flag = FALSE;
                cursor = gdk_cursor_new(GDK_ARROW);
                gdk_window_set_cursor(window, cursor);
                g_object_unref(cursor);
                break;
            }
            case GDK_MOTION_NOTIFY:
            {
                if(drag_flag)
                {
                    gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
                    gtk_window_move(GTK_WINDOW(widget), x +
                        event->button.x - move_x, y + 
                        event->button.y - move_y);
                    gtk_window_get_position(GTK_WINDOW(widget),
                        &(priv->osd_window_pos_x),
                        &(priv->osd_window_pos_y));
                }
                if(resize_flag)
                {
                    width = event->button.x + 10;
                    priv->osd_window_width = width;
                    gtk_window_resize(GTK_WINDOW(widget), width,
                        allocation.height);
                }
                else
                {
                    if(event->button.x>allocation.width-10)
                        cursor = gdk_cursor_new(GDK_RIGHT_SIDE);
                    else
                        cursor = gdk_cursor_new(GDK_ARROW);
                    gdk_window_set_cursor(window, cursor);
                    g_object_unref(cursor);
                }
            }	
            default:
                break;
        }
    }
    switch(event->type)
    {
        case GDK_ENTER_NOTIFY:
        {
            priv->notify_flag = TRUE;
            break;
        }
        case GDK_LEAVE_NOTIFY:
        {
            priv->notify_flag = FALSE;
            break;
        }
        default:
            break;
    }
    return FALSE;
}

static gboolean rc_plugin_desklrc_update(gpointer data)
{
    RCPluginDesklrcPrivate *priv = (RCPluginDesklrcPrivate *)data;
    if(data==NULL) return TRUE;
    gtk_widget_queue_draw(priv->window);
    return TRUE;   
}

static gboolean rc_plugin_desklrc_draw(GtkWidget *widget, 
    cairo_t *cr, gpointer data)
{
    RCPluginDesklrcPrivate *priv = (RCPluginDesklrcPrivate *)data;
    gint64 pos = 0, offset1 = 0, offset2 = 0;
    GSequenceIter *iter1 = NULL, *iter_begin1 = NULL;
    GSequenceIter *iter2 = NULL, *iter_begin2 = NULL;
    RCLibLyricParsedData *parsed_data1, *parsed_data2;
    if(data==NULL) return FALSE;
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    pos = rclib_core_query_position();
    parsed_data1 = rclib_lyric_get_parsed_data(0);
    parsed_data2 = rclib_lyric_get_parsed_data(1);
    iter1 = rclib_lyric_get_line_iter(0, pos);
    iter2 = rclib_lyric_get_line_iter(1, pos);
    if(iter1!=NULL && g_sequence_iter_is_end(iter1)) iter1 = NULL;
    if(iter2!=NULL && g_sequence_iter_is_end(iter2)) iter2 = NULL;
    if(parsed_data1!=NULL)
    {
        offset1 = (gint64)parsed_data1->offset*GST_MSECOND;
        iter_begin1 = g_sequence_get_begin_iter(parsed_data1->seq);
    }
    if(parsed_data2!=NULL)
    {
        offset2 = (gint64)parsed_data2->offset*GST_MSECOND;
        iter_begin2 = g_sequence_get_begin_iter(parsed_data2->seq);
    }
    if(iter_begin1!=NULL && g_sequence_iter_is_end(iter_begin1))
        iter_begin1 = NULL;
    if(iter_begin2!=NULL && g_sequence_iter_is_end(iter_begin2))
        iter_begin2 = NULL;
    rc_plugin_desklrc_show(widget, priv, cr, pos, offset1, iter1,
        iter_begin1, offset2, iter2, iter_begin2);
    return FALSE;
}

static void rc_plugin_desklrc_load_conf(RCPluginDesklrcPrivate *priv)
{
    gchar *string = NULL;
    gint ivalue = 0;
    gboolean bvalue = FALSE;
    GError *error = NULL;
    if(priv==NULL || priv->keyfile==NULL) return;
    string = g_key_file_get_string(priv->keyfile, DESKLRC_ID,
        "NormalColor1", NULL);
    if(string!=NULL)
    {
        if(!gdk_rgba_parse(&(priv->bg_color1), string))
            gdk_rgba_parse(&(priv->bg_color1), "#4CFFFF");
    }
    else
        gdk_rgba_parse(&(priv->bg_color1), "#4CFFFF");
    g_free(string);
    string = g_key_file_get_string(priv->keyfile, DESKLRC_ID,
        "NormalColor2", NULL);
    if(string!=NULL)
    {
        if(!gdk_rgba_parse(&(priv->bg_color2), string))
            gdk_rgba_parse(&(priv->bg_color2), "#0000FF");
    }
    else
        gdk_rgba_parse(&(priv->bg_color2), "#0000FF");
    g_free(string);
    string = g_key_file_get_string(priv->keyfile, DESKLRC_ID,
        "HighLightColor1", NULL);
    if(string!=NULL)
    {
        if(!gdk_rgba_parse(&(priv->fg_color1), string))
            gdk_rgba_parse(&(priv->fg_color1), "#FF4C4C");
    }
    else
        gdk_rgba_parse(&(priv->fg_color1), "#FF4C4C");
    g_free(string);
    string = g_key_file_get_string(priv->keyfile, DESKLRC_ID,
        "HighLightColor2", NULL);
    if(string!=NULL)
    {
        if(!gdk_rgba_parse(&(priv->fg_color2), string))
            gdk_rgba_parse(&(priv->fg_color2), "#FFFF00");
    }
    else
        gdk_rgba_parse(&(priv->fg_color2), "#FFFF00");
    g_free(string);  
    ivalue = g_key_file_get_integer(priv->keyfile, DESKLRC_ID,
        "OSDWindowWidth", NULL);
    if(ivalue>=DESKLRC_OSD_WINDOW_MIN_WIDTH)
        priv->osd_window_width = ivalue;
    ivalue = g_key_file_get_integer(priv->keyfile, DESKLRC_ID,
        "OSDWindowPositionX", &error);
    if(error==NULL)
        priv->osd_window_pos_x = ivalue;
    else
    {
        g_error_free(error);
        error = NULL;
    }
    ivalue = g_key_file_get_integer(priv->keyfile, DESKLRC_ID,
        "OSDWindowPositionY", &error);
    if(error==NULL)
        priv->osd_window_pos_y = ivalue;
    else
    {
        g_error_free(error);
        error = NULL;
    }
    string = g_key_file_get_string(priv->keyfile, DESKLRC_ID,
        "Font", NULL);
    if(string!=NULL)
    {
        g_free(priv->font_string);
        priv->font_string = g_strdup(string);
    }
    g_free(string);
    bvalue = g_key_file_get_boolean(priv->keyfile, DESKLRC_ID,
        "OSDWindowMovable", &error);
    if(error==NULL)
        priv->movable = bvalue;
    else
    {
        g_error_free(error);
        error = NULL;
    }
    bvalue = g_key_file_get_boolean(priv->keyfile, DESKLRC_ID,
        "DrawStroke", &error);
    if(error==NULL)
        priv->draw_stroke = bvalue;
    else
    {
        g_error_free(error);
        error = NULL;
    } 
    bvalue = g_key_file_get_boolean(priv->keyfile, DESKLRC_ID,
        "ShowTwoLine", &error);
    if(error==NULL)
        priv->two_line = bvalue;
    else
    {
        g_error_free(error);
        error = NULL;
    }
}

static void rc_plugin_desklrc_save_conf(RCPluginDesklrcPrivate *priv)
{
    gchar *string;
    if(priv==NULL || priv->keyfile==NULL) return;
    string = gdk_rgba_to_string(&(priv->bg_color1));
    g_key_file_set_string(priv->keyfile, DESKLRC_ID,
        "NormalColor1", string);
    g_free(string);
    string = gdk_rgba_to_string(&(priv->bg_color2));
    g_key_file_set_string(priv->keyfile, DESKLRC_ID,
        "NormalColor2", string);
    g_free(string);    
    string = gdk_rgba_to_string(&(priv->fg_color1));
    g_key_file_set_string(priv->keyfile, DESKLRC_ID,
        "HighLightColor1", string);
    g_free(string);
    string = gdk_rgba_to_string(&(priv->fg_color2));
    g_key_file_set_string(priv->keyfile, DESKLRC_ID,
        "HighLightColor2", string);
    g_free(string);
    g_key_file_set_string(priv->keyfile, DESKLRC_ID,
        "Font", priv->font_string);
    g_key_file_set_integer(priv->keyfile, DESKLRC_ID,
        "OSDWindowWidth", priv->osd_window_width);
    if(priv->osd_window_pos_x>=0)
    {
        g_key_file_set_integer(priv->keyfile, DESKLRC_ID,
            "OSDWindowPositionX", priv->osd_window_pos_x);
    }
    if(priv->osd_window_pos_y>=0)
    {
        g_key_file_set_integer(priv->keyfile, DESKLRC_ID,
            "OSDWindowPositionY", priv->osd_window_pos_y);
    }
    g_key_file_set_boolean(priv->keyfile, DESKLRC_ID,
        "OSDWindowMovable", priv->movable);
    g_key_file_set_boolean(priv->keyfile, DESKLRC_ID,
        "DrawStroke", priv->draw_stroke);
    g_key_file_set_boolean(priv->keyfile, DESKLRC_ID,
        "ShowTwoLine", priv->two_line);
}

static void rc_plugin_desklrc_apply_movable(RCPluginDesklrcPrivate *priv)
{
    cairo_region_t *region;
    GdkWindow *window;
    if(priv==NULL || priv->window==NULL) return;
    window = gtk_widget_get_window(priv->window);
    if(window==NULL) return;
    if(priv->movable)
        gdk_window_input_shape_combine_region(window, NULL, 0, 0);
    else
    {
        region = cairo_region_create();
        gdk_window_input_shape_combine_region(window, region, 0, 0);
        cairo_region_destroy(region);
    }
}

static void rc_plugin_desklrc_shutdown_cb(RCLibPlugin *plugin,
    gpointer data)
{
    RCPluginDesklrcPrivate *priv = (RCPluginDesklrcPrivate *)data;
    if(data==NULL) return;
    rc_plugin_desklrc_save_conf(priv);
}

static gboolean rc_plugin_desklrc_load(RCLibPluginData *plugin)
{
    RCPluginDesklrcPrivate *priv = &desklrc_priv;
    PangoFontDescription *desc;
    GdkScreen *screen;
    GdkVisual *visual;
    gint width, height;
    GdkGeometry window_hints =
    {
        .min_width = 500,
        .min_height = 50,
        .base_width = 600,
        .base_height = 50
    };
    screen = gdk_screen_get_default();
    if(!gdk_screen_is_composited(screen))
    {
        g_warning("DeskLRC: Composite effect is not availabe, "
            "this plug-in cannot be enabled!");
        return FALSE;
    }
    priv->window = gtk_window_new(GTK_WINDOW_POPUP);
    screen = gtk_widget_get_screen(priv->window);
    visual = gdk_screen_get_rgba_visual(screen);
    if(visual!=NULL)
        gtk_widget_set_visual(priv->window, visual);
    else
    {
        g_warning("DeskLRC: Transparent is NOT supported!");
        gtk_widget_destroy(priv->window);
        priv->window = NULL;
        return FALSE;
    }
    g_object_set(priv->window, "title", _("OSD Lyric Window"),
        "app-paintable", TRUE, "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
        "window-position", GTK_WIN_POS_CENTER, "decorated", FALSE, NULL);    
    priv->layout = gtk_widget_create_pango_layout(priv->window, NULL);
    desc = pango_font_description_from_string(priv->font_string);
    pango_layout_set_font_description(priv->layout, desc);
    pango_font_description_free(desc);
    pango_layout_set_text(priv->layout, _("RhythmCat Music Player"), -1);
    pango_layout_get_pixel_size(priv->layout, &width, &height);
    gtk_window_set_geometry_hints(GTK_WINDOW(priv->window), 
        GTK_WIDGET(priv->window), &window_hints,
        GDK_HINT_MIN_SIZE);
    gtk_window_resize(GTK_WINDOW(priv->window), priv->osd_window_width,
        100);
    if(priv->osd_window_pos_x>=0 && priv->osd_window_pos_y>=0)
    {
        gtk_window_move(GTK_WINDOW(priv->window), priv->osd_window_pos_x,
            priv->osd_window_pos_y);
    }
    gtk_widget_add_events(priv->window, GDK_ENTER_NOTIFY_MASK |
        GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
        GDK_POINTER_MOTION_HINT_MASK);
    gtk_widget_realize(priv->window);
    rc_plugin_desklrc_apply_movable(priv);
    g_signal_connect(priv->window, "draw",
        G_CALLBACK(rc_plugin_desklrc_draw), priv);
    g_signal_connect(priv->window, "button-press-event",
        G_CALLBACK(rc_plugin_desklrc_drag), priv);
    g_signal_connect(priv->window, "motion-notify-event",
        G_CALLBACK(rc_plugin_desklrc_drag), priv);
    g_signal_connect(priv->window, "button-release-event",
        G_CALLBACK(rc_plugin_desklrc_drag), priv);
    g_signal_connect(priv->window, "enter-notify-event",
        G_CALLBACK(rc_plugin_desklrc_drag), priv);
    g_signal_connect(priv->window, "leave-notify-event",
        G_CALLBACK(rc_plugin_desklrc_drag), priv);
    priv->timeout_id = g_timeout_add(100,
        (GSourceFunc)rc_plugin_desklrc_update, priv);
    gtk_widget_show_all(priv->window);
    return TRUE;
}

static gboolean rc_plugin_desklrc_unload(RCLibPluginData *plugin)
{
    RCPluginDesklrcPrivate *priv = &desklrc_priv;
    if(priv->timeout_id>0)
        g_source_remove(priv->timeout_id);
    if(priv->window!=NULL)
    {
        gtk_widget_destroy(priv->window);
        priv->window = NULL;
    }
    return TRUE;
}

static gboolean rc_plugin_desklrc_configure(RCLibPluginData *plugin)
{
    RCPluginDesklrcPrivate *priv = &desklrc_priv;
    PangoFontDescription *desc;
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label[4];
    GtkWidget *grid;
    GtkWidget *color_grid;
    GtkWidget *font_button;
    GtkWidget *bg_color_button1;
    GtkWidget *bg_color_button2;
    GtkWidget *hi_color_button1;
    GtkWidget *hi_color_button2;
    GtkWidget *window_width_spin;
    GtkWidget *window_movable_checkbox;
    GtkWidget *draw_stroke_checkbox;
    GtkWidget *two_line_mode_checkbox;
    gint i, result;
    dialog = gtk_dialog_new_with_buttons(_("Desktop Lyric Preferences"), NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    grid = gtk_grid_new();
    color_grid = gtk_grid_new();
    label[0] = gtk_label_new(_("Font: "));
    label[1] = gtk_label_new(_("Text Color: "));
    label[2] = gtk_label_new(_("High-light Color: "));
    label[3] = gtk_label_new(_("OSD Window Width: "));
    if(priv->font_string)
        font_button = gtk_font_button_new_with_font(priv->font_string);
    else
        font_button = gtk_font_button_new();
    bg_color_button1 = gtk_color_button_new_with_rgba(&(priv->bg_color1));
    bg_color_button2 = gtk_color_button_new_with_rgba(&(priv->bg_color2));
    hi_color_button1 = gtk_color_button_new_with_rgba(&(priv->fg_color1));
    hi_color_button2 = gtk_color_button_new_with_rgba(&(priv->fg_color2));
    window_width_spin = gtk_spin_button_new_with_range(0, 4000, 1);
    window_movable_checkbox = gtk_check_button_new_with_mnemonic(
        _("Movable OSD Window"));
    draw_stroke_checkbox = gtk_check_button_new_with_mnemonic(
        _("Draw strokes on the lyric text"));
    two_line_mode_checkbox = gtk_check_button_new_with_mnemonic(
        _("Draw two lines of lyric text"));
    for(i=0;i<4;i++)
    {
        g_object_set(label[i], "xalign", 0.0, "yalign", 0.5, NULL);
    }
    g_object_set(window_width_spin, "numeric", FALSE, "value",
        (gdouble)priv->osd_window_width, "hexpand-set", TRUE, "hexpand",
        TRUE, "vexpand-set", TRUE, "vexpand", FALSE, NULL);       
    g_object_set(font_button, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(bg_color_button1, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(bg_color_button2, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(hi_color_button1, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(hi_color_button2, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(window_movable_checkbox, "hexpand-set", TRUE, "hexpand",
        TRUE, "vexpand-set", TRUE, "vexpand", FALSE, "active",
        priv->movable, NULL);
    g_object_set(draw_stroke_checkbox, "hexpand-set", TRUE, "hexpand",
        TRUE, "vexpand-set", TRUE, "vexpand", FALSE, "active",
        priv->draw_stroke, NULL);
    g_object_set(two_line_mode_checkbox, "hexpand-set", TRUE, "hexpand",
        TRUE, "vexpand-set", TRUE, "vexpand", FALSE, "active",
        priv->two_line, NULL);
    g_object_set(color_grid, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);    
    gtk_grid_attach(GTK_GRID(color_grid), label[1], 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(color_grid), bg_color_button1, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(color_grid), bg_color_button2, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(color_grid), label[2], 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(color_grid), hi_color_button1, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(color_grid), hi_color_button2, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label[0], 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), font_button, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), color_grid, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), label[3], 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), window_width_spin, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), window_movable_checkbox, 0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), draw_stroke_checkbox, 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), two_line_mode_checkbox, 0, 5, 2, 1);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    if(result==GTK_RESPONSE_ACCEPT)
    {
        g_free(priv->font_string);
        priv->font_string = g_strdup(gtk_font_button_get_font_name(
            GTK_FONT_BUTTON(font_button)));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(bg_color_button1),
            &(priv->bg_color1));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(bg_color_button2),
            &(priv->bg_color2));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(hi_color_button1),
            &(priv->fg_color1));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(hi_color_button2),
            &(priv->fg_color2));
        priv->osd_window_width = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(window_width_spin));
        priv->movable = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(window_movable_checkbox));
        priv->draw_stroke = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(draw_stroke_checkbox));
        priv->two_line = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(two_line_mode_checkbox));
        desc = pango_font_description_from_string(priv->font_string);
        pango_layout_set_font_description(priv->layout, desc);
        pango_font_description_free(desc);
        rc_plugin_desklrc_apply_movable(priv);
        if(priv->window!=NULL)
            gtk_widget_queue_draw(priv->window);
        rc_plugin_desklrc_save_conf(priv);
    }
    gtk_widget_destroy(dialog);
    return TRUE;
}

static gboolean rc_plugin_desklrc_init(RCLibPluginData *plugin)
{
    RCPluginDesklrcPrivate *priv = &desklrc_priv;
    GdkScreen *screen;
    gint screen_width, screen_height;
    priv->movable = TRUE;
    priv->two_line = TRUE;
    priv->font_string = g_strdup("Monospace 22");
    gdk_rgba_parse(&(priv->bg_color1), "#4CFFFF");
    gdk_rgba_parse(&(priv->bg_color2), "#0000FF");
    gdk_rgba_parse(&(priv->fg_color1), "#FF4C4C");
    gdk_rgba_parse(&(priv->fg_color2), "#FFFF00");
    screen = gdk_screen_get_default();
    screen_width = gdk_screen_get_width(screen);
    screen_height = gdk_screen_get_height(screen);
    if(screen_width>0)
    {
        priv->osd_window_width = 0.8 * screen_width;
        priv->osd_window_pos_x = 0.1 * screen_width;
    }
    else
    {
        priv->osd_window_width = 500;
        priv->osd_window_pos_x = 100;
    }
    if(screen_height>0)
        priv->osd_window_pos_y = 0.8 * screen_height;
    else
        priv->osd_window_pos_y = 100;
    priv->keyfile = rclib_plugin_get_keyfile();
    rc_plugin_desklrc_load_conf(priv);
    rclib_plugin_signal_connect("shutdown",
        G_CALLBACK(rc_plugin_desklrc_shutdown_cb), priv);
    return TRUE;
}

static void rc_plugin_desklrc_destroy(RCLibPluginData *plugin)
{
    RCPluginDesklrcPrivate *priv = &desklrc_priv;
    g_free(priv->font_string);
}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = DESKLRC_ID,
    .name = N_("Desktop Lyric Plug-in"),
    .version = "0.5",
    .description = N_("Show lyrics on your desktop~!"),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_desklrc_load,
    .unload = rc_plugin_desklrc_unload,
    .configure = rc_plugin_desklrc_configure,
    .destroy = rc_plugin_desklrc_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_desklrc_init, rcplugin_info);



