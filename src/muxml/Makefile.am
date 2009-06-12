data_DATA = \
	muxhtml.css \
	muxhtml.xsl \
	muxhtml.js

bin_SCRIPTS = moonunit-xml

EXTRA_DIST = $(data_DATA)
CLEANFILES = moonunit-xml

moonunit-xml: moonunit-xml.sh
	cp $< $@
	chmod +x $@
