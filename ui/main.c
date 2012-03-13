#include <glib.h>
#include <gst/gst.h>
#include <glib/gprintf.h>
#include <rclib.h>
#include "ui-player.h"
#include "mpris2.h"
#include "common.h"

static void rc_main_app_activate_cb(GApplication *application,
    gpointer data)
{
    const gchar *home_dir = NULL;
    gchar *plugin_dir;
    gchar *plugin_conf;
    rc_ui_player_init(GTK_APPLICATION(application));
    rclib_settings_apply();
    rc_mpris2_init();
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    plugin_conf = g_build_filename(home_dir, ".RhythmCat2", "plugins.conf",
        NULL);
    rclib_plugin_init(plugin_conf);
    g_free(plugin_conf);
    plugin_dir = g_build_filename(home_dir, ".RhythmCat2", "Plugins", NULL);
    g_mkdir_with_parents(plugin_dir, 0700);
    rclib_plugin_load_from_dir(plugin_dir);
    g_free(plugin_dir);
    rclib_plugin_load_from_configure();
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    GError *error = NULL;
    gchar *data_dir = NULL;
    const gchar *home_dir = NULL;
    gint status;
    g_set_application_name("RhythmCat2");
    g_set_prgname("RhythmCat2");
    app = gtk_application_new("org.rhythmcat.RhythmCat2", 0);
    if(!g_application_register(G_APPLICATION(app), NULL, &error))
    {
        g_warning("Cannot register player: %s", error->message);
        g_error_free(error);
        error = NULL;
    }
    if(g_application_get_is_registered(G_APPLICATION(app)))
    {
        if(g_application_get_is_remote(G_APPLICATION(app)))
        {
            g_message("This player is running already!");
            exit(0);
        }
    }
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    data_dir = g_build_filename(home_dir, ".RhythmCat2", NULL);
    if(!rclib_init(&argc, &argv, data_dir, &error))
    {
        g_error("Cannot load core: %s", error->message);
        g_error_free(error);
        g_free(data_dir);
        return 1;
    }
    gdk_threads_init();
    g_print("LibRhythmCat loaded. Version: %d.%d.%d, build date: %s\n",
        rclib_major_version, rclib_minor_version, rclib_micro_version,
        rclib_build_date);
    g_signal_connect(app, "activate", G_CALLBACK(rc_main_app_activate_cb),
        NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    rclib_plugin_exit();
    rc_mpris2_exit();
    rclib_exit();
    g_free(data_dir);
    return status;
}

