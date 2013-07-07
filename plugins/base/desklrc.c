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

#define DESKLRC_ID "rc2-native-desktop-lyric"

#define DESKLRC_MAX_LINE_NUMBER 4

#define DESKLRC_OSD_WINDOW_MIN_WIDTH 320
#define DESKLRC_DEFAULT_FONT_NAME "Monospace 22"
#define DESKLRC_DEFAULT_OUTLINE_WIDTH 3
#define DESKLRC_BORDER_WIDTH 5
#define DESKLRC_DEFAULT_WIDTH 1024

#define DESKLRC_MARGIN_TOP 10
#define DESKLRC_MARGIN_BOTTOM 10
#define DESKLRC_MARGIN_LEFT 10
#define DESKLRC_MARGIN_RIGHT 10
#define DESKLRC_LINE_PADDING 5

typedef enum {
    DESKLRC_WINDOW_NORMAL,
    DESKLRC_WINDOW_DOCK,
}DesklrcWindowMode;

typedef enum {
    DESKLRC_LAYOUT_TEXT1_LINE1 = 0,
    DESKLRC_LAYOUT_TEXT1_LINE2 = 1,
    DESKLRC_LAYOUT_TEXT2_LINE1 = 2,
    DESKLRC_LAYOUT_TEXT2_LINE2 = 3,
    DESKLRC_LAYOUT_LAST = 4
}DesklrcTextLayout;

typedef struct DesklrcRenderContext
{
    gchar *font_name;
    gint outline_width;
    GdkRGBA linear_normal_colors[3];
    GdkRGBA linear_active_colors[3];
    gdouble linear_pos[3];
    PangoContext *pango_context;
    PangoLayout *pango_layout;
    gchar *text;
    gdouble blur_radius;
    gint font_height;
}DesklrcRenderContext;

typedef struct DesklrcPrivate
{
    GtkWidget *window;
    DesklrcRenderContext *render_context;
    DesklrcWindowMode mode;
    cairo_surface_t *lyric_surface[DESKLRC_LAYOUT_LAST];
    cairo_surface_t *lyric_active_surface[2];
    gchar *lyric_text[DESKLRC_LAYOUT_LAST];
    gdouble lyric_percent[2];
    gint lyric_line_num[2];
    gboolean composited;
    gboolean movable;
    gboolean notify_flag;
    gboolean update_shape;
    PangoLayout *layout;
    GdkRGBA bg_color1;
    GdkRGBA bg_color2;
    GdkRGBA bg_color3;
    GdkRGBA fg_color1;
    GdkRGBA fg_color2;
    GdkRGBA fg_color3;
    GdkPixbuf *bg_pixbuf;
    gchar *font_string;
    gboolean two_line;
    gboolean two_track;
    gint osd_window_width;
    gint osd_window_pos_x;
    gint osd_window_pos_y;
    gulong timeout_id;
    gulong shutdown_id;
    GKeyFile *keyfile;
}DesklrcPrivate;

typedef struct DesklrcPixel
{
    guint64 alpha;
    guint64 red;
    guint64 green;
    guint64 blue;
}DesklrcPixel;

static DesklrcPrivate desklrc_priv = {0};

static inline gint desklrc_pos_to_index(gint x, gint y,
    gint width, gint height)
{
    if(x >= width || y >= height || x < 0 || y < 0)
        return -1;
    return y * width + x;
}

static inline void desklrc_num_to_pixel_with_factor(
    DesklrcPixel *pixel, guint32 value, gint factor)
{
    if(pixel==NULL) return;
    /* This only works for type CAIRO_FORMAT_ARGB32 */
    
    pixel->alpha = ((value >> 24) & 0xff) * factor;
    pixel->red = ((value >> 16) & 0xff) * factor;
    pixel->green = ((value >> 8) & 0xff) * factor;
    pixel->blue = (value & 0xff) * factor;
}

