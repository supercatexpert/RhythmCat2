
lib_LTLIBRARIES = librhythmcat-2.0.la

librhythmcat_2_0_sources = \
    rclib-core.c  rclib-cue.c rclib-db.c  rclib-tag.c rclib-player.c \
    rclib-util.c rclib-lyric.c rclib-settings.c rclib-dbus.c rclib-album.c \
    rclib-plugin.c rclib.c
    
librhythmcat_2_0_builtsources = rclib-marshal.c

librhythmcat_2_0_headers = \
    rclib-core.h rclib-cue.h rclib-db.h rclib-tag.h rclib-util.h \
    rclib-player.h rclib-lyric.h rclib-settings.h rclib-dbus.h \
    rclib-album.h rclib-plugin.h rclib.h rclib-common.h

librhythmcat_2_0_builtheaders = rclib-marshal.h

rclib-marshal.c: rclib-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rclib_marshal $(srcdir)/rclib-marshal.list \
        --body > rclib-marshal.c

rclib-marshal.h: rclib-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rclib_marshal $(srcdir)/rclib-marshal.list \
        --header > rclib-marshal.h

librhythmcat_2_0_la_SOURCES = \
    $(librhythmcat_2_0_sources) $(librhythmcat_2_0_builtsources)

INCLUDES = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\"

librhythmcat_2_0_la_LIBADD = @GLIB2_LIBS@ @GSTREAMER_LIBS@ @JSON_LIBS@

CLEAN_FILES = $(BUILT_SOURCES)

librhythmcat_2_0includedir=$(includedir)/librhythmcat2
librhythmcat_2_0include_HEADERS = \
    $(librhythmcat_2_0_headers) $(librhythmcat_2_0_builtheaders)
    
librhythmcat_2_0_la_SOURCES += $(BUILT_SOURCES)

librhythmcat_2_0_la_LDFLAGS = -export-dynamic -O2

EXTRA_DIST = rclib-marshal.list librhythmcat-2.0.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librhythmcat-2.0.pc

