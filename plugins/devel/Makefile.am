plugindir = $(libdir)/RhythmCat2

helloworld_la_LDFLAGS = -module -avoid-version -export-dynamic -O2

plugin_LTLIBRARIES = \
    helloworld.la
    
noinst_LTLIBRARIES =

helloworld_la_SOURCES = helloworld.c

helloworld_la_LIBADD = @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ @JSON_LIBS@

INCLUDES = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @GTK_CFLAGS@ @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib -I$(top_srcdir)/ui

SUFFIXES = .c .so
.c.so:
	$(LIBTOOL) --mode=compile $(CC) -DHAVE_CONFIG_H -I$(top_builddir) $(AM_CPPFLAGS) $(CFLAGS) -c $< -o tmp$@.lo $(PLUGIN_CFLAGS)
	$(LIBTOOL) --mode=link    $(CC) $(CFLAGS) -o libtmp$@.la -rpath $(plugindir) tmp$@.lo $(LIBS) $(LDFLAGS) -module -avoid-version $(PLUGIN_LIBS)
	@rm -f tmp$@.lo tmp$@.o libtmp$@.la
	@cp .libs/libtmp$@*.so $@
	@rm -rf .libs/libtmp$@.*
