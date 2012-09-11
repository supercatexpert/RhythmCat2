/*
 * RhythmCat Main Module
 * The main module for the player.
 *
 * rc-main.c
 * This file is part of RhythmCat Music Player (GTK+ Version)
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
#include <gst/gst.h>
#include <glib/gprintf.h>
#include <rclib.h>
#include "rc-main.h"
#include "rc-ui-player.h"
#include "rc-ui-window.h"
#include "rc-ui-listview.h"
#include "rc-common.h"
#include "rc-ui-style.h"
#include "rc-ui-resources.h"
#include "rc-ui-dialog.h"
#include "rc-ui-spectrum.h"

#ifdef ENABLE_INTROSPECTION
#include <girepository.h>
#endif

/**
 * SECTION: rc-main
 * @Short_description: Main application functions
 * @Title: RCMainApplication
 * @Include: rc-main.h
 *
 * The #RCMainApplication is a class which inherits #GtkApplication. It
 * manages the initialization, startup, running, and uninitialzation of
 * the program.
 *
 * It also provides some utility functions.
 */

#ifdef DEBUG_FLAG
    static gboolean main_make_debug_flag = TRUE;
#else
    static gboolean main_make_debug_flag = FALSE;
#endif

struct _RCMainApplicationPrivate
{
    gint dummy;
};

static GObject *main_application_instance = NULL;
static gpointer rc_main_application_parent_class = NULL;
static gchar main_app_id[] = "org.rhythmcat.RhythmCat2";
static gboolean main_debug_flag = FALSE;
static gboolean main_malloc_flag = FALSE;
static gchar **main_remaining_args = NULL;
static gchar *main_data_dir = NULL;
static gchar *main_user_dir = NULL;

static inline void rc_main_settings_init()
{
    if(!rclib_settings_has_key("MainUI", "MinimizeToTray", NULL))
        rclib_settings_set_boolean("MainUI", "MinimizeToTray", FALSE);
    if(!rclib_settings_has_key("MainUI", "MinimizeWhenClose", NULL))
        rclib_settings_set_boolean("MainUI", "MinimizeWhenClose", FALSE);
    if(!rclib_settings_has_key("MainUI", "SpectrumStyle", NULL))
    {
        rclib_settings_set_integer("MainUI", "SpectrumStyle",
            RC_UI_SPECTRUM_STYLE_WAVE_MULTI);
    }
}

static gboolean rc_main_autosave_idle(gpointer data)
{
    rc_ui_dialog_show_load_autosaved();
    return FALSE;
}

