#!/bin/bash

for file in "$@"
do
	objcopy -w \
		-R ".moonunit_text" -R ".moonunit_data" \
		-N '__mu_t_*'       -N '__mu_f_*' \
		-N '__mu_fs_*'      -N '__mu_ft_*' \
		-N '__mu_ls'        -N '__mu_lt' \
		$file
done