AC_INIT([moonunit], [0.1], [bkoropoff __at__ gmail_com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

WANT_LIBELF=yes

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--enable-debug],
		[enable debugging])],
	[
		AC_DEFINE(DEBUG)
		CFLAGS="$CFLAGS -O0 -g"
	],
	[])

AC_ARG_WITH([libelf],
	[AS_HELP_STRING([--with-libelf=<dir>],
		[use libelf located in <dir> to scan libraries for unit tests, or 'no' to disable])],
	[
		if test x"$withval" = x"no"
		then
			WANT_LIBELF=no
			LIBELF_INCLUDES=
	 		LIBELF_LDFLAGS=
	 	elif test x"$withval" = x"yes"
	 	then
	 		WANT_LIBELF=yes
	 		LIBELF_INCLUDES=
	 		LIBELF_LDFLAGS=
	 	else
			LIBELF_INCLUDES="-I$withval/include"
	 		LIBELF_LDFLAGS="-L$withval/lib",
	 	fi
	],
	[
		LIBELF_INCLUDES=
	 	LIBELF_LDFLAGS=
	])

saved_LDFLAGS=$LDFLAGS
saved_CPPFLAGS=$CPPFLAGS

LDFLAGS="$LDFLAGS $LIBELF_LDFLAGS"
CPPFLAGS="$CPPFLAGS $LIBELF_INCLUDES"

if test x"$WANT_LIBELF" = x"yes"
then
	AC_CHECK_HEADER([libelf.h],[FOUND_LIBELF_H=yes],[FOUND_LIBELF_H=no])
	AC_CHECK_LIB(elf, elf_begin, [FOUND_LIBELF_LIB=yes], [FOUND_LIBELF_LIB=no])

	if test x"$FOUND_LIBELF_H" = x"yes" -a \
    	    x"$FOUND_LIBELF_LIB" = x"yes"
	then
		LIBELF_LIBADD="$LIBELF_LDFLAGS -lelf"
		AC_DEFINE(HAVE_LIBELF)
		FOUND_LIBELF=yes
		BACKEND=libelf
	else
		FOUND_LIBELF=no
		BACKEND=nm
	fi
else
	FOUND_LIBELF=no
	BACKEND=nm
fi

AM_CONDITIONAL([HAVE_LIBELF], [test x"$FOUND_LIBELF" = x"yes"])

AC_SUBST(LIBELF_INCLUDES)
AC_SUBST(LIBELF_LIBADD)
	
LDFLAGS=$saved_LDFLAGS
CPPFLAGS=$saved_CPPFLAGS
			 

AC_OUTPUT(Makefile src/Makefile src/libmoonunit/Makefile src/moonunit/Makefile 
		  include/Makefile include/moonunit/Makefile src/liburpc/Makefile
		  include/urpc/Makefile)

echo ""
echo ""
echo "Configuration summary"
echo
echo "Backend:               $BACKEND"
echo ""
