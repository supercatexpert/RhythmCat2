SUBDIRS = images icons

EXTRA_DIST = $(man_MANS)

RhythmCat_uninstalldir = ${datadir}/RhythmCat2

man_MANS = RhythmCat2.1

# Rule to update the icon cache after installing the icon
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/RhythmCat2/images

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

menudir = ${datadir}/applications
menu_DATA = RhythmCat2.desktop

