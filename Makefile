.PHONY: all lib builddir longbow clean test check coverage install remove

# Conditionally include local settings if the file exists
-include settings.local

# You can override this on the command-line: `make PREFIX=/tmp/folio intall`
# or by putting the setting in the file 'settings.local' which is not part
# of the Folio distribution (i.e. it won't be overwritten).
PREFIX ?= /usr/local
export PREFIX

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
LONGBOW_DIR ?= /usr/local
export LONGBOW_DIR

all: builddir lib examples test

###########
# Make sure we can find longbow

longbow:
	# todo

###########
# cleanup

clean: lib_clean examples_clean test_clean coverage_clean

###########
# Library setup

lib : builddir longbow
	$(MAKE) -C src all

lib_clean:
	rm -rf build
	$(MAKE) -C src clean

builddir:
	mkdir -p $(notdir $(BUILDDIR))
	mkdir -p $(notdir $(BUILDDIR))/private

install remove:
	$(MAKE) -C src $@

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


#####
# Dependencies

depend:
	$(MAKE) -C src depend

#####
#
#   Copyright (c) 2017, Palo Alto Research Center
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice, this
#     list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

