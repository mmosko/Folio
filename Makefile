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
export TESTDIR := ../test
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
	$(MAKE) -C test check

#####
# coverage testing
# coverage will make a separate build into coverage/ with the --coverage flag defined
# It will not affect the build/ directory

coverage_clean:
	$(MAKE) -C coverage clean

coverage:
	$(MAKE) -C coverage coverage

