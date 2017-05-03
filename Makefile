.PHONY: all lib builddir longbow clean test check coverage

SHELL = /bin/sh
CC    = gcc

###########
# Directory names

export TOPDIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
export BUILDABSDIR := $(TOPDIR)/build
export BUILDDIR := ../build
export SOURCEDIR := ../src
export INCLUDEDIR := ../include
export COVERDIR := $(TOPDIR)/coverage

# The root directory holding lib/liblongBow* and include/LongBow
export LONGBOW_DIR := /usr/local

all: lib examples test

###########
# Make sure we can find longbow

longbow:
	# todo

###########
# cleanup

clean: lib_clean examples_clean test_clean coverage_clean

###########
# Library setup

lib : $(info $$TOPDIR is [${TOPDIR}]) builddir longbow
	$(MAKE) -C src all

lib_clean:
	$(MAKE) -C src clean

builddir:
	mkdir -p $(BUILDDIR)

#####
# Examples

examples: lib
	$(MAKE) -C examples all

examples_clean:
	$(MAKE) -C examples clean

#####
# unit tests

test: lib
	$(MAKE) -C test all

test_clean:
	$(MAKE) -C test clean


check: test
	prove --ignore-exit build/test_* 2> /dev/null

# Remove the verbose coverage files, but leaves $(COVERDIR).  That is only cleaned by "clean" target.
coverage_clean:
	rm -f *.gcno *.gcda folio_test.info

coverage:
	lcov --base-directory . --directory . --zerocounters -q
	${MAKE} COVER_FLAG="--coverage" clean check
	lcov --base-directory . --directory . -c -o folio_test.info
	lcov --remove folio_test.info "/usr*" -o folio_test.info
	rm -rf $(COVERDIR)
	genhtml -o $(COVERDIR) -t "folio test coverage" --num-spaces 4 folio_test.info
	gcovr -d -r . -e '^test\/' -C -o $(COVERDIR)/report.txt
	cat $(COVERDIR)/report.txt
	${MAKE} coverage_clean test_clean

