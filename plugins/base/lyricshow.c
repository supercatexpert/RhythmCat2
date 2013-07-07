/*
 * Lyric Show Plugin
 * Show lyric in a  window.
 *
 * lyricshow.c
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

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <rclib.h>
#include <rc-ui-menu.h>

#define LRCSHOW_ID "rc2-native-lyric-show"

typedef struct RCPluginLrcshowPriv
{
    GtkWidget *lrc_window;
    GtkWidget *lrc_scene;
    GtkToggleAction *action;
    guint menu_id;
    PangoLayout *layout;
    cairo_surface_t **lrc_normal_surface;
    cairo_surface_t **lrc_active_surface;
    guint lrc_number;
    gint mode;
    guint track;
    gchar *font_string;
    gint font_height;
    guint line_distance;
    gint drag_to_linenum;
    gint drag_from_linenum;
    gint drag_height;
    gint window_x;
    gint window_y;
    GdkRGBA background;
    GdkRGBA text_color;
    GdkRGBA text_hilight;
    gboolean drag_flag;
    gboolean drag_action;
    gulong lyric_found_id;
    gulong timeout_id;
    gulong shutdown_id;
    gboolean show_window;
    GKeyFile *keyfile;
}RCPluginLrcshowPriv;

static RCPluginLrcshowPriv lrcshow_priv = {0};

static inline void rc_plugin_lrcshow_update_font_height(
    RCPluginLrcshowPriv *priv)
{
    PangoContext *context;
    PangoFontMetrics *metrics;
    gint ascent, descent;
    if(priv==NULL || priv->layout==NULL) return;
    context = pango_layout_get_context(priv->layout);
    if(context==NULL) return;
    metrics = pango_context_get_metrics(context,
        pango_layout_get_font_description(priv->layout), NULL);
    if(metrics==NULL)
    {
        g_warning("Cannot get font metrics!");
    }
    priv->font_height = 0;
    ascent = pango_font_metrics_get_ascent(metrics);
    descent = pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);
    priv->font_height += PANGO_PIXELS(ascent + descent);
}

static inline void rc_plugin_lrcshow_update_font(RCPluginLrcshowPriv *priv)
{
    PangoFontDescription *desc;
    if(priv==NULL || priv->layout==NULL) return;
    if(priv->font_string!=NULL)
        desc = pango_font_description_from_string(priv->font_string);
    else
        desc = pango_font_description_from_string("Monospace 10");
    pango_layout_set_font_description(priv->layout, desc);
    pango_font_description_free(desc);
    rc_plugin_lrcshow_update_font_height(priv);
}

static inline void rc_plugin_lrcshow_set_font(RCPluginLrcshowPriv *priv,
    const gchar *font_string)
{
    if(priv==NULL || priv->layout==NULL) return;
    g_free(priv->font_string);
    priv->font_string = g_strdup(font_string);
}

static void rc_plugin_lrcshow_load_conf(RCPluginLrcshowPriv *priv)
{
    gchar *string = NULL;
    gboolean bvalue;
    gint ivalue;
    GError *error = NULL;
    if(priv==NULL || priv->keyfile==NULL) return;
    string = g_key_file_get_string(priv->keyfile, LRCSHOW_ID, "Font",
        NULL);
    if(string!=NULL)
    {
        g_free(priv->font_string);
        priv->font_string = string;
    }
    priv->line_distance = g_key_file_get_integer(priv->keyfile, LRCSHOW_ID,
        "LineDistance", NULL);
    if(priv->line_distance<0) priv->line_distance = 0;
    string = g_key_file_get_string(priv->keyfile, LRCSHOW_ID,
        "BackgroundColor", NULL);
    if(string!=NULL)
    {
        if(!gdk_rgba_parse(&(priv->background), string))
            gdk_rgba_parse(&(priv->background), "#3B5673");
    }
    else
        gdk_rgba_parse(&(priv->background), "#3B5673");
    g_free(string);
    string = g_key_file_get_string(priv->keyfile, LRCSHOW_ID,
        "TextNormalColor", NULL);
    if(string!=NULL)
    {
        if(!gdk_rgba_parse(&(priv->text_color), string))
            gdk_rgba_parse(&(priv->text_color), "#FFFFFF");
    }
    else
        gdk_rgba_parse(&(priv->text_color), "#FFFFFF");
    g_free(string);
    string = g_key_file_get_string(priv->keyfile, LRCSHOW_ID,
        "TextHighLightColor", NULL);
    if(string!=NULL)
    {
        if(!gdk_rgba_parse(&(priv->text_hilight), string))
            gdk_rgba_parse(&(priv->text_hilight), "#5CA6D6");
    }
    else
        gdk_rgba_parse(&(priv->text_hilight), "#5CA6D6");
    g_free(string);
    priv->mode = g_key_file_get_integer(priv->keyfile, LRCSHOW_ID,
        "Mode", NULL);
    if(priv->mode<0 || priv->mode>2) priv->mode = 0;
    bvalue = g_key_file_get_boolean(priv->keyfile, LRCSHOW_ID,
        "ShowWindow", &error);
    if(error==NULL)
        priv->show_window = bvalue;
    else
    {
        g_error_free(error);
        error = NULL;
    }
    ivalue = g_key_file_get_integer(priv->keyfile, LRCSHOW_ID,
        "WindowPositionX", &error);
    if(error==NULL)
        priv->window_x = ivalue;
    else
    {
        priv->window_x = -1;
        g_error_free(error);
        error = NULL;
    }
    ivalue = g_key_file_get_integer(priv->keyfile, LRCSHOW_ID,
        "WindowPositionY", &error);
    if(error==NULL)
        priv->window_y = ivalue;
    else
    {
        priv->window_y = -1;
        g_error_free(error);
        error = NULL;
    }
}

static void rc_plugin_lrcshow_save_conf(RCPluginLrcshowPriv *priv)
{
    gchar *string;
    if(priv==NULL || priv->keyfile==NULL) return;
    g_key_file_set_string(priv->keyfile, LRCSHOW_ID, "Font",
        priv->font_string);
    g_key_file_set_integer(priv->keyfile, LRCSHOW_ID, "LineDistance",
        priv->line_distance);
    string = gdk_rgba_to_string(&(priv->background));
    g_key_file_set_string(priv->keyfile, LRCSHOW_ID, "BackgroundColor",
        string);
    g_free(string);
    string = gdk_rgba_to_string(&(priv->text_color));
    g_key_file_set_string(priv->keyfile, LRCSHOW_ID, "TextNormalColor",
        string);
    g_free(string);
    string = gdk_rgba_to_string(&(priv->text_hilight));
    g_key_file_set_string(priv->keyfile, LRCSHOW_ID, "TextHighLightColor",
        string);
    g_free(string);
    g_key_file_set_boolean(priv->keyfile, LRCSHOW_ID, "ShowWindow",
        priv->show_window);
    if(priv->window_x>=0)
    {
        g_key_file_set_integer(priv->keyfile, LRCSHOW_ID,
            "WindowPositionX", priv->window_x);
    }
    if(priv->window_y>=0)
    {
        g_key_file_set_integer(priv->keyfile, LRCSHOW_ID,
            "WindowPositionY", priv->window_y);
    }
}

static inline void rc_plugin_lrcshow_draw_bg(RCPluginLrcshowPriv *priv,
    cairo_t *cr)
{
    gdk_cairo_set_source_rgba(cr, &(priv->background));
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
}

static void rc_plugin_lrcshow_show(GtkWidget *widget,
    RCPluginLrcshowPriv *priv, cairo_t *cr, gint64 pos, gint64 offset,
    GSequenceIter *iter, GSequenceIter *iter_begin)
{
    GtkAllocation allocation;
    cairo_surface_t *surface;
    gint surface_width, surface_height;
    GSequenceIter *iter_now, *iter_foreach;
    GSequence *seq;
    const RCLibLyricData *lrc_data = NULL;
    gint t_height;
    gfloat lrc_x, lrc_y;
    gfloat lrc_y_offset = 0.0;
    gfloat line_height;
    gfloat percent = 0.0;
    gfloat text_percent = 0.0;
    gfloat low, high;
    gint64 time_passed = 0;
    gint64 time_length = 0;
    gint index_now, index_foreach;
    if(widget==NULL || priv==NULL || priv->layout==NULL || cr==NULL)
        return;
    if(iter!=NULL)
        iter_now = iter;
    else
        iter_now = iter_begin;
    if(iter_now==NULL) return;
    if(g_sequence_iter_is_end(iter_now)) return;
    gtk_widget_get_allocation(widget, &allocation);
    t_height = priv->font_height;
    line_height = t_height + priv->line_distance;
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    if(priv->drag_action)
    {
        iter_now = g_sequence_iter_move(iter_now, (gint)(priv->drag_height /
            line_height));
        if(g_sequence_iter_is_end(iter_now))
            iter_now = g_sequence_iter_prev(iter_now);
        lrc_y_offset = priv->drag_height % (gint)line_height;
        if(iter!=NULL)
            priv->drag_to_linenum = g_sequence_iter_get_position(iter_now);
        else
            priv->drag_to_linenum = -1;
    }
    else if(iter!=NULL)
    {
        lrc_data = g_sequence_get(iter);
        if(lrc_data!=NULL)
            time_passed = pos - (lrc_data->time+offset);
    }
    if(lrc_data!=NULL && !priv->drag_action)
    {
        if(lrc_data->length>0)
            percent = (gdouble)time_passed / lrc_data->length;
        else
        {
            time_length = rclib_core_query_duration() -
                (lrc_data->time+offset);
            if(time_length>0)
                percent = (gdouble)time_passed / time_length;
        }
        lrc_y_offset = line_height * percent;
    }

    seq = g_sequence_iter_get_sequence(iter_now);
    if(iter!=NULL)
        index_now = g_sequence_iter_get_position(iter_now);
    else
        index_now = -1;
    for(iter_foreach = g_sequence_get_begin_iter(seq);
        !g_sequence_iter_is_end(iter_foreach);
        iter_foreach = g_sequence_iter_next(iter_foreach))
    {
        index_foreach = g_sequence_iter_get_position(iter_foreach);
        lrc_y = (gfloat)allocation.height/2 - lrc_y_offset +
            (t_height + priv->line_distance) * (gfloat)
            (index_foreach - index_now);
        lrc_y = roundf(lrc_y);
        if(lrc_y<0 || lrc_y>allocation.height) continue;
        if(index_foreach!=index_now)
        {
            if(priv->lrc_normal_surface==NULL) continue;
            surface = priv->lrc_normal_surface[index_foreach];
            if(surface==NULL) continue;
            surface_width = cairo_image_surface_get_width(surface);
            surface_height = cairo_image_surface_get_height(surface);
            lrc_x = roundf((allocation.width - surface_width) / 2);
            cairo_save(cr);
            cairo_rectangle(cr, lrc_x, lrc_y, surface_width, surface_height);
            cairo_clip(cr);
            cairo_set_source_surface(cr, surface, lrc_x, lrc_y);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            cairo_paint(cr);
            cairo_restore(cr);
        }
        else
        {
            if(priv->lrc_active_surface==NULL) continue;
            surface = priv->lrc_active_surface[index_foreach];
            if(surface==NULL) continue;
            surface_width = cairo_image_surface_get_width(surface);
            surface_height = cairo_image_surface_get_height(surface);
            if(surface_width>allocation.width && !priv->drag_action)
            {
                low = (gfloat)allocation.width / surface_width /2;
                high = 1.0 - low;
                if(percent<=low)
                    text_percent = 0.0;
                else if(percent<high)
                    text_percent = (percent-low) / (high-low);
                else
                    text_percent = 1.0;
                lrc_x = 10 + (allocation.width-surface_width-20)*text_percent;
            }
            else
                lrc_x = (allocation.width - surface_width) / 2;
            lrc_x = roundf(lrc_x);
            cairo_save(cr);
            cairo_rectangle(cr, lrc_x, lrc_y, surface_width, surface_height);
            cairo_clip(cr);
            cairo_set_source_surface(cr, surface, lrc_x, lrc_y);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            cairo_paint(cr);
            cairo_restore(cr);
        }
    }
}

static gboolean rc_plugin_lrcshow_drag(GtkWidget *widget, GdkEvent *event,
    gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    GdkCursor *cursor = NULL;
    GdkWindow *window;
    gint dy = 0;
    gint64 pos = 0;
    const RCLibLyricParsedData *parsed_data;
    GSequenceIter *iter;
    RCLibLyricData *lrc_data = NULL;
    static gint sy = 0;
    if(!priv->drag_flag) return FALSE;
    window = gtk_widget_get_window(widget);
    if(event->button.button==1)
    {
        switch(event->type)
        {
            case GDK_BUTTON_PRESS:
                cursor = gdk_cursor_new(GDK_HAND1);
                gdk_window_set_cursor(window, cursor);
                g_object_unref(cursor);
                priv->drag_action = TRUE;
                sy = event->button.y;
                pos = rclib_core_query_position();
                iter = rclib_lyric_get_line_iter(priv->track, pos);
                if(iter!=NULL)
                    priv->drag_from_linenum =
                        g_sequence_iter_get_position(iter);
                else
                    priv->drag_from_linenum = -1;
                break;
            case GDK_BUTTON_RELEASE:
                cursor = gdk_cursor_new(GDK_ARROW);
                gdk_window_set_cursor(window, cursor);
                g_object_unref(cursor);
                if(!priv->drag_action) break;
                priv->drag_action = FALSE;
                priv->drag_height = 0;
                if(priv->drag_from_linenum!=priv->drag_to_linenum)
                {
                    parsed_data = rclib_lyric_get_parsed_data(priv->track);
                    if(parsed_data==NULL || parsed_data->filename==NULL) break;
                    if(priv->drag_to_linenum>=0)
                    {
                        iter = g_sequence_get_iter_at_pos(parsed_data->seq,
                            priv->drag_to_linenum);
                        lrc_data = g_sequence_get(iter);
                    }
                    if(lrc_data!=NULL)
                    {
                        pos = lrc_data->time +
                            (gint64)parsed_data->offset*GST_MSECOND;
                    }
                    rclib_core_set_position(pos);
                }
                break;
            case GDK_MOTION_NOTIFY:
                if(!priv->drag_action) break;
                dy = sy - event->button.y;
                priv->drag_height = dy;
                break;
            default:
                break;
        }
    }
    return FALSE;
}

static gboolean rc_plugin_lrcshow_draw(GtkWidget *widget, cairo_t *cr,
    gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    GSequenceIter *iter, *iter_begin;
    const RCLibLyricParsedData *parsed_data;
    gint64 pos;
    if(data==NULL) return FALSE;
    rc_plugin_lrcshow_draw_bg(priv, cr);
    parsed_data = rclib_lyric_get_parsed_data(priv->track);
    if(parsed_data==NULL || parsed_data->seq==NULL)
        return FALSE;
    pos = rclib_core_query_position();
    iter = rclib_lyric_get_line_iter(priv->track, pos);
    iter_begin = g_sequence_get_begin_iter(parsed_data->seq);
    rc_plugin_lrcshow_show(widget, priv, cr, pos,
        (gint64)parsed_data->offset*GST_MSECOND, iter, iter_begin);
    return FALSE;
}

static void rc_plugin_lrcshow_view_menu_toggled(GtkToggleAction *toggle,
    gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    gboolean flag;
    if(data==NULL) return;
    flag = gtk_toggle_action_get_active(toggle);
    if(flag)
        gtk_window_present(GTK_WINDOW(priv->lrc_window));
    else
        gtk_widget_hide(priv->lrc_window);
    priv->show_window = flag;
    if(priv->keyfile!=NULL)
    {
        g_key_file_set_boolean(priv->keyfile, LRCSHOW_ID, "ShowWindow",
            flag);
    }
}

static gboolean rc_plugin_lrcshow_window_delete_event_cb(GtkWidget *widget,
    GdkEvent *event, gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    if(data==NULL) return TRUE;
    gtk_toggle_action_set_active(priv->action, FALSE);
    return TRUE;
}

static void rc_plugin_lrcshow_window_destroy_cb(GtkWidget *widget,
    gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    if(data==NULL) return;
    gtk_window_get_position(GTK_WINDOW(widget),
        &(priv->window_x), &(priv->window_y));
    gtk_widget_destroyed(priv->lrc_window, &(priv->lrc_window));
}


static gboolean rc_plugin_lrcshow_update(gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    if(data==NULL) return TRUE;
    gtk_widget_queue_draw(priv->lrc_scene);
    return TRUE;   
}

static void rc_plugin_lrcshow_lyric_ready_cb(RCLibLyric *lyric, guint index,
   gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    RCLibLyricData *lyric_data;
    cairo_t *cr;
    gint w, h;
    gint lrc_number = 0;
    gint i;
    GSequenceIter *iter, *iter_begin;
    const RCLibLyricParsedData *parsed_data;
    if(data==NULL) return;
    if(priv->mode>0)
    {
        if(index>0) priv->track = 1;
        else priv->track = 0;
    }
    else priv->track = 0;

    /* Draw the text on the image surface. */
    parsed_data = rclib_lyric_get_parsed_data(priv->track);
    if(parsed_data==NULL || parsed_data->seq==NULL)
        return;
    lrc_number = g_sequence_get_length(parsed_data->seq);
    priv->lrc_number = lrc_number;
    if(priv->lrc_normal_surface!=NULL)
    {
        for(i=0;priv->lrc_normal_surface[i]!=NULL;i++)
            cairo_surface_destroy(priv->lrc_normal_surface[i]);
        g_free(priv->lrc_normal_surface);
        priv->lrc_normal_surface = NULL;
    }
    if(priv->lrc_active_surface!=NULL)
    {
        for(i=0;priv->lrc_active_surface[i]!=NULL;i++)
            cairo_surface_destroy(priv->lrc_active_surface[i]);
        g_free(priv->lrc_active_surface);
        priv->lrc_active_surface = NULL;
    }
    priv->lrc_normal_surface = g_new0(cairo_surface_t *, lrc_number+1);
    priv->lrc_active_surface = g_new0(cairo_surface_t *, lrc_number+1);
    iter_begin = g_sequence_get_begin_iter(parsed_data->seq);
    iter = iter_begin;
    for(i=0;i<lrc_number;i++)
    {
        lyric_data = g_sequence_get(iter);
        if(lyric_data==NULL) continue;
        pango_layout_set_text(priv->layout, lyric_data->text, -1);
        pango_layout_get_pixel_size(priv->layout, &w, &h);

        priv->lrc_normal_surface[i] = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, w, h);
        cr = cairo_create(priv->lrc_normal_surface[i]);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);
        cairo_move_to(cr, 0, 0);
        gdk_cairo_set_source_rgba(cr, &(priv->text_color));
        pango_cairo_show_layout(cr, priv->layout);
        cairo_destroy(cr);

        priv->lrc_active_surface[i] = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, w, h);
        cr = cairo_create(priv->lrc_active_surface[i]);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint(cr);
        cairo_move_to(cr, 0, 0);
        gdk_cairo_set_source_rgba(cr, &(priv->text_hilight));
        pango_cairo_show_layout(cr, priv->layout);
        cairo_destroy(cr);
        iter = g_sequence_iter_next(iter);
    }
}

