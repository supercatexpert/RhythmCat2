bin_PROGRAMS = RhythmCat2

RhythmCat2_SOURCES = ui-listmodel.c ui-player.c ui-listview.c ui-menu.c \
    ui-dialog.c ui-slabel.c ui-spectrum.c ui-listmodel.h ui-listview.h \
    ui-player.h ui-menu.h ui-dialog.h ui-slabel.h ui-spectrum.h common.h \
    ui-img-icon.xpm ui-img-nocov.xpm main.c

RhythmCat2_DEPENDENCIES = @LIBOBJS@

INCLUDES = @GLIB2_CFLAGS@ @GTK_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_builddir)/lib

RhythmCat2_LDADD = @LIBOBJS@ @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ \
    @JSON_LIBS@ $(top_builddir)/lib/librhythmcat-2.0.la


