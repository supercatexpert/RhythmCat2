/*
 * RhythmCat UI Player Module
 * The main UI for the player.
 *
 * ui-player.c
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

#include "ui-player.h"
#include "ui-listview.h"
#include "ui-menu.h"
#include "ui-dialog.h"
#include "ui-slabel.h"
#include "ui-spectrum.h"
#include "ui-img-icon.xpm"
#include "ui-img-nocov.xpm"
#include "common.h"

#define RC_UI_PLAYER_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RC_UI_PLAYER_TYPE, RCUiPlayerPrivate)

typedef struct RCUiPlayerPrivate
{
    GtkApplication *app;
    GtkUIManager *ui_manager;
    GtkWidget *main_window;
    GtkWidget *main_vbox;
    GtkWidget *title_label;
    GtkWidget *artist_label;
    GtkWidget *album_label;
    GtkWidget *time_label;
    GtkWidget *length_label;
    GtkWidget *info_label;
    GtkWidget *album_image;
    GtkWidget *album_eventbox;
    GtkWidget *album_frame;
    GtkWidget *lyric1_slabel;
    GtkWidget *lyric2_slabel;
    GtkWidget *spectrum_widget;
    GtkWidget *ctrl_play_image;
    GtkWidget *ctrl_stop_image;
    GtkWidget *ctrl_prev_image;
    GtkWidget *ctrl_next_image;
    GtkWidget *ctrl_open_image;
    GtkWidget *ctrl_play_button;
    GtkWidget *ctrl_stop_button;
    GtkWidget *ctrl_prev_button;
    GtkWidget *ctrl_next_button;
    GtkWidget *ctrl_open_button;
    GtkWidget *menu_image;
    GtkWidget *menu_eventbox;
    GtkWidget *volume_button;
    GtkWidget *time_scale;
    GtkWidget *catalog_listview;
    GtkWidget *playlist_listview;
    GtkWidget *catalog_scr_window;
    GtkWidget *playlist_scr_window;
    GtkWidget *list_paned;
    GtkWidget *progress_eventbox;
    GtkWidget *progress_spinner;
    GtkWidget *progress_label;
    GtkWidget *import_spinner;
    GtkWidget *refresh_spinner;
    GdkPixbuf *cover_default_pixbuf;
    GdkPixbuf *icon_pixbuf;
    GdkPixbuf *menu_pixbuf;
    GdkPixbuf *cover_using_pixbuf;
    gchar *cover_file_path;
    GtkStatusIcon *tray_icon;
    guint cover_image_width;
    guint cover_image_height;
    gboolean cover_set_flag;
    gboolean update_seek_scale;
    gboolean import_work_flag;
    gboolean refresh_work_flag;
    gulong update_timeout;
}RCUiPlayerPrivate;

static GObject *ui_player_instance = NULL;
static GtkApplication *ui_player_app = NULL;

static inline void rc_ui_player_title_label_set_value(
    RCUiPlayerPrivate *priv, const gchar *uri, const gchar *title)
{
    gchar *filename = NULL;
    gchar *rtitle = NULL;
    if(title!=NULL && strlen(title)>0)
    {
        gtk_label_set_text(GTK_LABEL(priv->title_label), title);  
    }
    else
    {
        filename = g_filename_from_uri(uri, NULL, NULL);
        if(filename!=NULL)
        {
            rtitle = rclib_tag_get_name_from_fpath(filename);
            g_free(filename);
        }
        if(rtitle!=NULL)
        {
            gtk_label_set_text(GTK_LABEL(priv->title_label), rtitle);
            g_free(rtitle);
        }
        else
            gtk_label_set_text(GTK_LABEL(priv->title_label),
                _("Unknown Title"));
    }
}

static inline void rc_ui_player_artist_label_set_value(
    RCUiPlayerPrivate *priv, const gchar *artist)
{
    if(artist!=NULL && strlen(artist)>0)
        gtk_label_set_text(GTK_LABEL(priv->artist_label), artist);
    else
        gtk_label_set_text(GTK_LABEL(priv->artist_label),
            _("Unknown Artist"));
}

static inline void rc_ui_player_album_label_set_value(
    RCUiPlayerPrivate *priv, const gchar *album)
{
    if(album!=NULL && strlen(album)>0)
        gtk_label_set_text(GTK_LABEL(priv->album_label), album);
    else
        gtk_label_set_text(GTK_LABEL(priv->album_label),
            _("Unknown Album"));
}

static inline void rc_ui_player_time_label_set_value(
    RCUiPlayerPrivate *priv, gint64 pos)
{
    gint min, sec;
    gchar *string;
    if(priv==NULL) return;
    if(pos>=0)
    {
        sec = (gint)(pos / GST_SECOND);
        min = sec / 60;
        sec = sec % 60;
        string = g_strdup_printf("%02d:%02d", min, sec);
        gtk_label_set_text(GTK_LABEL(priv->time_label), string);
        g_free(string);
    }
    else
        gtk_label_set_text(GTK_LABEL(priv->time_label), "--:--");
}

static gboolean rc_ui_player_cover_image_set_pixbuf(RCUiPlayerPrivate *priv,
    const GdkPixbuf *pixbuf)
{
    GdkPixbuf *album_new_pixbuf;
    if(priv==NULL) return FALSE;
    album_new_pixbuf = gdk_pixbuf_scale_simple(pixbuf,
        priv->cover_image_width, priv->cover_image_height,
        GDK_INTERP_HYPER);
    if(album_new_pixbuf==NULL)
    {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
            "Cannot convert pixbuf for cover image!");
        return FALSE;
    }
    gtk_image_set_from_pixbuf(GTK_IMAGE(priv->album_image),
        album_new_pixbuf);
    g_object_unref(album_new_pixbuf);
    return TRUE;
}

static gboolean rc_ui_player_cover_image_set_gst_buffer(
    RCUiPlayerPrivate *priv, const GstBuffer *buf)
{
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;
    gboolean flag;
    GError *error = NULL;
    if(priv==NULL || buf==NULL) return FALSE;;
    loader = gdk_pixbuf_loader_new();
    if(!gdk_pixbuf_loader_write(loader, buf->data, buf->size, &error))
    {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
            "Cannot load cover image from GstBuffer: %s", error->message);
        g_error_free(error);
        g_object_unref(loader);
        return FALSE;
    }
    pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    if(pixbuf!=NULL) g_object_ref(pixbuf);
    gdk_pixbuf_loader_close(loader, NULL);
    g_object_unref(loader);
    if(pixbuf==NULL) return FALSE;
    if(priv->cover_using_pixbuf!=NULL)
        g_object_unref(priv->cover_using_pixbuf);
    priv->cover_using_pixbuf = g_object_ref(pixbuf);
    flag = rc_ui_player_cover_image_set_pixbuf(priv, pixbuf);
    g_object_unref(pixbuf);
    return flag;
}

static gboolean rc_ui_player_cover_image_set_file(RCUiPlayerPrivate *priv,
    const gchar *file)
{
    gboolean flag;
    GdkPixbuf *pixbuf;
    if(priv==NULL || file==NULL) return FALSE;
    if(!g_file_test(file, G_FILE_TEST_EXISTS)) return FALSE;
    pixbuf = gdk_pixbuf_new_from_file(file, NULL);
    if(pixbuf==NULL) return FALSE;
    g_free(priv->cover_file_path);
    priv->cover_file_path = g_strdup(file);
    flag = rc_ui_player_cover_image_set_pixbuf(priv, pixbuf);
    g_object_unref(pixbuf);
    return flag;
}

static void rc_ui_player_new_duration_cb(RCLibCore *core, gint64 duration,
    gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    gchar *length;
    gint min, sec;
    if(data==NULL || duration<0) return;
    sec = (gint)(duration / GST_SECOND);
    min = sec / 60;
    sec = sec % 60;
    length = g_strdup_printf("%02d:%02d", min, sec);
    gtk_label_set_text(GTK_LABEL(priv->length_label), length);
    g_free(length);
}

static void rc_ui_player_tag_found_cb(RCLibCore *core,
    const RCLibCoreMetadata *metadata, const gchar *uri, gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    gchar *filename = NULL;
    RCLibCoreSourceType type;
    gint ret;
    GString *info;
    RCLibDbPlaylistData *playlist_data;
    GSequenceIter *iter = rclib_core_get_db_reference();
    if(data==NULL || metadata==NULL || uri==NULL) return;
    if(iter!=NULL)
    {
        playlist_data = g_sequence_get(iter);
        type = rclib_core_get_source_type();
        ret = g_strcmp0(uri, playlist_data->uri);
        if(type==RCLIB_CORE_SOURCE_NORMAL && ret!=0) return;
    }
    rc_ui_player_title_label_set_value(priv, uri, metadata->title);
    rc_ui_player_artist_label_set_value(priv, metadata->artist);
    rc_ui_player_album_label_set_value(priv, metadata->album);
    if(metadata->duration>0)
        rc_ui_player_new_duration_cb(core, metadata->duration,
            data);
    info = g_string_new(NULL);
    if(metadata->ftype!=NULL && strlen(metadata->ftype)>0)
        g_string_printf(info, "%s ", metadata->ftype);
    else
        g_string_printf(info, "%s ", _("Unknown Format"));
    if(metadata->bitrate>0)
        g_string_append_printf(info, _("%d kbps "),
            metadata->bitrate / 1000);
    if(metadata->rate>0)
        g_string_append_printf(info, _("%d Hz "), metadata->rate);
    switch(metadata->channels)
    {
        case 1:
            g_string_append(info, _("Mono"));
            break;
        case 2:
            g_string_append(info, _("Stereo"));
            break;
        default:
            g_string_append_printf(info, _("%d channels"),
                metadata->channels);
            break;
    }
    gtk_label_set_text(GTK_LABEL(priv->info_label), info->str);
    g_string_free(info, TRUE);
    if(!priv->cover_set_flag)
    {
        filename = rclib_util_search_cover(uri, metadata->title,
            metadata->artist, metadata->album);
        if(filename!=NULL)
            rc_ui_player_cover_image_set_file(priv, filename);
        else   
            rc_ui_player_cover_image_set_gst_buffer(priv,
                metadata->image);
    }
}

static void rc_ui_player_uri_changed_cb(RCLibCore *core, const gchar *uri,
    gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    GSequenceIter *reference;
    RCLibDbPlaylistData *playlist_data = NULL;
    gchar *filename = NULL;
    if(data==NULL) return;
    reference = rclib_core_get_db_reference();
    gtk_image_set_from_pixbuf(GTK_IMAGE(priv->album_image),
        priv->cover_default_pixbuf);
    if(priv->cover_using_pixbuf!=NULL)
        g_object_unref(priv->cover_using_pixbuf);
    if(priv->cover_file_path!=NULL)
        g_free(priv->cover_file_path);
    priv->cover_using_pixbuf = NULL;
    priv->cover_file_path = NULL;
    priv->cover_set_flag = FALSE;
    if(reference!=NULL)
        playlist_data = g_sequence_get(reference);
    if(playlist_data!=NULL)
    {
        playlist_data = g_sequence_get(reference);
        if(playlist_data!=NULL)
        {
            rc_ui_player_title_label_set_value(priv, uri,
                playlist_data->title);
            rc_ui_player_artist_label_set_value(priv,
                playlist_data->artist);
            rc_ui_player_album_label_set_value(priv,
                playlist_data->album);
        }
    }
    else
    {
        rc_ui_player_title_label_set_value(priv, uri, NULL);
        rc_ui_player_artist_label_set_value(priv, NULL);
        rc_ui_player_album_label_set_value(priv, NULL);
        gtk_label_set_text(GTK_LABEL(priv->info_label),
            _("Unknown Format"));
    }
    if(reference!=NULL)
        filename = g_strdup(rclib_db_playlist_get_album_bind(reference));
    if(filename==NULL)
        filename = rclib_util_search_cover(uri, NULL, NULL, NULL);
    if(filename!=NULL)
    {
        if(rc_ui_player_cover_image_set_file(priv, filename))
            priv->cover_set_flag = TRUE;
    }
    g_free(filename);
}

static void rc_ui_player_spectrum_updated_cb(RCLibCore *core, guint rate,
    guint bands, gfloat threshold, const GValue *magnitudes, gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL || magnitudes==NULL) return;
    rc_ui_spectrum_widget_set_magnitudes(RC_UI_SPECTRUM_WIDGET(
        priv->spectrum_widget), rate, bands, threshold, magnitudes);
}

static gboolean rc_ui_player_play_button_clicked_cb()
{
    GstState state;
    if(rclib_core_get_state(&state, NULL, GST_CLOCK_TIME_NONE))
    {
        if(state==GST_STATE_PLAYING)
        {
            rclib_core_pause();
            return FALSE;
        }
        else if(state==GST_STATE_PAUSED)
        {
            rclib_core_play();
            return FALSE;
        }
    }
    rclib_core_play();
    return FALSE;
}

static gboolean rc_ui_player_stop_button_clicked_cb()
{
    rclib_core_stop();
    return FALSE;
}

static gboolean rc_ui_player_prev_button_clicked_cb()
{
    rclib_player_play_prev(FALSE, TRUE, FALSE);
    return FALSE;
}

static gboolean rc_ui_player_next_button_clicked_cb()
{
    rclib_player_play_next(FALSE, TRUE, FALSE);
    return FALSE;
}

static gboolean rc_ui_player_open_button_clicked_cb()
{
    rc_ui_dialog_add_music();
    return FALSE;
}

static gboolean rc_ui_player_adjust_volume(GtkScaleButton *widget,
    gdouble volume, gpointer data)
{
    g_signal_handlers_block_by_func(widget,
        G_CALLBACK(rc_ui_player_adjust_volume), data);
    rclib_core_set_volume(volume);
    g_signal_handlers_unblock_by_func(widget,
        G_CALLBACK(rc_ui_player_adjust_volume), data);
    return FALSE;
}

static void rc_ui_player_core_state_changed_cb(RCLibCore *core,
    GstState state, gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return;
    if(state==GST_STATE_PLAYING)
    {
        gtk_image_set_from_stock(GTK_IMAGE(priv->ctrl_play_image),
            GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
    }
    else
    {
        gtk_image_set_from_stock(GTK_IMAGE(priv->ctrl_play_image),
            GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR);
    }
    if(state!=GST_STATE_PLAYING && state!=GST_STATE_PAUSED)
    {
        gtk_widget_set_sensitive(priv->time_scale, FALSE);
        rc_ui_player_time_label_set_value(priv, -1);
        rc_ui_scrollable_label_set_text(RC_UI_SCROLLABLE_LABEL(
            priv->lyric1_slabel), NULL);
        rc_ui_scrollable_label_set_text(RC_UI_SCROLLABLE_LABEL(
            priv->lyric2_slabel), NULL);
        rc_ui_spectrum_widget_clean(RC_UI_SPECTRUM_WIDGET(
            priv->spectrum_widget));
    }
    else
    {
        gtk_widget_set_sensitive(priv->time_scale, TRUE);
    }
    gtk_widget_queue_draw(priv->catalog_listview);
    gtk_widget_queue_draw(priv->playlist_listview);
}

static void rc_ui_player_volume_changed_cb(RCLibCore *core, gdouble volume,
    gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL || priv==NULL) return;
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(priv->volume_button),
        volume);
}

static void rc_ui_player_lyric_timer_cb(RCLibLyric *lyric, guint index,
    gint64 pos, const RCLibLyricData *lyric_data, gpointer data)
{
    gint64 passed = 0, len = 0;
    gdouble percent = 0.0;
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL || priv==NULL) return;
    if(lyric_data==NULL)
    {
        if(index==0)
            rc_ui_scrollable_label_set_text(RC_UI_SCROLLABLE_LABEL(
                priv->lyric1_slabel), NULL);
        else
            rc_ui_scrollable_label_set_text(RC_UI_SCROLLABLE_LABEL(
                priv->lyric2_slabel), NULL);
        return;
    }
    if(lyric_data->length>0)
        len = lyric_data->length;
    else
    {
        len = rclib_core_query_duration();
        if(len>0) len = len - lyric_data->time;
    }
    passed = pos - lyric_data->time;
    if(len>0) percent = 1.2 * (gdouble)passed / len;
    if(index==0)
    {
        rc_ui_scrollable_label_set_text(RC_UI_SCROLLABLE_LABEL(
            priv->lyric1_slabel), lyric_data->text);
        rc_ui_scrollable_label_set_percent(RC_UI_SCROLLABLE_LABEL(
            priv->lyric1_slabel), percent);
    }
    else
    {
        rc_ui_scrollable_label_set_text(RC_UI_SCROLLABLE_LABEL(
            priv->lyric2_slabel), lyric_data->text);
        rc_ui_scrollable_label_set_percent(RC_UI_SCROLLABLE_LABEL(
            priv->lyric2_slabel), percent);
    } 
}

static gboolean rc_ui_player_seek_scale_button_pressed(GtkWidget *widget, 
    GdkEventButton *event, gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return FALSE;
    if(event->button==3) return TRUE;
    priv->update_seek_scale = FALSE;

    /* HACK: we want the behaviour you get with the middle button, so we
     * mangle the event.  clicking with other buttons moves the slider in
     * step increments, clicking with the middle button moves the slider to
     * the location of the click.
     */
    event->button = 2;
    return FALSE;
}

