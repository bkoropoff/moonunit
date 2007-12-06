AUTOMAKE_OPTIONS= foreign

SUBDIRS = src include

if HAVE_DOXYGEN
SUBDIRS += doc
endif

CHECK_PLUGINS="--plugin=$(top_builddir)/src/moonunit-unix/unix.la"

check-local: $(top_builddir)/src/moonunit/moonunit-lt $(top_builddir)/src/moonunit/libexample.la
	$(top_builddir)/src/moonunit/moonunit-lt \
	$(CHECK_ARGS) $(CHECK_PLUGINS) \
	$(top_builddir)/src/moonunit/libexample.la

.PHONY: run