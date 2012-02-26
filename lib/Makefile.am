
lib_LTLIBRARIES = librhythmcat-2.0.la

librhythmcat_2_0_la_SOURCES = \
    rclib-core.c  rclib-cue.c rclib-db.c  rclib-tag.c rclib-player.c \
    rclib-util.c rclib-lyric.c rclib-settings.c rclib-dbus.c rclib-album.c \
    rclib.c rclib-core.h rclib-cue.h rclib-db.h rclib-tag.h rclib-util.h \
    rclib-player.h rclib-lyric.h rclib-settings.h rclib-dbus.h rclib-album.h \
    rclib.h rclib-marshal.list

INCLUDES = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -DG_LOG_DOMAIN=\"LibRhythmCat\"

librhythmcat_2_0_la_LIBADD = @GLIB2_LIBS@ @GSTREAMER_LIBS@ @JSON_LIBS@

BUILT_SOURCES=

CLEAN_FILES = $(BUILT_SOURCES)

rclib-marshal.c: rclib-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rclib_marshal $(srcdir)/rclib-marshal.list \
        --body > rclib-marshal.c

rclib-marshal.h: rclib-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rclib_marshal $(srcdir)/rclib-marshal.list \
        --header > rclib-marshal.h

BUILT_SOURCES += \
	rclib-marshal.c rclib-marshal.h

librhythmcat_2_0_la_SOURCES += $(BUILT_SOURCES)

librhythmcat_2_0_la_LDFLAGS = -export-dynamic -O2

EXTRA_DIST = rclib-marshal.list

