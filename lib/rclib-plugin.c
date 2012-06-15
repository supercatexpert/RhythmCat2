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
    RCLIB_TYPE_PLUGIN, RCLibPluginPrivate)
    
typedef struct RCLibPluginPrivate
{
    GHashTable *plugin_table;
    GHashTable *loader_table;
    GKeyFile *keyfile;
    gchar *configure_path;
}RCLibPluginPrivate;

enum
{
    SIGNAL_REGISTERED,
    SIGNAL_LOADED,
    SIGNAL_UNLOADED,
    SIGNAL_UNREGISTERED,
    SIGNAL_SHUTDOWN,
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

static RCLibPluginLoaderInfo *rclib_plugin_find_loader_for_plugin(
    const RCLibPluginData *plugin)
{
    const gchar *ext;
    gchar *little_case;
    RCLibPluginLoaderInfo *loader = NULL;
    RCLibPluginPrivate *priv;
    g_return_val_if_fail(plugin!=NULL, NULL);
    g_return_val_if_fail(plugin_instance!=NULL, NULL);
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    g_return_val_if_fail(priv!=NULL, NULL);
    if(priv->loader_table==NULL) return NULL;
    ext = strrchr(plugin->path, '.');
    if(ext==NULL) return NULL;
    if(strlen(ext)<1) return NULL;
    ext++;
    little_case = g_ascii_strdown(ext, -1);
    if(little_case==NULL) return NULL;
    loader = g_hash_table_lookup(priv->loader_table, little_case);
    g_free(little_case);
    return loader;
}

static void rclib_plugin_data_free(RCLibPluginData *plugin)
{
    RCLibPluginLoaderInfo *loader;
    if(plugin==NULL) return;
    if(plugin->native)
    {
        if(plugin->info!=NULL && plugin->info->destroy!=NULL)
        {
            plugin->info->destroy(plugin);
        }
        if(plugin->handle!=NULL)
            g_module_close(plugin->handle);
    }
    else
    {
        loader = rclib_plugin_find_loader_for_plugin(plugin);
        if(loader!=NULL && loader->destroy!=NULL)
            loader->destroy(plugin);
    }
    g_free(plugin->error);
    g_free(plugin->path);
    if(plugin->dependent_list!=NULL)
        g_slist_free(plugin->dependent_list);
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
    gchar *conf_data;
    gsize conf_size;
    GError *error = NULL;
    RCLibPluginPrivate *priv = RCLIB_PLUGIN_GET_PRIVATE(RCLIB_PLUGIN(
        object));
    if(priv->keyfile!=NULL && priv->configure_path!=NULL)
    {
        conf_data = g_key_file_to_data(priv->keyfile, &conf_size, &error);
        if(conf_data!=NULL)
        {
            if(!g_file_set_contents(priv->configure_path, conf_data,
                conf_size, &error))
            {
                g_warning("Cannot save plug-in configure data to file: %s",
                    error->message);
                g_error_free(error);
            }
            g_free(conf_data);
        }
        else
        {
            g_warning("Cannot save plug-in configure data: %s",
                error->message);
            g_error_free(error);
        }
    }
    g_free(priv->configure_path);
    rclib_plugin_destroy_all();
    if(priv->plugin_table!=NULL)
        g_hash_table_unref(priv->plugin_table);
    if(priv->loader_table!=NULL)
        g_hash_table_unref(priv->loader_table);
    if(priv->keyfile!=NULL)
        g_key_file_free(priv->keyfile);
    G_OBJECT_CLASS(rclib_plugin_parent_class)->finalize(object);
}

static GObject *rclib_plugin_constructor(GType type, guint n_construct_params,
    GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(plugin_instance!=NULL) return plugin_instance;
    retval = G_OBJECT_CLASS(rclib_plugin_parent_class)->constructor
        (type, n_construct_params, construct_params);
    plugin_instance = retval;
    return retval;
}

static void rclib_plugin_class_init(RCLibPluginClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_plugin_parent_class = g_type_class_peek_parent(klass);
    object_class->finalize = rclib_plugin_finalize;
    object_class->constructor = rclib_plugin_constructor;
    g_type_class_add_private(klass, sizeof(RCLibPluginPrivate));
    
    /**
     * RCLibPlugin::registered:
     * @plugin: the #RCLibPlugin that received the signal
     * @data: the plug-in data (#RCLibPluginData)
     *
     * The ::registered signal is emitted when a new plug-in registered.
     */
    plugin_signals[SIGNAL_REGISTERED] = g_signal_new("registered",
        RCLIB_TYPE_PLUGIN, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPluginClass, registered), NULL, NULL,
        g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER,
        NULL);
        
