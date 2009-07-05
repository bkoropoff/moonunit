bin_PROGRAMS = moonunit
bin_SCRIPTS = moonunit-lt moonunit-stub

moonunit_SOURCES = main.c option.c run.c gdb.c multilog.c upopt.c
moonunit_LDADD = $(top_builddir)/src/libmoonunit/libmoonunit.la @LIB_pthread@
if CPLUSPLUS_ENABLED
moonunit_SOURCES += dummy.cpp
moonunit_LINK = $(CXXLINK) $(moonunit_LDFLAGS)
else
moonunit_LINK = $(LINK) $(moonunit_LDFLAGS)
endif

EXTRA_DIST = moonunit-lt.sh.in moonunit-strip.sh.in moonunit-stub.sh.in option.h run.h gdb.h multilog.h upopt.h

CLEANFILES = moonunit-lt moonunit-stub

moonunit-lt: moonunit-lt.sh
	cp $< $@
	chmod +x $@

moonunit-strip: moonunit-strip.sh
	cp $< $@
	chmod +x $@

moonunit-stub: moonunit-stub.sh
	cp $< $@
	chmod +x $@

AM_CPPFLAGS = -I$(top_srcdir)/include $(MU_CPPFLAGS)
AM_CFLAGS = $(MU_CFLAGS)
AM_LDFLAGS = $(MU_LDFLAGS)