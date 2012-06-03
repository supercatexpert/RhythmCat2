plugindir = $(libdir)/RhythmCat2/plugins
crawlermoduledir= $(libdir)/RhythmCat2/plugins/CrawlerModules

lrccrawler_ttplayer_la_LDFLAGS = -module -avoid-version -export-dynamic -O2

plugin_LTLIBRARIES = 
crawlermodule_LTLIBRARIES =    
noinst_LTLIBRARIES =

AM_CFLAGS = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @GTK_CFLAGS@ @JSON_GLIB_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib -I$(top_srcdir)/ui

if WITH_GTK_UI
AM_CFLAGS += @GTK_CFLAGS@ -I$(top_srcdir)/ui
endif

if WITH_NATIVE_PLUGINS

if WITH_GTK_UI

if HAVE_LIBCURL

crawlermodule_LTLIBRARIES += \
    lrccrawler-ttplayer.la

lrccrawler_ttplayer_la_CFLAGS = $(AM_CFLAGS) @LIBCURL_CFLAGS@ -I$(top_srcdir)/plugins/extra
lrccrawler_ttplayer_la_SOURCES = lrccrawler-ttplayer.c ../extra/lrccrawler-common.c
lrccrawler_ttplayer_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @LIBCURL_LIBS@
lrccrawler_ttplayer_la_LIBTOOLFLAGS = --tag=disable-static

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