static gboolean rc_ui_player_seek_scale_button_released(GtkWidget *widget, 
    GdkEventButton *event, gpointer data)
{
    gdouble percent;
    gint64 pos, duration;
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return FALSE;
    duration = rclib_core_query_duration();
    percent = gtk_range_get_value(GTK_RANGE(widget));

    /* HACK: middle button */
    event->button = 2;
    priv->update_seek_scale = TRUE;
    pos = (gdouble)duration * (percent/100);
    rclib_core_set_position(pos);
    return FALSE;
}

static void rc_ui_player_seek_scale_value_changed(GtkRange *range,
    gpointer data)
{
    gdouble percent;
    gint64 pos, duration;
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return;
    if(!priv->update_seek_scale)
    {
        percent = gtk_range_get_value(range);
        duration = rclib_core_query_duration();
        pos = (gdouble)duration * (percent/100);
        rc_ui_player_time_label_set_value(priv, pos);
    }
}

static gboolean rc_ui_player_update_time_info(gpointer data)
{
    gdouble percent;
    gint64 pos, duration;
    GstState state;
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return TRUE;
    rclib_core_get_state(&state, NULL, GST_CLOCK_TIME_NONE);
    switch(state)
    {
        case GST_STATE_PLAYING:
            if(!priv->update_seek_scale) break;
            duration = rclib_core_query_duration();
            pos = rclib_core_query_position();
            rc_ui_player_time_label_set_value(priv, pos);
            if(duration>0)
                percent = (gdouble)pos / duration * 100;
            else
                percent = 0.0;
            g_signal_handlers_block_by_func(priv->time_scale,
                G_CALLBACK(rc_ui_player_seek_scale_value_changed), data);
            gtk_range_set_value(GTK_RANGE(priv->time_scale),
                percent);
            g_signal_handlers_unblock_by_func(priv->time_scale,
                G_CALLBACK(rc_ui_player_seek_scale_value_changed), data);
            break;
        case GST_STATE_PAUSED:
            break;
        default:
            rc_ui_player_time_label_set_value(priv, -1);
            gtk_range_set_value(GTK_RANGE(priv->time_scale), 0.0);
            break;
    }
    return TRUE;
}

