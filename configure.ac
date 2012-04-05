#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_CONFIG_MACRO_DIR(m4)
m4_define([rc_major_version], [1])
m4_define([rc_minor_version], [9])
m4_define([rc_micro_version], [2])
m4_define([rc_version], [rc_major_version.rc_minor_version.rc_micro_version])

AC_PREREQ([2.61])
AC_INIT([RhythmCat], [rc_version], [https://github.com/supercatexpert/RhythmCat2/issues/])
AM_INIT_AUTOMAKE([RhythmCat], [rc_version])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

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

# Set of available languages. (Obsoleted!)
#ALL_LINGUAS="zh_CN zh_TW ja_JP"

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_LIBTOOL
CFLAGS="-Wall -O3 -DINSTALL_PATH=\\\"$datadir/RhythmCat2/\\\""

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, no)
if test "$GLIB_GENMARSHAL" = no; then
    AC_MSG_ERROR([glib-genmarshal not found!])
fi

# Check for gtk-doc
m4_ifdef([GTK_DOC_CHECK], [GTK_DOC_CHECK([1.14],[--flavour no-tmpl])], \
    [AM_CONDITIONAL([ENABLE_GTK_DOC], false)])

# Checks for libraries.
PKG_CHECK_MODULES([GLIB2], [glib-2.0 >= 2.30, gthread-2.0 >= 2.30, \
    gmodule-2.0 >= 2.30, gio-2.0 >= 2.30])
PKG_CHECK_MODULES([GSTREAMER], [gstreamer-0.10 >= 0.10.30, \
    gstreamer-pbutils-0.10 >= 0.10.30 gstreamer-plugins-base-0.10 >= 0.10.30 \
    gstreamer-fft-0.10])
PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0.0], have_gtk3=yes, have_gtk3=no)
PKG_CHECK_MODULES([JSON], [json >= 0.9])

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
GETTEXT_PACKAGE=RhythmCat2
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", \
    [Define to the Gettext package name])
AM_GLIB_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.18.1])
AM_GNU_GETTEXT([external])

# Checks for features
AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], \
    [enable debug mode [default=no]]), enable_debug=yes, enable_debug=no)
AM_CONDITIONAL(DEBUG_MODE, test "x$enable_debug" = "xyes")

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

AC_CONFIG_FILES([lib/librhythmcat-2.0.pc ui/rhythmcat-2.0.pc])
AC_OUTPUT([Makefile lib/Makefile ui/Makefile \
    plugins/Makefile plugins/base/Makefile plugins/extra/Makefile \
    plugins/devel/Makefile plugins/restricted/Makefile \
    data/Makefile data/icons/Makefile data/images/Makefile \
    docs/Makefile docs/reference/Makefile docs/reference/lib/Makefile \
    docs/reference/ui/Makefile docs/reference/lib/version.xml \
    docs/reference/ui/version.xml po/Makefile.in po/Makefile])

AM_COND_IF(ENABLE_GTK_DOC, enable_gtk_doc=yes, enable_gtk_doc=no)
with_console_ui=no

AC_OUTPUT
echo
echo $PACKAGE $VERSION

echo
echo Build GTK+ 3 UI............. : $with_gtk_ui
echo Build Console UI............ : $with_console_ui
echo
echo Enable DEBUG.................: $enable_debug
echo Enable Gtk-Doc...............: $enable_gtk_doc
echo
echo With Native Plug-ins.........: $with_native_plugins
echo With Python3 Plug-ins........: $with_python3_plugins
echo
echo configure complete, now type \'make\'
echo



