
html: $(STAMP_HTML)
install-html: $(STAMP_INSTALL_HTML)

STAMP_HTML = doc-html-doxygen
STAMP_INSTALL_HTML = doc-install-html-doxygen

#doxygen
DOXY_SRC_CFG = $(abs_srcdir)/api/libabigail.doxy
DOXY_BLD_CFG = api/html.doxy

stamp-doxygen-cfg: $(DOXY_SRC_CFG)
	sed -e "s=LIBABIGAIL_INPUT=${abs_srcdir}/..=g" < $(DOXY_SRC_CFG) > $(DOXY_BLD_CFG)

doc-html-doxygen: stamp-doxygen-cfg
	doxygen $(DOXY_BLD_CFG)

doc-install-html-doxgen: 

# By adding these files here, automake will remove them for 'make clean'
CLEANFILES = api/html.doxy

# To remove directories.
clean-local:
	rm -rf api/html
