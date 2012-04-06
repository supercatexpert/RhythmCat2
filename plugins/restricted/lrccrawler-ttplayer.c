/*
 * Lyric Crawler TTPlayer Module
 * Get Lyric from TTPlayer Lyric Search Server.
 *
 * crawler_ttplayer.c
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

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <curl/curl.h>
#include <lrccrawler-common.h>


const static gchar *music_base_url = "http://ttlrcct.qianqian.com/dll/"
    "lyricsvr.dll?sh?";
const static gchar *lrc_base_url = "http://ttlrcct.qianqian.com/dll/"
    "lyricsvr.dll?dl?";

static gint32 rc_crawler_module_ttplayer_calc_download_code(gint id,
    const char *data)
{
    gint32 encoded_id;
    gint32 encoded_data = 0;
    gint32 encoded_data2 = 0;
    gint32 ret;
    gsize len;
    gint i;
    guint32 id1 = id & 0xff;
    guint32 id2 = (id >> 8) & 0xff;
    guint32 id3 = (id >> 16) & 0xff;
    guint32 id4 = (id >> 24) & 0xff;
    if(id3 == 0) id3 = ~id2 & 0xff;
    if(id4 == 0) id4 = ~id1 & 0xff;
    encoded_id = (id1 << 24) | (id3 << 16) | (id2 << 8) | id4;
    len = strlen(data);
    for(i=len-1;i>=0;i--)
        encoded_data = encoded_data + data[i] + (encoded_data<<(i%2+4));
    for(i=0;i<len;i++)
        encoded_data2 = encoded_data2 + data[i] + (encoded_data2<<(i%2+3));
    ret = ((encoded_data ^ encoded_id) + (encoded_data2 | id)) * 
        (encoded_data2 | encoded_id);
    ret = ret * (encoded_data ^ id);
    return ret;
}

static void rc_crawler_module_xml_parse_start(GMarkupParseContext *context,
    const gchar *element_name, const gchar **attribute_names,
    const gchar **attribute_values, gpointer user_data, GError **error)
{
    guint i;
    const gchar *attr_name;
    gboolean id_flag = FALSE;
    gint id = 0;
    gchar *title = NULL;
    gchar *artist = NULL;
    gchar *url;
    GSList *list = *(GSList **)user_data;
    gchar *tmp;
    RCLyricCrawlerSearchData *search_data;
    if(g_strcmp0(element_name, "lrc")==0)
    {
        for(i=0;attribute_names[i]!=NULL;i++)
        {
            attr_name = attribute_names[i];
            if(g_strcmp0(attr_name, "id")==0 && attribute_values[i]!=NULL)
            {
                if(sscanf(attribute_values[i], "%d", &id)>0)
                {
                    id_flag = TRUE;
                }
            }
            else if(g_strcmp0(attr_name, "title")==0
                && attribute_values[i]!=NULL)
            {
                if(title!=NULL) g_free(title);
                title = g_strdup(attribute_values[i]);
            }
            else if(g_strcmp0(attr_name, "artist")==0
                && attribute_values[i]!=NULL)
            {
                if(artist!=NULL) g_free(artist);
                artist = g_strdup(attribute_values[i]);
            }
        }
        if(id_flag)
        {
            tmp = g_strdup_printf("%s%s", artist, title);
            url = g_strdup_printf("%sId=%d&Code=%d",
                lrc_base_url, id,
                rc_crawler_module_ttplayer_calc_download_code(id, tmp));
            g_free(tmp);
            search_data = g_new0(RCLyricCrawlerSearchData, 1);
            search_data->title = title;
            search_data->artist = artist;
            search_data->url = url;
            list = g_slist_append(list, search_data);
            *(GSList **)user_data = list;
        }
        else
        {
            if(title!=NULL) g_free(title);
            if(artist!=NULL) g_free(artist);
        }
    }
}

static gchar *rc_crawler_module_ttsearch_encoder(const gchar *text)
{
    gchar *utf16_str;
    gchar *result;
    gsize utf16_len;
    gsize i;
    gchar *tmp;
    guint x, y;
    guint tmp_len;
    if(text==NULL) return NULL;
    tmp = g_utf8_strdown(text, -1);
    if(tmp==NULL) return NULL;
    tmp_len = strlen(tmp);
    for(x=0, y=0;x<tmp_len;x++)
    {
        if(!isblank(tmp[x]))
        {
            tmp[y] = tmp[x];
            y++;
        }
        if(tmp[x]=='\0') break;
    }
    tmp[y] = '\0';
    utf16_str = g_convert(tmp, -1, "UTF16LE", "UTF-8", NULL,
        &utf16_len, NULL);
    g_free(tmp);
    if(utf16_str==NULL) return NULL;
    if(utf16_len<=0)
    {
        g_free(utf16_str);
        return NULL;
    }
    result = g_new0(gchar, 2*utf16_len+1);
    for(i=0;i<utf16_len;i++)
    {
        if((guchar)utf16_str[i]!=L' ')
        g_snprintf(result+2*i, 3, "%02X", (guchar)utf16_str[i]);
    }
    g_free(utf16_str);
    return result;
}

static GSList *rc_crawler_module_get_url_list(const gchar *title,
    const gchar *artist)
{
    gboolean flag;
    gchar *tmp_file;
    gchar *xml_data;
    gchar *search_url;
    gsize xml_len;
    gchar *esc_title, *esc_artist;
    GMarkupParseContext *context;
    GSList *lrc_url_list = NULL;
    GMarkupParser parser =
    {
        .start_element = rc_crawler_module_xml_parse_start,
        .end_element = NULL,
        .text = NULL,
        .passthrough = NULL,
        .error = NULL
    };
    if(title==NULL && artist==NULL) return NULL;
    if(title!=NULL)
        esc_title = rc_crawler_module_ttsearch_encoder(title);
    else
        esc_title = g_strdup("");
    if(artist!=NULL)
        esc_artist = rc_crawler_module_ttsearch_encoder(artist);
    else
        esc_artist = g_strdup("");
    search_url = g_strconcat(music_base_url, "Artist=", esc_artist,
        "&Title=", esc_title, "&Flags=0", NULL);
    g_free(esc_title);
    g_free(esc_artist);
    tmp_file = g_build_filename(g_get_tmp_dir(), "RC-TTLRCHTM.tmp", NULL);
    flag = rc_lrccrawler_common_download_file(search_url, tmp_file);
    g_free(search_url);
    if(!flag)
    {
        g_free(tmp_file);
        return NULL;
    }
    flag = g_file_get_contents(tmp_file, &xml_data, &xml_len, NULL);
    g_free(tmp_file);
    if(!flag) return NULL;
    context = g_markup_parse_context_new(&parser, 0, &lrc_url_list, NULL);
    flag = g_markup_parse_context_parse(context, xml_data, -1, NULL);
    g_free(xml_data);
    g_markup_parse_context_free(context);
    if(!flag)
    {
        return NULL;
    }
    return lrc_url_list;
}

static gboolean rc_crawler_module_download_file(const gchar *url,
    const gchar *file)
{
    return rc_lrccrawler_common_download_file(url, file);
}

static gboolean rc_crawler_module_init(RCLyricCrawlerModule *module)
{
    return TRUE;
}

static RCLyricCrawlerModuleInfo rc_lrccrawler_module_info = {
    .magic = RC_LRCCRAWLER_MODULE_MAGIC_NUMBER,
    .name = "TTPlayer",
    .desc = "Download lyric from TTPlayer",
    .author = "SuperCat",
    .version = "0.5",
    .search_get_url_list = rc_crawler_module_get_url_list,
    .download_file = rc_crawler_module_download_file
};

RC_LRCCRAWLER_INIT_MODULE(rc_lrccrawler_module_info.name,
    rc_crawler_module_init, rc_lrccrawler_module_info);

