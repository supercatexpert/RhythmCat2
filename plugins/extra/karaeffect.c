/*
 * Karaoke Effect Plug-in
 * Remove voice from playing music.
 *
 * karaeffect.c
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
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <rclib.h>

typedef struct RCPluginKaraEffectPrivate
{
    GstElement *kara_element;
}RCPluginKaraEffectPrivate;

static RCPluginKaraEffectPrivate karaeff_priv = {0};

static gboolean rc_plugin_karaeff_load(RCLibPluginData *plugin)
{
    RCPluginKaraEffectPrivate *priv = &karaeff_priv;
    gboolean flag;
    priv->kara_element = gst_element_factory_make("audiokaraoke", NULL);
    flag = rclib_core_effect_plugin_add(priv->kara_element);
    if(!flag)
    {
        gst_object_unref(priv->kara_element);
        priv->kara_element = NULL;
    }
    return flag;
}

static gboolean rc_plugin_karaeff_unload(RCLibPluginData *plugin)
{
    RCPluginKaraEffectPrivate *priv = &karaeff_priv;
    if(priv->kara_element!=NULL)
    {
        rclib_core_effect_plugin_remove(priv->kara_element);
        priv->kara_element = NULL;
    }
    return TRUE;
}

static gboolean rc_plugin_karaeff_init(RCLibPluginData *plugin)
{

    return TRUE;
}

static void rc_plugin_karaeff_destroy(RCLibPluginData *plugin)
{

}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = "rc2-native-karaoke-effect",
    .name = N_("Karaoke Audio Effect"),
    .version = "0.1",
    .description = N_("Remove voice from the music."),
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = rc_plugin_karaeff_load,
    .unload = rc_plugin_karaeff_unload,
    .configure = NULL,
    .destroy = rc_plugin_karaeff_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN(rcplugin_info.id, rc_plugin_karaeff_init, rcplugin_info);


