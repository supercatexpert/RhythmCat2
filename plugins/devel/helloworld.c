#include <rclib.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>

static GtkWidget *window = NULL;

gboolean hw_load(RCLibPluginData *plugin)
{
    GtkWidget *label;
    g_printf("Hello world plug-in loaded!\n");
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    label = gtk_label_new("Hello, world!");
    gtk_container_add(GTK_CONTAINER(window), label);
    gtk_widget_show_all(window);
    return TRUE;
}

gboolean hw_unload(RCLibPluginData *plugin)
{
    g_printf("Hello world plug-in unloaded!\n");
    gtk_widget_destroy(window);
    return TRUE;
}

gboolean hw_init(RCLibPluginData *plugin)
{
    g_printf("Hello world plug-in initialized!\n");
    return TRUE;
}

void hw_destroy(RCLibPluginData *plugin)
{
    g_printf("Hello world plug-in got destroyed!\n");
}

static RCLibPluginInfo rcplugin_info = {
    .magic = RCLIB_PLUGIN_MAGIC,
    .major_version = RCLIB_PLUGIN_MAJOR_VERSION,
    .minor_version = RCLIB_PLUGIN_MINOR_VERSION,
    .type = RCLIB_PLUGIN_TYPE_MODULE,
    .id = "rc2-devel-hello-world",
    .name = "Hello world plug-in",
    .version = "0.1",
    .description = "Just a test plug-in",
    .author = "SuperCat",
    .homepage = "http://supercat-lab.org/",
    .load = hw_load,
    .unload = hw_unload,
    .configure = NULL,
    .destroy = hw_destroy,
    .extra_info = NULL
};

RCPLUGIN_INIT_PLUGIN("rc2-devel-hello-world", hw_init, rcplugin_info);

