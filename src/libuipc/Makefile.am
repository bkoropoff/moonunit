AUTOMAKE_OPTIONS= foreign

SUBDIRS=

noinst_LTLIBRARIES=liburpc.la

liburpc_la_SOURCES= \
	marshal.c shmem.c wire.c

liburpc_la_LIBADD = -lrt

AM_CPPFLAGS = -I@top_srcdir@/include/urpc