static inline guint32 desklrc_pixel_to_num_with_divisor(
    DesklrcPixel *pixel, gint divisor)
{
    guint32 alpha = pixel->alpha / divisor;
    if(alpha > 0xff) alpha = 0xff;
    guint32 red = pixel->red / divisor;
    if(red > 0xff) red = 0xff;
    guint32 green = pixel->green / divisor;
    if(green > 0xff) green = 0xff;
    guint32 blue = pixel->blue / divisor;
    if(blue > 0xff) blue = 0xff;
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

static inline void desklrc_pixel_plus(
    DesklrcPixel *adder_sum, DesklrcPixel *adder2)
{
    adder_sum->alpha += adder2->alpha;
    adder_sum->red += adder2->red;
    adder_sum->green += adder2->green;
    adder_sum->blue += adder2->blue;
}

static gint *desklrc_calc_kernel(gdouble sigma, gint *size)
{
    gint kernel_size = ceil (sigma * 6);
    if(kernel_size%2==0) kernel_size++;
    gint orig = kernel_size / 2;
    if(size) *size = kernel_size;
    gdouble *kernel_double = g_new(double, kernel_size);
    gdouble sum = 0.0;
    gint *kernel = g_new(int, kernel_size);
    gint i;
    gdouble factor = 1.0 / sqrt (2.0 * M_PI * sigma * sigma);
    gdouble denom = 1.0 / (2.0 * sigma * sigma);
    for(i=0;i<kernel_size;i++)
    {
        kernel_double[i] = factor*exp(-(i - orig)*(i - orig)*denom);
        sum+=kernel_double[i];
    }
    /* convert to pixed point number */
    for(i=0;i<kernel_size;i++)
    {
        kernel[i] = kernel_double[i]/sum*(1<<(sizeof(gint)/2*8));
    }
    g_free(kernel_double);
    return kernel;
}

static void desklrc_apply_kernel(cairo_surface_t *surface,
    const gint *kernel, gint kernel_size)
{
    if(kernel==NULL) return;
    if(kernel_size<=0 || kernel_size%2==0) return;
    static const int DIR[2][2] = {{0, 1}, {1, 0}};
    guint32 *pixels = (guint32 *)cairo_image_surface_get_data(surface);
    gint width = cairo_image_surface_get_width(surface);
    gint height = cairo_image_surface_get_height(surface);
    if(pixels==NULL || width <= 0 || height <= 0)
    {
        g_warning("Invalid image surface");
        return;
    }
    gint kernel_orig = kernel_size / 2;
    gint d, i, x, y;
    for(d=0;d<2;d++)
    {
        guint32 *old_pixels = g_new(guint32, width * height);
        memcpy(old_pixels, pixels, sizeof(guint32)*width*height);
        for(x=0;x<width;x++)
        {
            for (y = 0; y < height; y++)
            {
                DesklrcPixel final_value = {0};
                gint sum = 0;
                for(i=0;i<kernel_size;i++)
                {
                    gint x1 = x + (i - kernel_orig) * DIR[d][0];
                    gint y1 = y + (i - kernel_orig) * DIR[d][1];
                    gint index1 = desklrc_pos_to_index(x1, y1,
                        width, height);
                    if(index1>0)
                    {
                        sum += kernel[i];
                        DesklrcPixel value;
                        desklrc_num_to_pixel_with_factor(&value,
                            old_pixels[index1], kernel[i]);
                        desklrc_pixel_plus (&final_value, &value);
                    }
                }
                gint index = desklrc_pos_to_index(x, y, width,
                    height);
                pixels[index] = desklrc_pixel_to_num_with_divisor(
                    &final_value, sum);
            }
        }
        g_free(old_pixels);
    }
}

/* The blur shadow effect function from OSDLyric. */
static void desklrc_gussian_blur(cairo_surface_t *surface,
    gdouble sigma)
{
    if(surface==NULL) return;
    if(sigma<=0.0) return;
    cairo_format_t format = cairo_image_surface_get_format(surface);
    if(format!=CAIRO_FORMAT_ARGB32)
    {
        g_warning("The surface format is %d, only ARGB32 is supported",
            format);
        return;
    }
    gint kernel_size;
    gint *kernel = desklrc_calc_kernel(sigma, &kernel_size);
    desklrc_apply_kernel(surface, kernel, kernel_size);
    g_free(kernel);
}

static void desklrc_paint_pixbuf_rect(cairo_t *cr,
    GdkPixbuf *source, gdouble src_x, gdouble src_y, gdouble src_w,
    gdouble src_h, gdouble des_x, gdouble des_y, gdouble des_w,
    gdouble des_h)
{
    gdouble sw, sh;
    if(cr==NULL || source==NULL) return;
    cairo_save(cr);
    sw = des_w / src_w;
    sh = des_h / src_h;
    cairo_translate(cr, des_x, des_y);
    cairo_rectangle(cr, 0, 0, des_w, des_h);
    cairo_scale(cr, sw, sh);
    cairo_clip(cr);
    gdk_cairo_set_source_pixbuf(cr, source, -src_x, -src_y);
    cairo_paint(cr);
    cairo_restore(cr);
}

static void desklrc_clear_cairo(cairo_t *cr)
{
    if(cr==NULL) return;
    cairo_save(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_restore(cr);
}

static void desklrc_render_update_font_height(
    DesklrcRenderContext *context)
{
    PangoFontMetrics *metrics;
    gint ascent, descent;
    metrics = pango_context_get_metrics(context->pango_context,
        pango_layout_get_font_description(context->pango_layout), NULL);
    if(metrics==NULL)
    {
        g_warning("Cannot get font metrics!");
    }
    context->font_height = 0;
    ascent = pango_font_metrics_get_ascent(metrics);
    descent = pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);
    context->font_height += PANGO_PIXELS(ascent + descent);
}

static void desklrc_render_update_font(DesklrcRenderContext *context)
{
    PangoFontDescription *font_desc;
    if(context==NULL) return;
    font_desc = pango_font_description_from_string(context->font_name);
    pango_layout_set_font_description(context->pango_layout, font_desc);
    pango_font_description_free(font_desc);
    desklrc_render_update_font_height(context);
}

static void desklrc_render_set_font_name(DesklrcRenderContext *context,
    const gchar *font_name)
{
    if(context==NULL || font_name==NULL) return;
    g_free(context->font_name);
    context->font_name = g_strdup(font_name);
    desklrc_render_update_font(context);
}

static DesklrcRenderContext *desklrc_render_context_new()
{
    DesklrcRenderContext *context = g_new0(DesklrcRenderContext, 1);
    context->font_name = g_strdup(DESKLRC_DEFAULT_FONT_NAME);
    context->linear_pos[0] = 0.0;
    context->linear_pos[1] = 0.5;
    context->linear_pos[2] = 1.0;
    gdk_rgba_parse(&(context->linear_normal_colors[0]), "#4CFFFF");
    gdk_rgba_parse(&(context->linear_normal_colors[1]), "#0000FF");
    gdk_rgba_parse(&(context->linear_normal_colors[2]), "#4CFFFF");
    gdk_rgba_parse(&(context->linear_active_colors[0]), "#FFFF00");
    gdk_rgba_parse(&(context->linear_active_colors[1]), "#FF4C4C");
    gdk_rgba_parse(&(context->linear_active_colors[2]), "#FFFF00");
    context->pango_context = gdk_pango_context_get();
    context->pango_layout = pango_layout_new(context->pango_context);
    context->text = NULL;
    context->blur_radius = 0.0;
    desklrc_render_update_font(context);
    context->outline_width = DESKLRC_DEFAULT_OUTLINE_WIDTH;
    return context;
}

static void desklrc_render_context_destroy(DesklrcRenderContext *context)
{
    if(context==NULL) return;
    g_free(context->font_name);
    if(context->pango_layout!=NULL)
        g_object_unref(context->pango_layout);
    if(context->pango_context!=NULL)
        g_object_unref(context->pango_context);
    g_free(context->text);
    g_free(context);
}

static void desklrc_render_set_text(DesklrcRenderContext *context,
    const gchar *text)
{
    if(context==NULL || text==NULL) return;
    if(context->text!=NULL)
    {
        if(g_strcmp0(context->text, text)==0)
            return;
        g_free(context->text);
    }
    context->text = g_strdup(text);
    pango_layout_set_text(context->pango_layout, text, -1);
}

static void desklrc_render_get_pixel_size(DesklrcRenderContext *context,
    const gchar *text, gint *width, gint *height)
{
    gint w, h;
    if(context==NULL || text==NULL || width==NULL || height==NULL) return;
    desklrc_render_set_text(context, text);
    pango_layout_get_pixel_size (context->pango_layout, &w, &h);
    if(width!=NULL)
    {
        *width = round(w + context->outline_width +
            context->blur_radius * 2);
    }
    if(height!=NULL)
    {
        *height = round(h + context->outline_width +
            context->blur_radius * 2);
    }
}

static void desklrc_render_paint_text(DesklrcRenderContext *context,
    cairo_t *cr, const gchar *text, gdouble xpos, gdouble ypos,
    gboolean active)
{
    gint width, height;
    gint i;
    cairo_pattern_t *pattern;
    if(context==NULL || cr==NULL || text==NULL) return;
    desklrc_render_set_text(context, text);
    xpos += context->outline_width / 2.0 + context->blur_radius;
    ypos += context->outline_width / 2.0 + context->blur_radius;
    pango_layout_get_pixel_size(context->pango_layout, &width, &height);
    cairo_move_to(cr, xpos, ypos);
    cairo_save(cr);
    pango_cairo_layout_path(cr, context->pango_layout);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.8);
    if(context->outline_width>0)
    {
        cairo_set_line_width(cr, context->outline_width);
        if(context->blur_radius>1e-4)
        {
            cairo_stroke_preserve(cr);
            cairo_fill(cr);
            desklrc_gussian_blur(cairo_get_target(cr),
                context->blur_radius);
        }
        else
        {
            cairo_stroke(cr);
        }
    }
    cairo_restore(cr);
    cairo_save(cr);
    cairo_new_path(cr);
    pattern = cairo_pattern_create_linear(xpos, ypos, xpos, ypos+height);
    for(i=0;i<3;i++)
    {
        if(active)
        {
            cairo_pattern_add_color_stop_rgb(pattern, context->linear_pos[i],
                context->linear_active_colors[i].red,
                context->linear_active_colors[i].green,
                context->linear_active_colors[i].blue);
        }
        else
        {
            cairo_pattern_add_color_stop_rgb(pattern, context->linear_pos[i],
                context->linear_normal_colors[i].red,
                context->linear_normal_colors[i].green,
                context->linear_normal_colors[i].blue);
        }
    }
    cairo_set_source(cr, pattern);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_move_to(cr, xpos, ypos);
    pango_cairo_show_layout(cr, context->pango_layout);
    cairo_pattern_destroy(pattern);
    cairo_restore(cr);
}

