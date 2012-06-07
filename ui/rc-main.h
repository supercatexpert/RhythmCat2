/*
 * RhythmCat Main Module Header Declaration
 *
 * rc-main.h
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

#ifndef HAVE_RC_MAIN_H
#define HAVE_RC_MAIN_H

#include <glib.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

#define RC_TYPE_MAIN_APPLICATION (rc_main_application_get_type())
#define RC_MAIN_APPLICATION(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_TYPE_MAIN_APPLICATION, RCMainApplication))
#define RC_MAIN_APPLICATION_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), \
    RC_TYPE_MAIN_APPLICATION, RCMainApplicationClass))
#define RC_IS_MAIN_APPLICATION(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), \
    RC_TYPE_MAIN_APPLICATION))
#define RC_IS_MAIN_APPLICATION_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), \
    RC_TYPE_MAIN_APPLICATION))
#define RC_MAIN_APPLICATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RC_TYPE_MAIN_APPLICATION, RCMainApplicationClass))

typedef struct _RCMainApplication RCMainApplication;
typedef struct _RCMainApplicationClass RCMainApplicationClass;

/**
 * RCMainApplication:
 *
 * The data structure used for #RCMainApplication class.
 */

struct _RCMainApplication {
    /*< private >*/
    GtkApplication parent;
};

/**
 * RCMainApplicationClass:
 *
 * #RCMainApplicationClass class.
 */

struct _RCMainApplicationClass {
    /*< private >*/
    GtkApplicationClass parent_class;
};

/*< private >*/
GType rc_main_application_get_type();

gint rc_main_run(gint *argc, gchar **argv[]);
void rc_main_exit();
const gchar *rc_main_get_data_dir();
const gchar *rc_main_get_user_dir();

G_END_DECLS

#endif

