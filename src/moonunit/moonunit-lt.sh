#!/bin/bash

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
	if [ $arg == "--gdb" ]
	then
		gdb="gdb --args"
	elif echo $arg | grep '\.la$' 2>&1 >/dev/null
	then
		dlopens=("${dlopens[@]}" -dlopen $arg)
		command=("${command[@]}" `dirname $arg`/$objdir/`basename $arg | sed 's/\.la/.so/'`)
	else
		command=("${command[@]}" $arg)
	fi
done

libtool --mode=execute "${dlopens[@]}" $gdb $moonunit "${command[@]}"
