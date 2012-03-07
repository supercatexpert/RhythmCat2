/*
 * RhythmCat Plug-in Support & Manager Module
 * Provide plug-in support and manage the plug-ins.
 *
 * rclib-plugin.c
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

#include "rclib-plugin.h"
#include "rclib-common.h"

/**
 * SECTION: rclib-plugin
 * @Short_description: The plug-in support system
 * @Title: RCLibPlugin
 * @Include: rclib-plugin.h
 *
 * The #RCLibPlugin is a class which provides plug-in support. It manages
 * all the plug-ins and plug-in loaders, and makes them usable in the
 * player.
 */

#define RCLIB_PLUGIN_GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    RCLIB_PLUGIN_TYPE, RCLibPluginPrivate)
    
typedef struct RCLibPluginPrivate
{
    GHashTable *plugin_table;
    gint dummy;
}RCLibPluginPrivate;

enum
{
    SIGNAL_REGISTERED,
    SIGNAL_LAST
};

static GObject *plugin_instance = NULL;
static gpointer rclib_plugin_parent_class = NULL;
static gint plugin_signals[SIGNAL_LAST] = {0};

static gboolean rclib_plugin_is_native(const char *filename)
{
    const char *last_period;
    gchar *little_case;
    gboolean flag;
    last_period = strrchr(filename, '.');
    if(last_period == NULL) return FALSE;
    little_case = g_ascii_strdown(last_period, -1);
    flag = (g_strcmp0(little_case, ".so")==0) ||
        (g_strcmp0(little_case, ".dll")==0) ||
        (g_strcmp0(little_case, ".sl")==0);
    g_free(little_case);
    return flag;
}

static void rclib_plugin_data_free(RCLibPluginData *plugin)
{
    if(plugin==NULL) return;
    g_free(plugin->error);
    g_free(plugin->path);
    if(plugin->info!=NULL)
        ;
    if(plugin->dependent_list!=NULL)
        g_list_free(plugin->dependent_list);
    if(plugin->handle!=NULL)
        g_module_close(plugin->handle);
    g_free(plugin);
}

static inline RCLibPluginData *rclib_pliugin_data_new()
{
    RCLibPluginData *plugin = g_new0(RCLibPluginData, 1);
    plugin->ref_count = 1;
    return plugin;
}

static void rclib_plugin_finalize(GObject *object)
{
    RCLibPluginPrivate *priv = RCLIB_PLUGIN_GET_PRIVATE(RCLIB_PLUGIN(
        object));
    if(priv->plugin_table!=NULL)
        g_hash_table_unref(priv->plugin_table);
    G_OBJECT_CLASS(rclib_plugin_parent_class)->finalize(object);
}

static void rclib_plugin_class_init(RCLibPluginClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_plugin_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_plugin_finalize;
    g_type_class_add_private(klass, sizeof(RCLibPluginPrivate));
    
    /**
     * RCLibPlugin::registered:
     * @plugin: the #RCLibPlugin that received the signal
     * @data: the plug-in data (#RCLibPluginData)
     *
     * The ::registered signal is emitted when a new plug-in registered.
     */
    plugin_signals[SIGNAL_REGISTERED] = g_signal_new("registered",
        RCLIB_PLUGIN_TYPE, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPluginClass, registered), NULL, NULL,
        g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER,
        NULL);
}

static void rclib_plugin_instance_init(RCLibPlugin *plugin)
{
    RCLibPluginPrivate *priv = RCLIB_PLUGIN_GET_PRIVATE(plugin);
    bzero(priv, sizeof(RCLibPluginPrivate));
    priv->plugin_table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)rclib_plugin_data_free);
}