static gboolean desklrc_window_panel_visible(DesklrcPrivate *priv)
{
    if(priv==NULL) return FALSE;
    return (priv->mode==DESKLRC_WINDOW_NORMAL) ||
        (priv->movable && priv->notify_flag);
}

static gboolean desklrc_paint_bg(DesklrcPrivate *priv, cairo_t *cr)
{
    GtkAllocation allocation;
    gint w, h;
    gint sw, sh;
    GtkStyleContext *context;
    if(priv==NULL || cr==NULL) return FALSE;
    if(priv->window==NULL) return FALSE;
    gtk_widget_get_allocation(priv->window, &allocation);
    w = allocation.width;
    h = allocation.height;
    desklrc_clear_cairo(cr);
    if(desklrc_window_panel_visible(priv))
    {
        if(priv->bg_pixbuf!=NULL)
        {
            if(priv->composited)
            {
                desklrc_clear_cairo(cr);
            }
            else
            {
                cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
                cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
                cairo_rectangle(cr, 0, 0, w, h);
                cairo_fill(cr);
            }
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            sw = gdk_pixbuf_get_width(priv->bg_pixbuf);
            sh = gdk_pixbuf_get_height(priv->bg_pixbuf);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                0, 0, DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                0, 0, DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                0, sh - DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, 0, h - DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                sw - DESKLRC_BORDER_WIDTH, 0, DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, w - DESKLRC_BORDER_WIDTH, 0,
                DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                sw - DESKLRC_BORDER_WIDTH, sh - DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                w - DESKLRC_BORDER_WIDTH, h - DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                0, DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                sh - DESKLRC_BORDER_WIDTH * 2, 0, DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, h - DESKLRC_BORDER_WIDTH * 2);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                sw - DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, sh - DESKLRC_BORDER_WIDTH * 2,
                w - DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, h - DESKLRC_BORDER_WIDTH * 2);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                DESKLRC_BORDER_WIDTH, 0, sw - DESKLRC_BORDER_WIDTH * 2,
                DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH, 0,
                w - DESKLRC_BORDER_WIDTH * 2, DESKLRC_BORDER_WIDTH);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                DESKLRC_BORDER_WIDTH, sh - DESKLRC_BORDER_WIDTH,
                sw - DESKLRC_BORDER_WIDTH * 2, DESKLRC_BORDER_WIDTH,
                DESKLRC_BORDER_WIDTH, h - DESKLRC_BORDER_WIDTH,
                w - DESKLRC_BORDER_WIDTH * 2, DESKLRC_BORDER_WIDTH);
            desklrc_paint_pixbuf_rect(cr, priv->bg_pixbuf,
                DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                sw - DESKLRC_BORDER_WIDTH * 2, sh - DESKLRC_BORDER_WIDTH * 2,
                DESKLRC_BORDER_WIDTH, DESKLRC_BORDER_WIDTH,
                w - DESKLRC_BORDER_WIDTH * 2, h - DESKLRC_BORDER_WIDTH * 2);  
        }
        else
        {
            context = gtk_widget_get_style_context(priv->window);
            gtk_render_background(context, cr, 0, 0, w, h);
        }
    }
    return TRUE;
}

