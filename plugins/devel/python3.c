/*
 * RhythmCat Plug-in Python3 Loader
 * Python3 plug-in loader for RhythmCat Library based player.
 *
 * python3.c
 * This file is part of RhythmCat Library Plug-ins (LibRhythmCat Plug-ins)
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

#include <Python.h>
#include <pythonrun.h>
#include <signal.h>
#include <wchar.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <rclib.h>

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif

#define G_LOG_DOMAIN "Py3Loader"
#define PY3LD_ID "rc2-python3-loader"

static gboolean python_loaded = FALSE;

static void py3ld_add_module_path(const gchar *module_path)
{
    PyObject *pathlist, *pathstring;
    pathlist = PySys_GetObject("path");
    pathstring = PyUnicode_FromString(module_path);
    if(PySequence_Contains(pathlist, pathstring)==0)
        PyList_Insert(pathlist, 0, pathstring);
    Py_DECREF(pathstring);
}

gboolean py3ld_check_is_imported(const gchar *name)
{
    PyObject *module_dict;
    PyObject *key;
    gboolean flag = FALSE;
    if(name==NULL) return FALSE;
    module_dict = PyImport_GetModuleDict();
    if(module_dict==NULL) return FALSE;
    key = PyUnicode_FromString(name);
    if(PyDict_Contains(module_dict, key)>0)
        flag = TRUE;
    Py_DECREF(key);
    return flag;
}

static gboolean py3ld_load(RCLibPluginData *plugin)
{
    g_debug("Python3 loader loaded.");
    return TRUE;
}

static gboolean py3ld_unload(RCLibPluginData *plugin)
{
    g_debug("Python3 loader unloaded.");
    return TRUE;
}

static gboolean py3ld_init(RCLibPluginData *plugin)
{
    gint ret;
    gchar *plugin_dir;
    wchar_t *argv[] = {L"RhythmCat2", NULL };
    struct sigaction old_sigint;
    if(Py_IsInitialized())
    {
        g_message("Initialized already!");
        return FALSE;
    }
    ret = sigaction(SIGINT, NULL, &old_sigint);
    if(ret!=0)
    {
        g_warning("Cannot save SIGINT handler!");
        return FALSE;
    }
    Py_Initialize();
    python_loaded = TRUE;
    ret = sigaction(SIGINT, &old_sigint, NULL);
    if(ret!=0)
    {
        g_warning("Cannot restore SIGINT handler!");
        if(Py_IsInitialized()) Py_Finalize();
        return FALSE;
    }
    PySys_SetArgv(1, argv);
    PyRun_SimpleString("import sys; sys.path.pop(0)\n");
    plugin_dir = g_path_get_dirname(plugin->path);
    if(plugin_dir!=NULL)
        py3ld_add_module_path(plugin_dir);
    g_free(plugin_dir);
    return TRUE;
}

static void py3ld_destroy(RCLibPluginData *plugin)
{
    if(Py_IsInitialized() && python_loaded) Py_Finalize();
}

static gboolean py3ld_plugin_probe(RCLibPluginData *plugin)
{
    RCLibPluginInfo *info;
    PyObject *py_module = NULL;
    PyObject *fromlist, *locals;
    PyObject *plugin_info;
    PyObject *py_magic, *py_major_ver, *py_minor_ver, *py_id;
    PyObject *py_name, *py_version, *py_desc, *py_author, *py_homepage;
    PyObject *string_utf8;
    PyObject *probe_func, *result;
    gboolean flag = TRUE;
    gchar *plugin_dir;
    gchar *module_name;
    if(plugin==NULL) return FALSE;
    if(plugin->path==NULL) return FALSE;
    plugin_dir = g_path_get_dirname(plugin->path);
    if(plugin_dir!=NULL)
        py3ld_add_module_path(plugin_dir);
    g_free(plugin_dir);
    module_name = rclib_tag_get_name_from_fpath(plugin->path);
    if(!py3ld_check_is_imported(module_name))
    {
        fromlist = PyTuple_New(0);
        py_module = PyImport_ImportModuleEx(module_name, NULL, NULL,
            fromlist);
        Py_DECREF(fromlist);
    }
    else
    {
        g_printf("Repeat import!\n");
    }
    g_free(module_name);
    if(py_module==NULL)
    {
        PyErr_Print();
        return FALSE;
    }
    locals = PyModule_GetDict(py_module);
    if(locals==NULL)
    {
        Py_DECREF(py_module);
        g_warning("Cannot open module dict!");
        return FALSE;
    }
    plugin_info = PyDict_GetItemString(locals, "RC2Py3PluginInfo");  
    if(plugin_info==NULL)
    {
        Py_DECREF(py_module);
        g_warning("Cannot find plug-in info (Dict: \"RC2Py3PluginInfo\")!");
        return FALSE;
    }
    if(!PyDict_Check(plugin_info))
    {
        Py_DECREF(py_module);
        g_warning("Wrong type of plug-in info (should be a dict type)!");
        return FALSE;
    }
    py_magic = PyDict_GetItemString(plugin_info, "magic");
    py_major_ver = PyDict_GetItemString(plugin_info, "major_version");
    py_minor_ver = PyDict_GetItemString(plugin_info, "minor_version");
    py_id = PyDict_GetItemString(plugin_info, "id");
    py_name = PyDict_GetItemString(plugin_info, "name");
    py_version = PyDict_GetItemString(plugin_info, "version");
    py_desc = PyDict_GetItemString(plugin_info, "description");
    py_author = PyDict_GetItemString(plugin_info, "author");
    py_homepage = PyDict_GetItemString(plugin_info, "homepage");
    if(py_magic!=NULL && py_major_ver!=NULL && py_minor_ver!=NULL &&
        py_id!=NULL && py_name!=NULL && PyLong_Check(py_magic) &&
        PyLong_Check(py_major_ver) && PyLong_Check(py_minor_ver) &&
        PyUnicode_Check(py_id) && PyUnicode_Check(py_name))
    {
        info = g_new0(RCLibPluginInfo, 1);
        info->type = RCLIB_PLUGIN_TYPE_MODULE;
        info->magic = PyLong_AsLong(py_magic);
        info->major_version = PyLong_AsLong(py_major_ver);
        info->minor_version = PyLong_AsLong(py_minor_ver);
        string_utf8 = PyUnicode_AsUTF8String(py_id);
        if(string_utf8!=NULL)
        {
            info->id = g_strdup(PyBytes_AsString(string_utf8));
            Py_DECREF(string_utf8);
        }
        string_utf8 = PyUnicode_AsUTF8String(py_name);
        if(string_utf8!=NULL)
        {
            info->name = g_strdup(PyBytes_AsString(string_utf8));
            Py_DECREF(string_utf8);
        }
        string_utf8 = PyUnicode_AsUTF8String(py_version);
        if(string_utf8!=NULL)
        {
            info->version = g_strdup(PyBytes_AsString(string_utf8));
            Py_DECREF(string_utf8);
        }
        string_utf8 = PyUnicode_AsUTF8String(py_desc);
        if(string_utf8!=NULL)
        {
            info->description = g_strdup(PyBytes_AsString(string_utf8));
            Py_DECREF(string_utf8);
        }
        string_utf8 = PyUnicode_AsUTF8String(py_author);
        if(string_utf8!=NULL)
        {
            info->author = g_strdup(PyBytes_AsString(string_utf8));
            Py_DECREF(string_utf8);
        }
        string_utf8 = PyUnicode_AsUTF8String(py_homepage);
        if(string_utf8!=NULL)
        {
            info->homepage = g_strdup(PyBytes_AsString(string_utf8));
            Py_DECREF(string_utf8);
        }
        info->depends = g_new0(gchar *, 2);
        info->depends[0] = g_strdup(PY3LD_ID);
    }
    else
    {
        Py_DECREF(py_module);
        g_warning("Necessary item in dict \"RC2Py3PluginInfo\" is missing!");
        return FALSE;
    }
    probe_func = PyDict_GetItemString(locals, "Probe");
    if(probe_func!=NULL)
    {
        result = PyObject_CallFunction(probe_func, NULL);
        if(result!=NULL)
        {
            if(result!=Py_True) flag = FALSE;
            Py_DECREF(result);
        }
    }
    if(!flag)
    {
        Py_DECREF(py_module);
        if(info!=NULL)
        {
            g_free(info->id);
            g_free(info->name);
            g_free(info->version);
            g_free(info->description);
            g_free(info->author);
            g_free(info->homepage);
            g_strfreev(info->depends);
            g_free(info);
        }
        g_warning("Probe function in plug-in returned False!");
        return FALSE;
    }
    plugin->handle = py_module;
    plugin->info = info;
    return TRUE;
}

static gboolean py3ld_plugin_load(RCLibPluginData *plugin)
{
    PyObject *locals, *load_func, *result;
    gboolean flag = FALSE;
    if(plugin==NULL) return FALSE;
    if(plugin->path==NULL) return FALSE;
    if(plugin->handle==NULL) return FALSE;
    locals = PyModule_GetDict((PyObject *)plugin->handle);
    load_func = PyDict_GetItemString(locals, "Load");
    if(load_func!=NULL)
    {
        result = PyObject_CallFunction(load_func, NULL);
        if(result!=NULL)
        {
            if(result==Py_True) flag = TRUE;
            Py_DECREF(result);
        }
    }
    return flag;
}

static gboolean py3ld_plugin_unload(RCLibPluginData *plugin)
{
    PyObject *locals, *unload_func, *result;
    gboolean flag = FALSE;
    if(plugin==NULL) return FALSE;
    if(plugin->path==NULL) return FALSE;
    if(plugin->handle==NULL) return FALSE;
    locals = PyModule_GetDict((PyObject *)plugin->handle);
    unload_func = PyDict_GetItemString(locals, "Unload");
    if(unload_func!=NULL)
    {
        result = PyObject_CallFunction(unload_func, NULL);
        if(result!=NULL)
        {
            if(result==Py_True) flag = TRUE;
            Py_DECREF(result);
        }
    }
    while(PyGC_Collect());
    return flag;
}

static void py3ld_plugin_destroy(RCLibPluginData *plugin)
{
    PyObject *locals, *destroy_func, *result;
    if(plugin==NULL) return;
    if(plugin->handle!=NULL)
    {
        locals = PyModule_GetDict((PyObject *)plugin->handle);
        destroy_func = PyDict_GetItemString(locals, "Destroy");
        if(destroy_func!=NULL)
        {
            result = PyObject_CallFunction(destroy_func, NULL);
            if(result!=NULL) Py_DECREF(result);
        }
    }
    while(PyGC_Collect());
    if(plugin->info!=NULL)
    {
        g_free(plugin->info->id);
        g_free(plugin->info->name);
        g_free(plugin->info->version);
        g_free(plugin->info->description);
        g_free(plugin->info->author);
        g_free(plugin->info->homepage);
        g_strfreev(plugin->info->depends);
        g_free(plugin->info);
    }
    if(plugin->handle!=NULL)
        Py_DECREF((PyObject *)plugin->handle);
}

static const gchar *py3ld_extensions[] = {"py", "pyo", "pyc", "python",
    NULL};

static RCLibPluginLoaderInfo rcplugin_loader_info = {
    .extensions = py3ld_extensions,
    .probe = py3ld_plugin_probe,
    .load = py3ld_plugin_load,
    .unload = py3ld_plugin_unload,
    .destroy = py3ld_plugin_destroy
};

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_LOADER,
    .id = PY3LD_ID,
    .name = "Python3 plug-in loader",
    .version = "0.1",
    .description = "The loader for loading Python3 plug-ins",
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = py3ld_load,
    .unload = py3ld_unload,
    .destroy = py3ld_destroy,
    .configure = NULL,
    .depends = NULL,
    .extra_info = &rcplugin_loader_info
};

RCPLUGIN_INIT_PLUGIN(PY3LD_ID, py3ld_init, rcplugin_info);

