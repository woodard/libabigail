SUBDIRS = include src tools tests doc bash-completion
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
headers = config.h

m4datadir = $(datadir)/aclocal
m4data_DATA = $(srcdir)/abigail.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libabigail.pc

#bashcompletiondir = $(datadir)/bash-completion/completions
#dist_bashcompletion_DATA =

EXTRA_DIST = 			\
README COPYING ChangeLog	\
COPYING-LGPLV2 COPYING-LGPLV3	\
COPYING-GPLV3 gen-changelog.py	\
$(headers) $(m4data_DATA) 	\
libabigail.pc.in

update-changelog:
	python $(srcdir)/gen-changelog.py > $(srcdir)/ChangeLog

RELEASED_BRANCH = master
tag-release-only:
	git tag -m "$(PACKAGE_NAME) release $(VERSION)" \
	"$(PACKAGE_NAME)-$(VERSION)" $(RELEASED_BRANCH)

tag-release: tag-release-only


