AUTOMAKE_OPTIONS= foreign

SUBDIRS = src include

if HAVE_DOXYGEN
SUBDIRS += doc
endif

run: $(top_builddir)/src/moonunit/moonunit-lt $(top_builddir)/src/moonunit/libexample.la
	$(top_builddir)/src/moonunit/moonunit-lt $(top_builddir)/src/moonunit/libexample.la

run-debug: $(top_builddir)/src/moonunit/moonunit-lt $(top_builddir)/src/moonunit/libexample.la
	$(top_builddir)/src/moonunit/moonunit-lt --gdb $(top_builddir)/src/moonunit/libexample.la

	
.PHONY: run