static gboolean desklrc_is_string_empty(const gchar *str)
{
    gint i;
    guint len;
    if(str==NULL) return TRUE;
    len = strlen(str);
    for(i=0;i<len;i++)
    {
        if(str[i] != ' ')
            return FALSE;
    }
    return TRUE;
}

static void desklrc_draw_lyric_surface(DesklrcPrivate *priv,
    cairo_surface_t **surface, const gchar *lyric, gboolean active)
{
    gint w = 0, h = 0;
    cairo_t *cr;
    if(priv==NULL || surface==NULL) return;
    if(*surface!=NULL)
    {
        cairo_surface_destroy(*surface);
        *surface = NULL;
    }
    if(lyric==NULL) return;
    if(!gtk_widget_get_realized(priv->window))
        gtk_widget_realize(priv->window);
    desklrc_render_get_pixel_size(priv->render_context, lyric, &w, &h);
    *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cr = cairo_create(*surface);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    if(!desklrc_is_string_empty(lyric))
    {
        desklrc_render_paint_text(priv->render_context, cr, lyric,
            0, 0, active);
    }
    cairo_destroy(cr);
}

static inline void desklrc_update_color(DesklrcPrivate *priv)
{
    if(priv==NULL || priv->render_context==NULL) return;
    priv->render_context->linear_normal_colors[0] = priv->bg_color1;
    priv->render_context->linear_normal_colors[1] = priv->bg_color2;
    priv->render_context->linear_normal_colors[2] = priv->bg_color3;
    priv->render_context->linear_active_colors[0] = priv->fg_color1;
    priv->render_context->linear_active_colors[1] = priv->fg_color2;
    priv->render_context->linear_active_colors[2] = priv->fg_color3;
}

static inline void desklrc_draw_drag_shadow(cairo_t *cr, gint width,
    gint height)
{
    cairo_save(cr);
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
    cairo_restore(cr);
}

