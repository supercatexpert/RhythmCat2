plugindir = $(libdir)/RhythmCat2/plugins

lrccrawler_la_LDFLAGS = -module -avoid-version -export-dynamic -O2

plugin_LTLIBRARIES = 

noinst_LTLIBRARIES =

noinst_HEADERS = lrccrawler-common.h

INCLUDES = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@  @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib
if WITH_GTK_UI
INCLUDES += @GTK_CFLAGS@ -I$(top_srcdir)/ui
endif

if HAVE_LIBCURL
INCLUDES += @LIBCURL_CFLAGS@
endif

if WITH_NATIVE_PLUGINS

if WITH_GTK_UI

if HAVE_LIBCURL

plugin_LTLIBRARIES += \
    lrccrawler.la
    
lrccrawler_la_SOURCES = lrccrawler.c lrccrawler-common.c
lrccrawler_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_LIBS@ \
    @LIBCURL_LIBS@ $(top_builddir)/lib/librhythmcat-2.0.la \
    $(top_builddir)/ui/librhythmcat-ui-2.0.la
lrccrawler_la_LIBTOOLFLAGS = --tag=disable-static

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
