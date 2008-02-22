HTML_TARGETS =
HTML_INSTALL_TARGETS =

if HAVE_DOXYGEN
HTML_TARGETS += doc-doxygen
HTML_INSTALL_TARGETS += install-doc-doxygen

html/index.html: Doxyfile @top_srcdir@/include/moonunit/test.h
	( cat $<; echo GENERATE_HTML=yes ) | $(DOXYGEN) -

doc-doxygen: html/index.html
endif

if HAVE_DOCBOOK
man_MANS = moonunit.1 moonunit-lt.1 moonunit-stub.1 moonunit-strip.1
HTML_TARGETS += doc-docbook-html

index.html moonunit-lt.1.html moonunit-stub.1.html moonunit-strip.1.html: index.xml moonunit.xml moonunit-lt.xml moonunit-stub.xml moonunit-strip.xml docbook-html.xsl
	$(XSLTPROC) --param citerefentry.link 1 --param use.id.as.filename 1 --xinclude docbook-html.xsl @srcdir@/index.xml

doc-docbook-html: index.html
endif

html-local: $(HTML_TARGETS)

moonunit.1 moonunit-lt.1 moonunit-stub.1 moonunit-strip.1: index.xml moonunit.xml moonunit-lt.xml moonunit-stub.xml moonunit-strip.xml docbook-man.xsl
	$(XSLTPROC) --xinclude docbook-man.xsl @srcdir@/index.xml

install-doc-doxygen:
	mkdir -p $(DESTDIR)$(htmldir)
	cp -r html $(DESTDIR)$(htmldir)

install-html-local: $(HTML_INSTALL_TARGETS)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldir)

clean-local:
	rm -rf html *.html *.1

all-local: html

install-data-local: install-html

.PHONY: $(HTML_TARGETS)