static void desklrc_show(GtkWidget *widget, DesklrcPrivate *priv, cairo_t *cr)
{
    GtkAllocation allocation;
    cairo_surface_t *surface;
    gint surface_width, surface_height;
    gint index1 = 0, index2 = 0;
    gboolean two_track = FALSE;
    gboolean two_line = FALSE;
    gfloat percent1 = 0.0, percent2 = 0.0;
    gfloat start_x, start_y;
    gfloat percent_lower, percent_upper;
    gfloat percent_text;
    gtk_widget_get_allocation(widget, &allocation);
    if(priv->notify_flag)
    {
        desklrc_draw_drag_shadow(cr, allocation.width,
            allocation.height);
    }
    
    index1 = priv->lyric_line_num[0];
    percent1 = priv->lyric_percent[0];
    
    /* Draw the first track lyric of line 1 */
    surface = priv->lyric_surface[DESKLRC_LAYOUT_TEXT1_LINE1];
    if(surface!=NULL)
    {
        surface_width = cairo_image_surface_get_width(surface);
        surface_height = cairo_image_surface_get_height(surface);
        if(priv->lyric_surface[DESKLRC_LAYOUT_TEXT2_LINE1]!=NULL)
            two_track = TRUE;
        two_line = priv->two_line;
        if(two_line)
            start_x = DESKLRC_MARGIN_LEFT + (index1%2)*allocation.width/3;
        else
            start_x = DESKLRC_MARGIN_LEFT;
        if(start_x+surface_width>allocation.width)
        {
            percent_lower = (gfloat)(allocation.width - start_x) / 2 /
                surface_width;
            percent_upper = 1.0 - percent_lower;
            if(percent1>=percent_upper)
                percent_text = 1.0;
            else if(percent1>=percent_lower)
            {
                percent_text = (percent1 - percent_lower) /
                    (percent_upper - percent_lower);
            }
            else
                percent_text = 0.0;
            
            start_x += (gfloat)(allocation.width - surface_width - start_x -
                DESKLRC_MARGIN_LEFT - DESKLRC_MARGIN_RIGHT) * percent_text;
        }
        if(two_line && two_track)
        {
            if(index1%2==0)
                start_y = DESKLRC_MARGIN_TOP;
            else
            {
                start_y = DESKLRC_MARGIN_TOP + 2 * (DESKLRC_LINE_PADDING +
                    priv->render_context->font_height);
            }
        }
        else if(two_line && !two_track)
        {
            if(index1%2==0)
                start_y = DESKLRC_MARGIN_TOP;
            else
                start_y = DESKLRC_MARGIN_TOP + DESKLRC_LINE_PADDING +
                    priv->render_context->font_height;
        }
        else
        {
            start_y = DESKLRC_MARGIN_TOP;
        }
        cairo_save(cr);
        if(priv->lyric_active_surface[0]!=NULL)
        {
            cairo_rectangle(cr, start_x + surface_width * percent1, start_y,
                surface_width * (1.0 - percent1), surface_height);
        }
        else
        {
            cairo_rectangle(cr, start_x, start_y, surface_width,
                surface_height);
        }
        cairo_clip(cr);
        cairo_set_source_surface(cr, surface, start_x, start_y);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_paint(cr);
        cairo_restore(cr);
        if(priv->lyric_active_surface[0]!=NULL)
        {
            cairo_save(cr);
            cairo_rectangle(cr, start_x, start_y, surface_width * percent1,
                surface_height);
            cairo_clip(cr);
            cairo_set_source_surface(cr, priv->lyric_active_surface[0],
                start_x, start_y);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            cairo_paint(cr);
            cairo_restore(cr);
        }
    }

    /* Draw the first track lyric of line 2 */
    surface = priv->lyric_surface[DESKLRC_LAYOUT_TEXT1_LINE2];
    index1++;
    if(surface!=NULL)
    {
        surface_width = cairo_image_surface_get_width(surface);
        surface_height = cairo_image_surface_get_height(surface);
        start_x = DESKLRC_MARGIN_LEFT + (index1%2)*allocation.width/3;
        if(percent1<0.5 && start_x+surface_width>allocation.width)
        {
            start_x += allocation.width - surface_width - start_x -
                DESKLRC_MARGIN_LEFT - DESKLRC_MARGIN_RIGHT;
        }
        if(two_track)
        {
            if(index1%2==0)
            {
                start_y = DESKLRC_MARGIN_TOP;
            }
            else
            {
                start_y = DESKLRC_MARGIN_TOP + 2 * (DESKLRC_LINE_PADDING +
                    priv->render_context->font_height);
            }
        }
        else
        {
            if(index1%2==0)
                start_y = DESKLRC_MARGIN_TOP;
            else
                start_y = DESKLRC_MARGIN_TOP + DESKLRC_LINE_PADDING +
                    priv->render_context->font_height;
        }
        cairo_save(cr);
        cairo_rectangle(cr, start_x, start_y, surface_width,
            surface_height);
        cairo_clip(cr);
        cairo_set_source_surface(cr, surface, start_x, start_y);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    index2 = priv->lyric_line_num[1];
    percent2 = priv->lyric_percent[1];
    
    /* Draw the second track lyric of line 1 */
    surface = priv->lyric_surface[DESKLRC_LAYOUT_TEXT2_LINE1];
    if(surface!=NULL)
    {
        surface_width = cairo_image_surface_get_width(surface);
        surface_height = cairo_image_surface_get_height(surface);
        if(priv->lyric_surface[DESKLRC_LAYOUT_TEXT2_LINE2]!=NULL)
            two_line = TRUE;
        if(two_line)
            start_x = DESKLRC_MARGIN_LEFT + (index2%2)*allocation.width/3;
        else
            start_x = DESKLRC_MARGIN_LEFT;
        if(start_x+surface_width>allocation.width)
        {
            percent_lower = (gfloat)(allocation.width - start_x) / 2 /
                surface_width;
            percent_upper = 1.0 - percent_lower;
            if(percent2>=percent_upper)
                percent_text = 1.0;
            else if(percent2>=percent_lower)
            {
                percent_text = (percent2 - percent_lower) /
                    (percent_upper - percent_lower);
            }
            else
                percent_text = 0.0;
            start_x += (gfloat)(allocation.width - surface_width - start_x -
                DESKLRC_MARGIN_LEFT - DESKLRC_MARGIN_RIGHT) * percent_text;
        }
        if(two_line)
        {
            if(index2%2==0)
                start_y = DESKLRC_MARGIN_TOP + DESKLRC_LINE_PADDING +
                    priv->render_context->font_height;
            else
            {
                start_y = DESKLRC_MARGIN_TOP + 3 * (DESKLRC_LINE_PADDING +
                    priv->render_context->font_height);
            }
        }
        else
        {
            start_y = DESKLRC_MARGIN_TOP + DESKLRC_LINE_PADDING +
                priv->render_context->font_height;
        }
        cairo_save(cr);
        if(priv->lyric_active_surface[1]!=NULL)
        {
            cairo_rectangle(cr, start_x + surface_width * percent2, start_y,
                surface_width * (1.0 - percent2), surface_height);
        }
        else
        {
            cairo_rectangle(cr, start_x, start_y, surface_width,
                surface_height);
        }
        cairo_clip(cr);
        cairo_set_source_surface(cr, surface, start_x, start_y);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_paint(cr);
        cairo_restore(cr);
        if(priv->lyric_active_surface[1]!=NULL)
        {
            cairo_save(cr);
            cairo_rectangle(cr, start_x, start_y, surface_width * percent2,
                surface_height);
            cairo_clip(cr);
            cairo_set_source_surface(cr, priv->lyric_active_surface[1],
                start_x, start_y);
            cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
            cairo_paint(cr);
            cairo_restore(cr);
        }
    }

    /* Draw the second track lyric of line 2 */
    surface = priv->lyric_surface[DESKLRC_LAYOUT_TEXT2_LINE2];
    index2++;
    if(surface!=NULL)
    {
        surface_width = cairo_image_surface_get_width(surface);
        surface_height = cairo_image_surface_get_height(surface);
        start_x = DESKLRC_MARGIN_LEFT + (index2%2)*allocation.width/3;
        if(percent2<0.5 && start_x+surface_width>allocation.width)
        {
            start_x += allocation.width - surface_width - start_x -
                DESKLRC_MARGIN_LEFT - DESKLRC_MARGIN_RIGHT;
        }
        if(index2%2==0)
        {
            start_y = DESKLRC_MARGIN_TOP + DESKLRC_LINE_PADDING +
                priv->render_context->font_height;
        }
        else
        {
            start_y = DESKLRC_MARGIN_TOP + 3 * (DESKLRC_LINE_PADDING +
                priv->render_context->font_height);
        }
        cairo_save(cr);
        cairo_rectangle(cr, start_x, start_y, surface_width,
            surface_height);
        cairo_clip(cr);
        cairo_set_source_surface(cr, surface, start_x, start_y);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_paint(cr);
        cairo_restore(cr);
    }

}

static gboolean desklrc_drag(GtkWidget *widget, GdkEvent *event,
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
    DesklrcPrivate *priv = (DesklrcPrivate *)data;
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

static gboolean desklrc_update(gpointer data)
{
    DesklrcPrivate *priv = (DesklrcPrivate *)data;
    if(data==NULL) return TRUE;
    gtk_widget_queue_draw(priv->window);
    return TRUE;   
}

static inline void desklrc_render_lyric_surface(DesklrcPrivate *priv,
    GSequenceIter *iter1, GSequenceIter *iter_begin1, GSequenceIter *iter2,
    GSequenceIter *iter_begin2)
{
    const RCLibLyricData *lyric_data;
    const gchar *text;
    guint line;
    gboolean active;
       
    /* Render the lyric layout of the first track, line 1 */
    line = DESKLRC_LAYOUT_TEXT1_LINE1;
    text = NULL;
    G_STMT_START
    {
        if(iter1==NULL) iter1 = iter_begin1;
        if(iter1==NULL) break;
        lyric_data = g_sequence_get(iter1);
        if(lyric_data==NULL) break;
        text = lyric_data->text;
    }
    G_STMT_END;
    if(g_strcmp0(priv->lyric_text[line], text)!=0)
    {
        desklrc_draw_lyric_surface(priv, &(priv->lyric_surface[line]),
            text, FALSE);
        desklrc_draw_lyric_surface(priv, &(priv->lyric_active_surface[0]),
            text, TRUE);
        g_free(priv->lyric_text[line]);
        priv->lyric_text[line] = g_strdup(text);
    }
    
    /* Render the lyric layout of the first track, line 2 */
    line = DESKLRC_LAYOUT_TEXT1_LINE2;
    if(priv->two_line)
    {
        text = NULL;
        G_STMT_START
        {
            if(iter1==NULL) iter1 = iter_begin1;
            if(iter1==NULL) break;
            if(priv->lyric_percent[0]>=0.5)
            {
                iter1 = g_sequence_iter_next(iter1);
            }
            else
            {
                if(g_sequence_iter_is_begin(iter1)) break;
                iter1 = g_sequence_iter_prev(iter1);
            }
            if(iter1==NULL || g_sequence_iter_is_end(iter1)) break;
            lyric_data = g_sequence_get(iter1);
            if(lyric_data==NULL) break;
            text = lyric_data->text;
        }
        G_STMT_END;
        if(g_strcmp0(priv->lyric_text[line], text)!=0)
        {
            if(priv->lyric_percent[0]>=0.5) active = FALSE;
            else active = TRUE;
            desklrc_draw_lyric_surface(priv, &(priv->lyric_surface[line]),
                text, active);
            g_free(priv->lyric_text[line]);
            priv->lyric_text[line] = g_strdup(text);
        }    
    }
    else
    {
        if(priv->lyric_surface[line]!=NULL)
        {
            cairo_surface_destroy(priv->lyric_surface[line]);
            priv->lyric_surface[line] = NULL;
        }
        g_free(priv->lyric_text[line]);
        priv->lyric_text[line] = NULL;
    }

    if(iter_begin2!=NULL)
    {
        /* Render the lyric layout of the second track, line 1 */
        line = DESKLRC_LAYOUT_TEXT2_LINE1;
        text = NULL;
        G_STMT_START
        {
            if(iter2==NULL) iter2 = iter_begin2;
            if(iter2==NULL) break;
            lyric_data = g_sequence_get(iter2);
            if(lyric_data==NULL) break;
            text = lyric_data->text;
        }
        G_STMT_END;
        if(g_strcmp0(priv->lyric_text[line], text)!=0)
        {
            desklrc_draw_lyric_surface(priv, &(priv->lyric_surface[line]),
                text, FALSE);
            desklrc_draw_lyric_surface(priv,
                &(priv->lyric_active_surface[1]), text, TRUE);
            g_free(priv->lyric_text[line]);
            priv->lyric_text[line] = g_strdup(text);
        }
    
        /* Render the lyric layout of the second track, line 2 */
        line = DESKLRC_LAYOUT_TEXT2_LINE2;
        if(priv->two_line)
        {
            text = NULL;
            G_STMT_START
            {
                if(iter2==NULL) iter2 = iter_begin2;
                if(iter2==NULL) break;
                if(priv->lyric_percent[1]>=0.5)
                {
                    iter2 = g_sequence_iter_next(iter2);
                }
                else
                {
                    if(g_sequence_iter_is_begin(iter2)) break;
                    iter2 = g_sequence_iter_prev(iter2);
                }
                if(iter2==NULL || g_sequence_iter_is_end(iter2)) break;
                lyric_data = g_sequence_get(iter2);
                if(lyric_data==NULL) break;
                text = lyric_data->text;
            }
            G_STMT_END;
            if(g_strcmp0(priv->lyric_text[line], text)!=0)
            {
                if(priv->lyric_percent[1]>=0.5) active = FALSE;
                else active = TRUE;
                desklrc_draw_lyric_surface(priv,
                    &(priv->lyric_surface[line]), text, active);
                g_free(priv->lyric_text[line]);
                priv->lyric_text[line] = g_strdup(text);
            }    
        }
        else
        {
            if(priv->lyric_surface[line]!=NULL)
            {
                cairo_surface_destroy(priv->lyric_surface[line]);
                priv->lyric_surface[line] = NULL;
            }
            g_free(priv->lyric_text[line]);
            priv->lyric_text[line] = NULL;
        }
    }
    else
    {
        for(line=DESKLRC_LAYOUT_TEXT2_LINE1;line<=DESKLRC_LAYOUT_TEXT2_LINE2;
            line++)
        {
            if(priv->lyric_surface[line]!=NULL)
            {
                cairo_surface_destroy(priv->lyric_surface[line]);
                priv->lyric_surface[line] = NULL;
            }
            g_free(priv->lyric_text[line]);
            priv->lyric_text[line] = NULL;
        }
        if(priv->lyric_active_surface[1]!=NULL)
        {
            cairo_surface_destroy(priv->lyric_active_surface[1]);
            priv->lyric_active_surface[1] = NULL;
        }
    } 
}

static gboolean desklrc_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    DesklrcPrivate *priv = (DesklrcPrivate *)data;
    GtkAllocation allocation;
    gint64 pos = 0, offset1 = 0, offset2 = 0;
    GSequenceIter *iter1 = NULL, *iter_begin1 = NULL;
    GSequenceIter *iter2 = NULL, *iter_begin2 = NULL;
    const RCLibLyricParsedData *parsed_data1 = NULL;
    const RCLibLyricParsedData *parsed_data2 = NULL;
    const RCLibLyricData *lyric_data;
    gint64 duration = 0;
    gint64 time_passed = 0, time_length = 0;
    guint line = 1;
    gint alloc_height = 0;
    if(data==NULL) return FALSE;
    gtk_widget_get_allocation(widget, &allocation);
    desklrc_paint_bg(priv, cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    pos = rclib_core_query_position();
    duration = rclib_core_query_duration();
    parsed_data1 = rclib_lyric_get_parsed_data(0);
    iter1 = rclib_lyric_get_line_iter(0, pos);
    if(priv->two_track)
    {
        parsed_data2 = rclib_lyric_get_parsed_data(1);
        iter2 = rclib_lyric_get_line_iter(1, pos);
    }
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
    if(iter1==NULL && iter_begin1==NULL && iter_begin2!=NULL)
    {
        iter1 = iter2;
        iter_begin1 = iter_begin2;
        iter2 = NULL;
        iter_begin2 = NULL;
    }
    priv->lyric_percent[0] = 0.0;
    priv->lyric_percent[1] = 0.0;
    priv->lyric_line_num[0] = 0;
    priv->lyric_line_num[1] = 0;
    if(iter1!=NULL)
    {
        lyric_data = g_sequence_get(iter1);
        if(lyric_data!=NULL)
        {
            time_passed = pos - (lyric_data->time+offset1);
            if(lyric_data->length>0)
                time_length = lyric_data->length;
            else
                time_length = duration - (lyric_data->time+offset1);
            if(time_passed>0)
                priv->lyric_percent[0] = (gdouble)time_passed / time_length;
            priv->lyric_line_num[0] = g_sequence_iter_get_position(iter1);
        }
    }
    if(iter2!=NULL)
    {
        lyric_data = g_sequence_get(iter2);
        if(lyric_data!=NULL)
        {
            line = 2;
            time_passed = pos - (lyric_data->time+offset2);
            if(lyric_data->length>0)
                time_length = lyric_data->length;
            else
                time_length = duration - (lyric_data->time+offset2);
            if(time_passed>0)
                priv->lyric_percent[1] = (gdouble)time_passed / time_length;
            priv->lyric_line_num[1] = g_sequence_iter_get_position(iter2);
        }
    }    
    if(priv->two_line)
        line *= 2;
    alloc_height = line * priv->render_context->font_height +
        (line - 1) * DESKLRC_LINE_PADDING +
        priv->render_context->outline_width + DESKLRC_MARGIN_TOP +
        DESKLRC_MARGIN_BOTTOM;
    if(allocation.width!=priv->osd_window_width ||
        allocation.height!=alloc_height)
    {
        gtk_window_resize(GTK_WINDOW(widget), priv->osd_window_width,
            alloc_height);
    }
    gtk_widget_get_allocation(widget, &allocation);    
    desklrc_render_lyric_surface(priv, iter1, iter_begin1, iter2,
        iter_begin2);
    desklrc_show(widget, priv, cr);
    return FALSE;
}

static void desklrc_load_conf(DesklrcPrivate *priv)
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
    priv->bg_color3 = priv->bg_color1;
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
    priv->fg_color3 = priv->fg_color1;
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
        "ShowTwoTrack", &error);
    if(error==NULL)
        priv->two_track = bvalue;
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

static void desklrc_save_conf(DesklrcPrivate *priv)
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
        "ShowTwoTrack", priv->two_track);
    g_key_file_set_boolean(priv->keyfile, DESKLRC_ID,
        "ShowTwoLine", priv->two_line);
}