static void rc_main_app_activate(GApplication *application)
{
    GtkSettings *settings;
    gchar *theme;
    gchar *theme_file;
    gchar *plugin_dir;
    gchar *plugin_conf;
    GFile *prefixdir_gfile;
    GFile *libdir_gfile;
    gchar *libdir_name = NULL;
    RCLibDbCatalogIter *catalog_iter = NULL;
    RCLibDbPlaylistIter *playlist_iter = NULL;
    RCLibDbCatalogSequence *catalog = NULL;
    RCLibDbPlaylistSequence *playlist = NULL;
    GFile *file;
    gchar *uri;
    gint i;
    gboolean theme_flag = FALSE;
    gboolean column_flag;
    guint column_flags = 0;
    guint plugin_number = 0;
    gchar *tmp_string;
    GError *error = NULL;
    if(application!=NULL)
        rc_ui_player_init(GTK_APPLICATION(application));
    else
        rc_ui_player_init(NULL);
    if(!rclib_settings_get_boolean("MainUI", "DisableTheme", NULL))
    {
        settings = gtk_settings_get_default();
        g_object_set(settings, "gtk-theme-name", "Adwaita",
            "gtk-application-prefer-dark-theme", TRUE, NULL);
        theme = rclib_settings_get_string("MainUI", "Theme", NULL);
        if(theme!=NULL && strlen(theme)>0)
        {
            if(g_str_has_prefix(theme, "embedded-theme:"))
            {
                theme_flag = rc_ui_style_embedded_theme_set_by_name(
                    theme+14);
            }
            else
            {
                theme_file = g_build_filename(theme, "gtk3.css", NULL);
                theme_flag = rc_ui_style_css_set_file(theme_file);
                g_free(theme_file);
            }
            if(!theme_flag)
            {
                rc_ui_style_embedded_theme_set_default();
            }
        }
        else
        {
            rc_ui_style_embedded_theme_set_default();
        }
        g_free(theme); 
    }
    rclib_settings_apply();
    if(rclib_settings_has_key("MainUI", "HideCoverImage", NULL))
    {
        rc_ui_main_window_cover_image_set_visible(
            !rclib_settings_get_boolean("MainUI", "HideCoverImage", NULL));
    }
    if(rclib_settings_has_key("MainUI", "HideLyricLabels", NULL))
    {
        rc_ui_main_window_lyric_labels_set_visible(
            !rclib_settings_get_boolean("MainUI", "HideLyricLabels", NULL));
    }
    if(rclib_settings_has_key("MainUI", "HideSpectrumWidget", NULL))
    {
        rc_ui_main_window_spectrum_set_visible(
            !rclib_settings_get_boolean("MainUI", "HideSpectrumWidget",
            NULL));
    }
    if(rclib_settings_get_boolean("MainUI", "UseMultiColumns", NULL))
    {
        rc_ui_listview_playlist_set_column_display_mode(TRUE);
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnArtistEnabled", NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnAlbumEnabled", NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnTrackNumberEnabled", NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnYearEnabled", NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnFileTypeEnabled", NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE;
        column_flag = rclib_settings_get_boolean("MainUI",
            "PlaylistColumnRatingEnabled", NULL);
        if(column_flag)
            column_flags |= RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING;
        rc_ui_listview_playlist_set_enabled_columns(
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_ARTIST |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_ALBUM |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_TRACK |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_YEAR |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_FTYPE |
            RC_UI_LISTVIEW_PLAYLIST_COLUMN_RATING,
            column_flags);
    }
    else
    {
        tmp_string = rclib_settings_get_string("MainUI",
            "PlaylistTitleFormat", NULL);
        if(tmp_string!=NULL && g_strstr_len(tmp_string, -1, "%TITLE")!=NULL)
            rc_ui_listview_playlist_set_title_format(tmp_string);
        g_free(tmp_string);
    }
    error = NULL;
    i = rclib_settings_get_integer("MainUI", "SpectrumStyle",
        &error);
    if(error==NULL)
    {
        rc_ui_main_window_spectrum_set_style(i);
    }
    else
    {
        rc_ui_main_window_spectrum_set_style(
            RC_UI_SPECTRUM_STYLE_WAVE_MULTI);
        g_error_free(error);
        error = NULL;
    }
    plugin_conf = g_build_filename(main_user_dir, "plugins.conf",
        NULL);
    rclib_plugin_init(plugin_conf);
    g_free(plugin_conf);
    plugin_dir = g_build_filename(main_user_dir, "Plugins", NULL);
    g_mkdir_with_parents(plugin_dir, 0700);
    rclib_plugin_load_from_dir(plugin_dir);
    prefixdir_gfile = g_file_new_for_path(PREFIXDIR);
    libdir_gfile = g_file_new_for_path(LIBDIR);
    if(prefixdir_gfile!=NULL && libdir_gfile!=NULL)
    {
        libdir_name = g_file_get_relative_path(prefixdir_gfile,
            libdir_gfile);
    }
    if(prefixdir_gfile!=NULL) g_object_unref(prefixdir_gfile);
    if(libdir_gfile!=NULL) g_object_unref(libdir_gfile);
    if(libdir_name==NULL) libdir_name = g_strdup("lib");
    plugin_dir = g_build_filename(main_data_dir, "..", "..", libdir_name,
        "RhythmCat2", "plugins", NULL);
    g_free(libdir_name);
    if(g_file_test(plugin_dir, G_FILE_TEST_IS_DIR))
        plugin_number = rclib_plugin_load_from_dir(plugin_dir);
    g_free(plugin_dir);
    if(plugin_number==0)
    {
        plugin_dir = g_build_filename(LIBDIR, "RhythmCat2", "plugins", NULL);
        if(g_file_test(plugin_dir, G_FILE_TEST_IS_DIR))
            rclib_plugin_load_from_dir(plugin_dir);
        g_free(plugin_dir);
    }
    rclib_plugin_load_from_configure();
    catalog = rclib_db_get_catalog();
    if(rclib_settings_get_boolean("Player", "LoadLastPosition", NULL) &&
        catalog!=NULL)
    {
        catalog_iter = rclib_db_catalog_sequence_get_iter_at_pos(catalog,
            rclib_settings_get_integer("Player", "LastPlayedCatalog", NULL));
        playlist = NULL;
        if(catalog_iter!=NULL)
        {
            rclib_db_catalog_data_iter_get(catalog_iter,
                RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
                RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        }
        if(playlist!=NULL)
        {
            playlist_iter = rclib_db_playlist_sequence_get_iter_at_pos(
                playlist, rclib_settings_get_integer("Player",
                "LastPlayedMusic", NULL));
        }
        if(playlist_iter!=NULL)
        {
            rclib_player_play_db(playlist_iter);
            if(!rclib_settings_get_boolean("Player", "AutoPlayWhenStartup",
                NULL))
            {
                rclib_core_pause();
            }
        }
        
    }
    else if(rclib_settings_get_boolean("Player", "AutoPlayWhenStartup",
        NULL) && catalog!=NULL)
    {
        if(catalog!=NULL)
            catalog_iter = rclib_db_catalog_sequence_get_begin_iter(catalog);
        playlist = NULL;
        if(catalog_iter!=NULL)
        {
            rclib_db_catalog_data_iter_get(catalog_iter,
                RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST, &playlist,
                RCLIB_DB_CATALOG_DATA_TYPE_NONE);
        }
        if(playlist!=NULL)
        {
            playlist_iter = rclib_db_playlist_sequence_get_begin_iter(
                playlist);
        }
        if(playlist_iter!=NULL)
            rclib_player_play_db(playlist_iter);
    }
    rc_ui_main_window_show();
    if(main_remaining_args!=NULL)
    {
        if(catalog!=NULL)
            catalog_iter = rclib_db_catalog_sequence_get_begin_iter(catalog);
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
    if(rclib_db_autosaved_exist())
        g_idle_add(rc_main_autosave_idle, NULL);
}

static void rc_main_app_open(GApplication *application, GFile **files,
    gint n_files, const gchar *hint)
{
    GtkTreeIter tree_iter;
    RCLibDbCatalogIter *catalog_iter = NULL;
    RCLibDbCatalogSequence *catalog;
    gint i;
    gchar *uri;
    if(files==NULL) return;
    if(rc_ui_listview_catalog_get_cursor(&tree_iter))
        catalog_iter = tree_iter.user_data;
    else
    {
        catalog = rclib_db_get_catalog();
        if(catalog!=NULL)
            catalog_iter = rclib_db_catalog_sequence_get_begin_iter(catalog);
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

static void rc_main_application_finalize(GObject *object)
{
    G_OBJECT_CLASS(rc_main_application_parent_class)->finalize(object);
}

static GObject *rc_main_application_constructor(GType type,
    guint n_construct_params, GObjectConstructParam *construct_params)
{
    GObject *retval;
    if(main_application_instance!=NULL) return main_application_instance;
    retval = G_OBJECT_CLASS(rc_main_application_parent_class)->constructor
        (type, n_construct_params, construct_params);
    main_application_instance = retval;
    g_object_add_weak_pointer(retval, (gpointer)&main_application_instance);
    return retval;
}

static void rc_main_application_class_init(RCMainApplicationClass *klass)
{
    GObjectClass *object_class;
    GApplicationClass *application_class;
    object_class = G_OBJECT_CLASS(klass);
    rc_main_application_parent_class = g_type_class_peek_parent(klass);
    object_class->constructor = rc_main_application_constructor;
    object_class->finalize = rc_main_application_finalize;
    
    application_class = G_APPLICATION_CLASS(klass);
    application_class->open = rc_main_app_open;
    application_class->activate = rc_main_app_activate; 
    
    g_type_class_add_private(klass, sizeof(RCMainApplicationPrivate));
}

static void rc_main_application_instance_init(RCMainApplication *app)
{
    RCMainApplicationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE(app,
        RC_TYPE_MAIN_APPLICATION, RCMainApplicationPrivate);
    app->priv = priv;
}

GType rc_main_application_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo app_info = {
        .class_size = sizeof(RCMainApplicationClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rc_main_application_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCMainApplication),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)rc_main_application_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(GTK_TYPE_APPLICATION,
            g_intern_static_string("RCMainApplication"), &app_info, 0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

/**
 * rc_main_run:
 * @argc: (inout): address of the <parameter>argc</parameter> parameter of
 *     your main() function (or 0 if @argv is %NULL). This will be changed if 
 *     any arguments were handled
 * @argv: (array length=argc) (inout) (allow-none): address of the
 *     <parameter>argv</parameter> parameter of main(), or %NULL
 *
 * Start to run the player.
 *
 * Returns: The return value of the player.
 */

gint rc_main_run(gint *argc, gchar **argv[])
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
    RCLibDbCatalogSequence *catalog;
    GError *error = NULL;
    const gchar *home_dir = NULL;
    gint status;
    GFile **remote_files;
    guint remote_file_num;
    guint i;
    gchar *locale_dir;
    setlocale(LC_ALL, NULL);
    main_data_dir = rclib_util_get_data_dir("RhythmCat2", *argv[0]);
    locale_dir = g_build_filename(main_data_dir, "..", "locale", NULL);
    if(g_file_test(locale_dir, G_FILE_TEST_IS_DIR))
    {
        bindtextdomain(GETTEXT_PACKAGE, locale_dir);
        g_free(locale_dir);
    }
    else
    {
        bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    }
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gst_init_get_option_group());
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    #ifdef ENABLE_INTROSPECTION
	    g_option_context_add_group(context,
	        g_irepository_get_option_group());
    #endif
    if(!g_option_context_parse(context, argc, argv, &error))
    {
        g_print(_("%s\nRun '%s --help' to see a full list of available "
            "command line options.\n"), error->message, *argv[0]);
        g_error_free(error);
        g_option_context_free(context);
        exit(1);
    }
    g_option_context_free(context);
    g_random_set_seed(time(0));
    if(main_malloc_flag)
        g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);
    if(main_debug_flag || main_make_debug_flag)
        g_setenv("G_MESSAGES_DEBUG", "all", TRUE);
    g_set_application_name("RhythmCat2");
    g_set_prgname("RhythmCat2");
    app = g_object_new(RC_TYPE_MAIN_APPLICATION, "application-id",
        main_app_id, "flags", G_APPLICATION_HANDLES_OPEN, NULL);
    if(app!=NULL)
    {
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
    }
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    main_user_dir = g_build_filename(home_dir, ".RhythmCat2", NULL);
    if(!rclib_init(argc, argv, main_user_dir, &error))
    {
        g_error("Cannot load core: %s", error->message);
        g_error_free(error);
        g_free(main_user_dir);
        g_free(main_data_dir);
        main_user_dir = NULL;
        return 1;
    }
    rc_main_settings_init();
    gdk_threads_init();
    g_print("LibRhythmCat loaded. Version: %d.%d.%d, build date: %s\n",
        rclib_major_version, rclib_minor_version, rclib_micro_version,
        rclib_build_date);
    catalog = rclib_db_get_catalog();
    if(catalog!=NULL && rclib_db_catalog_sequence_get_length(catalog)==0)
        rclib_db_catalog_add(_("Default Playlist"), NULL,
        RCLIB_DB_CATALOG_TYPE_PLAYLIST);
    g_resources_register(rc_ui_resources_get_resource());
    if(app!=NULL)
        status = g_application_run(G_APPLICATION(app), *argc, *argv);
    else /* If GtkApplication is not available, use fallback functions. */
    {
        gtk_init(argc, argv);
        rc_main_app_activate(NULL);
        gtk_main();
        status = 0;
    }
    g_resources_unregister(rc_ui_resources_get_resource());
    g_object_unref(app);
    return status;
}

/**
 * rc_main_exit:
 *
 * Exit from the player, save configure data and release all used resources.
 */

void rc_main_exit()
{
    rc_ui_player_exit();
    rclib_plugin_exit();
    rclib_exit();
    g_free(main_data_dir);
    g_free(main_user_dir);
}

/**
 * rc_main_get_data_dir:
 *
 * Get the program data directory path of the player.
 *
 * Returns: the program data directory path.
 */

const gchar *rc_main_get_data_dir()
{
    return main_data_dir;
}

/**
 * rc_main_get_user_dir:
 *
 * Get the user data directory path of the player.
 * (should be ~/.RhythmCat2)
 *
 * Returns: the user data directory path.
 */

const gchar *rc_main_get_user_dir()
{
    return main_user_dir;
}

