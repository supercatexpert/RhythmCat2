if WITH_GTK_UI

bin_PROGRAMS = RhythmCat2
noinst_LTLIBRARIES = librhythmcat-ui-2.0.la

noinst_HEADERS = \
    rc-ui-img-icon.xpm rc-ui-img-nocov.xpm rc-ui-unset-star-inline.h \
    rc-ui-no-star-inline.h rc-ui-set-star-inline.h

librhythmcat_ui_2_0_sources = \
    rc-ui-listmodel.c rc-ui-player.c rc-ui-window.c rc-ui-listview.c \
    rc-ui-menu.c rc-ui-dialog.c rc-ui-slabel.c rc-ui-spectrum.c \
    rc-ui-style.c rc-ui-plugin.c rc-ui-settings.c rc-ui-effect.c \
    rc-ui-cell-renderer-rating.c rc-ui-library-model.c rc-main.c

librhythmcat_ui_2_0_headers = \
    rc-ui-listmodel.h rc-ui-listview.h rc-ui-player.h rc-ui-window.h \
    rc-ui-menu.h rc-ui-dialog.h rc-ui-slabel.h rc-ui-style.h \
    rc-ui-spectrum.h rc-ui-plugin.h rc-ui-settings.h rc-ui-effect.h \
    rc-ui-cell-renderer-rating.h rc-ui-library-model.h rc-main.h

librhythmcat_ui_2_0_builtsources = rc-ui-resources.c rc-marshal.c

librhythmcat_ui_2_0_builtheaders = rc-ui-resources.h rc-marshal.h

RhythmCat2_SOURCES = \
    main.c

librhythmcat_ui_2_0_resourcefiles = \
    rc-ui-resources.gresource.xml \
    resources/themes/Monochrome/monochrome.css \
    resources/themes/Monochrome/scale-slider-horz-dark.png \
    resources/themes/Monochrome/scale-slider-horz-backdrop-dark.png \
    resources/themes/Monochrome/scale-slider-horz-insensitive-dark.png \
    resources/themes/Monochrome/scale-slider-horz-backdrop-insensitive-dark.png

rc-ui-resources.c: $(librhythmcat_ui_2_0_resourcefiles)
	@GLIB_COMPILE_RESOURCE@ $(top_srcdir)/ui/rc-ui-resources.gresource.xml \
	    --target=rc-ui-resources.c --generate-source --c-name=rc_ui_resources

rc-ui-resources.h: $(librhythmcat_ui_2_0_resourcefiles)
	@GLIB_COMPILE_RESOURCE@ $(top_srcdir)/ui/rc-ui-resources.gresource.xml \
	    --target=rc-ui-resources.h --generate-header --c-name=rc_ui_resources

rc-marshal.c: rc-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rc_marshal $(top_srcdir)/ui/rc-marshal.list \
        --body > rc-marshal.c

rc-marshal.h: rc-marshal.list
	@GLIB_GENMARSHAL@ --prefix=rc_marshal $(top_srcdir)/ui/rc-marshal.list \
        --header > rc-marshal.h

librhythmcat_ui_2_0_la_SOURCES = \
    $(librhythmcat_ui_2_0_sources) $(librhythmcat_ui_2_0_builtsources)
librhythmcat_ui_2_0includedir=$(includedir)/rhythmcat2
librhythmcat_ui_2_0include_HEADERS = rc-common.h \
    $(librhythmcat_ui_2_0_headers) $(librhythmcat_ui_2_0_builtheaders)

librhythmcat_ui_2_0_la_LDFLAGS = -fPIC -export-dynamic -O2
librhythmcat_ui_2_0_la_LIBADD = @LIBOBJS@ @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la

if HAVE_INTROSPECTION
    librhythmcat_ui_2_0_la_LIBADD += @GINTROSPECTION_LIBS@
endif

CLEANFILES = $(BUILT_SOURCES)

RhythmCat2_DEPENDENCIES = @LIBOBJS@ librhythmcat-ui-2.0.la
RhythmCat2_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*" -O2

AM_CFLAGS = @GLIB2_CFLAGS@ @GTK_CFLAGS@ @GSTREAMER_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -I$(top_srcdir)/lib -DLIBDIR=\"$(libdir)\" \
    -DPREFIXDIR=\"$(prefix)\"
    
if HAVE_INTROSPECTION
    AM_CFLAGS += -DENABLE_INTROSPECTION @GINTROSPECTION_CFLAGS@
endif

librhythmcat_ui_2_0_la_CFLAGS = $(AM_CFLAGS)
RhythmCat2_CFLAGS = $(AM_CFLAGS)

if DEBUG_MODE
    librhythmcat_ui_2_0_la_CFLAGS += -DDEBUG_MODE=1 -g
    RhythmCat2_CFLAGS += -DDEBUG_MODE=1 -g
endif

RhythmCat2_LDADD = @LIBOBJS@ @GLIB2_LIBS@ @GTK_LIBS@ @GSTREAMER_LIBS@ \
    $(top_builddir)/lib/librhythmcat-2.0.la librhythmcat-ui-2.0.la
    
if PLATFORM_WIN32
    RhythmCat2_LDADD += -lkernel32
    RhythmCat2_LDFLAGS += -mwindows
endif 

EXTRA_DIST = \
    rc-marshal.list \
    rhythmcat-2.0.pc.in \
    rc-ui-resources.gresource.xml
    
SUBDIRS = resources
    
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rhythmcat-2.0.pc

if HAVE_INTROSPECTION
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = 
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)/lib \
    --include-uninstalled=$(top_builddir)/lib/RhythmCatLib-2.0.gir --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(top_builddir)/lib

introspection_sources = $(librhythmcat_ui_2_0_sources) $(librhythmcat_ui_2_0_headers)

RhythmCat-2.0.gir: RhythmCat2
RhythmCat_2_0_gir_SCANNERFLAGS = --identifier-prefix=RC --symbol-prefix=rc
RhythmCat_2_0_gir_NAMESPACE = RhythmCat
RhythmCat_2_0_gir_VERSION = 2.0
RhythmCat_2_0_gir_INCLUDES = GObject-2.0 Gio-2.0 Gtk-3.0 Gst-0.10 GstPbutils-0.10 GstBase-0.10 GstFft-0.10
RhythmCat_2_0_gir_CFLAGS = $(AM_CFLAGS)
RhythmCat_2_0_gir_PROGRAM = $(builddir)/RhythmCat2
RhythmCat_2_0_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += RhythmCat-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

endif

endif