    /**
     * RCLibPlugin::loaded:
     * @plugin: the #RCLibPlugin that received the signal
     * @data: the plug-in data (#RCLibPluginData)
     *
     * The ::loaded signal is emitted when a plug-in is loaded.
     */
    plugin_signals[SIGNAL_LOADED] = g_signal_new("loaded",
        RCLIB_TYPE_PLUGIN, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPluginClass, loaded), NULL, NULL,
        g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER,
        NULL);
        
    /**
     * RCLibPlugin::unloaded:
     * @plugin: the #RCLibPlugin that received the signal
     * @data: the plug-in data (#RCLibPluginData)
     *
     * The ::unloaded signal is emitted when a plug-in is unloaded.
     */
    plugin_signals[SIGNAL_UNLOADED] = g_signal_new("unloaded",
        RCLIB_TYPE_PLUGIN, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPluginClass, unloaded), NULL, NULL,
        g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER,
        NULL);
        
    /**
     * RCLibPlugin::unregistered:
     * @plugin: the #RCLibPlugin that received the signal
     * @id: the ID of the plug-in
     *
     * The ::unregistered signal is emitted when a plug-in is unregistered.
     */    
    plugin_signals[SIGNAL_UNREGISTERED] = g_signal_new("unregistered",
        RCLIB_TYPE_PLUGIN, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPluginClass, unregistered), NULL, NULL,
        g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING,
        NULL);

    /**
     * RCLibPlugin::shutdown:
     * @plugin: the #RCLibPlugin that received the signal
     *
     * The ::shutdown signal is emitted when the plug-in support module
     * is going to be shut down, so plug-in should save their configure
     * data when receive this signal.
     */   
    plugin_signals[SIGNAL_SHUTDOWN] = g_signal_new("shutdown",
        RCLIB_TYPE_PLUGIN, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibPluginClass, shutdown), NULL, NULL,
        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0, G_TYPE_NONE, NULL);
}

static void rclib_plugin_instance_init(RCLibPlugin *plugin)
{
    RCLibPluginPrivate *priv = RCLIB_PLUGIN_GET_PRIVATE(plugin);
    memset(priv, 0, sizeof(RCLibPluginPrivate));
    priv->plugin_table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)rclib_plugin_data_unref);
    priv->loader_table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, (GDestroyNotify)NULL);
    priv->keyfile = g_key_file_new();
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

GType rclib_plugin_data_get_type(void)
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_boxed_type_register_static(
            g_intern_static_string("RCLibPluginData"),
            (GBoxedCopyFunc)rclib_plugin_data_ref,
            (GBoxedFreeFunc)rclib_plugin_data_unref);
        g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rclib_plugin_init:
 * @file: the configure file path
 *
 * Initialize the plug-in support system instance.
 *
 * Returns: Whether the initialization succeeded.
 */

gboolean rclib_plugin_init(const gchar *file)
{
    RCLibPluginPrivate *priv;
    GError *error = NULL;
    g_message("Loading plug-in support system....");
    plugin_instance = g_object_new(RCLIB_TYPE_PLUGIN, NULL);
    if(file!=NULL)
    {
        priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
        if(g_file_test(file, G_FILE_TEST_EXISTS))
        {
            if(!g_key_file_load_from_file(priv->keyfile, file,
                G_KEY_FILE_NONE, &error))
            {
                g_warning("Cannot open plug-in configure file: %s",
                    error->message);
                g_error_free(error);
            }
        }
        priv->configure_path = g_strdup(file);
    }
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
    if(plugin_instance!=NULL)
    {
        g_signal_emit(plugin_instance, plugin_signals[SIGNAL_SHUTDOWN], 0);
        g_signal_handlers_disconnect_matched(plugin_instance,
            G_SIGNAL_MATCH_ID, plugin_signals[SIGNAL_SHUTDOWN], 0, NULL,
            NULL, NULL);
        g_object_unref(plugin_instance);
    }
    plugin_instance = NULL;
    g_message("Plug-in support system exited.");
}