static void rc_plugin_lrcshow_shutdown_cb(RCLibPlugin *plugin, gpointer data)
{
    RCPluginLrcshowPriv *priv = (RCPluginLrcshowPriv *)data;
    if(data==NULL) return;
    if(priv->lrc_window!=NULL)
    {
        gtk_window_get_position(GTK_WINDOW(priv->lrc_window),
            &(priv->window_x), &(priv->window_y));
    }
    rc_plugin_lrcshow_save_conf(priv);
}

static gboolean rc_plugin_lrcshow_load(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    priv->lrc_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    priv->lrc_scene = gtk_drawing_area_new();
    gtk_widget_set_size_request(priv->lrc_window, 300, 400);
    gtk_widget_add_events(priv->lrc_scene, GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK |
        GDK_POINTER_MOTION_HINT_MASK);
    priv->layout = gtk_widget_create_pango_layout(priv->lrc_scene, NULL);
    rc_plugin_lrcshow_update_font(priv);
    priv->action = gtk_toggle_action_new("RC2ViewPluginLyricShow",
        _("Lyric Show"), _("Show/hide lyric show window"), NULL);
    gtk_toggle_action_set_active(priv->action, TRUE);
    priv->menu_id = rc_ui_menu_add_menu_action(GTK_ACTION(priv->action),
        "/RC2MenuBar/ViewMenu/ViewSep2", "RC2ViewPluginLyricShow",
        "RC2ViewPluginLyricShow", TRUE);
    g_object_set(priv->lrc_window, "title", _("Lyric Show"),
        "window-position", GTK_WIN_POS_CENTER, NULL);
    gtk_container_add(GTK_CONTAINER(priv->lrc_window), priv->lrc_scene);
    if(priv->window_x>=0 && priv->window_y>=0)
    {
        gtk_window_move(GTK_WINDOW(priv->lrc_window), priv->window_x,
            priv->window_y);
    }
    gtk_widget_show_all(priv->lrc_window);    
    g_signal_connect(priv->lrc_scene, "draw",
        G_CALLBACK(rc_plugin_lrcshow_draw), priv);
    g_signal_connect(priv->lrc_scene, "button-press-event",
        G_CALLBACK(rc_plugin_lrcshow_drag), priv);
    g_signal_connect(priv->lrc_scene, "motion-notify-event",
        G_CALLBACK(rc_plugin_lrcshow_drag), priv);
    g_signal_connect(priv->lrc_scene, "button-release-event",
        G_CALLBACK(rc_plugin_lrcshow_drag), priv);
    g_signal_connect(priv->action, "toggled",
        G_CALLBACK(rc_plugin_lrcshow_view_menu_toggled), priv);
    g_signal_connect(priv->lrc_window, "destroy",
        G_CALLBACK(rc_plugin_lrcshow_window_destroy_cb), priv);
    g_signal_connect(priv->lrc_window, "delete-event",
        G_CALLBACK(rc_plugin_lrcshow_window_delete_event_cb), priv);
    priv->lyric_found_id = rclib_lyric_signal_connect("lyric-ready",
        G_CALLBACK(rc_plugin_lrcshow_lyric_ready_cb), priv);
    priv->timeout_id = g_timeout_add(100,
        (GSourceFunc)rc_plugin_lrcshow_update, priv);
    if(!priv->show_window)
        gtk_toggle_action_set_active(priv->action, FALSE);
    rc_plugin_lrcshow_lyric_ready_cb(NULL, 0, priv);
    return TRUE;
}