GType rclib_plugin_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo plugin_info = {
        .class_size = sizeof(RCLibPluginClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_plugin_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibPlugin),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rclib_plugin_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibPlugin"), &plugin_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_plugin_init:
 *
 * Initialize the plug-in support system instance.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rclib_plugin_init()
{
    g_message("Loading plug-in support system....");
    plugin_instance = g_object_new(RCLIB_PLUGIN_TYPE, NULL);
    g_message("Plug-in support system loaded.");
    return TRUE;
}

/**
 * rclib_plugin_exit:
 *
 * Unload the plug-in support system instance.
 */

void rclib_plugin_exit()
{
    if(plugin_instance!=NULL) g_object_unref(plugin_instance);
    plugin_instance = NULL;
    g_message("Plug-in support system exited.");
}

/**
 * rclib_plugin_get_instance:
 *
 * Get the running #RCLibPlugin instance.
 *
 * Returns: The running instance.
 */

GObject *rclib_plugin_get_instance()
{
    return plugin_instance;
}

/**
 * rclib_plugin_signal_connect:
 * @name: the name of the signal
 * @callback: the the #GCallback to connect
 * @data: the user data
 *
 * Connect the GCallback function to the given signal for the running
 * instance of #RCLibPlugin object.
 *
 * Returns: The handler ID.
 */

gulong rclib_plugin_signal_connect(const gchar *name,
    GCallback callback, gpointer data)
{
    if(plugin_instance==NULL) return 0;
    return g_signal_connect(plugin_instance, name, callback, data);
}

/**
 * rclib_plugin_signal_disconnect:
 * @handler_id: handler id of the handler to be disconnected
 *
 * Disconnects a handler from the running #RCLibPlugin instance so it
 * will not be called during any future or currently ongoing emissions
 * of the signal it has been connected to. The #handler_id becomes
 * invalid and may be reused.
 */

void rclib_plugin_signal_disconnect(gulong handler_id)
{
    if(plugin_instance==NULL) return;
    g_signal_handler_disconnect(plugin_instance, handler_id);
}

/**
 * rclib_plugin_probe:
 * @filename: file path to the plug-in file
 *
 * Probe the plug-in file.
 *
 * Returns: The plug-in data, #NULL if the probe operation failed.
 */

RCLibPluginData *rclib_plugin_probe(const gchar *filename)
{
    RCLibPluginData *plugin = NULL;
    gboolean (*plugin_init)(RCLibPluginData *plugin) = NULL;
    gpointer unpunned;
    gboolean native;
    const gchar *error_msg;
    g_return_val_if_fail(filename!=NULL, NULL);
    if(!g_file_test(filename, G_FILE_TEST_EXISTS))
        return NULL;
    native = rclib_plugin_is_native(filename);
    plugin = rclib_pliugin_data_new();
    plugin->path = g_strdup(filename);
    if(native) /* Native C/C++ plug-in */
    {
        plugin->native = TRUE;
        plugin->handle = g_module_open(filename, G_MODULE_BIND_LOCAL);
        if(plugin->handle==NULL)
        {
            error_msg = g_module_error();
            if(error_msg!=NULL)
                plugin->error = g_strdup(error_msg);
            else
                plugin->error = g_strdup(_("Unknown error!"));
            g_warning("Error when loading plug-in %s: %s", filename,
                plugin->error);
            plugin->handle = g_module_open(filename, G_MODULE_BIND_LAZY |
                G_MODULE_BIND_LOCAL);
            plugin->unloadable = TRUE;
            if(plugin->handle==NULL)
                return plugin;
        }
        if(!g_module_symbol(plugin->handle, "rcplugin_init", &unpunned))
        {
            g_warning("Plug-in %s is not usable, because 'rcplugin_init' "
                "symbol could not be found. Does the plug-in call macro "
                "RCPLUGIN_INIT_PLUGIN()?", filename);
            g_module_close(plugin->handle);
            error_msg = g_module_error();
            plugin->handle = NULL;
            plugin->unloadable = TRUE;
            plugin->error = g_strdup_printf(_("Cannot find symbol "
                "'rcplugin_init' in the plug-in %s"), filename);
            return plugin;
        }
        plugin_init = unpunned;
    }
    else /* Other plug-in types */
    {
    
    
    }
    if(plugin_init==NULL)
    {
        plugin->unloadable = TRUE;
        return plugin;
    }
    if(!(plugin_init(plugin)) || plugin->info==NULL)
    {
        plugin->error = g_strdup(_("This plugin does not have info data!"));
        g_warning("Plugin %s does not have info data!", filename);
        plugin->unloadable = TRUE;
        return plugin;
    }
    if(plugin->info->id==NULL || strlen(plugin->info->id)==0)
    {
        plugin->error = g_strdup(_("This plugin has not defined an ID!"));
        g_warning("Plugin %s has not defined an ID!", filename);
        plugin->unloadable = TRUE;
        return plugin;
    }
    if(plugin->info->magic!=RCLIB_PLUGIN_MAGIC)
    {
        plugin->error = g_strdup(_("This plugin has wrong magic number!"));
        g_warning("Plugin %s has wrong magic number!", filename);
        plugin->unloadable = TRUE;
        return plugin;
    }
    if(plugin->info->major_version!=RCLIB_PLUGIN_MAJOR_VERSION ||
        plugin->info->minor_version>RCLIB_PLUGIN_MINOR_VERSION)
    {
        plugin->error = g_strdup_printf(_("ABI version mismatch %d.%d.x "
            "(need %d.%d.x)"), plugin->info->major_version,
            plugin->info->minor_version, RCLIB_PLUGIN_MAJOR_VERSION,
            RCLIB_PLUGIN_MINOR_VERSION);
        g_warning("Plugin %s is not loadable: ABI version mismatch %d.%d.x "
            "(need %d.%d.x)", filename, plugin->info->major_version,
            plugin->info->minor_version, RCLIB_PLUGIN_MAJOR_VERSION,
            RCLIB_PLUGIN_MINOR_VERSION);
        plugin->unloadable = TRUE;
        return plugin;
    }
    return plugin;
}

/**
 * rclib_plugin_register:
 * @plugin: the plug-in data
 *
 * Register the plug-in to the player.
 *
 * Returns: Whether the register operation succeeded.
 */

gboolean rclib_plugin_register(RCLibPluginData *plugin)
{
    RCLibPluginPrivate *priv;
    g_return_val_if_fail(plugin!=NULL, FALSE);
    g_return_val_if_fail(plugin_instance!=NULL, FALSE);
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    g_return_val_if_fail(priv!=NULL, FALSE);
    if(plugin->info==NULL) return FALSE;
    if(plugin->info->id==NULL || strlen(plugin->info->id)==0)
        return FALSE;
    if(plugin->unloadable)
        return FALSE;
    if(g_hash_table_lookup(priv->plugin_table, plugin->info->id)!=NULL)
        return TRUE;
    if(plugin->info->type==RCLIB_PLUGIN_TYPE_LOADER)
    {
    
    }
    g_hash_table_insert(priv->plugin_table, g_strdup(plugin->info->id),
        rclib_plugin_data_ref(plugin));
    g_signal_emit(plugin_instance, plugin_signals[SIGNAL_REGISTERED], 0,
        plugin);
    return TRUE;
}

/**
 * rclib_plugin_data_ref:
 * @plugin: the plug-in data
 *
 * Increase the reference count of plug-data by 1.
 *
 * Returns: The plug-in data.
 */

RCLibPluginData *rclib_plugin_data_ref(RCLibPluginData *plugin)
{
    if(plugin==NULL) return NULL;
    plugin->ref_count++;
    return plugin;
}

/**
 * rclib_plugin_data_unref:
 * @plugin: the plug-in data
 *
 * Decrease the reference count of plug-data by 1, if the reference
 * count down to zero, the data will be freed.
 */

void rclib_plugin_data_unref(RCLibPluginData *plugin)
{
    if(plugin==NULL) return;
    plugin->ref_count--;
    if(plugin->ref_count<=0)
        rclib_plugin_data_free(plugin);
}

/**
 * rclib_plugin_load_from_dir:
 * @dirname: the path of the directory which contains the plug-in files
 *
 * Load plug-in files from given directory path, and register them.
 *
 * Returns: Loaded plug-in number.
 */

guint rclib_plugin_load_from_dir(const gchar *dirname)
{
    guint number = 0;
    GDir *gdir;
    GError *error = NULL;
    const gchar *filename;
    gchar *path;
    RCLibPluginData *plugin_data;
    if(dirname==NULL) return 0;
    gdir = g_dir_open(dirname, 0, &error);
    if(gdir==NULL)
    {
        g_warning("Cannot open plug-in directory: %s", error->message);
        g_error_free(error);
        return 0;
    }
    while((filename=g_dir_read_name(gdir))!=NULL)
    {
        path = g_build_filename(dirname, filename, NULL);
        if(rclib_plugin_is_native(filename)) /* Native C/C++ plug-in */
        {
            plugin_data = rclib_plugin_probe(path);
            if(plugin_data!=NULL)
            {
                if(plugin_data->handle!=NULL && !plugin_data->unloadable)
                {
                    if(rclib_plugin_register(plugin_data))
                    {
                        g_message("Plug-in: %s initialized.",
                            plugin_data->info->id);
                    }
                }
                rclib_plugin_data_unref(plugin_data);
            }
        }
        else /* Other plug-in type */
        {
        }
        g_free(path);
    }
    g_dir_close(gdir);
    return number;
}



