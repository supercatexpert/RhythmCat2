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

#define LRCSHOW_ID "rc2-lyric-show"

typedef struct RCPluginLrcshowPriv
{
    GtkWidget *lrc_window;
    GtkWidget *lrc_scene;
    PangoLayout *layout;
    gint mode;
    guint track;
    gchar *font_string;
    guint line_distance;
    gint drag_to_linenum;
    gint drag_from_linenum;
    gint drag_height;
    GdkRGBA background;
    GdkRGBA text_color;
    GdkRGBA text_hilight;
    gboolean drag_flag;
    gboolean drag_action;
    gulong lyric_found_id;
    gulong timeout_id;
    GKeyFile *keyfile;
}RCPluginLrcshowPriv;

static RCPluginLrcshowPriv lrcshow_priv = {0};

static void rc_plugin_lrcshow_load_conf(RCPluginLrcshowPriv *priv)
{
    gchar *string = NULL;
    if(priv==NULL) return;
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
}

static void rc_plugin_lrcshow_save_conf(RCPluginLrcshowPriv *priv)
{
    gchar *string;
    if(priv==NULL) return;
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
    gint width = 400, height = 100;
    GtkAllocation allocation;
    GSequenceIter *iter_now, *iter_foreach;
    gchar *text;
    const RCLibLyricData *lrc_data = NULL;
    gfloat lrc_height, lrc_width;
    gint t_height, t_width;
    gfloat lrc_x, lrc_y;
    gfloat lrc_y_offset = 0.0;
    gfloat line_height;
    gfloat percent = 0.0;
    gint64 time_passed = 0;
    gint index = 0, index_now = -1;
    if(widget==NULL || priv==NULL || cr==NULL) return;
    if(iter!=NULL)
        iter_now = iter;
    else
        iter_now = iter_begin;
    if(iter_now==NULL) return;
    gtk_widget_get_allocation(widget, &allocation);
    width = allocation.width;
    height = allocation.height;
    pango_layout_set_text(priv->layout, "FontSizeTest", -1);
    pango_layout_get_size(priv->layout, &t_width, &t_height);
    lrc_height = (gdouble)t_height / PANGO_SCALE;
    line_height = lrc_height + priv->line_distance;
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
        index_now = priv->drag_to_linenum;
    }
    else if(iter!=NULL)
    {
        lrc_data = g_sequence_get(iter);
        if(lrc_data!=NULL)
            time_passed = pos - lrc_data->time+offset;
        index_now = g_sequence_iter_get_position(iter);
    }
    if(lrc_data!=NULL && lrc_data->length>0 && !priv->drag_action)
    {
        percent = (gdouble)time_passed / lrc_data->length;
        lrc_y_offset = line_height * percent;
    }
    iter_foreach = g_sequence_iter_move(iter_now,
        -(gfloat)allocation.height/2/line_height);
    index = g_sequence_iter_get_position(iter_foreach);
    lrc_y = allocation.height/2 + line_height*(index-index_now-1);
    lrc_y -= lrc_y_offset;
    if(priv->drag_action) iter = iter_now;
    for(;!g_sequence_iter_is_end(iter_foreach);
        iter_foreach = g_sequence_iter_next(iter_foreach))
    {
        lrc_data = g_sequence_get(iter_foreach);
        if(lrc_data==NULL) continue;
        index = g_sequence_iter_get_position(iter_foreach);
        pango_layout_set_text(priv->layout, lrc_data->text, -1);
        pango_layout_get_size(priv->layout, &t_width, &t_height);
        lrc_width = (gfloat)t_width/PANGO_SCALE;
        lrc_x = (width - lrc_width) / 2;
        if(iter_foreach==iter)
        {
            if(lrc_width>width && !priv->drag_action)
                lrc_x = 10 + (width-lrc_width-20)*percent;
            cairo_move_to(cr, lrc_x, lrc_y);
            gdk_cairo_set_source_rgba(cr, &(priv->text_hilight));
        }
        else
        {
            cairo_move_to(cr, lrc_x, lrc_y);
            gdk_cairo_set_source_rgba(cr, &(priv->text_color));
        }
        pango_cairo_show_layout(cr, priv->layout);
        lrc_y+=(gfloat)t_height / PANGO_SCALE+priv->line_distance;
        if(lrc_y>height) break;
    }
    if(priv->drag_action)
    {
        cairo_move_to(cr, 0, height / 2);
        cairo_set_source_rgba(cr, priv->text_color.red,
            priv->text_color.green, priv->text_color.blue,
            priv->text_color.alpha * 0.8);
        cairo_line_to(cr, width, height / 2);
        cairo_stroke(cr);
        lrc_data = g_sequence_get(iter_now);
        if(lrc_data!=NULL)
            time_passed = (lrc_data->time+offset) / GST_SECOND;
        else
            time_passed = 0;
        text = g_strdup_printf("%02d:%02d", (gint)time_passed/60,
            (gint)time_passed%60);
        pango_layout_set_text(priv->layout, text, -1);
        g_free(text);
        pango_layout_get_size(priv->layout, &t_width, &t_height);
        cairo_move_to(cr, width - t_width/PANGO_SCALE,
            height/2 - t_height/PANGO_SCALE);
        pango_cairo_show_layout(cr, priv->layout);
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
    RCLibLyricParsedData *parsed_data;
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
                gdk_cursor_unref(cursor);
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
                gdk_cursor_unref(cursor);
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
    RCLibLyricParsedData *parsed_data;
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
    if(data==NULL) return;
    if(priv->mode>0)
    {
        if(index>0) priv->track = 1;
        else priv->track = 0;
    }
    else priv->track = 0;
}

