bin_PROGRAMS = RhythmCat2
noinst_LTLIBRARIES = librhythmcat-ui-2.0.la

NOINST_H_FILES = \
    rc-ui-css.h rc-ui-img-icon.xpm rc-ui-img-nocov.xpm

librhythmcat_ui_2_0_sources = \
    rc-ui-listmodel.c rc-ui-player.c rc-ui-listview.c rc-ui-menu.c \
    rc-ui-dialog.c rc-ui-slabel.c rc-ui-spectrum.c rc-ui-style.c \
    rc-ui-plugin.c rc-ui-settings.c rc-ui-effect.c rc-mpris2.c rc-main.c

librhythmcat_ui_2_0_headers = \
    rc-ui-listmodel.h rc-ui-listview.h rc-ui-player.h rc-ui-menu.h \
    rc-ui-dialog.h rc-ui-slabel.h rc-ui-style.h rc-ui-spectrum.h \
    rc-ui-plugin.h rc-ui-settings.h rc-ui-effect.h rc-mpris2.h rc-main.h \
    rc-common.h 

RhythmCat2_SOURCES = \
    main.c

RhythmCat2_headers = \
    rc-ui-listmodel.h rc-ui-listview.h rc-ui-player.h rc-ui-menu.h \
    rc-ui-dialog.h rc-ui-slabel.h rc-ui-style.h rc-ui-spectrum.h \
    rc-ui-plugin.h rc-ui-settings.h rc-ui-effect.h rc-mpris2.h rc-main.h \
    rc-common.h rc-ui-css.h rc-ui-img-icon.xpm rc-ui-img-nocov.xpm

librhythmcat_ui_2_0_la_SOURCES = \
    $(librhythmcat_ui_2_0_sources)
librhythmcat_ui_2_0includedir=$(includedir)/rhythmcat2
librhythmcat_ui_2_0include_HEADERS = \
    $(librhythmcat_ui_2_0_headers) $(NOINST_H_FILES)

librhythmcat_ui_2_0_la_LDFLAGS = -export-dynamic -O2 -no-undefined -export-symbols-regex "^[[^_]].*"
librhythmcat_ui_2_0_la_LIBADD = @LIBOBJS@ @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ \
    @JSON_LIBS@ $(top_builddir)/lib/librhythmcat-2.0.la

RhythmCat2_DEPENDENCIES = @LIBOBJS@ librhythmcat-ui-2.0.la
RhythmCat2_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*" -O2

INCLUDES = @GLIB2_CFLAGS@ @GTK_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib

RhythmCat2_LDADD = @LIBOBJS@ @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ \
    @JSON_LIBS@ $(top_builddir)/lib/librhythmcat-2.0.la librhythmcat-ui-2.0.la

EXTRA_DIST = \
    rhythmcat-2.0.pc.in
    
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rhythmcat-2.0.pc

