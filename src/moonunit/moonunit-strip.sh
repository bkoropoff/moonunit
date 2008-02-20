#!/bin/bash

arg="$1"

usage()
{
    local name=$(basename $1)
    cat << __EOF__
$name -- remove MoonUnit tests from a library or object file

  This script strips a library or object file of data and code
  sections containing MoonUnit tests, as well as all strippable
  related symbols.

Usage: $name [options] <file1>...
    -?, -h, --help        Display this usage information
__EOF__
}

while [ -n "$arg" ]
do
    shift
    case "$arg" in
        -?|-h|--help)
            usage "$0"
            exit 0
            ;;
        *)
            files=("${files[@]}" "$arg")
            ;;
    esac
    arg="$1"
done

if [ -n "$files" ]
then
    for file in "${files[@]}"
    do
        objcopy -w \
            -R ".moonunit_text" -R ".moonunit_data" \
            -N '__mu_t_*'       -N '__mu_f_*' \
            -N '__mu_fs_*'      -N '__mu_ft_*' \
            -N '__mu_ls'        -N '__mu_lt' \
            $file
    done
else
    usage "$0"
    exit 1
fi