static void rc_ui_player_import_updated_cb(RCLibDb *db, gint remaining,
    gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    gboolean active;
    if(data==NULL) return;
    gchar *text;
    g_object_get(priv->progress_spinner, "active", &active, NULL);
    if(remaining>0)
    {
        priv->import_work_flag = TRUE;
        gtk_widget_show(priv->progress_eventbox);
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressImportCancel"), TRUE);
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressImportStatus"), TRUE);
        text = g_strdup_printf(_("Importing: %d remaining"), remaining);
        gtk_action_set_label(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressImportStatus"), text);
        g_free(text);
        if(!active) gtk_spinner_start(GTK_SPINNER(priv->progress_spinner));
    }
    else
    {
        priv->import_work_flag = FALSE;
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressImportCancel"), FALSE);
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressImportStatus"), FALSE);
        gtk_action_set_label(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressImportStatus"),
            _("Importing: 0 remaining"));
        if(!priv->refresh_work_flag)
        {
            gtk_widget_hide(priv->progress_eventbox);
            if(active) gtk_spinner_stop(GTK_SPINNER(priv->progress_spinner));
        }
    }
}

static void rc_ui_player_refresh_updated_cb(RCLibDb *db, gint remaining,
    gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    gboolean active;
    gchar *text;
    if(data==NULL) return;
    g_object_get(priv->progress_spinner, "active", &active, NULL);
    if(remaining>0)
    {
        priv->refresh_work_flag = TRUE;
        gtk_widget_show(priv->progress_eventbox);
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressRefreshCancel"), TRUE);
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressRefreshStatus"), TRUE);
        text = g_strdup_printf(_("Refreshing: %d remaining"), remaining);
        gtk_action_set_label(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressRefreshStatus"), text);
        g_free(text);
        if(!active) gtk_spinner_start(GTK_SPINNER(priv->progress_spinner));
    }
    else
    {
        priv->refresh_work_flag = FALSE;
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressRefreshCancel"), FALSE);
        gtk_action_set_visible(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressRefreshStatus"), FALSE);
        gtk_action_set_label(gtk_ui_manager_get_action(priv->ui_manager,
            "/ProgressPopupMenu/ProgressRefreshStatus"),
            _("Refreshing: 0 remaining"));
        if(!priv->import_work_flag)
        {
            gtk_widget_hide(priv->progress_eventbox);
            if(active) gtk_spinner_stop(GTK_SPINNER(priv->progress_spinner));
        }
    }
}

static void rc_ui_player_tray_icon_popup(GtkStatusIcon *icon, guint button,
    guint activate_time, gpointer data)  
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return;
    gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(priv->ui_manager,
        "/TrayPopupMenu")), NULL, NULL, gtk_status_icon_position_menu,
        icon, 3, activate_time);
}

