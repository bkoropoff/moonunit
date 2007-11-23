AUTOMAKE_OPTIONS = foreign

html-local: Doxyfile
	( cat $<; echo GENERATE_HTML=yes ) | $(DOXYGEN) -

install-html-local:
	mkdir -p $(DESTDIR)$(htmldir)
	cp -r html $(DESTDIR)$(htmldir)

uninstall-local:
	rm -rf $(DESTDIR)$(htmldir)/html

clean-local:
	rm -rf html

all-local: html

install-data-local: install-html