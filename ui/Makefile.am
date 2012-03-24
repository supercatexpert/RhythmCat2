bin_PROGRAMS = RhythmCat2

RhythmCat2_SOURCES = \
    rc-ui-listmodel.c rc-ui-player.c rc-ui-listview.c rc-ui-menu.c \
    rc-ui-dialog.c rc-ui-slabel.c rc-ui-spectrum.c rc-ui-style.c \
    rc-ui-plugin.c rc-ui-settings.c rc-ui-effect.c rc-mpris2.c rc-main.c \
    main.c

RhythmCat2_headers = \
    rc-ui-listmodel.h rc-ui-listview.h rc-ui-player.h rc-ui-menu.h \
    rc-ui-dialog.h rc-ui-slabel.h rc-ui-style.h rc-ui-spectrum.h \
    rc-ui-plugin.h rc-ui-settings.h rc-ui-effect.h rc-mpris2.h rc-main.h \
    rc-common.h rc-ui-css.h rc-ui-img-icon.xpm rc-ui-img-nocov.xpm

RhythmCat2includedir=$(includedir)/ui
RhythmCat2include_HEADERS = $(RhythmCat2_headers)

RhythmCat2_DEPENDENCIES = @LIBOBJS@
RhythmCat2_LDFLAGS = -export-dynamic

INCLUDES = @GLIB2_CFLAGS@ @GTK_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib

RhythmCat2_LDADD = @LIBOBJS@ @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ \
    @JSON_LIBS@ $(top_builddir)/lib/librhythmcat-2.0.la

EXTRA_DIST = \
    rhythmcat-2.0.pc.in
    
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rhythmcat-2.0.pc