static void rc_ui_player_tray_icon_activated(GtkStatusIcon *icon,
    gpointer data)
{

}

static gboolean rc_ui_player_album_menu_popup(GtkWidget *widget,
    GdkEventButton *event, gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return FALSE;
    if(event->button!=3) return FALSE;
    gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(priv->ui_manager,
        "/AlbumPopupMenu")), NULL, NULL, NULL, widget, 3, event->time);
    return FALSE;
}

static gboolean rc_ui_player_main_menu_popup(GtkWidget *widget,
    GdkEventButton *event, gpointer data)  
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return FALSE;
    gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(priv->ui_manager,
        "/RC2MainPopupMenu")), NULL, NULL, NULL, widget, 1, event->time);
    return FALSE;
}

static gboolean rc_ui_player_progress_menu_popup(GtkWidget *widget,
    GdkEventButton *event, gpointer data)  
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(data==NULL) return FALSE;
    gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(priv->ui_manager,
        "/ProgressPopupMenu")), NULL, NULL, NULL, widget, 1, event->time);
    return FALSE;
}

static void rc_ui_player_destroy(GtkWidget *widget, gpointer data)
{
    RCUiPlayerPrivate *priv = (RCUiPlayerPrivate *)data;
    if(priv!=NULL)
        gtk_widget_destroyed(priv->main_window, &(priv->main_window));
    gtk_main_quit();
}

