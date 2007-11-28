AUTOMAKE_OPTIONS= foreign

SUBDIRS = src include

if HAVE_DOXYGEN
SUBDIRS += doc
endif

run: $(top_builddir)/src/moonunit/moonunit-lt $(top_builddir)/src/moonunit/libexample.la
	$(top_builddir)/src/moonunit/moonunit-lt $(RUN_ARGS) $(top_builddir)/src/moonunit/libexample.la

	
.PHONY: run