/**
 * rclib_plugin_get_instance:
 *
 * Get the running #RCLibPlugin instance.
 *
 * Returns: (transfer none): The running instance.
 */

GObject *rclib_plugin_get_instance()
{
    return plugin_instance;
}

/**
 * rclib_plugin_signal_connect:
 * @name: the name of the signal
 * @callback: (scope call): the the #GCallback to connect
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
    if(!g_signal_handler_is_connected(plugin_instance, handler_id)) return;
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
    RCLibPluginLoaderInfo *loader = NULL;
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
        plugin->handle = g_module_open(filename, 0);
        if(plugin->handle==NULL)
        {
            error_msg = g_module_error();
            if(error_msg!=NULL)
                plugin->error = g_strdup(error_msg);
            else
                plugin->error = g_strdup(_("Unknown error!"));
            g_warning("Error when loading plug-in %s: %s", filename,
                plugin->error);
            plugin->handle = g_module_open(filename, G_MODULE_BIND_LAZY);
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
        plugin->native = FALSE;
        loader = rclib_plugin_find_loader_for_plugin(plugin);
        if(loader==NULL || loader->probe==NULL)
        {
            plugin->unloadable = TRUE;
            return plugin;
        }
        plugin_init = loader->probe;
    }
    if(plugin_init==NULL)
    {
        plugin->unloadable = TRUE;
        return plugin;
    }
    if(!plugin_init(plugin))
    {
        plugin->error = g_strdup(_("Cannot initialize the plug-in!"));
        g_warning("Plugin %s cannot be initialized!", filename);
        plugin->unloadable = TRUE;
        return plugin;
    }
    if(plugin->info==NULL)
    {
        plugin->error = g_strdup(_("This plug-in does not have info data!"));
        g_warning("Plugin %s does not have info data!", filename);
        plugin->unloadable = TRUE;
        return plugin;
    }
    if(plugin->info->id==NULL || strlen(plugin->info->id)==0)
    {
        plugin->error = g_strdup(_("This plug-in has not defined an ID!"));
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
        g_warning("Plug-in %s is not loadable: ABI version mismatch %d.%d.x "
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
    RCLibPluginLoaderInfo *loader;
    const gchar * const *exts;
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
    {
        plugin->unloadable = TRUE;
        return FALSE;
    }
    if(plugin->info->type==RCLIB_PLUGIN_TYPE_LOADER)
    {
        if(plugin->info->extra_info==NULL) return FALSE;
        loader = plugin->info->extra_info;
        if(loader->extensions==NULL) return FALSE;
        for(exts=loader->extensions;*exts!=NULL;exts++)
        {
            g_hash_table_insert(priv->loader_table, g_strdup(*exts),
                loader);
        }
    }
    if(priv->keyfile!=NULL)
    {
        if(!g_key_file_has_key(priv->keyfile, plugin->info->id, "Enabled",
            NULL))
        {
            g_key_file_set_boolean(priv->keyfile, plugin->info->id,
                "Enabled", FALSE);
        }
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
    g_atomic_int_add(&(plugin->ref_count), 1);
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
    if(g_atomic_int_dec_and_test(&(plugin->ref_count)))
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
    GSList *np_list = NULL, *foreach;
    RCLibPluginData *plugin_data;
    if(dirname==NULL) return 0;
    gdir = g_dir_open(dirname, 0, &error);
    if(gdir==NULL)
    {
        g_warning("Cannot open plug-in directory: %s", error->message);
        g_error_free(error);
        return 0;
    }
    g_message("Searching plug-ins in directory: %s", dirname);
    while((filename=g_dir_read_name(gdir))!=NULL)
    {
        path = g_build_filename(dirname, filename, NULL);
        if(rclib_plugin_is_native(filename))
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
                        number++;
                    }
                }
                rclib_plugin_data_unref(plugin_data);
            }
        }
        else /* Non-native plug-in should be loaded later */
        {
            np_list = g_slist_prepend(np_list, g_strdup(path));
        }   
        g_free(path);
    }
    g_dir_close(gdir);
    for(foreach=np_list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        path = foreach->data;
        plugin_data = rclib_plugin_probe(path);
        if(plugin_data!=NULL)
        {
            if(plugin_data->handle!=NULL && !plugin_data->unloadable)
            {
                if(rclib_plugin_register(plugin_data))
                {
                    g_message("Plug-in: %s initialized.",
                        plugin_data->info->id);
                    number++;
                }
            }
            rclib_plugin_data_unref(plugin_data);
        }
    }
    g_slist_free_full(np_list, g_free);
    g_message("Found %u plug-ins in the directory.", number);
    return number;
}