static void rc_ui_player_set_default_style(RCUiPlayerPrivate *priv)
{
    PangoAttrList *title_attr_list, *artist_attr_list, *album_attr_list;
    PangoAttrList *time_attr_list, *length_attr_list, *info_attr_list;
    PangoAttrList *catalog_attr_list, *playlist_attr_list;
    PangoAttrList *lyric_attr_list, *progress_attr_list;
    PangoAttribute *title_attr[2], *artist_attr[2], *album_attr[2];
    PangoAttribute *time_attr[2], *length_attr[2], *info_attr[2];
    PangoAttribute *catalog_attr[2], *playlist_attr[2], *lyric_attr[2];
    PangoAttribute *progress_attr[2];
    title_attr_list = pango_attr_list_new();
    artist_attr_list = pango_attr_list_new();
    album_attr_list = pango_attr_list_new();
    time_attr_list = pango_attr_list_new();
    length_attr_list = pango_attr_list_new();
    info_attr_list = pango_attr_list_new();
    catalog_attr_list = pango_attr_list_new();
    playlist_attr_list = pango_attr_list_new();
    lyric_attr_list = pango_attr_list_new();
    progress_attr_list = pango_attr_list_new();
    title_attr[0] = pango_attr_size_new_absolute(17 * PANGO_SCALE);
    title_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
    artist_attr[0] = pango_attr_size_new_absolute(14 * PANGO_SCALE);
    artist_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    album_attr[0] = pango_attr_size_new_absolute(14 * PANGO_SCALE);
    album_attr[1] = pango_attr_style_new(PANGO_STYLE_ITALIC);
    time_attr[0] = pango_attr_size_new_absolute(26 * PANGO_SCALE);
    time_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    length_attr[0] = pango_attr_size_new_absolute(15 * PANGO_SCALE);
    length_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    info_attr[0] = pango_attr_size_new_absolute(12 * PANGO_SCALE);
    info_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    catalog_attr[0] = pango_attr_size_new_absolute(13 * PANGO_SCALE);
    catalog_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    playlist_attr[0] = pango_attr_size_new_absolute(13 * PANGO_SCALE);
    playlist_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    lyric_attr[0] = pango_attr_size_new_absolute(13 * PANGO_SCALE);
    lyric_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    progress_attr[0] = pango_attr_size_new_absolute(13 * PANGO_SCALE);
    progress_attr[1] = pango_attr_weight_new(PANGO_WEIGHT_NORMAL);
    pango_attr_list_insert(title_attr_list, title_attr[0]);
    pango_attr_list_insert(title_attr_list, title_attr[1]);
    pango_attr_list_insert(artist_attr_list, artist_attr[0]);
    pango_attr_list_insert(artist_attr_list, artist_attr[1]);
    pango_attr_list_insert(album_attr_list, album_attr[0]);
    pango_attr_list_insert(album_attr_list, album_attr[1]);
    pango_attr_list_insert(time_attr_list, time_attr[0]);
    pango_attr_list_insert(time_attr_list, time_attr[1]);
    pango_attr_list_insert(info_attr_list, info_attr[0]);
    pango_attr_list_insert(info_attr_list, info_attr[1]);
    pango_attr_list_insert(length_attr_list, length_attr[0]);
    pango_attr_list_insert(length_attr_list, length_attr[1]);
    pango_attr_list_insert(catalog_attr_list, catalog_attr[0]);
    pango_attr_list_insert(catalog_attr_list, catalog_attr[1]);
    pango_attr_list_insert(playlist_attr_list, playlist_attr[0]);
    pango_attr_list_insert(playlist_attr_list, playlist_attr[1]);
    pango_attr_list_insert(lyric_attr_list, lyric_attr[0]);
    pango_attr_list_insert(lyric_attr_list, lyric_attr[1]);
    pango_attr_list_insert(progress_attr_list, progress_attr[0]);
    pango_attr_list_insert(progress_attr_list, progress_attr[1]);
    gtk_label_set_attributes(GTK_LABEL(priv->title_label),
        title_attr_list);
    pango_attr_list_unref(title_attr_list);
    gtk_label_set_attributes(GTK_LABEL(priv->artist_label),
        artist_attr_list);
    pango_attr_list_unref(artist_attr_list);
    gtk_label_set_attributes(GTK_LABEL(priv->album_label),
        album_attr_list);
    pango_attr_list_unref(album_attr_list);
    gtk_label_set_attributes(GTK_LABEL(priv->info_label),
        info_attr_list);
    pango_attr_list_unref(info_attr_list);
    gtk_label_set_attributes(GTK_LABEL(priv->time_label),
        time_attr_list);
    pango_attr_list_unref(time_attr_list);
    gtk_label_set_attributes(GTK_LABEL(priv->length_label),
        length_attr_list);
    pango_attr_list_unref(length_attr_list);
    rc_ui_listview_catalog_set_pango_attributes(catalog_attr_list);
    rc_ui_listview_playlist_set_pango_attributes(playlist_attr_list);
    pango_attr_list_unref(catalog_attr_list);
    pango_attr_list_unref(playlist_attr_list);
    rc_ui_scrollable_label_set_attributes(RC_UI_SCROLLABLE_LABEL(
        priv->lyric1_slabel), lyric_attr_list);
    rc_ui_scrollable_label_set_attributes(RC_UI_SCROLLABLE_LABEL(
        priv->lyric2_slabel), lyric_attr_list);
    pango_attr_list_unref(lyric_attr_list);
    gtk_label_set_attributes(GTK_LABEL(priv->progress_label),
        progress_attr_list);
    pango_attr_list_unref(progress_attr_list);
    gtk_widget_set_size_request(priv->time_scale, -1, 20);
    gtk_widget_set_size_request(gtk_scrolled_window_get_vscrollbar(
        GTK_SCROLLED_WINDOW(priv->catalog_scr_window)), 15, -1);
    gtk_widget_set_size_request(gtk_scrolled_window_get_vscrollbar(
        GTK_SCROLLED_WINDOW(priv->playlist_scr_window)), 15, -1);
}

static void rc_ui_player_layout_init(RCUiPlayerPrivate *priv)
{
    GtkWidget *panel_hbox;
    GtkWidget *panel_vbox;
    GtkWidget *info_hbox;
    GtkWidget *mmd_vbox;
    GtkWidget *time_vbox;
    GtkWidget *lyric_vbox;
    GtkWidget *ctrl_button_hbox;
    GtkWidget *player_vbox;
    GtkWidget *buttom_hbox;
    GtkWidget *progress_hbox;
    player_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    panel_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    panel_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    mmd_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
    time_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    lyric_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    info_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    progress_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    ctrl_button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    buttom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(priv->catalog_scr_window),
        priv->catalog_listview);
    gtk_container_add(GTK_CONTAINER(priv->playlist_scr_window),
        priv->playlist_listview);
    gtk_paned_pack1(GTK_PANED(priv->list_paned), priv->catalog_scr_window,
        TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(priv->list_paned), priv->playlist_scr_window,
        TRUE, FALSE);
    gtk_container_child_set(GTK_CONTAINER(priv->list_paned),
        priv->catalog_scr_window, "resize", FALSE, "shrink", FALSE, NULL);
    gtk_container_set_border_width(GTK_CONTAINER(priv->list_paned), 0);
    gtk_paned_set_position(GTK_PANED(priv->list_paned), 160);
    gtk_container_add(GTK_CONTAINER(priv->album_eventbox), priv->album_image);
    gtk_container_add(GTK_CONTAINER(priv->album_frame), priv->album_eventbox);
    gtk_box_pack_start(GTK_BOX(progress_hbox), priv->progress_spinner, FALSE,
        FALSE, 0);
    gtk_box_pack_start(GTK_BOX(progress_hbox), priv->progress_label, FALSE,
        FALSE, 0);
    gtk_container_add(GTK_CONTAINER(priv->progress_eventbox), progress_hbox);
    gtk_box_pack_start(GTK_BOX(mmd_vbox), priv->title_label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mmd_vbox), priv->artist_label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mmd_vbox), priv->album_label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mmd_vbox), priv->info_label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(time_vbox), priv->time_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(time_vbox), priv->length_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctrl_button_hbox), priv->ctrl_prev_button,
        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctrl_button_hbox), priv->ctrl_play_button,
        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctrl_button_hbox), priv->ctrl_stop_button,
        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctrl_button_hbox), priv->ctrl_next_button,
        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ctrl_button_hbox), priv->time_scale, TRUE,
        TRUE, 0);
    gtk_box_pack_end(GTK_BOX(ctrl_button_hbox), priv->volume_button, FALSE,
        FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttom_hbox), priv->menu_eventbox, FALSE,
        FALSE, 0);
    gtk_box_pack_end(GTK_BOX(buttom_hbox), priv->progress_eventbox, FALSE,
        FALSE, 5);
    gtk_box_pack_start(GTK_BOX(info_hbox), mmd_vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(info_hbox), time_vbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(lyric_vbox), priv->lyric1_slabel, FALSE,
        FALSE, 0);
    gtk_box_pack_start(GTK_BOX(lyric_vbox), priv->lyric2_slabel, FALSE,
        FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel_vbox), info_hbox, FALSE,
        FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel_vbox), lyric_vbox, FALSE, FALSE, 4);
    gtk_box_pack_end(GTK_BOX(panel_vbox), priv->spectrum_widget, TRUE,
        TRUE, 8);
    gtk_box_pack_start(GTK_BOX(panel_hbox), priv->album_frame, FALSE,
        FALSE, 2);
    gtk_box_pack_start(GTK_BOX(panel_hbox), panel_vbox, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(player_vbox), panel_hbox, FALSE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(player_vbox), ctrl_button_hbox, FALSE,
        FALSE, 0);
    gtk_box_pack_start(GTK_BOX(player_vbox), priv->list_paned, TRUE,
        TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->main_vbox), player_vbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(priv->main_vbox), buttom_hbox, FALSE,
        FALSE, 0);
    gtk_container_add(GTK_CONTAINER(priv->main_window), priv->main_vbox);
}

