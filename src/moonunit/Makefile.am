bin_PROGRAMS = moonunit
bin_SCRIPTS = moonunit-lt moonunit-strip moonunit-stub

check_LTLIBRARIES = example.la

moonunit_SOURCES = main.c option.c run.c gdb.c multilog.c upopt.c
moonunit_LDADD = $(top_builddir)/src/libmoonunit/libmoonunit.la @LIB_pthread@

LINKSTYLE = -DMU_LINK_STYLE=MU_LINK_WEAK

EXAMPLE_SOURCES = $(srcdir)/example.c

if CPLUSPLUS_ENABLED
EXAMPLE_SOURCES += $(srcdir)/example_cpp.cpp
endif

example_la_SOURCES = $(EXAMPLE_SOURCES) example_stub.c
example_la_CPPFLAGS = $(AM_CPPFLAGS) $(LINKSTYLE)
example_la_LDFLAGS = -module -avoid-version -export-dynamic -rpath $(libdir)

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
		CPPFLAGS="$(CPPFLAGS) $(AM_CPPFLAGS) $(LINKSTYLE)" \
		CPP="$(CPP)"

AM_CPPFLAGS = -I$(top_srcdir)/include

CHECK_PLUGINS= \
	--plugin $(top_builddir)/src/plugins/unix.la \
	--plugin $(top_builddir)/src/plugins/console.la \
	--plugin $(top_builddir)/src/plugins/xml.la	

check-local: moonunit-lt libexample_mu.la
	./moonunit-lt $(CHECK_ARGS) $(CHECK_PLUGINS) example.la

