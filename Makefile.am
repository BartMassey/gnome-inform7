SUBDIRS = src po data pixmaps

ACLOCAL_AMFLAGS = -I m4

desktopdir = $(datadir)/applications
dist_desktop_DATA = gnome-inform7.desktop

gsettings_SCHEMAS = com.inform7.GUI.gschema.xml

@GSETTINGS_RULES@

docdir = $(datadir)/doc/$(PACKAGE)
dist_doc_DATA = AUTHORS ChangeLog COPYING NEWS README THANKS TODO

EXTRA_DIST = \
	INSTALL \
	autogen.sh \
	intltool-update.in \
	intltool-extract.in \
	intltool-merge.in \
	$(gsettings_SCHEMAS)

if BUILDING_RPM
EXTRA_DIST += gnome-inform7.spec
endif

MAINTAINERCLEANFILES = \
	ABOUT-NLS INSTALL config.rpath m4 \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	$(NULL)

if BUILDING_RPM
RPMDIR=`$(RPM) --eval %_rpmdir`
SRCRPMDIR=`$(RPM) --eval %_srcrpmdir`
RPMARCH=`$(RPM) --eval %_target_cpu`
RPM_RELEASE=`$(EGREP) ^Release: <gnome-inform7.spec | cut -f2 -d " "`
BINRPM = $(PACKAGE)-$(VERSION)-$(RPM_RELEASE).$(RPMARCH).rpm
DEBUGRPM = $(PACKAGE)-debuginfo-$(VERSION)-$(RPM_RELEASE).$(RPMARCH).rpm
SRCRPM = $(PACKAGE)-$(VERSION)-$(RPM_RELEASE).src.rpm
rpm: dist
	$(ECHO) "Building RPM package..."; \
	if $(RPMBUILD) -ta --clean $(PACKAGE)-$(VERSION).tar.gz; then \
		if $(TEST) -e $(RPMDIR)/$(BINRPM); then \
			$(CP) $(RPMDIR)/$(BINRPM) $(builddir); \
		elif $(TEST) -e $(RPMDIR)/$(RPMARCH)/$(BINRPM); then \
			$(CP) $(RPMDIR)/$(RPMARCH)/$(BINRPM) $(builddir); \
		fi; \
		$(TEST) -e $(SRCRPMDIR)/$(SRCRPM) && \
		$(CP) $(SRCRPMDIR)/$(SRCRPM) $(builddir); \
		if $(TEST) -e $(RPMDIR)/$(DEBUGRPM); then \
			$(CP) $(RPMDIR)/$(DEBUGRPM) $(builddir); \
		elif $(TEST) -e $(RPMDIR)/$(RPMARCH)/$(DEBUGRPM); then \
			$(CP) $(RPMDIR)/$(RPMARCH)/$(DEBUGRPM) $(builddir); \
		else \
			$(ECHO) "No debug package; install redhat-rpm-config if you want one."; \
		fi; \
	fi
else
rpm:
	@$(ECHO) \
	"To build an RPM package, you must run configure with --enable-rpm."; \
	exit 1
endif

if BUILDING_DEB
DEBSOURCEPKG=$(PACKAGE)_$(VERSION).orig.tar.gz
DEBSOURCEDIR=$(PACKAGE)-$(VERSION)
deb: dist
	@$(ECHO) "Building Debian package..."; \
	if $(TEST) ! -d debian; then \
		$(ECHO) "The files needed for building a Debian package are not" \
		"included by default in the distribution. To build a package, check" \
		"out the project from source control."; \
		exit 1; \
	fi; \
	$(MV) $(PACKAGE)-$(VERSION).tar.gz $(DEBSOURCEPKG) && \
	$(TAR) --extract --gunzip --file $(DEBSOURCEPKG) && \
	$(MKDIR_P) $(DEBSOURCEDIR)/debian && \
	$(CP) \
		debian/changelog \
		debian/compat \
		debian/control \
		debian/copyright \
		debian/docs \
		debian/menu \
		debian/rules \
		$(DEBSOURCEDIR)/debian && \
	cd $(DEBSOURCEDIR) && \
	$(DEBUILD) -rfakeroot -D -us -uc; \
	cd $(srcdir); \
	rm --force $(DEBSOURCEPKG); \
	rm --recursive --force $(DEBSOURCEDIR)
else
deb:
	@$(ECHO) \
	"To build a Debian package, you must run configure with --enable-deb."; \
	exit 1
endif

-include $(top_srcdir)/git.mk
