AUTOMAKE_OPTIONS= foreign

SUBDIRS = src include

if HAVE_DOXYGEN
SUBDIRS += doc
endif

check-local: $(top_builddir)/src/moonunit/moonunit-lt $(top_builddir)/src/moonunit/libexample.la
	$(top_builddir)/src/moonunit/moonunit-lt $(CHECK_ARGS) $(top_builddir)/src/moonunit/libexample.la

	
.PHONY: run