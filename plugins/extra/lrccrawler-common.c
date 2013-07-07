/*
 * Lyric Crawler Common Module
 * Provide common service for Lyric Crawler modules.
 *
 * lrccrawler_common.c
 * This file is part of RhythmCat Lyric Crawler
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

#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include "lrccrawler-common.h"

static gboolean crawler_download_cancel_flag = FALSE;
static gint crawler_proxy_type = -1;
static gchar *crawler_proxy_address = NULL;
static guint16 crawler_proxy_port = 0;
static gchar *crawler_proxy_userpwd = NULL;

static size_t rc_lrccrawler_common_download_write_cb(void *buffer,
    size_t size, size_t nmemb, void *userdata)
{
    size_t ret_size;
    if(crawler_download_cancel_flag) return 0;
    ret_size = fwrite(buffer, size, nmemb, *(FILE **)userdata);
    return ret_size;  
}

gboolean rc_lrccrawler_common_download_file(const gchar *url,
    const gchar *file)
{
    gboolean flag = FALSE;
    CURL *curl;
    CURLcode res = 0;
    gdouble length = 0;
    FILE *fp;
    if(file!=NULL)
        fp = g_fopen(file, "w");
    else
        return FALSE;
    if(fp==NULL) return FALSE;
    curl = curl_easy_init();
    if(curl==NULL)
    {
        fclose(fp);
        return FALSE;
    }
    crawler_download_cancel_flag = FALSE;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(crawler_proxy_address!=NULL)
    {
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE,  crawler_proxy_type);
        curl_easy_setopt(curl, CURLOPT_PROXY, crawler_proxy_address);
        curl_easy_setopt(curl, CURLOPT_PROXYPORT,  crawler_proxy_port);
        curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, crawler_proxy_userpwd);
    }
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 "
        "(X11; Linux x86_64; rv:10.0.0) Gecko/20120101 Firefox/10.0.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        &rc_lrccrawler_common_download_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fp);
    res = curl_easy_perform(curl);
    if(res==CURLE_OK)
    {      
        res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, 
            &length);  
        flag = TRUE;
    }
    fclose(fp);
    curl_easy_cleanup(curl);
    return flag;
}

gboolean rc_lrccrawler_common_post_data(const gchar *url, const gchar *refer,
    const gchar *user_agent, const gchar *post_data, gsize post_len,
    const gchar *file)
{
    gboolean flag = FALSE;
    CURL *curl;
    CURLcode res = 0;
    gdouble length = 0;
    FILE *fp;
    if(file!=NULL)
        fp = g_fopen(file, "w");
    else
        return FALSE;
    if(fp==NULL) return FALSE;
    curl = curl_easy_init();
    if(curl==NULL)
    {
        fclose(fp);
        return FALSE;
    }
    crawler_download_cancel_flag = FALSE;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if(crawler_proxy_address!=NULL)
    {
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE,  crawler_proxy_type);
        curl_easy_setopt(curl, CURLOPT_PROXY, crawler_proxy_address);
        curl_easy_setopt(curl, CURLOPT_PROXYPORT,  crawler_proxy_port);
        curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, crawler_proxy_userpwd);
    }
    if(refer!=NULL)
        curl_easy_setopt(curl, CURLOPT_REFERER, refer);
    if(user_agent!=NULL)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
    if(post_data!=NULL)
    {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_len);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        &rc_lrccrawler_common_download_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &fp);
    res = curl_easy_perform(curl);
    if(res==CURLE_OK)
    {      
        res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
            &length);  
        flag = TRUE;
    }
    fclose(fp);
    curl_easy_cleanup(curl);
    return flag;
}

void rc_lrccrawler_common_operation_cancel()
{
    if(!crawler_download_cancel_flag)
        crawler_download_cancel_flag = TRUE;
}

void rc_lrccrawler_common_set_proxy(gint type, const gchar *addr, guint16 port,
    const gchar *username, const gchar *passwd)
{
    if(crawler_proxy_address!=NULL) g_free(crawler_proxy_address);
    if(crawler_proxy_userpwd!=NULL) g_free(crawler_proxy_userpwd);
    crawler_proxy_address = NULL;
    crawler_proxy_userpwd = NULL;
    if(addr==NULL)
    {
        crawler_proxy_type = -1;
        crawler_proxy_port = 0;
        return;        
    }
    crawler_proxy_type = type;
    crawler_proxy_address = g_strdup(addr);
    crawler_proxy_port = port;
    if(username!=NULL && passwd!=NULL)
        crawler_proxy_userpwd = g_strdup_printf("%s:%s", username, passwd);
}

