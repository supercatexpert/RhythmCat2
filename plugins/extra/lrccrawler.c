/*
 * Lyric Crawler
 * Search and download lyric files from the Internet.
 *
 * crawler.c
 * This file is part of <RhythmCat>
 *
 * Copyright (C) 2010 - SuperCat, license: GPL v3
 *
 * <RhythmCat> is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * <RhythmCat> is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with <RhythmCat>; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <curl/curl.h>
#include <gst/gst.h>
#include <gtk/gtk.h>
#include <rclib.h>
#include <rc-ui-menu.h>
#include "lrccrawler-common.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif

#define G_LOG_DOMAIN "LyricCrawler"
#define LRCCRAWLER_ID "rc2-lyric-crawler"

enum
{
    CRAWLER_RESULT_COLUMN_TITLE,
    CRAWLER_RESULT_COLUMN_ARTIST,
    CRAWLER_RESULT_COLUMN_URL,
    CRAWLER_RESULT_COLUMN_LAST
};

enum
{
    CRAWLER_SEARCH_COLUMN_TEXT,
    CRAWLER_SEARCH_COLUMN_INDEX,
    CRAWLER_SEARCH_COLUMN_MODULE,
    CRAWLER_SEARCH_COLUMN_LAST
};

typedef struct RCPluginLyricCrawlerDownResultData
{
    gboolean result;
    gboolean auto_flag;
    gchar *path;
}RCPluginLyricCrawlerDownResultData;

typedef struct RCPluginLyricCrawlerPrivate
{
    GtkWidget *search_window;
    GtkWidget *server_combobox;
    GtkWidget *result_treeview;
    GtkWidget *title_entry;
    GtkWidget *artist_entry;
    GtkWidget *search_button;
    GtkWidget *download_button;
    GtkWidget *save_file_entry;
    GtkWidget *result_label;
    GtkWidget *auto_search_checkbutton;
    GtkToggleAction *action;
    GSList *crawler_module_list;
    gboolean auto_search;
    gboolean auto_search_mode;
    RCLyricCrawlerModule *current_module;
    GThread *search_thread;
    GThread *download_thread;
    gulong missing_signal;
    guint menu_id;
    GKeyFile *keyfile;
}RCPluginLyricCrawlerPrivate;

static RCPluginLyricCrawlerPrivate lyric_crawler_priv = {0};

static gboolean rc_plugin_lrccrawler_search_idle_func(gpointer data)
{
    GSList *list = data;
    GSList *list_foreach;
    GtkTreeIter iter;
    gchar *tmp;
    guint len = 0;
    RCLyricCrawlerSearchData *search_data;
    GtkTreePath *path;
    GtkListStore *store = NULL;
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    if(data==NULL)
    {
        if(priv->search_window!=NULL)
        {
            gtk_label_set_text(GTK_LABEL(priv->result_label),
                _("No matched lyric was found"));
            gtk_widget_set_sensitive(priv->search_button, TRUE);
        }
        return FALSE;
    }
    if(priv->search_window!=NULL)
    {
        store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(
            priv->result_treeview)));
    }
    if(store!=NULL) gtk_list_store_clear(store);
    for(list_foreach=list;list_foreach!=NULL;
        list_foreach=g_slist_next(list_foreach))
    {
        search_data = list_foreach->data;
        if(search_data==NULL) continue;
        if(store!=NULL)
        {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, CRAWLER_RESULT_COLUMN_TITLE,
                search_data->title, CRAWLER_RESULT_COLUMN_ARTIST,
                search_data->artist, CRAWLER_RESULT_COLUMN_URL,
                search_data->url, -1);
        }
        g_free(search_data->url);
        g_free(search_data->title);
        g_free(search_data->artist);
        g_free(search_data);
        len++;
    }
    g_slist_free(list);
    if(len>0)
    {
        tmp = g_strdup_printf(_("Matched lyric number: %u"), len);
        if(priv->search_window!=NULL)
            gtk_label_set_text(GTK_LABEL(priv->result_label), tmp);
        g_free(tmp);
        if(priv->search_window!=NULL)
        {
            gtk_widget_set_sensitive(priv->search_button, TRUE);
            gtk_widget_set_sensitive(priv->download_button, TRUE);
            path = gtk_tree_path_new_first();
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(priv->result_treeview),
                path, NULL, FALSE);
            gtk_tree_path_free(path);
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(priv->action), TRUE);
        }
    }
    else
    {
        if(priv->search_window!=NULL)
        {
            gtk_label_set_text(GTK_LABEL(priv->result_label),
                _("No matched lyric was found"));
        }
    }    
    return FALSE;
}

static gboolean rc_plugin_lrccrawler_download_idle_func(gpointer data)
{
    RCPluginLyricCrawlerDownResultData *down_data =
        (RCPluginLyricCrawlerDownResultData *)data;
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    if(data==NULL) return FALSE;
    if(down_data->result)
    {
        if(priv->search_window!=NULL)
        {
            gtk_label_set_text(GTK_LABEL(priv->result_label),
                _("Downloaded successfully"));
        }
        if(down_data->auto_flag && down_data->path!=NULL)
            rclib_lyric_load_file(down_data->path, 0);
    }
    else
    {
        if(priv->search_window!=NULL)
        {
            gtk_label_set_text(GTK_LABEL(priv->result_label),
                _("Failed to downloaded"));
        }
    }
    g_free(down_data->path);
    g_free(down_data);
    if(priv->search_window!=NULL)
    {
        gtk_widget_set_sensitive(priv->search_button, TRUE);
        gtk_widget_set_sensitive(priv->download_button, TRUE);
    }
    return FALSE;
}

static gpointer rc_plugin_lrccrawler_down_lyric_thread_func(
    gpointer data)
{
    gchar **args = (gchar **)data;
    gboolean flag;
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    RCPluginLyricCrawlerDownResultData *down_data;
    if(args==NULL || args[0]==NULL || args[1]==NULL ||
        priv->current_module==NULL || priv->current_module->info==NULL)
    {
        if(args[0]!=NULL) g_free(args[0]);
        if(args[1]!=NULL) g_free(args[1]);
        g_free(args);
        priv->download_thread = NULL;
        g_thread_exit(NULL);
        return NULL;
    }
    g_debug("Download Thread started, downloading lyric file....");
    flag = priv->current_module->info->download_file(args[0], args[1]);
    if(flag)
        g_debug("Download lyric file successfully. :)");
    else
        g_debug("Cannot download or write lyric file. :(");
    down_data = g_new0(RCPluginLyricCrawlerDownResultData, 1);
    down_data->result = flag;
    down_data->auto_flag = priv->auto_search_mode;
    down_data->path = g_strdup(args[1]);
    priv->auto_search_mode = FALSE;
    g_idle_add(rc_plugin_lrccrawler_download_idle_func, down_data);
    g_free(args[0]);
    g_free(args[1]);
    g_free(args);
    priv->download_thread = NULL;
    g_thread_exit(NULL);
    return NULL;
}

static gpointer rc_plugin_lrccrawler_search_lyric_thread_func(
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    RCLibTagMetadata *mmd = data;
    GSList *list;
    if(priv->current_module==NULL || priv->current_module->info==NULL ||
        mmd==NULL)
    {
        priv->search_thread = NULL;
        g_thread_exit(NULL);
        return NULL;
    }
    g_debug("Search Thread started, searching lyrics....");
    list = priv->current_module->info->search_get_url_list(
        mmd->title, mmd->artist);
    if(list!=NULL)
        g_debug("Found %u result(s) in searching.", g_slist_length(list));
    else
        g_debug("Found nothing in searching. :(");
    g_idle_add(rc_plugin_lrccrawler_search_idle_func, list);
    rclib_tag_free(mmd);
    priv->search_thread = NULL;
    g_thread_exit(NULL);
    return NULL;
}

static gboolean rc_plugin_lrccrawler_load_lyric_crawler_module(
    const gchar *file)
{
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    RCLyricCrawlerModule *module_data;
    gboolean (*module_init)(RCLyricCrawlerModule *module) = NULL;
    GModule *module = NULL;
    gpointer unpunned = NULL;
    if(file==NULL) return FALSE;
    module = g_module_open(file, G_MODULE_BIND_LAZY);
    if(module==NULL) return FALSE;
    if(!g_module_symbol(module, "rc_lrccrawler_module_init", &unpunned))
    {
        g_module_close(module);
        return FALSE;
    }
    module_init = unpunned;
    module_data = g_new0(RCLyricCrawlerModule, 1);
    module_data->handle = module;
    module_data->path = g_strdup(file);
    if(!module_init(module_data) || module_data->info->magic!=
        RC_LRCCRAWLER_MODULE_MAGIC_NUMBER)
    {
        g_module_close(module);
        g_free(module_data->path);
        g_free(module_data);
        return FALSE;
    }
    g_debug("Loaded lyric crawler module: %s", module_data->info->name);
    priv->crawler_module_list = g_slist_prepend(priv->crawler_module_list,
        module_data);
    return TRUE;
}

static void rc_plugin_lrccrawler_change_search_server(GtkComboBox *widget,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    GtkTreeModel *model;
    GtkTreeIter iter;
    RCLyricCrawlerModule *module = NULL;
    if(data==NULL) return;
    model = gtk_combo_box_get_model(widget);
    if(model==NULL) return;
    if(!gtk_combo_box_get_active_iter(widget, &iter)) return;
    gtk_tree_model_get(model, &iter, CRAWLER_SEARCH_COLUMN_MODULE,
        &module, -1);
    if(module==NULL) return;
    priv->current_module = module;
    if(priv->current_module->info!=NULL)
    {
        g_key_file_set_string(priv->keyfile, LRCCRAWLER_ID, 
            "LyricServer", priv->current_module->info->name);
    }
}

static void rc_plugin_lrccrawler_moudle_get_playing_tag_cb(GtkButton *button,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    gchar *path, *tmp;
    const gchar *home_dir = NULL;
    const RCLibCoreMetadata *mmd = rclib_core_get_metadata();
    if(data==NULL) return;
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    if(mmd!=NULL)
    {
        if(mmd->title!=NULL)
        {
            gtk_entry_set_text(GTK_ENTRY(priv->title_entry), mmd->title);
            if(mmd->artist!=NULL && strlen(mmd->artist)>0)
                tmp = g_strdup_printf("%s - %s.LRC", mmd->artist, mmd->title);
            else if(mmd->title!=NULL && strlen(mmd->title)>0)
                tmp = g_strdup_printf("%s.LRC", mmd->title);
            else
                tmp = g_strdup("New-Lyric.LRC");
            path = g_build_filename(home_dir, ".RhythmCat2", "Lyrics",
                tmp, NULL);
            g_free(tmp);
            gtk_entry_set_text(GTK_ENTRY(priv->save_file_entry), path);
            g_free(path);
        }
        else
            gtk_entry_set_text(GTK_ENTRY(priv->title_entry), "");
        if(mmd->artist!=NULL)
            gtk_entry_set_text(GTK_ENTRY(priv->artist_entry), mmd->artist);
        else
            gtk_entry_set_text(GTK_ENTRY(priv->artist_entry), "");
    }
}

static void rc_plugin_lrccrawler_search_button_clicked(GtkButton *button,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    RCLibTagMetadata *mmd;
    const gchar *text;
    GError *error = NULL;
    if(data==NULL) return;
    if(priv->search_thread!=NULL) return;
    if(priv->current_module==NULL) return;
    mmd = g_new0(RCLibTagMetadata, 1);
    text = gtk_entry_get_text(GTK_ENTRY(priv->title_entry));
    if(text!=NULL && strlen(text)>0)
        mmd->title = g_strdup(text);
    text = gtk_entry_get_text(GTK_ENTRY(priv->artist_entry));
    if(text!=NULL && strlen(text)>0)
        mmd->artist = g_strdup(text);
    priv->search_thread = g_thread_new("LyricCrawler-Search-Thread",
        rc_plugin_lrccrawler_search_lyric_thread_func, (gpointer)mmd);
    g_thread_unref(priv->search_thread);
    if(error!=NULL)
    {
        g_warning("Cannot start search thread: %s", error->message);
        rclib_tag_free(mmd);
        g_error_free(error);
        return;
    }
    gtk_widget_set_sensitive(priv->search_button, FALSE);
    gtk_widget_set_sensitive(priv->download_button, FALSE);
    gtk_label_set_text(GTK_LABEL(priv->result_label), _("Searching..."));
}

static void rc_plugin_lrccrawler_download_button_clicked(
    GtkButton *button, gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    gchar **args;
    GtkTreeIter iter;
    GtkTreeModel *model;
    gchar *url;
    const gchar *path;
    GError *error = NULL;
    GtkTreeSelection *selection;
    if(data==NULL) return;
    if(priv->download_thread!=NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(priv->result_treeview));
    if(model==NULL) return;
    path = gtk_entry_get_text(GTK_ENTRY(priv->save_file_entry));
    if(path==NULL || strlen(path)==0) return;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
        priv->result_treeview));
    if(selection==NULL) return;
    if(!gtk_tree_selection_get_selected(selection, NULL, &iter)) return;
    gtk_tree_model_get(model, &iter, CRAWLER_RESULT_COLUMN_URL, &url, -1);
    if(url==NULL) return;
    args = g_new0(gchar *, 2);
    args[0] = url;
    args[1] = g_strdup(path);
    priv->download_thread = g_thread_new("LyricCrawler-Download-Thread",
        rc_plugin_lrccrawler_down_lyric_thread_func, args);
    g_thread_unref(priv->download_thread);
    if(error!=NULL)
    {
        g_warning("Cannot start download thread: %s", error->message);
        g_error_free(error);
        g_free(args[0]);
        g_free(args[1]);
        g_free(args);
        return;
    }
    gtk_widget_set_sensitive(priv->search_button, FALSE);
    gtk_widget_set_sensitive(priv->download_button, FALSE);
    gtk_label_set_text(GTK_LABEL(priv->result_label),
        _("Downloading lyric file..."));
}

static void rc_plugin_lrccrawler_set_proxy_cb(GtkButton *button,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *main_grid;
    GtkWidget *grid1, *grid2;
    GtkWidget *info_grid;
    GtkWidget *proxy_type_label;
    GtkWidget *proxy_addr_label;
    GtkWidget *proxy_port_label;
    GtkWidget *proxy_user_label;
    GtkWidget *proxy_pass_label;
    GtkWidget *proxy_type_combobox;
    GtkWidget *proxy_addr_entry;
    GtkWidget *proxy_port_spin;
    GtkWidget *proxy_user_entry;
    GtkWidget *proxy_pass_entry;
    GtkListStore *list_store;
    GtkTreeIter iter;
    GtkCellRenderer *renderer;
    gchar *string;
    gint ret, type;
    const gchar *proxy_addr_text;
    const gchar *proxy_user_text;
    const gchar *proxy_pass_text;
    if(data==NULL) return;
    dialog = gtk_dialog_new_with_buttons(_("Proxy Settings"), NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK,
        GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    proxy_type_label = gtk_label_new(_("Proxy Type: "));
    proxy_addr_label = gtk_label_new(_("Address: "));
    proxy_port_label = gtk_label_new(_("Port: "));
    proxy_user_label = gtk_label_new(_("User name: "));
    proxy_pass_label = gtk_label_new(_("Password: "));
    list_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, "HTTP", 1, 0, -1);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, "HTTP (1.0)", 1, 1, -1);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, "Socks 4", 1, 2, -1);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, "Socks 4a", 1, 3, -1);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, "Socks 5", 1, 4, -1);
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, "Socks 5 (Hostname)", 1, 5, -1);
    proxy_type_combobox = gtk_combo_box_new_with_model(
        GTK_TREE_MODEL(list_store));
    g_object_unref(list_store);
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(proxy_type_combobox),
        renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(proxy_type_combobox),
        renderer, "text", 0, NULL);
    proxy_addr_entry = gtk_entry_new();
    proxy_user_entry = gtk_entry_new();
    proxy_pass_entry = gtk_entry_new();
    proxy_port_spin = gtk_spin_button_new_with_range(0, 65535, 1);
    string = g_key_file_get_string(priv->keyfile, LRCCRAWLER_ID,
        "LyricProxyAddress", NULL);
    if(string!=NULL && strlen(string)>0)
        gtk_entry_set_text(GTK_ENTRY(proxy_addr_entry), string);
    g_free(string);
    ret = g_key_file_get_integer(priv->keyfile, LRCCRAWLER_ID,
        "LyricProxyType", NULL);
    switch(ret)
    {
        case CURLPROXY_HTTP:
            type = 0;
            break;
        case CURLPROXY_HTTP_1_0:
            type = 1;
            break;
        case CURLPROXY_SOCKS4:
            type = 2;
            break;
        case CURLPROXY_SOCKS4A:
            type = 3;
            break;
        case CURLPROXY_SOCKS5:
            type = 4;
            break;
        case CURLPROXY_SOCKS5_HOSTNAME:
            type = 5;
            break;
        default:
            type = 0;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(proxy_type_combobox), type);
    ret = g_key_file_get_integer(priv->keyfile, LRCCRAWLER_ID,
        "LyricProxyPort", NULL);
    if(ret<0 || ret>65535) ret = 0;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(proxy_port_spin), ret);
    string = g_key_file_get_string(priv->keyfile, LRCCRAWLER_ID,
        "LyricProxyUser", NULL);
    if(string!=NULL)
    {
        if(strlen(string)>0)
            gtk_entry_set_text(GTK_ENTRY(proxy_user_entry), string);
        g_free(string);
    }
    string = g_key_file_get_string(priv->keyfile, LRCCRAWLER_ID,
        "LyricProxyPassword", NULL);
    if(string!=NULL)
    {
        if(strlen(string)>0)
            gtk_entry_set_text(GTK_ENTRY(proxy_pass_entry), string);
        g_free(string);
    }
    main_grid = gtk_grid_new();
    grid1 = gtk_grid_new();
    grid2 = gtk_grid_new();
    info_grid = gtk_grid_new();
    g_object_set(grid1, "column-spacing", 4, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    g_object_set(grid2, "column-spacing", 4, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    g_object_set(info_grid, "row-spacing", 2, "column-spacing", 4,
        "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    g_object_set(main_grid, "row-spacing", 2, NULL);
    g_object_set(proxy_type_combobox, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(proxy_addr_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(proxy_port_spin, "numeric", FALSE, NULL);
    g_object_set(proxy_user_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(proxy_pass_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, "visibility", FALSE, NULL);
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));    
    gtk_grid_attach(GTK_GRID(grid1), proxy_type_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid1), proxy_type_combobox, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid2), proxy_addr_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid2), proxy_addr_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid2), proxy_port_label, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid2), proxy_port_spin, 3, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), proxy_user_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), proxy_user_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), proxy_pass_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), proxy_pass_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), grid1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), grid2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), info_grid, 0, 2, 1, 1);
    gtk_container_add(GTK_CONTAINER(content_area), main_grid);
    gtk_widget_show_all(dialog);
    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    if(ret==GTK_RESPONSE_ACCEPT)
    {
        ret = gtk_combo_box_get_active(GTK_COMBO_BOX(proxy_type_combobox));
        switch(ret)
        {
            case 0:
                type = CURLPROXY_HTTP;
                break;
            case 1:
                type = CURLPROXY_HTTP_1_0;
                break;
            case 2:
                type = CURLPROXY_SOCKS4;
                break;
            case 3:
                type = CURLPROXY_SOCKS4A;
                break;
            case 4:
                type = CURLPROXY_SOCKS5;
                break;
            case 5:
                type = CURLPROXY_SOCKS5_HOSTNAME;
                break;
            default:
                type = CURLPROXY_HTTP;
        }
        proxy_addr_text = gtk_entry_get_text(GTK_ENTRY(proxy_addr_entry));
        proxy_user_text = gtk_entry_get_text(GTK_ENTRY(proxy_user_entry));
        proxy_pass_text = gtk_entry_get_text(GTK_ENTRY(proxy_pass_entry));
        ret = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
            proxy_port_spin));
        g_key_file_set_string(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyAddress", proxy_addr_text);
        g_key_file_set_integer(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyType", type);
        g_key_file_set_integer(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyPort", ret);
        g_key_file_set_string(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyUser", proxy_user_text);
        g_key_file_set_string(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyPassword", proxy_pass_text);
        if(proxy_addr_text!=NULL && strlen(proxy_addr_text)<1)
            proxy_addr_text = NULL;
        if(proxy_user_text!=NULL && strlen(proxy_user_text)<1)
            proxy_user_text = NULL;
        if(proxy_pass_text!=NULL && strlen(proxy_pass_text)<1)
            proxy_pass_text = NULL;
        rc_lrccrawler_common_set_proxy(type, proxy_addr_text, ret,
            proxy_user_text, proxy_pass_text);
    }
    gtk_widget_destroy(dialog);
    
}

static void rc_plugin_lrccrawler_auto_search_check_button_cb(GtkWidget *widget,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    if(data==NULL) return;
    gboolean flag = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    priv->auto_search = flag;
    g_key_file_set_boolean(priv->keyfile, LRCCRAWLER_ID, "AutoSearch", flag);
}

static void rc_plugin_lrccrawler_close_button_clicked(GtkButton *button,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    if(data==NULL) return;
    gtk_toggle_action_set_active(priv->action, FALSE);
}

static gboolean rc_plugin_lrccrawler_search_window_delete_event_cb(
    GtkWidget *widget, GdkEvent *event, gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    if(data==NULL) return TRUE;
    gtk_toggle_action_set_active(priv->action, FALSE);
    return TRUE;
}

static inline void rc_plugin_lrccrawler_search_window_init(
    RCPluginLyricCrawlerPrivate *priv)
{
    GtkWidget *main_grid;
    GtkWidget *server_grid;
    GtkWidget *info_grid;
    GtkWidget *file_grid;
    GtkWidget *button_grid;
    GtkWidget *button_hbox;
    GtkWidget *scrolled_window;
    GtkWidget *server_label;
    GtkWidget *proxy_button;
    GtkWidget *title_label;
    GtkWidget *artist_label;
    GtkWidget *get_tag_button;
    GtkWidget *save_label;
    GtkWidget *close_button;
    GtkTreeViewColumn *columns[3];
    GtkCellRenderer *renderers[3];
    GtkCellRenderer *renderer = NULL;
    GtkListStore *result_store;
    GtkListStore *search_module_store;
    GtkTreeIter iter;
    gchar *path;
    guint i;
    GSList *list_foreach;
    gchar *current_module_name;
    RCLyricCrawlerModule *module_data;
    const gchar *home_dir = NULL;
    const gchar *module_name;
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    priv->search_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    main_grid = gtk_grid_new();
    server_grid = gtk_grid_new();
    info_grid = gtk_grid_new();
    file_grid = gtk_grid_new();
    button_grid = gtk_grid_new();
    button_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    g_object_set(priv->search_window, "title", _("Lyric Crawler"),
        "window-position", GTK_WIN_POS_CENTER, NULL);
    server_label = gtk_label_new(_("Lyric Server: "));
    title_label = gtk_label_new(_("Title: "));
    artist_label = gtk_label_new(_("Artist: "));
    save_label = gtk_label_new(_("Save lyric file to: "));
    priv->result_label = gtk_label_new(_("Ready"));
    g_object_set(priv->result_label, "xalign", 0.0, "yalign", 0.5, NULL);
    path = g_build_filename(home_dir, ".RhythmCat2", "Lyrics",
        "New file.LRC", NULL);
    priv->save_file_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(priv->save_file_entry), path);
    g_free(path);
    proxy_button = gtk_button_new_with_label(_("Proxy Settings"));
    get_tag_button = gtk_button_new_with_label(_("Get Playing Tag"));
    priv->search_button = gtk_button_new_with_label(_("Search!"));
    priv->download_button = gtk_button_new_with_label(_("Download!"));
    close_button = gtk_button_new_with_label(_("Close"));
    priv->auto_search_checkbutton = gtk_check_button_new_with_label(
        _("Auto search lyric"));
    priv->title_entry = gtk_entry_new();
    priv->artist_entry = gtk_entry_new();
    search_module_store = gtk_list_store_new(CRAWLER_SEARCH_COLUMN_LAST,
        G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
    current_module_name = g_key_file_get_string(priv->keyfile, LRCCRAWLER_ID,
        "LyricServer", NULL);    
    for(list_foreach=priv->crawler_module_list, i=0;list_foreach!=NULL;
        list_foreach=g_slist_next(list_foreach))
    {
        module_data = list_foreach->data;
        if(module_data==NULL) continue;
        if(module_data->info!=NULL && module_data->info->name!=NULL)
            module_name = module_data->info->name;
        else
            module_name = "";
        if(current_module_name!=NULL && module_data->info!=NULL &&
            g_strcmp0(current_module_name, module_data->info->name)==0)
        {
            priv->current_module = module_data;
        }
        gtk_list_store_append(search_module_store, &iter);
        gtk_list_store_set(search_module_store, &iter,
            CRAWLER_SEARCH_COLUMN_TEXT, module_name,
            CRAWLER_SEARCH_COLUMN_INDEX, i, -1);
        i++;
    }
    g_free(current_module_name);
    priv->server_combobox = gtk_combo_box_new_with_model(
        GTK_TREE_MODEL(search_module_store));
    g_object_unref(search_module_store);
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->server_combobox),
        renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(priv->server_combobox),
        renderer, "text", 0, NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(priv->server_combobox), 0);
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(search_module_store),
        &iter))
    {
        do
        {
            gtk_tree_model_get(GTK_TREE_MODEL(search_module_store), &iter,
                CRAWLER_SEARCH_COLUMN_MODULE, &module_data, -1);
            if(module_data==priv->current_module)
            {
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
                    priv->server_combobox), &iter);
            }
        }
        while(gtk_tree_model_iter_next(GTK_TREE_MODEL(search_module_store),
            &iter));
    }    
    result_store = gtk_list_store_new(CRAWLER_RESULT_COLUMN_LAST,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    priv->result_treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
        result_store));
    g_object_unref(result_store);
    for(i=0;i<CRAWLER_RESULT_COLUMN_LAST;i++)
    {
        renderers[i] = gtk_cell_renderer_text_new();
        columns[i] = gtk_tree_view_column_new();
        g_object_set(renderers[i], "ellipsize", PANGO_ELLIPSIZE_END,
            "ellipsize-set", TRUE, NULL);
        gtk_tree_view_column_pack_start(columns[i], renderers[i], TRUE);
        gtk_tree_view_column_add_attribute(columns[i], renderers[i],
            "text", i);
        gtk_tree_view_column_set_resizable(columns[i], TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(priv->result_treeview),
            columns[i]);
    }
    gtk_tree_view_column_set_title(columns[0], _("Title"));
    gtk_tree_view_column_set_title(columns[1], _("Artist"));
    gtk_tree_view_column_set_title(columns[2], _("URL"));
    gtk_tree_view_column_set_min_width(columns[0], 120);
    gtk_tree_view_column_set_min_width(columns[1], 80);
    gtk_tree_view_column_set_min_width(columns[2], 120);
    g_object_set(main_grid, "row-spacing", 2, NULL);
    g_object_set(server_grid, "column-spacing", 2, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    g_object_set(info_grid, "column-spacing", 2, "row-spacing", 2,
        "hexpand-set", TRUE, "hexpand", TRUE, NULL);
    g_object_set(file_grid, "column-spacing", 2, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    g_object_set(button_grid, "column-spacing", 6, "hexpand-set", TRUE,
        "hexpand", TRUE, NULL);
    g_object_set(priv->server_combobox, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->title_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->artist_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->result_label, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(scrolled_window, "expand", TRUE, NULL);
    g_object_set(priv->save_file_entry, "hexpand-set", TRUE, "hexpand",
        TRUE, NULL);
    g_object_set(priv->download_button, "sensitive", FALSE, NULL);
    g_object_set(button_hbox, "hexpand-set", TRUE, "hexpand",
        TRUE, "layout-style", GTK_BUTTONBOX_END, "spacing", 4, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), priv->result_treeview);
    gtk_box_pack_start(GTK_BOX(button_hbox), priv->download_button,
        FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(button_hbox), close_button,
        FALSE, FALSE, 5);
    gtk_grid_attach(GTK_GRID(server_grid), server_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(server_grid), priv->server_combobox, 1, 0,
        1, 1);
    gtk_grid_attach(GTK_GRID(server_grid), proxy_button, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), title_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), priv->title_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), get_tag_button, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), artist_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), priv->artist_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(info_grid), priv->search_button, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(file_grid), save_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(file_grid), priv->save_file_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(button_grid), priv->auto_search_checkbutton,
        0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(button_grid), button_hbox, 1, 0, 1, 1);    
    gtk_grid_attach(GTK_GRID(main_grid), server_grid, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), info_grid, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), scrolled_window, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), priv->result_label, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), file_grid, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(main_grid), button_grid, 0, 5, 1, 1);
    gtk_container_add(GTK_CONTAINER(priv->search_window), main_grid);
    gtk_widget_show_all(priv->search_window);
    gtk_widget_hide(priv->search_window);
    g_signal_connect(priv->search_window, "delete-event",
        G_CALLBACK(rc_plugin_lrccrawler_search_window_delete_event_cb),
        priv);
    g_signal_connect(get_tag_button, "clicked",
        G_CALLBACK(rc_plugin_lrccrawler_moudle_get_playing_tag_cb), priv);
    g_signal_connect(priv->search_button, "clicked",
        G_CALLBACK(rc_plugin_lrccrawler_search_button_clicked), priv);
    g_signal_connect(proxy_button, "clicked",
        G_CALLBACK(rc_plugin_lrccrawler_set_proxy_cb), priv);
    g_signal_connect(priv->download_button, "clicked",
        G_CALLBACK(rc_plugin_lrccrawler_download_button_clicked), priv);
    g_signal_connect(close_button, "clicked",
        G_CALLBACK(rc_plugin_lrccrawler_close_button_clicked), priv);
    g_signal_connect(priv->server_combobox, "changed",
        G_CALLBACK(rc_plugin_lrccrawler_change_search_server), priv);
    g_signal_connect(priv->auto_search_checkbutton, "toggled",
        G_CALLBACK(rc_plugin_lrccrawler_auto_search_check_button_cb),
        priv);
}

static void rc_plugin_lrccrawler_may_missing_cb(RCLibLyric *lyric,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    gchar *uri = NULL;
    gchar *filepath = NULL;
    GSequenceIter *iter = NULL;
    gchar *rtitle;
    gchar *title = NULL, *artist = NULL;
    gchar *full_path;
    RCLibDbPlaylistData *playlist_data = NULL;
    const gchar *home_dir;
    if(data==NULL) return;
    if(!priv->auto_search) return;
    if(gtk_widget_get_visible(priv->search_window)) return;
    uri = rclib_core_get_uri();
    if(uri==NULL) return;
    iter = rclib_core_get_db_reference();
    if(iter!=NULL)
        playlist_data = g_sequence_get(iter);
    if(playlist_data!=NULL && playlist_data->title!=NULL &&
        strlen(playlist_data->title)>0)
    {
        gtk_entry_set_text(GTK_ENTRY(priv->title_entry),
            playlist_data->title);
        title = g_strdup(playlist_data->title);
    }
    else
    {
        filepath = g_filename_from_uri(uri, NULL, NULL);
        rtitle = rclib_tag_get_name_from_fpath(filepath);
        g_free(filepath);
        gtk_entry_set_text(GTK_ENTRY(priv->title_entry), rtitle);
        title = g_strdup(rtitle);
        g_free(rtitle);
    }
    if(playlist_data!=NULL && playlist_data->artist!=NULL &&
        strlen(playlist_data->artist)>0)
    {
        gtk_entry_set_text(GTK_ENTRY(priv->artist_entry),
            playlist_data->artist);
        artist = g_strdup(playlist_data->artist);
    }
    else
    {
        gtk_entry_set_text(GTK_ENTRY(priv->artist_entry),
            "");
    }
    g_free(uri);
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    if(artist!=NULL && title!=NULL)
        filepath = g_strdup_printf("%s - %s.LRC", artist, title);
    else if(title!=NULL)
        filepath = g_strdup_printf("%s.LRC", title);
    else
        filepath = g_strdup("New file.LRC");
    g_free(title);
    g_free(artist);
    full_path = g_build_filename(home_dir, ".RhythmCat2", "Lyrics",
        filepath, NULL);
    g_free(filepath);
    gtk_entry_set_text(GTK_ENTRY(priv->save_file_entry), full_path);
    g_free(full_path);
    priv->auto_search_mode = TRUE;
    gtk_button_clicked(GTK_BUTTON(priv->search_button));
}

static void rc_plugin_lrccrawler_view_menu_toggled(GtkToggleAction *toggle,
    gpointer data)
{
    RCPluginLyricCrawlerPrivate *priv = (RCPluginLyricCrawlerPrivate *)data;
    gboolean flag;
    if(data==NULL) return;
    flag = gtk_toggle_action_get_active(toggle);
    if(flag)
        gtk_window_present(GTK_WINDOW(priv->search_window));
    else
        gtk_widget_hide(priv->search_window);
}

static void rc_plugin_lrccrawler_shutdown_cb(RCLibPlugin *plugin, gpointer data)
{

}

static gboolean rc_plugin_lrccrawler_load(RCLibPluginData *plugin)
{
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    GError *error = NULL;
    gboolean bvalue;
    gint proxy_type;
    gchar *proxy_addr;
    gint proxy_port;
    gchar *proxy_user;
    gchar *proxy_passwd;
    rc_plugin_lrccrawler_search_window_init(priv);
    priv->action = gtk_toggle_action_new("RC2ViewPluginLyricCrawler",
        _("Lyric Crawler"), _("Show/hide lyric crawler window"), NULL);
    priv->menu_id = rc_ui_menu_add_menu_action(GTK_ACTION(priv->action),
        "/RC2MenuBar/ViewMenu/ViewSep2", "RC2ViewPluginLyricCrawler",
        "RC2ViewPluginLyricCrawler", TRUE);
    bvalue = g_key_file_get_boolean(priv->keyfile, LRCCRAWLER_ID, 
        "AutoSearch", &error);
    if(error==NULL)
    {
        priv->auto_search = bvalue;
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            priv->auto_search_checkbutton), bvalue);
    }
    else
    {
        g_error_free(error);
        error = NULL;
        g_key_file_set_boolean(priv->keyfile, LRCCRAWLER_ID, 
            "AutoSearch", TRUE);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            priv->auto_search_checkbutton), TRUE);
    }
    proxy_addr = g_key_file_get_string(priv->keyfile, LRCCRAWLER_ID,
        "LyricProxyAddress", NULL);
    if(proxy_addr!=NULL)
    {
        proxy_type = g_key_file_get_integer(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyType", NULL);
        proxy_port = g_key_file_get_integer(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyPort", NULL);
        proxy_user = g_key_file_get_string(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyUser", NULL);
        proxy_passwd = g_key_file_get_string(priv->keyfile, LRCCRAWLER_ID,
            "LyricProxyPassword", NULL);
        rc_lrccrawler_common_set_proxy(proxy_type, proxy_addr, proxy_port,
            proxy_user, proxy_passwd);
        g_free(proxy_addr);
        g_free(proxy_user);
        g_free(proxy_passwd);
    }
    g_signal_connect(priv->action, "toggled",
        G_CALLBACK(rc_plugin_lrccrawler_view_menu_toggled), priv);
    priv->missing_signal = rclib_lyric_signal_connect("lyric-may-missing",
        G_CALLBACK(rc_plugin_lrccrawler_may_missing_cb), priv);
    return TRUE;
}

static gboolean rc_plugin_lrccrawler_unload(RCLibPluginData *plugin)
{
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    if(priv->missing_signal>0)
        rclib_lyric_signal_disconnect(priv->missing_signal);
    if(priv->menu_id>0)
    {
        rc_ui_menu_remove_menu_action(GTK_ACTION(priv->action),
            priv->menu_id);
        g_object_unref(priv->action);
    }
    priv->missing_signal = 0;
    if(priv->search_window!=NULL)
        gtk_widget_destroy(priv->search_window);
    priv->search_window = NULL; 
    return TRUE;
}

static gboolean rc_plugin_lrccrawler_init(RCLibPluginData *plugin)
{
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    GDir *dir = NULL;
    gchar *dir_path = NULL;
    gchar *tmp;
    const gchar *file_foreach;
    gchar *module_pattern;
    gchar *module_file;
    gint ret;
    ret = curl_global_init(CURL_GLOBAL_ALL);
    if(ret!=0)
    {
        g_warning("Cannot initialize CURL!");
        return FALSE;
    }
    priv->keyfile = rclib_plugin_get_keyfile();
    tmp = g_path_get_dirname(plugin->path);
    if(tmp!=NULL)
        dir_path = g_build_filename(tmp, "CrawlerModules", NULL);
    g_free(tmp);
    if(dir_path!=NULL)
        dir = g_dir_open(dir_path, 0, NULL);
    if(dir!=NULL)
    {
        module_pattern = g_strdup_printf("lrccrawler-[^\\.]+\\.%s",
            G_MODULE_SUFFIX);
        while((file_foreach=g_dir_read_name(dir))!=NULL)
        {
            if(g_regex_match_simple(module_pattern, file_foreach,
                G_REGEX_CASELESS, 0))
            {
                module_file = g_build_filename(dir_path, file_foreach, NULL);
                if(module_file!=NULL)
                {
                    rc_plugin_lrccrawler_load_lyric_crawler_module(
                        module_file);
                }
                g_free(module_file);
            }
        }
        g_free(module_pattern);
        g_dir_close(dir);
    }
    if(priv->crawler_module_list!=NULL)
        priv->current_module = priv->crawler_module_list->data;
    rclib_plugin_signal_connect("shutdown",
        G_CALLBACK(rc_plugin_lrccrawler_shutdown_cb), priv);
    return TRUE;
}

static void rc_plugin_lrccrawler_destroy(RCLibPluginData *plugin)
{
    RCPluginLyricCrawlerPrivate *priv = &lyric_crawler_priv;
    RCLyricCrawlerModule *module_data;
    GSList *list_foreach;
    rc_lrccrawler_common_operation_cancel();
    for(list_foreach=priv->crawler_module_list;list_foreach!=NULL;
        list_foreach=g_slist_next(list_foreach))
    {
        module_data = list_foreach->data;
        if(module_data==NULL) continue;
        if(module_data->handle!=NULL)
            g_module_close(module_data->handle);
        g_free(module_data->path);
        g_free(module_data);
    }
    g_slist_free(priv->crawler_module_list);
    curl_global_cleanup();
}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = LRCCRAWLER_ID,
    .name = N_("Lyric Crawler Plug-in"),
    .version = "0.5",
    .description = N_("Search and download lyric file from Internet sites."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_lrccrawler_load,
    .unload = rc_plugin_lrccrawler_unload,
    .configure = NULL,
    .destroy = rc_plugin_lrccrawler_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_lrccrawler_init, rcplugin_info);



