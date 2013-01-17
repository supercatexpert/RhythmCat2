/*
 * RhythmCat UI Library Views Header Declaration
 *
 * rc-ui-library-view.h
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

#ifndef HAVE_RC_UI_LIBRARY_VIEW_H
#define HAVE_RC_UI_LIBRARY_VIEW_H

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <rclib.h>

G_BEGIN_DECLS

#define RC_UI_TYPE_LIBRARY_LIST_VIEW (rc_ui_library_list_view_get_type())
#define RC_UI_LIBRARY_LIST_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_LIBRARY_LIST_VIEW, RCUiLibraryListView))
#define RC_UI_LIBRARY_LIST_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_TYPE_LIBRARY_LIST_VIEW, \
    RCUiLibraryListViewClass))
#define RC_UI_IS_LIBRARY_LIST_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_LIBRARY_LIST_VIEW))
#define RC_UI_IS_LIBRARY_LIST_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_LIBRARY_LIST_VIEW))
#define RC_UI_LIBRARY_LIST_VIEW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_LIBRARY_LIST_VIEW, \
    RCUiLibraryListView))
    
#define RC_UI_TYPE_LIBRARY_PROP_VIEW (rc_ui_library_prop_view_get_type())
#define RC_UI_LIBRARY_PROP_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
    RC_UI_TYPE_LIBRARY_PROP_VIEW, RCUiLibraryPropView))
#define RC_UI_LIBRARY_PROP_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), RC_UI_TYPE_LIBRARY_PROP_VIEW, \
    RCUiLibraryPropViewClass))
#define RC_UI_IS_LIBRARY_PROP_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
    RC_UI_TYPE_LIBRARY_PROP_VIEW))
#define RC_UI_IS_LIBRARY_PROP_VIEW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), RC_UI_TYPE_LIBRARY_PROP_VIEW))
#define RC_UI_LIBRARY_PROP_VIEW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), RC_UI_TYPE_LIBRARY_PROP_VIEW, \
    RCUiLibraryPropView))

typedef struct _RCUiLibraryListView RCUiLibraryListView;
typedef struct _RCUiLibraryPropView RCUiLibraryPropView;
typedef struct _RCUiLibraryListViewClass RCUiLibraryListViewClass;
typedef struct _RCUiLibraryPropViewClass RCUiLibraryPropViewClass;
typedef struct _RCUiLibraryListViewPrivate RCUiLibraryListViewPrivate;
typedef struct _RCUiLibraryPropViewPrivate RCUiLibraryPropViewPrivate;

/**
 * RCUiLibraryListView:
 *
 * The data structure used for #RCUiLibraryListView class.
 */

struct _RCUiLibraryListView {
    /*< private >*/
    GtkTreeView parent;
    RCUiLibraryListViewPrivate *priv;
};

/**
 * RCUiLibraryPropView:
 *
 * The data structure used for #RCUiLibraryPropView class.
 */

struct _RCUiLibraryPropView {
    /*< private >*/
    GtkTreeView parent;
    RCUiLibraryPropViewPrivate *priv;
};

/**
 * RCUiLibraryListViewClass:
 *
 * #RCUiLibraryListViewClass class.
 */

struct _RCUiLibraryListViewClass {
    /*< private >*/
    GtkTreeViewClass parent_class;
};

/**
 * RCUiLibraryPropViewClass:
 *
 * #RCUiLibraryPropViewClass class.
 */

struct _RCUiLibraryPropViewClass {
    /*< private >*/
    GtkTreeViewClass parent_class;
};

/*< private >*/
GType rc_ui_library_list_view_get_type();
GType rc_ui_library_prop_view_get_type();

/*< public >*/
GtkWidget *rc_ui_library_list_view_new();
GtkWidget *rc_ui_library_prop_view_new();

#endif
