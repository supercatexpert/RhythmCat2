plugindir = $(libdir)/RhythmCat2

plugin_LTLIBRARIES = 

    
noinst_LTLIBRARIES =

INCLUDES = @GLIB2_CFLAGS@ @GSTREAMER_CFLAGS@ @GTK_CFLAGS@ @JSON_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib -I$(top_srcdir)/ui

SUFFIXES = .c .so
.c.so:
	$(LIBTOOL) --mode=compile $(CC) -DHAVE_CONFIG_H -I$(top_builddir) $(AM_CPPFLAGS) $(CFLAGS) -c $< -o tmp$@.lo $(PLUGIN_CFLAGS)
	$(LIBTOOL) --mode=link    $(CC) $(CFLAGS) -o libtmp$@.la -rpath $(plugindir) tmp$@.lo $(LIBS) $(LDFLAGS) -module -avoid-version $(PLUGIN_LIBS)
	@rm -f tmp$@.lo tmp$@.o libtmp$@.la
	@cp .libs/libtmp$@*.so $@
	@rm -rf .libs/libtmp$@.*
