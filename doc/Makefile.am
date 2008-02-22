AUTOMAKE_OPTIONS = foreign

man_MANS = moonunit.1 moonunit-lt.1 moonunit-stub.1 moonunit-strip.1

html/index.html: Doxyfile @top_srcdir@/include/moonunit/test.h
	( cat $<; echo GENERATE_HTML=yes ) | $(DOXYGEN) -

doc-doxygen: html/index.html

index.html moonunit-lt.1.html moonunit-stub.1.html moonunit-strip.1.html: index.xml moonunit.xml moonunit-lt.xml moonunit-stub.xml moonunit-strip.xml docbook-html.xsl
	xsltproc --param citerefentry.link 1 --param use.id.as.filename 1 --xinclude @srcdir@/docbook-html.xsl @srcdir@/index.xml

doc-docbook-html: index.html

html-local: doc-doxygen doc-docbook-html

moonunit.1 moonunit-lt.1 moonunit-stub.1 moonunit-strip.1: index.xml moonunit.xml moonunit-lt.xml moonunit-stub.xml moonunit-strip.xml docbook-man.xsl
	xsltproc --xinclude @srcdir@/docbook-man.xsl @srcdir@/index.xml

install-html-local:
	mkdir -p $(DESTDIR)$(htmldir)
	cp -r html $(DESTDIR)$(htmldir)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldir)/html

clean-local:
	rm -rf html *.html *.1

all-local: html

install-data-local: install-html

.PHONY: doc-doxygen doc-docbook-html