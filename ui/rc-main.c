#include <glib.h>
#include <gst/gst.h>
#include <glib/gprintf.h>
#include <rclib.h>
#include "rc-ui-player.h"
#include "rc-ui-listview.h"
#include "rc-mpris2.h"
#include "rc-common.h"
#include "rc-ui-style.h"
#include "rc-ui-css.h"

static gchar main_app_id[] = "org.rhythmcat.RhythmCat2";
static gboolean main_debug_flag = FALSE;
static gboolean main_malloc_flag = FALSE;
static gchar **main_remaining_args = NULL;

static void rc_main_app_activate_cb(GApplication *application,
    gpointer data)
{
    const gchar *home_dir = NULL;
    gchar *plugin_dir;
    gchar *plugin_conf;
    GSequenceIter *catalog_iter = NULL;
    GSequence *catalog;
    GFile *file;
    gchar *uri;
    gint i;
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
    if(main_remaining_args!=NULL)
    {
        catalog = rclib_db_get_catalog();
        if(catalog!=NULL)
            catalog_iter = g_sequence_get_begin_iter(catalog);
        if(catalog_iter!=NULL)
        {
            for(i=0;main_remaining_args[i]!=NULL;i++)
            {
                file = g_file_new_for_commandline_arg(
                    main_remaining_args[i]);
                if(file==NULL) continue;
                uri = g_file_get_uri(file);
                if(uri!=NULL)
                    rclib_db_playlist_add_music(catalog_iter, NULL, uri);
                g_free(uri);
                g_object_unref(file);
            }
        }
    }
    
    rc_ui_style_css_set_data(rc_ui_css_default, sizeof(rc_ui_css_default));
}

static void rc_main_app_open_cb(GApplication *application, GFile **files,
    gint n_files, gchar *hint, gpointer data)
{
    GtkTreeIter tree_iter;
    GSequenceIter *catalog_iter = NULL;
    GSequence *catalog;
    gint i;
    gchar *uri;
    if(files==NULL) return;
    if(rc_ui_listview_catalog_get_cursor(&tree_iter))
        catalog_iter = tree_iter.user_data;
    else
    {
        catalog = rclib_db_get_catalog();
        if(catalog!=NULL)
            catalog_iter = g_sequence_get_begin_iter(catalog);
    }
    if(catalog_iter==NULL) return;
    for(i=0;i<n_files;i++)
    {
        if(files[i]==NULL) continue;
        uri = g_file_get_uri(files[i]);
        if(uri==NULL) continue;
        rclib_db_playlist_add_music(catalog_iter, NULL, uri);
        g_free(uri);
    }
}

static gboolean rc_main_print_version(const gchar *option_name,
    const gchar *value, gpointer data, GError **error)
{
    g_print(_("RhythmCat2, library version %d.%d.%d\n"
        "Copyright (C) 2012 SuperCat <supercatexpert@gmail.com>\n"
        "A music player based on GTK+ 3.0 & GStreamer 0.10\n"),
        rclib_major_version, rclib_minor_version, rclib_micro_version);
    exit(0);
    return TRUE;
}

int main(int argc, char *argv[])
{
    static GOptionEntry options[] =
    {
        {"debug", 'd', 0, G_OPTION_ARG_NONE, &main_debug_flag,
            N_("Enable debug mode"), NULL},
        {"use-std-malloc", 0, 0, G_OPTION_ARG_NONE, &main_malloc_flag,
            N_("Use malloc function in stardard C library"), NULL},
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY,
            &main_remaining_args, NULL, "[URI...]"},
        {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
            rc_main_print_version,
            N_("Output version information and exit"), NULL},
        {NULL}
    };
    GOptionContext *context;
    GtkApplication *app;
    GSequence *catalog;
    GError *error = NULL;
    gchar *data_dir = NULL;
    const gchar *home_dir = NULL;
    gint status;
    GFile **remote_files;
    guint remote_file_num;
    guint i;
    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gst_init_get_option_group());
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    setlocale(LC_ALL, NULL);
    if(!g_option_context_parse(context, &argc, &argv, &error))
    {
        g_print(_("%s\nRun '%s --help' to see a full list of available "
            "command line options.\n"), error->message, argv[0]);
        g_error_free(error);
        g_option_context_free(context);
        exit(1);
    }
    g_option_context_free(context);
    g_random_set_seed(time(0));
    if(main_malloc_flag)
        g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);
    if(main_debug_flag)
        ;
    g_set_application_name("RhythmCat2");
    g_set_prgname("RhythmCat2");
    app = gtk_application_new(main_app_id, G_APPLICATION_HANDLES_OPEN);
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
            if(main_remaining_args==NULL) exit(0);
            remote_file_num = g_strv_length(main_remaining_args);
            if(remote_file_num<1) exit(0);
            remote_files = g_new0(GFile *, remote_file_num);
            for(i=0;main_remaining_args[i]!=NULL;i++)
            {
                remote_files[i] = g_file_new_for_commandline_arg(
                    main_remaining_args[i]);
            }
            g_application_open(G_APPLICATION(app), remote_files,
                remote_file_num, "RhythmCat2::open");
            for(i=0;i<remote_file_num;i++)
                g_object_unref(remote_files[i]);
            g_free(remote_files);
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
    catalog = rclib_db_get_catalog();
    if(catalog!=NULL && g_sequence_get_length(catalog)==0)
        rclib_db_catalog_add(_("Default Playlist"), NULL,
        RCLIB_DB_CATALOG_TYPE_PLAYLIST);
    g_signal_connect(app, "activate", G_CALLBACK(rc_main_app_activate_cb),
        NULL);
    g_signal_connect(app, "open", G_CALLBACK(rc_main_app_open_cb), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    rc_ui_player_exit();
    g_object_unref(app);
    rclib_plugin_exit();
    rc_mpris2_exit();
    rclib_exit();
    g_free(data_dir);
    return status;
}

