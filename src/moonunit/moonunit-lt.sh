#!/bin/bash
#
# Copyright (c) 2007, Brian Koropoff
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the Moonunit project nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY BRIAN KOROPOFF ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL BRIAN KOROPOFF BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

dir=`dirname $0`
if [ -z "$dir" ]
then
    moonunit="moonunit"
else
    moonunit="$dir/moonunit"
fi

objdir=`libtool --config | grep ^objdir= | cut -d= -f2`

for arg in "$@"
do
    if echo $arg | grep "^--wrap=" >/dev/null 2>&1
    then
	wrap=`echo $arg | cut -d= -f2`	   
    elif echo $arg | grep '\.la$' >/dev/null 2>&1
    then
	dlopens=("${dlopens[@]}" -dlopen $arg)
	dylib=`dirname $arg`/$objdir/`basename $arg | sed 's/\.la/.so/'`
	stlib=`dirname $arg`/$objdir/`basename $arg | sed 's/\.la/.a/'`
	
	if [ -x "$dylib" ]
	then
	    :
	else
	    # Now we have to get creative
	    tempdir=`mktemp -d /tmp/moonunit-lt.XXXXXXXXXX`
	    destdir=`pwd`
	    cd ${tempdir}
	    ar x ${destdir}/${stlib}
	    libtool --mode=link cc -shared -o ${destdir}/${dylib} `ar t ${destdir}/${stlib}`
	    cd $destdir
	    rm -rf ${tempdir}
	fi
	command=("${command[@]}" `dirname $arg`/$objdir/`basename $arg | sed 's/\.la/.so/'`)
    else
	command=("${command[@]}" $arg)
    fi
done

libtool --mode=execute "${dlopens[@]}" $wrap $moonunit "${command[@]}"
