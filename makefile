SHELL := /usr/bin/env bash
config = release

CC       = gcc
CXX      = g++
CFLAGS   = -O2 -Wno-error $(NO_WARN) -DNDEBUG
CXXFLAGS = -O2 -Wno-error $(NO_WARN) -DNDEBUG -std=c++2a

ifeq ($(config),debug)
	CFLAGS   = -O0 -Wno-error -Wall $(WARN) -Wswitch -g
	CXXFLAGS = -O0 -Wno-error -Wall $(WARN) -Wswitch -g -std=c++2a
endif


src = src
obj = obj
bin = bin
program = html-macro

INCLUDES ::= $(shell find "$(src)" -type d -exec echo ' -I "{}"' \; )
LIB_INC    = 
LIB_LNK    = 
EXTERN_OBJ = 


NO_WARN = -Wno-multichar -Wno-format-security
W_FMT = -Wformat=1 -Wformat-contains-nul -Wformat-diag -Wformat-extra-args \
		-Wformat-overflow=1 -Wformat-truncation=1 -Wformat-zero-length
WARN = $(W_FMT) $(NO_WARN)


################################################################


.PHONY: all
all: $(bin)/$(program)

# Run program as test
.PHONY: run
run: ./$(bin)/$(program)
	./$(bin)/$(program)

.PHONY: clean
clean:
	rm -rf '$(obj)' '$(bin)' './makefile-targets.mk'
	@echo "Project cleaned."


################################################################


$(bin)/test-$(program): test/test.cpp | $(bin)
	@basename "$@"
	@$(CXX) $(filter %.cpp, $^) $(CXXFLAGS) $(INCLUDES) -o "$@"

.PHONY: test
test: $(bin)/$(program) $(bin)/test-$(program)
	./$(bin)/test-$(program)


################################################################


$(bin) $(obj):
	mkdir -p "$@"


################################################################


# Build makefile for target dependencies on all source files
.PHONY: compiler
compiler:
	rm -f 'makefile-targets.mk'
	$(MAKE) makefile-targets.mk
	
makefile-targets.mk: $(shell find "$(src)" -type d)
	@echo "Generating compilation targets."
	./makefile-targets.sh 1>"$@" || rm "$@"
	@echo "Done."

ifeq (,$(MAKECMDGOALS))
include makefile-targets.mk
else ifneq ( , $(filter all run $(obj)/%.o , $(MAKECMDGOALS)))
include makefile-targets.mk
endif