/**
 * rclib_plugin_load:
 * @plugin: the plug-in data
 *
 * Load and run the plug-in.
 *
 * Returns: Whether the plug-in is loaded successfully.
 */

gboolean rclib_plugin_load(RCLibPluginData *plugin)
{
    RCLibPluginPrivate *priv;
    RCLibPluginLoaderInfo *loader;
    RCLibPluginData *dep_plugin;
    gchar **deps;
    GSList *dep_list = NULL, *foreach = NULL;
    g_return_val_if_fail(plugin!=NULL, FALSE);
    if(plugin->loaded) return TRUE;
    if(plugin->unloadable) return FALSE;
    if(plugin->error!=NULL) return FALSE;
    if(plugin->info==NULL) return FALSE;

    /* Check if depended plug-ins are registered already */
    for(deps=plugin->info->depends;deps!=NULL && *deps!=NULL;deps++)
    {
        dep_plugin = rclib_plugin_lookup(*deps);
        if(dep_plugin==NULL)
        {
            if(dep_list!=NULL) g_slist_free(dep_list);
            g_warning("Necessary plug-in for plug-in %s is missing!",
                plugin->info->id);
            return FALSE;
        }
        dep_list = g_slist_prepend(dep_list, dep_plugin);
    }
    
    /* Load all depended plug-ins */
    for(foreach=dep_list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        dep_plugin = foreach->data;
        if(dep_plugin==NULL) continue;
        if(dep_plugin->loaded) continue;
        if(dep_plugin!=plugin) /* Prevent self infinity loop */
        {
            if(!rclib_plugin_load(dep_plugin))
            {
                g_slist_free(dep_list);
                g_warning("Cannot load necessary plug-in for plug-in %s!",
                    plugin->info->id);
                return FALSE;
            }
        }
    }
    
    /* Note depended plug-ins that this plug-in needs it */
    for(foreach=dep_list;foreach!=NULL;foreach=g_slist_next(foreach))
    {
        dep_plugin = foreach->data;
        if(dep_plugin==NULL) continue;
        dep_plugin->dependent_list = g_slist_prepend(
            dep_plugin->dependent_list, plugin->info->id);
    }
    if(dep_list!=NULL) g_slist_free(dep_list);
    
    if(plugin->native) /* Native C/C++ plug-in */
    {
        /*
         * If load function does not exist, therefore the load operation
         * is successful directly.
         */
        if(plugin->info->load==NULL) return TRUE;
        if(!plugin->info->load(plugin)) return FALSE;
    }
    else /* Use loaders to load the plug-in */
    {
        loader = rclib_plugin_find_loader_for_plugin(plugin);
        if(loader==NULL || loader->load==NULL) return FALSE;
        if(!loader->load(plugin)) return FALSE;
    }
    plugin->loaded = TRUE;
    if(plugin_instance!=NULL)
    {
        priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
        if(priv!=NULL && priv->keyfile!=NULL)
        {
            g_key_file_set_boolean(priv->keyfile, plugin->info->id,
                "Enabled", TRUE);
        }
    }
    g_signal_emit(plugin_instance, plugin_signals[SIGNAL_LOADED], 0,
        plugin);
    return TRUE;
}

/**
 * rclib_plugin_unload:
 * @plugin: the plug-in data
 *
 * Stop and unload the plug-in.
 *
 * Returns: Whether the plug-in is unloaded successfully.
 */

