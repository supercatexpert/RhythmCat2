/*
 * RhythmCat Library D-Bus Support Header Declaration
 *
 * rclib-dbus.h
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

#ifndef HAVE_RCLIB_DBUS_H
#define HAVE_RCLIB_DBUS_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define RCLIB_DBUS_TYPE (rclib_dbus_get_type())
#define RCLIB_DBUS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), RCLIB_DBUS_TYPE, \
    RCLibDBus))
#define RCLIB_DBUS_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), RCLIB_DBUS_TYPE, \
    RCLibDBusClass))
#define RCLIB_IS_DBUS(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), RCLIB_DBUS_TYPE))
#define RCLIB_IS_DBUS_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), RCLIB_DBUS_TYPE))
#define RCLIB_DBUS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_DBUS_TYPE, RCLibDBusClass))

typedef struct _RCLibDBus RCLibDBus;
typedef struct _RCLibDBusClass RCLibDBusClass;

/**
 * RCLibDBus:
 *
 * The D-Bus support. The contents of the #RCLibDBus structure are
 * private and should only be accessed via the provided API.
 */

struct _RCLibDBus {
    /*< private >*/
    GObject parent;
};

/**
 * RCLibDBusClass:
 *
 * #RCLibDBus class.
 */

struct _RCLibDBusClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*quit)(RCLibDBus *dbus);
    void (*raise)(RCLibDBus *dbus);
    void (*seeked)(RCLibDBus *bus, gint64 offset);
};

/*< private >*/
GType rclib_dbus_get_type();

/*< public >*/
gboolean rclib_dbus_init(const gchar *app_name,
    const gchar *first_property_name, ...);
void rclib_dbus_exit();
GObject *rclib_dbus_get_instance();
gulong rclib_dbus_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_dbus_signal_disconnect(gulong handler_id);

G_END_DECLS

#endif