static void desklrc_apply_movable(DesklrcPrivate *priv)
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

static void desklrc_shutdown_cb(RCLibPlugin *plugin,
    gpointer data)
{
    DesklrcPrivate *priv = (DesklrcPrivate *)data;
    if(data==NULL) return;
    desklrc_save_conf(priv);
}

static gboolean desklrc_load(RCLibPluginData *plugin)
{
    DesklrcPrivate *priv = &desklrc_priv;
    GdkScreen *screen;
    GdkVisual *visual;
    GdkGeometry window_hints =
    {
        .min_width = 500,
        .min_height = 50,
        .base_width = 600,
        .base_height = 50
    };
    screen = gdk_screen_get_default();
    priv->composited = gdk_screen_is_composited(screen);
    if(!priv->composited)
    {
        g_warning("DeskLRC: Composite effect is not availabe, "
            "this plug-in cannot be enabled!");
        return FALSE;
    }
    priv->render_context = desklrc_render_context_new();
    desklrc_render_set_font_name(priv->render_context, priv->font_string);
    desklrc_update_color(priv);
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
    desklrc_apply_movable(priv);
    g_signal_connect(priv->window, "draw",
        G_CALLBACK(desklrc_draw), priv);
    g_signal_connect(priv->window, "button-press-event",
        G_CALLBACK(desklrc_drag), priv);
    g_signal_connect(priv->window, "motion-notify-event",
        G_CALLBACK(desklrc_drag), priv);
    g_signal_connect(priv->window, "button-release-event",
        G_CALLBACK(desklrc_drag), priv);
    g_signal_connect(priv->window, "enter-notify-event",
        G_CALLBACK(desklrc_drag), priv);
    g_signal_connect(priv->window, "leave-notify-event",
        G_CALLBACK(desklrc_drag), priv);
    priv->timeout_id = g_timeout_add(100,
        (GSourceFunc)desklrc_update, priv);
    gtk_widget_show_all(priv->window);
    return TRUE;
}

