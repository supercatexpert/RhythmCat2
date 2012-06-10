#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_CONFIG_MACRO_DIR(m4)
m4_define([rc_major_version], [1])
m4_define([rc_minor_version], [9])
m4_define([rc_micro_version], [3])
m4_define([rc_version], [rc_major_version.rc_minor_version.rc_micro_version])

AC_PREREQ([2.61])
AC_INIT([RhythmCat], [rc_version], [https://github.com/supercatexpert/RhythmCat2/issues/])
AM_INIT_AUTOMAKE([RhythmCat], [rc_version])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

AM_SILENT_RULES([yes])

RC_MAJOR_VERSION=rc_major_version
RC_MINOR_VERSION=rc_minor_version
RC_MICRO_VERSIOn=rc_micro_version
RC_VERSION=rc_version

AC_SUBST(RC_MAJOR_VERSION)
AC_SUBST(RC_MINOR_VERSION)
AC_SUBST(RC_MICRO_VERSION)
AC_SUBST(RC_VERSION)

AC_DEFINE(RC_MAJOR_VERSION, [rc_major_version],
    [Define to the RhythmCat major version])
AC_DEFINE(RC_MINOR_VERSION, [rc_minor_version],
    [Define to the RhythmCat minor version])
AC_DEFINE(RC_MICRO_VERSION, [rc_micro_version],
    [Define to the RhythmCat micro version])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
CFLAGS="-Wall -O2 -DINSTALL_PATH=\\\"$datadir/RhythmCat2/\\\""

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)
if test "$GLIB_GENMARSHAL" = no; then
    AC_MSG_ERROR([glib-genmarshal not found!])
fi

AC_PATH_PROG(GLIB_COMPILE_RESOURCE, glib-compile-resources, no)
if test "$GLIB_COMPILE_RESOURCE" = no; then
    AC_MSG_ERROR([glib-compile-resources not found!])
fi

# Check for gtk-doc
m4_ifdef([GTK_DOC_CHECK], [GTK_DOC_CHECK([1.14],[--flavour no-tmpl])], \
    [AM_CONDITIONAL([ENABLE_GTK_DOC], false)])

# Checks for libraries.
PKG_CHECK_MODULES([GLIB2], [glib-2.0 >= 2.32, gthread-2.0 >= 2.32, \
    gmodule-2.0 >= 2.32, gio-2.0 >= 2.32])
PKG_CHECK_MODULES([GSTREAMER], [gstreamer-0.10 >= 0.10.30, \
    gstreamer-pbutils-0.10 >= 0.10.30 gstreamer-plugins-base-0.10 >= 0.10.30 \
    gstreamer-fft-0.10])
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.4.0], have_gtk3=yes, have_gtk3=no)

# Check additional libraries.
PKG_CHECK_MODULES([LIBCURL], [libcurl >= 7.0.0], have_libcurl=yes, \
    have_libcurl=no)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h stdio.h limits.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([bzero memset realpath strrchr strstr])

# Add the languages which your application supports to po/LINGUAS
IT_PROG_INTLTOOL([0.40.0])
GETTEXT_PACKAGE=RhythmCat2
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", \
    [Define to the Gettext package name])
AM_GLIB_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])

GOBJECT_INTROSPECTION_CHECK([1.32.0])

# Checks for features

AC_ARG_WITH(gtk-ui, AS_HELP_STRING([--without-gtk-ui], \
    [dont't compile GTK+ 3 UI [default=no]]), with_gtk_ui=no, \
    with_gtk_ui=yes)
if test "x$with_gtk_ui" != "xno"; then
    with_gtk_ui="yes"
    if test "x$have_gtk3" != "xyes"; then
        with_gtk_ui="no"
        echo "Please install GTK+ 3 library before compile GTK+ 3 UI!"
    fi
fi
AM_CONDITIONAL(WITH_GTK_UI, test "x$with_gtk_ui" != "xno")

AC_ARG_WITH(native-plugins, AS_HELP_STRING([--with-native-plugins], \
    [compile native plug-ins]), with_native_plugins=yes, \
    with_native_plugins=no)
AM_CONDITIONAL(WITH_NATIVE_PLUGINS, test "x$with_native_plugins" = "xyes") 

AC_ARG_WITH(python3-plugins, AS_HELP_STRING([--with-python3-plugins], \
    [compile Python3 plug-ins]), with_python3_plugins=yes,\
    with_python3_plugins=no)
if test "x$with_python3_plugins" = "xyes"; then
    PKG_CHECK_MODULES([PYTHON3], [python3 >= 3.1], have_python3=yes, \
        have_python3=no)
    if test "x$have_python3" != "xyes"; then
        with_python3_plugins="no"
        echo "Please install Python3 support before enable python plug-ins!"
    fi
fi
AM_CONDITIONAL(WITH_PYTHON3_PLUGINS, test "x$with_python3_plugins" = "xyes")

AM_CONDITIONAL(HAVE_LIBCURL, test "x$have_libcurl" = "xyes")

AM_COND_IF(ENABLE_GTK_DOC, enable_gtk_doc=yes, enable_gtk_doc=no)
with_console_ui=no

if test "$found_introspection" = "yes"; then
	enable_introspection=yes
    PKG_CHECK_MODULES([GINTROSPECTION], [gobject-introspection-1.0 >= 1.0])
else
	enable_introspection=no
fi

AC_MSG_CHECKING([for Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

AC_CONFIG_FILES([lib/librhythmcat-2.0.pc ui/rhythmcat-2.0.pc \
    Makefile lib/Makefile ui/Makefile ui/resources/Makefile \
    data/Makefile data/icons/Makefile data/images/Makefile \
    plugins/Makefile plugins/base/Makefile plugins/extra/Makefile \
    plugins/devel/Makefile plugins/restricted/Makefile docs/Makefile \
    docs/reference/Makefile docs/reference/lib/Makefile \
    docs/reference/ui/Makefile docs/reference/lib/version.xml \
    docs/reference/ui/version.xml po/Makefile.in po/Makefile \
])
AC_OUTPUT

echo "
$PACKAGE $VERSION

Build GTK+ 3 UI............. : $with_gtk_ui
Build Console UI............ : $with_console_ui

Enable Gtk-Doc...............: $enable_gtk_doc
Enable GObject Introspection.: $enable_introspection

With Native Plug-ins.........: $with_native_plugins
With Python3 Plug-ins........: $with_python3_plugins

configure complete, now type 'make'
"



