AUTOMAKE_OPTIONS= foreign

SUBDIRS = src include doc

CHECK_PLUGINS= \
	--plugin $(top_builddir)/src/moonunit-unix/unix.la \
	--plugin $(top_builddir)/src/moonunit-misc/console.la \
	--plugin $(top_builddir)/src/moonunit-misc/xml.la	

check-local: $(top_builddir)/src/moonunit/moonunit-lt $(top_builddir)/src/moonunit/libexample.la
	$(top_builddir)/src/moonunit/moonunit-lt \
	$(CHECK_ARGS) $(CHECK_PLUGINS) \
	$(top_builddir)/src/moonunit/libexample_mu.la

.PHONY: run
