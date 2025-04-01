#!/usr/bin/env bash


SRC=( "./src" )
OUT='./makefile-targets.mk'

INCLUDES=`find "${SRC[@]}" -type d -exec echo ' -I "{}"' \;`


################################################################


function genRules(){
	find "${SRC[@]}" -type f -regex ".*\.\(c\|cpp\)" | {
		j=()
		
		while read -r f; do
			(
				set -o pipefail
				eval g++ -MM $INCLUDES "$f" | perl -pe 's|\s*\\\n||g'
			) &
			j+=( $! )
		done
		
		for pid in ${j[@]}; do
			wait $pid || {
				wait
				return 1
			}
		done
		
	}
	
	return $?
}


set -o pipefail
objs=`genRules | sort | sed -r 's|^|$(obj)/|'` || exit 1


echo '# Auto-generated file. Do not touch!'
echo '' 
echo "${objs}"
echo ''
echo 'OBJS = \'
grep -oP '^.+(?=:)' <<<"$objs" | sed -r 's|^|\t|' | sed -r 's|$| \\|'
echo ''
echo ''
echo -e '$(OBJS): | $(obj)'
echo -e '\t@basename "$@"'
echo -e '\t@[ -z "$(filter %.cpp, $^)" ] || $(CXX) $(filter %.cpp, $^) $(INCLUDES) $(LIB_INC) -c $(CXXFLAGS) -o "$@"'
echo -e '\t@[ -z "$(filter %.c, $^)"   ] || $(CC)  $(filter %.c, $^)   $(INCLUDES) $(LIB_INC) -c $(CFLAGS)   -o "$@"'
echo ''
echo '$(bin)/$(program): $(EXTERN_OBJ) $(OBJS) | $(bin)'
echo -e '\t@basename "$@"'
echo -e '\t@$(CXX) $(filter %.o, $^) $(CXXFLAGS) $(LIB_LNK) -o "$@"'
echo ''
