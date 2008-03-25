HTML_TARGETS =
HTML_INSTALL_TARGETS =
DOCBOOK_XML = index.xml moonunit.xml moonunit-lt.xml moonunit-stub.xml moonunit-strip.xml
DOCBOOK_XML_INDEXLESS = $(DOCBOOK_XML:index.xml=)
DOCBOOK_HTML = $(DOCBOOK_XML:.xml=.html)
DOCBOOK_MAN = $(DOCBOOK_XML_INDEXLESS:.xml=.1)

EXTRA_DIST = $(DOCBOOK_XML)

if HAVE_DOXYGEN
HTML_TARGETS += doc-doxygen
HTML_INSTALL_TARGETS += install-doc-doxygen

html/index.html: Doxyfile @top_srcdir@/include/moonunit/test.h
	( cat $<; echo GENERATE_HTML=yes ) | $(DOXYGEN) -

doc-doxygen: html/index.html

install-doc-doxygen:
	mkdir -p $(DESTDIR)$(htmldir)/api
	cp html/* $(DESTDIR)$(htmldir)/api
endif

if HAVE_DOCBOOK
man_MANS = $(DOCBOOK_MAN)
HTML_TARGETS += doc-docbook-html
HTML_INSTALL_TARGETS += install-doc-docbook-html

$(DOCBOOK_HTML): $(DOCBOOK_XML) docbook-html.xsl
	$(XSLTPROC) --param citerefentry.link 1 --param use.id.as.filename 1 --xinclude docbook-html.xsl @srcdir@/index.xml

doc-docbook-html: $(DOCBOOK_HTML)

install-doc-docbook-html:
	mkdir -p $(DESTDIR)$(htmldir)/tools
	cp $(DOCBOOK_HTML) $(DESTDIR)$(htmldir)/tools
endif

html-local: $(HTML_TARGETS)

$(DOCBOOK_MAN): $(DOCKBOOK_XML) docbook-man.xsl
	$(XSLTPROC) --xinclude docbook-man.xsl @srcdir@/index.xml

install-html-local: $(HTML_INSTALL_TARGETS)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldir)

clean-local:
	rm -rf html *.html *.1

all-local: html

install-data-local: install-html-local

.PHONY: $(HTML_TARGETS)