gboolean rclib_plugin_unload(RCLibPluginData *plugin)
{
    RCLibPluginPrivate *priv;
    RCLibPluginLoaderInfo *loader;
    RCLibPluginData *dep_plugin;
    GSList *foreach = NULL;
    gchar **deps;
    g_return_val_if_fail(plugin!=NULL, FALSE);
    if(!plugin->loaded) return TRUE;
    if(plugin->unloadable) return FALSE;
    if(plugin->error!=NULL) return FALSE;
    if(plugin->info==NULL) return FALSE;
    
    /* Unload plug-ins that depend on this plug-in */
    for(foreach=plugin->dependent_list;foreach!=NULL;
        foreach=g_slist_next(foreach))
    {
        dep_plugin = foreach->data;
        if(dep_plugin==NULL) continue;
        if(!rclib_plugin_unload(dep_plugin))
        {
            g_warning("Unable to unload plug-ins which depend on plugin: %s!",
                plugin->info->id);
            return FALSE;
        }
    }
    
    /* Remove this plug-in from each dependency's dependent list */
    for(deps=plugin->info->depends;deps!=NULL && *deps!=NULL;deps++)
    {
        dep_plugin = rclib_plugin_lookup(*deps);
        if(dep_plugin!=NULL)
        {
            dep_plugin->dependent_list = g_slist_remove(
                dep_plugin->dependent_list, plugin->info->id);
        }
    }
    
    if(plugin->native) /* Native C/C++ plug-in */
    {
        /*
         * If unload function does not exist, therefore the unload operation
         * is successful directly.
         */
        if(plugin->info->unload==NULL) return TRUE;
        if(!plugin->info->unload(plugin)) return FALSE;
    }
    else /* Use loaders to load the plug-in */
    {
        loader = rclib_plugin_find_loader_for_plugin(plugin);
        if(loader==NULL || loader->unload==NULL) return FALSE;
        if(!loader->unload(plugin)) return FALSE;
    }
    plugin->loaded = FALSE;
    if(plugin_instance!=NULL)
    {
        priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
        if(priv!=NULL && priv->keyfile!=NULL)
        {
            g_key_file_set_boolean(priv->keyfile, plugin->info->id,
                "Enabled", FALSE);
        }
    }
    g_signal_emit(plugin_instance, plugin_signals[SIGNAL_UNLOADED], 0,
        plugin);
    return TRUE;
}

/**
 * rclib_plugin_reload:
 * @plugin: the plug-in data
 *
 * Reload and restart the plug-in.
 *
 * Returns: Whether the plug-in is reloaded successfully.
 */

gboolean rclib_plugin_reload(RCLibPluginData *plugin)
{
    if(plugin==NULL) return FALSE;
    if(!plugin->loaded) return FALSE;
    if(!rclib_plugin_unload(plugin)) return FALSE;
    if(!rclib_plugin_load(plugin)) return FALSE;
    return TRUE;
}

/**
 * rclib_plugin_is_loaded:
 * @plugin: the plug-in data
 *
 * Check whether the plug-in is loaded.
 *
 * Returns: Whether the plug-in is loaded.
 */

gboolean rclib_plugin_is_loaded(RCLibPluginData *plugin)
{
    if(plugin==NULL) return FALSE;
    return plugin->loaded;
}

/**
 * rclib_plugin_destroy:
 * @plugin: the plug-in data
 *
 * Exit the plug-in and destroy the plug-in data.
 */

void rclib_plugin_destroy(RCLibPluginData *plugin)
{
    RCLibPluginPrivate *priv;
    RCLibPluginLoaderInfo *loader;
    const gchar * const *exts;
    gchar *id;
    if(plugin==NULL) return;
    if(plugin_instance==NULL) return;
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    if(priv==NULL || priv->plugin_table==NULL) return;
    if(plugin->loaded) rclib_plugin_unload(plugin);
    if(!plugin->unloadable && plugin->info!=NULL &&
        priv->loader_table!=NULL && plugin->info->extra_info!=NULL &&
        plugin->info->type==RCLIB_PLUGIN_TYPE_LOADER)
    {
        loader = plugin->info->extra_info;
        for(exts=loader->extensions;exts!=NULL && *exts!=NULL;exts++)
        {
            g_hash_table_remove(priv->loader_table, *exts);
        }
    }
    id = g_strdup(plugin->info->id);
    g_hash_table_remove(priv->plugin_table, plugin->info->id);
    g_signal_emit(plugin_instance, plugin_signals[SIGNAL_UNREGISTERED], 0,
        id);
    g_free(id);
    rclib_plugin_data_free(plugin);
}

/**
 * rclib_plugin_foreach:
 * @func: (scope call): the function to call for each key/value pair
 * @data: user data to pass to the function
 *
 * Calls the given function for each of the key/value pairs in the
 * #GHashTable which contains the plug-in data.
 */

