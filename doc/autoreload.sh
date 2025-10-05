#!/usr/bin/env bash

while sleep 0.1 ; do
	find './doc' | entr -d -n -s 'make doc'
	if (( $? != 1 && $? != 2 )); then
		break
	fi
done
