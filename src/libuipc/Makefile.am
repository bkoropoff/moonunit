noinst_LTLIBRARIES=libuipc.la

libuipc_la_SOURCES= \
	marshal.c shmem.c wire.c

libuipc_la_LIBADD = @LIB_socket@ @LIB_rt@

AM_CPPFLAGS = -I@top_srcdir@/include/uipc
