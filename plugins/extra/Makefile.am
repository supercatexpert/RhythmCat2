plugindir = $(libdir)/RhythmCat2/plugins
crawlermoduledir= $(libdir)/RhythmCat2/plugins/CrawlerModules

karaeffect_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
echoeffect_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
lrccrawler_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
lrccrawler_lrc123_la_LDFLAGS = -module -avoid-version -export-dynamic -O2

plugin_LTLIBRARIES = 
crawlermodule_LTLIBRARIES =
noinst_LTLIBRARIES =

noinst_HEADERS = lrccrawler-common.h

AM_CFLAGS = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_GLIB_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib

if WITH_GTK_UI
AM_CFLAGS += @GTK_CFLAGS@ -I$(top_srcdir)/ui
endif

if HAVE_LIBCURL

endif

if WITH_NATIVE_PLUGINS

if WITH_GTK_UI

plugin_LTLIBRARIES += \
    karaeffect.la \
    echoeffect.la

karaeffect_la_SOURCES = karaeffect.c
karaeffect_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
karaeffect_la_LIBTOOLFLAGS = --tag=disable-static

echoeffect_la_SOURCES = echoeffect.c
echoeffect_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
echoeffect_la_LIBTOOLFLAGS = --tag=disable-static

if HAVE_LIBCURL

plugin_LTLIBRARIES += \
    lrccrawler.la

lrccrawler_la_CFLAGS = $(AM_CFLAGS) @LIBCURL_CFLAGS@
lrccrawler_la_SOURCES = lrccrawler.c lrccrawler-common.c
lrccrawler_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ \
    @LIBCURL_LIBS@ $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
lrccrawler_la_LIBTOOLFLAGS = --tag=disable-static

crawlermodule_LTLIBRARIES += \
    lrccrawler-lrc123.la

lrccrawler_lrc123_la_CFLAGS = $(AM_CFLAGS) @LIBCURL_CFLAGS@
lrccrawler_lrc123_la_SOURCES = lrccrawler-lrc123.c lrccrawler-common.c
lrccrawler_lrc123_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @LIBCURL_LIBS@
lrccrawler_lrc123_la_LIBTOOLFLAGS = --tag=disable-static

endif

endif

endif

SUFFIXES = .c .so
.c.so:
	$(LIBTOOL) --mode=compile $(CC) -DHAVE_CONFIG_H -I$(top_builddir) $(AM_CPPFLAGS) $(CFLAGS) -c $< -o tmp$@.lo $(PLUGIN_CFLAGS)
	$(LIBTOOL) --mode=link    $(CC) $(CFLAGS) -o libtmp$@.la -rpath $(plugindir) tmp$@.lo $(LIBS) $(LDFLAGS) -module -avoid-version $(PLUGIN_LIBS)
	@rm -f tmp$@.lo tmp$@.o libtmp$@.la
	@cp .libs/libtmp$@*.so $@
	@rm -rf .libs/libtmp$@.*