static gboolean rc_plugin_lrcshow_load(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    PangoFontDescription *desc;
    priv->lrc_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    priv->lrc_scene = gtk_drawing_area_new();
    gtk_widget_set_size_request(priv->lrc_window, 300, 400);
    gtk_widget_add_events(priv->lrc_scene, GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK |
        GDK_POINTER_MOTION_HINT_MASK);
    priv->layout = gtk_widget_create_pango_layout(priv->lrc_scene, NULL);
    desc = pango_font_description_from_string(priv->font_string);
    pango_layout_set_font_description(priv->layout, desc);
    pango_font_description_free(desc);
    gtk_container_add(GTK_CONTAINER(priv->lrc_window), priv->lrc_scene);
    gtk_widget_show_all(priv->lrc_window);
    g_signal_connect(priv->lrc_scene, "draw",
        G_CALLBACK(rc_plugin_lrcshow_draw), priv);
    g_signal_connect(priv->lrc_scene, "button-press-event",
        G_CALLBACK(rc_plugin_lrcshow_drag), priv);
    g_signal_connect(priv->lrc_scene, "motion-notify-event",
        G_CALLBACK(rc_plugin_lrcshow_drag), priv);
    g_signal_connect(priv->lrc_scene, "button-release-event",
        G_CALLBACK(rc_plugin_lrcshow_drag), priv);
    priv->lyric_found_id = rclib_lyric_signal_connect("lyric-ready",
        G_CALLBACK(rc_plugin_lrcshow_lyric_ready_cb), priv);
    priv->timeout_id = g_timeout_add(100,
        (GSourceFunc)rc_plugin_lrcshow_update, priv);
    return TRUE;
}

static gboolean rc_plugin_lrcshow_unload(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    g_source_remove(priv->timeout_id);
    if(priv->lyric_found_id>0)
        rclib_lyric_signal_disconnect(priv->lyric_found_id);
    if(priv->layout!=NULL)
        g_object_unref(priv->layout);
    if(priv->lrc_window!=NULL)
        gtk_widget_destroy(priv->lrc_window);
    return TRUE;
}

static gboolean rc_plugin_lrcshow_configure(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    PangoFontDescription *desc;
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
        desc = pango_font_description_from_string(priv->font_string);
        pango_layout_set_font_description(priv->layout, desc);
        pango_font_description_free(desc);
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
    gdk_rgba_parse(&(priv->background), "#3B5673");
    gdk_rgba_parse(&(priv->text_color), "#FFFFFF");
    gdk_rgba_parse(&(priv->text_hilight), "#5CA6D6");
    priv->font_string = g_strdup("Monospace 10");
    priv->drag_flag = TRUE;
    priv->keyfile = rclib_plugin_get_keyfile();
    rc_plugin_lrcshow_load_conf(priv);
    return TRUE;
}

static void rc_plugin_lrcshow_destroy(RCLibPluginData *plugin)
{
    RCPluginLrcshowPriv *priv = &lrcshow_priv;
    rc_plugin_lrcshow_save_conf(priv);
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