static gboolean rc_plugin_lrcshow_unload(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    if(priv->menu_id>0)
    {
        rc_ui_menu_remove_menu_action(GTK_ACTION(priv->action),
            priv->menu_id);
        g_object_unref(priv->action);
    }
    if(priv->timeout_id>0)
        g_source_remove(priv->timeout_id);
    if(priv->lyric_found_id>0)
    {
        rclib_lyric_signal_disconnect(priv->lyric_found_id);
        priv->lyric_found_id = 0;
    }
    if(priv->layout!=NULL)
        g_object_unref(priv->layout);
    if(priv->lrc_window!=NULL)
    {
        gtk_widget_destroy(priv->lrc_window);
        priv->lrc_window = NULL;
    }
    return TRUE;
}

static gboolean rc_plugin_lrcshow_configure(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *label[5];
    GtkWidget *grid;
    GtkWidget *font_button;
    GtkWidget *space_spin;
    GtkWidget *bg_color_button;
    GtkWidget *fg_color_button;
    GtkWidget *hi_color_button;
    gint i, result;
    dialog = gtk_dialog_new_with_buttons(_("Lyric Show Preferences"), NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    grid = gtk_grid_new();
    label[0] = gtk_label_new(_("Font: "));
    label[1] = gtk_label_new(_("Line spacing: "));
    label[2] = gtk_label_new(_("Background Color: "));
    label[3] = gtk_label_new(_("Frontground Color: "));
    label[4] = gtk_label_new(_("Highlight Color: "));
    if(priv->font_string!=NULL)
        font_button = gtk_font_button_new_with_font(priv->font_string);
    else
        font_button = gtk_font_button_new();
    space_spin = gtk_spin_button_new_with_range(0, 100, 1);
    bg_color_button = gtk_color_button_new_with_rgba(&(priv->background));
    fg_color_button = gtk_color_button_new_with_rgba(&(priv->text_color));
    hi_color_button = gtk_color_button_new_with_rgba(&(priv->text_hilight));
    for(i=0;i<5;i++)
    {
        g_object_set(label[i], "xalign", 0.0, "yalign", 0.5, NULL);
    }
    g_object_set(space_spin, "numeric", FALSE, "value",
        (gdouble)priv->line_distance, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(font_button, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(bg_color_button, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(fg_color_button, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);    
    g_object_set(hi_color_button, "hexpand-set", TRUE, "hexpand", TRUE,
        "vexpand-set", TRUE, "vexpand", FALSE, NULL);
    g_object_set(grid, "row-spacing", 4, "column-spacing", 4, "expand",
        TRUE, NULL);
    gtk_grid_attach(GTK_GRID(grid), label[0], 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), font_button, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label[1], 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), space_spin, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label[2], 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), bg_color_button, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label[3], 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), fg_color_button, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label[4], 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), hi_color_button, 1, 4, 1, 1);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    if(result==GTK_RESPONSE_ACCEPT)
    {
        g_free(priv->font_string);
        priv->font_string = g_strdup(gtk_font_button_get_font_name(
            GTK_FONT_BUTTON(font_button)));
        rc_plugin_lrcshow_update_font(priv);
        priv->line_distance = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(space_spin));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(bg_color_button),
            &(priv->background));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(fg_color_button),
            &(priv->text_color));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(hi_color_button),
            &(priv->text_hilight));
        rc_plugin_lrcshow_save_conf(priv);
    }
    gtk_widget_destroy(dialog);
    return TRUE;
}