static void rc_ui_player_signal_bind(RCUiPlayerPrivate *priv)
{
    priv->update_timeout = g_timeout_add(250, (GSourceFunc)
        rc_ui_player_update_time_info, priv);
    g_signal_connect(G_OBJECT(priv->ctrl_play_button), "clicked",
        G_CALLBACK(rc_ui_player_play_button_clicked_cb), priv);
    g_signal_connect(G_OBJECT(priv->ctrl_stop_button), "clicked",
        G_CALLBACK(rc_ui_player_stop_button_clicked_cb), priv);
    g_signal_connect(G_OBJECT(priv->ctrl_prev_button), "clicked",
        G_CALLBACK(rc_ui_player_prev_button_clicked_cb), priv);
    g_signal_connect(G_OBJECT(priv->ctrl_next_button), "clicked",
        G_CALLBACK(rc_ui_player_next_button_clicked_cb), priv);
    g_signal_connect(G_OBJECT(priv->ctrl_open_button), "clicked",
        G_CALLBACK(rc_ui_player_open_button_clicked_cb), priv);
    g_signal_connect(G_OBJECT(priv->album_eventbox), "button-release-event",
        G_CALLBACK(rc_ui_player_album_menu_popup), priv);
    g_signal_connect(G_OBJECT(priv->menu_eventbox), "button-press-event",
        G_CALLBACK(rc_ui_player_main_menu_popup), priv);
    g_signal_connect(G_OBJECT(priv->progress_eventbox), "button-press-event",
        G_CALLBACK(rc_ui_player_progress_menu_popup), priv);
    g_signal_connect(G_OBJECT(priv->volume_button), "value-changed",
        G_CALLBACK(rc_ui_player_adjust_volume), priv);
    g_signal_connect(G_OBJECT(priv->time_scale), "button-press-event",
        G_CALLBACK(rc_ui_player_seek_scale_button_pressed), priv);
    g_signal_connect(G_OBJECT(priv->time_scale), "button-release-event",
        G_CALLBACK(rc_ui_player_seek_scale_button_released), priv);
    g_signal_connect(G_OBJECT(priv->time_scale), "scroll-event",
        G_CALLBACK(gtk_true), priv);
    g_signal_connect(G_OBJECT(priv->time_scale), "value-changed",
        G_CALLBACK(rc_ui_player_seek_scale_value_changed), priv);
    g_signal_connect(GTK_STATUS_ICON(priv->tray_icon), "activate", 
        G_CALLBACK(rc_ui_player_tray_icon_activated), priv);
    g_signal_connect(GTK_STATUS_ICON(priv->tray_icon), "popup-menu",
        G_CALLBACK(rc_ui_player_tray_icon_popup), priv);
    g_signal_connect(G_OBJECT(priv->main_window), "destroy",
        G_CALLBACK(rc_ui_player_destroy), priv);
    rclib_core_signal_connect("tag-found",
        G_CALLBACK(rc_ui_player_tag_found_cb), priv);
    rclib_core_signal_connect("new-duration",
        G_CALLBACK(rc_ui_player_new_duration_cb), priv);
    rclib_core_signal_connect("state-changed",
        G_CALLBACK(rc_ui_player_core_state_changed_cb), priv);
    rclib_core_signal_connect("uri-changed",
        G_CALLBACK(rc_ui_player_uri_changed_cb), priv);
    rclib_core_signal_connect("volume-changed",
        G_CALLBACK(rc_ui_player_volume_changed_cb), priv);
    rclib_core_signal_connect("spectrum-updated",
        G_CALLBACK(rc_ui_player_spectrum_updated_cb), priv);
    rclib_lyric_signal_connect("lyric-timer",
        G_CALLBACK(rc_ui_player_lyric_timer_cb), priv);
    rclib_db_signal_connect("import-updated",
        G_CALLBACK(rc_ui_player_import_updated_cb), priv);
    rclib_db_signal_connect("refresh-updated",
        G_CALLBACK(rc_ui_player_refresh_updated_cb), priv);
}

static void rc_ui_player_finalize(GObject *object)
{
    RCUiPlayerPrivate *priv = RC_UI_PLAYER_GET_PRIVATE(RC_UI_PLAYER(object));
    g_source_remove(priv->update_timeout);
    if(priv->main_window!=NULL) gtk_widget_destroy(priv->main_window);
    g_object_unref(priv->app);
}

static void rc_ui_player_class_init(RCUiPlayerClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    object_class->finalize = rc_ui_player_finalize;
    g_type_class_add_private(klass, sizeof(RCUiPlayerPrivate));
}

