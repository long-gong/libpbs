# Makefile for libpbs

CXX = g++
ifneq (,$(which gio))
RM = gio -r
else
RM = rm -rf
endif
MKDIRS = mkdir -p

CXXFLAGS = -Wall -O3
INC = -I$(EXTERNAL) -I$(SRCDIR)
LIB = -lgtest -lgtest_main -lpthread
LDFLAGS =

TOP := $(shell pwd)
SRCDIR = $(TOP)/src
EXTERNAL = $(TOP)/3rd/include
TESTDIR = $(TOP)/test
SCRIPTDIR = $(TOP)/scripts
BUILD = $(TOP)/build

$(BUILD)/test_pbs_params: $(TESTDIR)/test_pbs_params.cpp
	@$(MKDIRS) $(BUILD)
	$(CXX) $< $(CXXFLAGS) $(INC) -o $@ $(LDFLAGS) $(LIB)

test: $(BUILD)/test_pbs_params
	(cd $(BUILD) && ./test_pbs_params)
	(cd $(BUILD) && valgrind --leak-check=yes ./test_pbs_params)

dependencies: $(SCRIPTDIR)/install_dependencies.sh
	@chmod +x ./ $^
	$^

clean:
	$(RM) $(BUILD)


.PHONY: test clean dependencies