static gboolean desklrc_unload(RCLibPluginData *plugin)
{
    DesklrcPrivate *priv = &desklrc_priv;
    if(priv->timeout_id>0)
        g_source_remove(priv->timeout_id);
    if(priv->window!=NULL)
    {
        gtk_widget_destroy(priv->window);
        priv->window = NULL;
    }
    if(priv->render_context!=NULL)
    {
        desklrc_render_context_destroy(priv->render_context);
        priv->render_context = NULL;
    }
    return TRUE;
}

static gboolean desklrc_configure(RCLibPluginData *plugin)
{
    DesklrcPrivate *priv = &desklrc_priv;
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
    GtkWidget *two_track_checkbox;
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
    two_track_checkbox = gtk_check_button_new_with_mnemonic(
        _("Draw two tracks the lyric texts"));
    two_line_mode_checkbox = gtk_check_button_new_with_mnemonic(
        _("Draw two lines of lyric texts"));
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
    g_object_set(two_track_checkbox, "hexpand-set", TRUE, "hexpand",
        TRUE, "vexpand-set", TRUE, "vexpand", FALSE, "active",
        priv->two_track, NULL);
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
    gtk_grid_attach(GTK_GRID(grid), two_track_checkbox, 0, 4, 2, 1);
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
        priv->bg_color3 = priv->bg_color1;
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(hi_color_button1),
            &(priv->fg_color1));
        gtk_color_button_get_rgba(GTK_COLOR_BUTTON(hi_color_button2),
            &(priv->fg_color2));
        priv->fg_color3 = priv->fg_color1;
        priv->osd_window_width = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(window_width_spin));
        priv->movable = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(window_movable_checkbox));
        priv->two_track = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(two_track_checkbox));
        priv->two_line = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(two_line_mode_checkbox));
        if(priv->render_context!=NULL)
        {
            desklrc_render_set_font_name(priv->render_context,
                priv->font_string);
        }
        desklrc_update_color(priv);
        desklrc_apply_movable(priv);
        if(priv->window!=NULL)
            gtk_widget_queue_draw(priv->window);
        desklrc_save_conf(priv);
    }
    gtk_widget_destroy(dialog);
    return TRUE;
}