static gboolean rc_plugin_lrcshow_init(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    priv->show_window = TRUE;
    priv->window_x = -1;
    priv->window_y = -1;
    gdk_rgba_parse(&(priv->background), "#3B5673");
    gdk_rgba_parse(&(priv->text_color), "#FFFFFF");
    gdk_rgba_parse(&(priv->text_hilight), "#5CA6D6");
    priv->font_string = g_strdup("Monospace 10");
    priv->drag_flag = TRUE;
    priv->keyfile = rclib_plugin_get_keyfile();
    rc_plugin_lrcshow_load_conf(priv);
    priv->shutdown_id = rclib_plugin_signal_connect("shutdown",
        G_CALLBACK(rc_plugin_lrcshow_shutdown_cb), priv);
    return TRUE;
}

static void rc_plugin_lrcshow_destroy(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    if(priv->shutdown_id>0)
        rclib_plugin_signal_disconnect(priv->shutdown_id);
    g_free(priv->font_string);
}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = LRCSHOW_ID,
    .name = N_("Lyric Show Plug-in"),
    .version = "0.5",
    .description = N_("Show lyric in the player or in a single window."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_lrcshow_load,
    .unload = rc_plugin_lrcshow_unload,
    .configure = rc_plugin_lrcshow_configure,
    .destroy = rc_plugin_lrcshow_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_lrcshow_init, rcplugin_info);

