
html: $(DO_HTML)
install-html: $(DO_INSTALL_HTML)

DO_HTML = doc-html-doxygen
DO_INSTALL_HTML = doc-install-html-doxygen

#doxygen
DOXY_SRC_CFG = ${abs_srcdir}/api/libabigail.doxy
DOXY_BLD_DIR = ${abs_builddir}/api
DOXY_BLD_CFG = ${abs_builddir}/api/html.doxy

$(DOXY_BLD_DIR):
	if test ! -d $(DOXY_BLD_DIR); then \
	  mkdir -p $(DOXY_BLD_DIR); \
	fi

doxygen-cfg: $(DOXY_SRC_CFG) $(DOXY_BLD_DIR)
	sed -e "s=LIBABIGAIL_INPUT=${abs_srcdir}/..=g" < $(DOXY_SRC_CFG) \
	> $(DOXY_BLD_CFG)

doc-html-doxygen: doxygen-cfg
	doxygen $(DOXY_BLD_CFG)

doc-install-html-doxygen: doc-html-doxygen
	test -z ${htmldir} || $(mkinstalldirs) ${htmldir}
	cp -r ${DOXY_BLD_DIR} "$(DESTDIR)@docdir@"

# By adding these files here, automake will remove them for 'make clean'
CLEANFILES = api/html.doxy 

# To remove directories.
clean-local:
	rm -rf api
