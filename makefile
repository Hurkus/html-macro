SHELL := /usr/bin/env bash
config = debug

CC = g++

ifeq ($(config),debug)
	CFLAGS = -O0 -Wno-error -Wall $(WARN) $(NO_WARN) -Wswitch -g -std=c++2a
else
	CFLAGS = -O2 -Wno-error $(WARN) -DNDEBUG -std=c++2a
endif

NO_WARN = -Wno-multichar -Wno-format-security
WARN = -Wformat=1 -Wformat-contains-nul -Wformat-diag -Wformat-extra-args \
		-Wformat-overflow=1 -Wformat-truncation=1 -Wformat-zero-length


EXE = html-macro

INCLUDES  := $(shell find 'src' -type d -exec echo ' -I "{}"' \; )
SRC_FILES := $(shell find 'src' -type f -iname '*.cpp')
SRC_NAMES := $(notdir $(SRC_FILES))
OBJ_FILES := $(patsubst %.cpp,obj/%.o,$(SRC_NAMES))
DEP_FILES := $(patsubst %.cpp,obj/%.d,$(SRC_NAMES))



################################################################


.PHONY: all
all: bin/$(EXE)

# Run program as test
.PHONY: run
run: bin/$(EXE)
	./bin/$(EXE)

.PHONY: clean
clean:
	rm -rf './bin/' './obj/'
	@echo "Project cleaned."


################################################################


bin/test-$(EXE): $(wildcard test/*.cpp) $(wildcard test/*.hpp) | bin/
	@basename "$@"
	@$(CXX) $(filter %.cpp, $^) $(CXXFLAGS) $(INCLUDES) -g -o "$@"

.PHONY: test
test: bin/$(EXE) bin/test-$(EXE)
	./bin/test-$(EXE)


################################################################


.PHONY: doc
doc: doc/documentation.html

doc/documentation.html: doc/main.html $(shell find './doc') bin/$(EXE) | bin/
	./bin/$(EXE) '$<' -o '$@'


################################################################


# Folders
%/:
	mkdir -p "$@"


.SECONDEXPANSION:
PERCENT := %

# Dependency files
obj/%.d: $$(filter $$(PERCENT)/%.cpp,$$(SRC_FILES)) | obj/
	@basename '$@'
	@g++ $< $(INCLUDES) $(CFLAGS) -MM -MT 'obj/$*.o' -MF '$@'

# Object files
obj/%.o: $$(filter $$(PERCENT)/%.cpp,$$(SRC_FILES)) | obj/
	@basename '$@'
	@g++ $(filter %.cpp,$^) $(INCLUDES) $(LIB_INC) $(CFLAGS) -c -o "$@"

bin/$(EXE): $(OBJ_FILES) | bin/
	@echo '$@'
	@g++ $(filter %.o,$^) $(INCLUDES) $(CFLAGS) -o "$@"


ifneq ($(MAKECMDGOALS),clean)
include $(DEP_FILES)
endif


################################################################
