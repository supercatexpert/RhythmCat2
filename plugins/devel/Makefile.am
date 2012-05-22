plugindir = $(libdir)/RhythmCat2/plugins

karamixer_la_LDFLAGS = -module -avoid-version -export-dynamic -O2
python3_la_LDFLAGS = -module -avoid-version -export-dynamic -O2

plugin_LTLIBRARIES =
noinst_LTLIBRARIES =

INCLUDES = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @JSON_GLIB_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib

if WITH_GTK_UI
INCLUDES += @GTK_CFLAGS@ -I$(top_srcdir)/ui
endif

plugin_LTLIBRARIES += \
    karamixer.la

if WITH_GTK_UI

karamixer_la_SOURCES = karamixer.c
karamixer_la_LIBADD = @GLIB2_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ @GTK_LIBS@
karamixer_la_LIBTOOLFLAGS = --tag=disable-static

endif

if WITH_PYTHON3_PLUGINS

plugin_LTLIBRARIES += \
    python3.la

python3_la_SOURCES = python3.c
python3_la_LIBADD = @GLIB2_LIBS@ @GSTREAMER_LIBS@ @JSON_GLIB_LIBS@ @PYTHON3_LIBS@
INCLUDES += @PYTHON3_CFLAGS@
python3_la_LIBTOOLFLAGS = --tag=disable-static

endif

SUFFIXES = .c .so
.c.so:
	$(LIBTOOL) --mode=compile $(CC) -DHAVE_CONFIG_H -I$(top_builddir) $(AM_CPPFLAGS) $(CFLAGS) -c $< -o tmp$@.lo $(PLUGIN_CFLAGS)
	$(LIBTOOL) --mode=link    $(CC) $(CFLAGS) -o libtmp$@.la -rpath $(plugindir) tmp$@.lo $(LIBS) $(LDFLAGS) -module -avoid-version $(PLUGIN_LIBS)
	@rm -f tmp$@.lo tmp$@.o libtmp$@.la
	@cp .libs/libtmp$@*.so $@
	@rm -rf .libs/libtmp$@.*