static void rc_ui_player_instance_init(RCUiPlayer *ui)
{
    RCUiPlayerPrivate *priv = NULL;
    GdkGeometry main_window_hints = {0};
    GtkAdjustment *position_adjustment;
    gdouble volume = 1.0;
    GList *window_list = NULL;
    priv = RC_UI_PLAYER_GET_PRIVATE(ui);
    priv->app = g_object_ref(ui_player_app);
    priv->cover_set_flag = FALSE;
    priv->update_seek_scale = TRUE;
    priv->cover_image_width = 160;
    priv->cover_image_height = 160;
    priv->cover_default_pixbuf = gdk_pixbuf_new_from_xpm_data(
        (const gchar **)&ui_image_default_cover);
    priv->icon_pixbuf = gdk_pixbuf_new_from_xpm_data(
        (const gchar **)&ui_image_icon);
    priv->menu_pixbuf = gdk_pixbuf_scale_simple(priv->icon_pixbuf, 24, 24,
        GDK_INTERP_HYPER);
    main_window_hints.min_width = 500;
    main_window_hints.min_height = 360;
    main_window_hints.base_width = 600;
    main_window_hints.base_height = 400;
    position_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 100.0,
        1.0, 2.0, 0.0));
    priv->tray_icon = gtk_status_icon_new_from_pixbuf(priv->icon_pixbuf);
    if(priv->app!=NULL)
        window_list = gtk_application_get_windows(priv->app);
    if(window_list!=NULL)
        priv->main_window = GTK_WIDGET(window_list->data);
    else
    {
        priv->main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_application(GTK_WINDOW(priv->main_window),
            priv->app);
    }
    priv->title_label = gtk_label_new(_("Unknown Title"));
    priv->artist_label = gtk_label_new(_("Unknown Artist"));
    priv->album_label = gtk_label_new(_("Unknown Album"));
    priv->info_label = gtk_label_new(_("Unknown Format"));
    priv->time_label = gtk_label_new("--:--");
    priv->length_label = gtk_label_new("00:00");
    priv->progress_label = gtk_label_new(_("Working"));
    priv->album_image = gtk_image_new_from_pixbuf(priv->cover_default_pixbuf);
    priv->album_eventbox = gtk_event_box_new();
    priv->album_frame = gtk_frame_new(NULL);
    priv->lyric1_slabel = rc_ui_scrollable_label_new();
    priv->lyric2_slabel = rc_ui_scrollable_label_new();
    priv->spectrum_widget = rc_ui_spectrum_widget_new();
    priv->volume_button = gtk_volume_button_new();
    priv->progress_spinner = gtk_spinner_new();
    priv->progress_eventbox = gtk_event_box_new();
    priv->ctrl_prev_image = gtk_image_new_from_stock(
        GTK_STOCK_MEDIA_PREVIOUS, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->ctrl_play_image = gtk_image_new_from_stock(
        GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->ctrl_stop_image = gtk_image_new_from_stock(
        GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->ctrl_next_image = gtk_image_new_from_stock(
        GTK_STOCK_MEDIA_NEXT, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->ctrl_open_image = gtk_image_new_from_stock(
        GTK_STOCK_OPEN, GTK_ICON_SIZE_SMALL_TOOLBAR);
    priv->menu_image = gtk_image_new_from_pixbuf(priv->menu_pixbuf);
    priv->ctrl_play_button = gtk_button_new();
    priv->ctrl_stop_button = gtk_button_new();
    priv->ctrl_prev_button = gtk_button_new();
    priv->ctrl_next_button = gtk_button_new();
    priv->ctrl_open_button = gtk_button_new();
    priv->menu_eventbox = gtk_event_box_new();
    priv->time_scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL,
        position_adjustment);
    rc_ui_listview_init();
    priv->catalog_listview = rc_ui_listview_get_catalog_widget();
    priv->playlist_listview = rc_ui_listview_get_playlist_widget();
    priv->catalog_scr_window = gtk_scrolled_window_new(NULL, NULL);
    priv->playlist_scr_window = gtk_scrolled_window_new(NULL, NULL);
    rc_ui_menu_init();
    priv->ui_manager = rc_ui_menu_get_ui_manager();
    gtk_window_add_accel_group(GTK_WINDOW(priv->main_window), 
        gtk_ui_manager_get_accel_group(priv->ui_manager));
    priv->list_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    priv->main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(priv->main_window, "RC2MainWindow");
    gtk_widget_set_name(priv->title_label, "RC2TitleLabel");
    gtk_widget_set_name(priv->artist_label, "RC2ArtistLabel");
    gtk_widget_set_name(priv->album_label, "RC2AlbumLabel");
    gtk_widget_set_name(priv->time_label, "RC2TimeLabel");
    gtk_widget_set_name(priv->length_label, "RC2LengthLabel");
    gtk_widget_set_name(priv->album_image, "RC2AlbumImage");
    gtk_widget_set_name(priv->lyric1_slabel, "RC2Lyric1ScrollableLabel");
    gtk_widget_set_name(priv->lyric2_slabel, "RC2Lyric2ScrollableLabel");
    gtk_widget_set_name(priv->volume_button, "RC2VolumeButton");
    gtk_widget_set_name(priv->progress_spinner, "RC2ProgressSpinner");
    gtk_widget_set_name(priv->ctrl_play_button, "RC2ControlButton");
    gtk_widget_set_name(priv->ctrl_stop_button, "RC2ControlButton");
    gtk_widget_set_name(priv->ctrl_prev_button, "RC2ControlButton");
    gtk_widget_set_name(priv->ctrl_next_button, "RC2ControlButton");
    gtk_widget_set_name(priv->ctrl_open_button, "RC2ControlButton");
    gtk_widget_set_name(priv->ctrl_open_button, "RC2MenuButton");
    gtk_widget_set_name(priv->catalog_scr_window, "RC2ListScrolledWindow");
    gtk_widget_set_name(priv->playlist_scr_window, "RC2ListScrolledWindow");
    gtk_widget_set_name(priv->list_paned, "RC2ListPaned");
    gtk_window_set_title(GTK_WINDOW(priv->main_window),
        "RhythmCat2 Music Player");
    gtk_window_set_icon(GTK_WINDOW(priv->main_window), priv->icon_pixbuf);
    gtk_window_set_position(GTK_WINDOW(priv->main_window),
        GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(priv->main_window), 600, 400);
    gtk_window_set_geometry_hints(GTK_WINDOW(priv->main_window), 
        GTK_WIDGET(priv->main_window), &main_window_hints,
        GDK_HINT_MIN_SIZE);
    gtk_window_set_has_resize_grip(GTK_WINDOW(priv->main_window), FALSE);
    gtk_misc_set_alignment(GTK_MISC(priv->title_label), 0.0, 0.5);
    gtk_misc_set_alignment(GTK_MISC(priv->artist_label), 0.0, 0.5);
    gtk_misc_set_alignment(GTK_MISC(priv->album_label), 0.0, 0.5);
    gtk_misc_set_alignment(GTK_MISC(priv->info_label), 0.0, 0.5);
    gtk_misc_set_alignment(GTK_MISC(priv->time_label), 1.0, 0.5);
    gtk_misc_set_alignment(GTK_MISC(priv->length_label), 1.0, 0.5);
    gtk_label_set_justify(GTK_LABEL(priv->time_label), GTK_JUSTIFY_RIGHT);
    gtk_label_set_ellipsize(GTK_LABEL(priv->title_label),
        PANGO_ELLIPSIZE_END);
    gtk_label_set_ellipsize(GTK_LABEL(priv->artist_label),
        PANGO_ELLIPSIZE_END);
    gtk_label_set_ellipsize(GTK_LABEL(priv->album_label),
        PANGO_ELLIPSIZE_END);
    gtk_label_set_ellipsize(GTK_LABEL(priv->info_label),
        PANGO_ELLIPSIZE_END);
    gtk_widget_set_size_request(priv->album_image, priv->cover_image_width,
        priv->cover_image_height);
    g_object_set(G_OBJECT(priv->tray_icon), "has-tooltip", TRUE,
        "tooltip-text", "RhythmCat2 Music Player", "title",
        "RhythmCat2", NULL);
    gtk_button_set_relief(GTK_BUTTON(priv->volume_button), GTK_RELIEF_NONE);
    g_object_set(priv->volume_button, "can-focus", FALSE, NULL);
    gtk_scale_set_draw_value(GTK_SCALE(priv->time_scale), FALSE);
    g_object_set(priv->time_scale, "can-focus", FALSE, NULL);
    gtk_widget_set_name(priv->time_scale, "RCTimeScalerBar");
    g_object_set(G_OBJECT(priv->volume_button), "size",
        GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
    if(rclib_core_get_volume(&volume))
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(priv->volume_button),
            volume);
    else
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(priv->volume_button),
            1.0);
    gtk_button_set_relief(GTK_BUTTON(priv->ctrl_prev_button),
        GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(priv->ctrl_play_button),
        GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(priv->ctrl_stop_button),
        GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(priv->ctrl_next_button),
        GTK_RELIEF_NONE);
    gtk_button_set_relief(GTK_BUTTON(priv->ctrl_open_button),
        GTK_RELIEF_NONE);
    g_object_set(priv->ctrl_play_button, "can-focus", FALSE, NULL);
    g_object_set(priv->ctrl_stop_button, "can-focus", FALSE, NULL);
    g_object_set(priv->ctrl_prev_button, "can-focus", FALSE, NULL);
    g_object_set(priv->ctrl_next_button, "can-focus", FALSE, NULL);
    g_object_set(priv->ctrl_open_button, "can-focus", FALSE, NULL);
    gtk_container_add(GTK_CONTAINER(priv->ctrl_play_button),
        priv->ctrl_play_image);
    gtk_container_add(GTK_CONTAINER(priv->ctrl_stop_button),
        priv->ctrl_stop_image);
    gtk_container_add(GTK_CONTAINER(priv->ctrl_prev_button),
        priv->ctrl_prev_image);
    gtk_container_add(GTK_CONTAINER(priv->ctrl_next_button),
        priv->ctrl_next_image);
    gtk_container_add(GTK_CONTAINER(priv->ctrl_open_button),
        priv->ctrl_open_image);
    gtk_container_add(GTK_CONTAINER(priv->menu_eventbox), priv->menu_image);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(
        priv->catalog_scr_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(
        priv->playlist_scr_window), GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
    rc_ui_player_layout_init(priv);
    rc_ui_player_set_default_style(priv);
    rc_ui_player_signal_bind(priv);
    gtk_widget_show_all(priv->main_window);
    gtk_widget_hide(priv->progress_eventbox);
}

GType rc_ui_player_get_type()
{
    static GType ui_type = 0;
    static const GTypeInfo ui_info = {
        .class_size = sizeof(RCUiPlayerClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_ui_player_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCUiPlayer),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_ui_player_instance_init
    };
    if(!ui_type)
    {
        ui_type = g_type_register_static(G_TYPE_OBJECT, "RCUiPlayer",
            &ui_info, 0);
    }
    return ui_type;
}

/**
 * rc_ui_player_init:
 * @app: the #GtkApplication for the program
 *
 * Initialize the main UI of the player.
 */

void rc_ui_player_init(GtkApplication *app)
{
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, "Loading main UI....");
    if(ui_player_instance!=NULL)
    {
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
            "Main UI is already initialized!");
        return;
    }
    ui_player_app = app;
    ui_player_instance = g_object_new(RC_UI_PLAYER_TYPE, NULL);
    rc_ui_menu_state_refresh();
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, "Main UI loaded.");
}

/**
 * rc_ui_player_exit:
 *
 * Unload the running instance of the main UI.
 */

void rc_ui_player_exit()
{
    g_object_unref(ui_player_instance);
    ui_player_instance = NULL;
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, "Main UI exited.");
}

/**
 * rc_ui_player_get_main_window:
 *
 * Get the main window of this player.
 *
 * Returns: The main window.
 */

GtkWidget *rc_ui_player_get_main_window()
{
    RCUiPlayerPrivate *priv = NULL;
    if(ui_player_instance==NULL) return NULL;
    priv = RC_UI_PLAYER_GET_PRIVATE(ui_player_instance);
    if(priv==NULL) return NULL;
    return priv->main_window;
}

/**
 * rc_ui_player_get_icon_image:
 *
 * Get the icon image of this player.
 *
 * Returns: The icon image of this player.
 */

const GdkPixbuf *rc_ui_player_get_icon_image()
{
    RCUiPlayerPrivate *priv = NULL;
    if(ui_player_instance==NULL) return NULL;
    priv = RC_UI_PLAYER_GET_PRIVATE(ui_player_instance);
    if(priv==NULL) return NULL;
    return priv->icon_pixbuf;
}


