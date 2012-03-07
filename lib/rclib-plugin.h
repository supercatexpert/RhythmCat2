/*
 * RhythmCat Library Plug-in Support & Manager Header Declaration
 *
 * rclib-plugin.h
 * This file is part of RhythmCat Library (LibRhythmCat)
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

#ifndef HAVE_RCLIB_PLUGIN_H
#define HAVE_RCLIB_PLUGIN_H

#include <glib.h>
#include <gmodule.h>
#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define RCLIB_PLUGIN_MAGIC 0x20120103
#define RCLIB_PLUGIN_MAJOR_VERSION 2
#define RCLIB_PLUGIN_MINOR_VERSION 0

#define RCLIB_PLUGIN_TYPE (rclib_plugin_get_type())
#define RCLIB_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RCLIB_PLUGIN_TYPE, RCLibPlugin))
#define RCLIB_PLUGIN_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RCLIB_PLUGIN_TYPE, RCLibPluginClass))
#define RCLIB_IS_PLUGIN(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RCLIB_PLUGIN_TYPE))
#define RCLIB_IS_PLUGIN_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RCLIB_PLUGIN_TYPE))
#define RCLIB_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_PLUGIN_TYPE, RCLibPluginClass))

/**
 * RCPLUGIN_INIT_PLUGIN:
 * @pluginname: the plug-in name
 * @initfunc: the initialization function for the plug-in, the
 * function take an argument of the pointer of #RCLibPluginData,
 * and should return TRUE if the initialization succeeded.
 * @pluginfo: the plug-in information (#RCLibPluginInfo)
 *
 * The macro for declare plug-in initialization function more easily.
 */

#define RCPLUGIN_INIT_PLUGIN(pluginname, initfunc, plugininfo) \
    G_MODULE_EXPORT gboolean rcplugin_init(RCLibPluginData *plugin) \
    { \
        plugin->info = &(plugininfo); \
        return initfunc((plugin)); \
    }

/**
 * RCPLUGIN_DESTROY_PLUGIN:
 * @exitfunc: the exit function for the plug-in, the
 * function take an argument of the pointer of #RCLibPluginData.
 *
 * The macro for declare plug-in destroy function more easily.
 */

#define RCPLUGIN_DESTROY_PLUGIN(exitfunc) \
    G_MODULE_EXPORT void rcplugin_destroy(RCLibPluginData *plugin) \
    { \
        exitfunc(plugin); \
    }

/**
 * RCLibPluginType:
 * @RCLIB_PLUGIN_TYPE_UNKNOWN: unknown plug-in type
 * @RCLIB_PLUGIN_TYPE_MODULE: normal plug-in type
 * @RCLIB_PLUGIN_TYPE_LOADER: plug-in loader type
 *
 * The enum type for plug-in type.
 */

typedef enum {
    RCLIB_PLUGIN_TYPE_UNKNOWN = 0,
    RCLIB_PLUGIN_TYPE_MODULE = 1,
    RCLIB_PLUGIN_TYPE_LOADER = 2
}RCLibPluginType;

typedef struct _RCLibPluginInfo RCLibPluginInfo;
typedef struct _RCLibPluginData RCLibPluginData;
typedef struct _RCLibPlugin RCLibPlugin;
typedef struct _RCLibPluginClass RCLibPluginClass;

/**
 * RCLibPluginInfo:
 * @magic: the magic number, should be equal to #RCLIB_PLUGIN_MAGIC
 * @major_version: the major version of the plug-in support module, should
 * be equal to #RCLIB_PLUGIN_MAJOR_VERSION
 * @minjor_verison: the minor version of the plug-in support module, should
 * be equal to #RCLIB_PLUGIN_MINJOR_VERSION
 * @type: the type of the plug-in
 * @id: the ID of the plug-in, should be unique
 * @name: the name of the plug-in
 * @version: the version of the plug-in
 * @description: the description of the plug-in
 * @author: the author of the plug-in
 * @homepage: the homepage URL of the plug-in
 * @load: the load function of the plug-in
 * @unload: the unload function of the plug-in
 * @configure: the configure function of the plug-in
 */

struct _RCLibPluginInfo {
    guint32 magic;
    guint32 major_version;
    guint32 minor_version;
    RCLibPluginType type;
    
    gchar *id;
    gchar *name;
    gchar *version;
    gchar *description;
    gchar *author;
    gchar *homepage;
    
    gboolean (*load)(RCLibPluginData *plugin);
    gboolean (*unload)(RCLibPluginData *plugin);
    gboolean (*configure)(RCLibPluginData *plugin);
};

/**
 * RCLibPluginData:
 * @native: whether the plug-in is a native C/C++ module
 * @loaded: whether the plug-in is loaded
 * @handle: the handle of the plug-in
 * @path: the file path of the plug-in
 * @info: the information data (#RCLibPluginInfo) of the plug-in
 * @error: the error message in the initialization progress
 * @ipc_data: (not used now)
 * @extra: extra data
 * @unloadable: whether the plug-in is not loadable
 * @dependent_list: the dependent list
 *
 * The plug-in data.
 */

struct _RCLibPluginData {
    /*< private >*/
    gint ref_count;
    
    /*< public >*/
    gboolean native;
    gboolean loaded;
    gpointer handle;
    gchar *path;
    RCLibPluginInfo *info;
    gchar *error;
    gpointer ipc_data;
    gpointer extra;
    gboolean unloadable;
    GList *dependent_list;

    void (*_rclib_plugin_reserved1)(void);
    void (*_rclib_plugin_reserved2)(void);
    void (*_rclib_plugin_reserved3)(void);
    void (*_rclib_plugin_reserved4)(void);
};

/**
 * RCLibPlugin:
 *
 * The plug-in support system. The contents of the #RCLibPlugin structure
 * are private and should only be accessed via the provided API.
 */

struct _RCLibPlugin {
    /*< private >*/
    GObject parent;
};

/**
 * RCLibPluginClass:
 *
 * #RCLibPlugin class.
 */

struct _RCLibPluginClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*registered)(RCLibPlugin *plugin, const RCLibPluginData *data);
};

/*< private >*/
GType rclib_plugin_get_type();

/*< public >*/
gboolean rclib_plugin_init();
void rclib_plugin_exit();
GObject *rclib_plugin_get_instance();
gulong rclib_plugin_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_plugin_signal_disconnect(gulong handler_id);
RCLibPluginData *rclib_plugin_probe(const gchar *filename);
gboolean rclib_plugin_register(RCLibPluginData *plugin);
RCLibPluginData *rclib_plugin_data_ref(RCLibPluginData *plugin);
void rclib_plugin_data_unref(RCLibPluginData *plugin);
guint rclib_plugin_load_from_dir(const gchar *dirname);

G_END_DECLS

#endif

