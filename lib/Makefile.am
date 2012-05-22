
lib_LTLIBRARIES = librhythmcat-2.0.la

librhythmcat_2_0_sources = \
    rclib-core.c  rclib-cue.c rclib-db.c  rclib-tag.c rclib-player.c \
    rclib-util.c rclib-lyric.c rclib-settings.c rclib-album.c \
    rclib-plugin.c rclib.c
    
librhythmcat_2_0_builtsources = rclib-marshal.c

librhythmcat_2_0_headers = \
    rclib-core.h rclib-cue.h rclib-db.h rclib-tag.h rclib-util.h \
    rclib-player.h rclib-lyric.h rclib-settings.h \
    rclib-album.h rclib-plugin.h rclib.h

librhythmcat_2_0_builtheaders = rclib-marshal.h

rclib-marshal.c: rclib-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rclib_marshal $(srcdir)/rclib-marshal.list \
        --body > rclib-marshal.c

rclib-marshal.h: rclib-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rclib_marshal $(srcdir)/rclib-marshal.list \
        --header > rclib-marshal.h

librhythmcat_2_0_la_SOURCES = \
    $(librhythmcat_2_0_sources) $(librhythmcat_2_0_builtsources)

INCLUDES = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_GLIB_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\"

librhythmcat_2_0_la_LIBADD = @GLIB2_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@

CLEANFILES = $(BUILT_SOURCES)

librhythmcat_2_0includedir=$(includedir)/librhythmcat2
librhythmcat_2_0include_HEADERS = rclib-common.h \
    $(librhythmcat_2_0_headers) $(librhythmcat_2_0_builtheaders)
    
librhythmcat_2_0_la_SOURCES += $(BUILT_SOURCES)

librhythmcat_2_0_la_LDFLAGS = -export-dynamic -O2

if PLATFORM_WIN32
    librhythmcat_2_0_la_LIBADD += -lkernel32
    librhythmcat_2_0_la_LDFLAGS += -mwindows
endif

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = 
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)/lib
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)/lib

introspection_sources = $(librhythmcat_2_0_sources) $(librhythmcat_2_0_headers)

RhythmCatLib-2.0.gir: librhythmcat-2.0.la
RhythmCatLib_2_0_gir_SCANNERFLAGS = --identifier-prefix=RCLib --symbol-prefix=rclib
RhythmCatLib_2_0_gir_NAMESPACE = RhythmCatLib
RhythmCatLib_2_0_gir_VERSION = 2.0
RhythmCatLib_2_0_gir_INCLUDES = GObject-2.0 Gst-0.10 GstPbutils-0.10 GstBase-0.10 GstFft-0.10
RhythmCatLib_2_0_gir_CFLAGS = $(INCLUDES)
RhythmCatLib_2_0_gir_LIBS = librhythmcat-2.0.la
RhythmCatLib_2_0_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += RhythmCatLib-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

EXTRA_DIST = rclib-marshal.list librhythmcat-2.0.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librhythmcat-2.0.pc

