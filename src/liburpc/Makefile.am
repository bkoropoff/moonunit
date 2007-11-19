AUTOMAKE_OPTIONS= foreign

SUBDIRS=

lib_LTLIBRARIES=liburpc.la

liburpc_la_SOURCES= \
	marshal.c shmem.c wire.c

liburpc_la_LIBADD = -lrt

INCLUDES = -I@top_srcdir@/include/urpc
