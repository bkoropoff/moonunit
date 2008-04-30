bin_PROGRAMS = moonunit
bin_SCRIPTS = moonunit-lt moonunit-strip moonunit-stub

check_LTLIBRARIES = example.la

moonunit_SOURCES = main.c option.c run.c gdb.c multilog.c upopt.c
moonunit_LDADD = $(top_builddir)/src/libmoonunit/libmoonunit.la @LIB_pthread@

EXAMPLE_SOURCES = $(srcdir)/example.c

example_la_SOURCES = $(EXAMPLE_SOURCES) example_stub.c
example_la_CPPFLAGS = $(AM_CPPFLAGS)
example_la_LDFLAGS = -module -avoid-version -export-dynamic -rpath $(libdir)
if CPLUSPLUS_ENABLED
EXAMPLE_SOURCES += $(srcdir)/example_cpp.cpp
example_la_LINK = $(CXXLINK) $(example_la_LDFLAGS)
else
example_la_LINK = $(LINK) $(example_la_LDFLAGS)
endif

EXTRA_DIST = moonunit-lt.sh moonunit-strip.sh moonunit-stub.sh option.h run.h gdb.h multilog.h
BUILT_SOURCES = example_stub.c
CLEANFILES = example_stub.c

moonunit-lt: moonunit-lt.sh moonunit
	cp $< $@
	chmod +x $@

moonunit-strip: moonunit-strip.sh
	cp $< $@
	chmod +x $@

moonunit-stub: moonunit-stub.sh
	cp $< $@
	chmod +x $@

example_stub.c: $(EXAMPLE_SOURCES) moonunit-stub
	$(top_builddir)/src/moonunit/moonunit-stub $(EXAMPLE_SOURCES) -o $@ \
		CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" \
		CXXCPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS)" \
		CPP="$(CPP)" CXXCPP="$(CXXCPP)"

AM_CPPFLAGS = -I$(top_srcdir)/include

CHECK_PLUGINS= \
	--plugin $(top_builddir)/src/plugins/unix.la \
	--plugin $(top_builddir)/src/plugins/console.la \
	--plugin $(top_builddir)/src/plugins/xml.la	

check-local: moonunit-lt example.la
	./moonunit-lt $(CHECK_ARGS) $(CHECK_PLUGINS) example.la