void rclib_plugin_foreach(GHFunc func, gpointer data)
{
    RCLibPluginPrivate *priv;
    if(func==NULL) return;
    if(plugin_instance==NULL) return;
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    if(priv==NULL || priv->plugin_table==NULL) return;
    g_hash_table_foreach(priv->plugin_table, func, data);
}

/**
 * rclib_plugin_lookup:
 * @id: the ID of the plug-in
 *
 * Lookup a plug-in in the registered plug-in table by the given ID.
 *
 * Returns: The plug-in data, #NULL if not found.
 */

RCLibPluginData *rclib_plugin_lookup(const gchar *id)
{
    RCLibPluginPrivate *priv;
    if(id==NULL) return NULL;
    if(plugin_instance==NULL) return NULL;
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    if(priv==NULL || priv->plugin_table==NULL) return NULL;
    return g_hash_table_lookup(priv->plugin_table, id);
}

/**
 * rclib_plugin_destroy_all:
 *
 * Destroy all plug-ins in the player.
 */

void rclib_plugin_destroy_all()
{
    RCLibPluginPrivate *priv;
    RCLibPluginData *plugin_data;
    GHashTableIter iter;
    GSList *native_list = NULL, *non_native_list = NULL;
    GSList *loader_list = NULL;
    if(plugin_instance==NULL) return;
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    if(priv==NULL || priv->plugin_table==NULL) return;
    g_hash_table_iter_init(&iter, priv->plugin_table);
    while(g_hash_table_iter_next(&iter, NULL, (gpointer *)&plugin_data))
    {
        if(plugin_data==NULL) continue;
        if(plugin_data->native)
        {
            if(plugin_data->info->type==RCLIB_PLUGIN_TYPE_LOADER)
                loader_list = g_slist_prepend(loader_list,
                    rclib_plugin_data_ref(plugin_data));
            else
                native_list = g_slist_prepend(native_list,
                    rclib_plugin_data_ref(plugin_data));
        }
        else
            non_native_list = g_slist_prepend(non_native_list,
                rclib_plugin_data_ref(plugin_data));
    }
    g_hash_table_remove_all(priv->plugin_table);
    g_slist_free_full(non_native_list, (GDestroyNotify)rclib_plugin_destroy);
    g_slist_free_full(native_list, (GDestroyNotify)rclib_plugin_destroy);
    g_slist_free_full(loader_list, (GDestroyNotify)rclib_plugin_destroy);
}

/**
 * rclib_plugin_load_from_configure:
 *
 * Load registered plug-ins enabled in the configure file.
 */

void rclib_plugin_load_from_configure()
{
    RCLibPluginPrivate *priv;
    gchar *key;
    RCLibPluginData *plugin_data;
    GHashTableIter iter;
    gboolean flag;
    if(plugin_instance==NULL) return;
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    if(priv==NULL || priv->keyfile==NULL) return;
    g_hash_table_iter_init(&iter, priv->plugin_table);
    while(g_hash_table_iter_next(&iter, (gpointer *)&key,
        (gpointer *)&plugin_data))
    {
        if(key==NULL || plugin_data==NULL) continue;
        flag = g_key_file_get_boolean(priv->keyfile, key, "Enabled", NULL);
        if(flag) rclib_plugin_load(plugin_data);
    }
}

/**
 * rclib_plugin_get_keyfile:
 *
 * Get the #GKeyFile configure data of the plug-ins.
 *
 * Returns: The configure data.
 */

GKeyFile *rclib_plugin_get_keyfile()
{
    RCLibPluginPrivate *priv;
    if(plugin_instance==NULL) return NULL;
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    if(priv==NULL) return NULL;
    return priv->keyfile;
}

/**
 * rclib_plugin_is_registed:
 * @id: the ID of the plug-in
 *
 * Check whether the given ID is registered in the plug-in table.
 *
 * Returns: Whether the given ID is registered.
 */

gboolean rclib_plugin_is_registed(const gchar *id)
{
    RCLibPluginPrivate *priv;
    if(id==NULL) return FALSE;
    if(plugin_instance==NULL) return FALSE;
    priv = RCLIB_PLUGIN_GET_PRIVATE(plugin_instance);
    if(priv==NULL || priv->plugin_table==NULL) return FALSE;
    return (g_hash_table_lookup(priv->plugin_table, id)!=NULL);
}

