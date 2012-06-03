plugindir = $(libdir)/RhythmCat2/plugins

lyricshow_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
notify_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
desklrc_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
mediakey_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
mpris2_la_LDFLAGS = -module -avoid-version -export-dynamic -O2

plugin_LTLIBRARIES = 
noinst_LTLIBRARIES =

AM_CFLAGS = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@  @JSON_GLIB_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib

if WITH_GTK_UI
AM_CFLAGS += @GTK_CFLAGS@ -I$(top_srcdir)/ui
endif

if WITH_NATIVE_PLUGINS

plugin_LTLIBRARIES += \
    mediakey.la

mediakey_la_SOURCES = mediakey.c
mediakey_la_LIBADD = @GLIB2_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la 
mediakey_la_LIBTOOLFLAGS = --tag=disable-static

if WITH_GTK_UI

plugin_LTLIBRARIES += \
    lyricshow.la \
    notify.la \
    desklrc.la \
    mpris2.la

lyricshow_la_SOURCES = lyricshow.c
lyricshow_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
lyricshow_la_LIBTOOLFLAGS = --tag=disable-static
    
notify_la_SOURCES = notify.c
notify_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
notify_la_LIBTOOLFLAGS = --tag=disable-static

desklrc_la_SOURCES = desklrc.c
desklrc_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
desklrc_la_LIBTOOLFLAGS = --tag=disable-static

mpris2_la_SOURCES = mpris2.c
mpris2_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
mpris2_la_LIBTOOLFLAGS = --tag=disable-static

endif

endif

SUFFIXES = .c .so
.c.so:
	$(LIBTOOL) --mode=compile $(CC) -DHAVE_CONFIG_H -I$(top_builddir) \
	    $(AM_CPPFLAGS) $(CFLAGS) -c $< -o tmp$@.lo $(PLUGIN_CFLAGS) --tag=disable-static
	$(LIBTOOL) --mode=link    $(CC) $(CFLAGS) -o libtmp$@.la -rpath \
	    $(plugindir) tmp$@.lo $(LIBS) $(LDFLAGS) -module -avoid-version \$(PLUGIN_LIBS) --tag=disable-static
	@rm -f tmp$@.lo tmp$@.o libtmp$@.la
	@cp .libs/libtmp$@*.so $@
	@rm -rf .libs/libtmp$@.*

