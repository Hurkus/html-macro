#!/bin/bash


while sleep 0.1 ; do
	find './doc' | entr -d -n -s 'echo Reload... ; make doc'
	
	if (( $? != 1 && $? != 2)); then
		break
	fi
	
done
