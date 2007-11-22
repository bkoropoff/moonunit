#!/bin/bash

for file in "$@"
do
	objcopy -R ".moonunit_text" -R ".moonunit_data" -w -N '__mu_t_*' -N '__mu_f_*' $file
done