static gboolean desklrc_init(RCLibPluginData *plugin)
{
    DesklrcPrivate *priv = &desklrc_priv;
    GdkScreen *screen;
    gint screen_width, screen_height;
    priv->movable = TRUE;
    priv->two_line = TRUE;
    priv->two_track = TRUE;
    priv->font_string = g_strdup("Monospace 22");
    gdk_rgba_parse(&(priv->bg_color1), "#4CFFFF");
    gdk_rgba_parse(&(priv->bg_color2), "#0000FF");
    gdk_rgba_parse(&(priv->bg_color3), "#4CFFFF");
    gdk_rgba_parse(&(priv->fg_color1), "#FF4C4C");
    gdk_rgba_parse(&(priv->fg_color2), "#FFFF00");
    gdk_rgba_parse(&(priv->fg_color3), "#4CFFFF");
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
    desklrc_load_conf(priv);
    priv->shutdown_id = rclib_plugin_signal_connect("shutdown",
        G_CALLBACK(desklrc_shutdown_cb), priv);
    return TRUE;
}

static void desklrc_destroy(RCLibPluginData *plugin)
{
    DesklrcPrivate *priv = &desklrc_priv;
    if(priv->shutdown_id>0)
        rclib_plugin_signal_disconnect(priv->shutdown_id);
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
    .load = desklrc_load,
    .unload = desklrc_unload,
    .configure = desklrc_configure,
    .destroy = desklrc_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, desklrc_init, rcplugin_info);



