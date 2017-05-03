.PHONY: all lib builddir longbow clean test check coverage

SHELL = /bin/sh
CC    = gcc

all: lib examples test

###########
# Directory names

TOPDIR := `pwd`
export BUILDDIR := $(TOPDIR)/build
export INCLUDEDIR := $(TOPDIR)/include
export COVERDIR := $(TOPDIR)/coverage

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
	$(MAKE) -p src all

lib_clean:
	$(MAKE) -p src clean

builddir:
	mkdir -p $(BUILDDIR)

#####
# Examples

examples: lib
	$(MAKE) -p examples all

examples_clean:
	$(MAKE) -p examples clean

#####
# unit tests

test: lib
	$(MAKE) -p test all

test_clean:
	$(MAKE) -p test clean


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
	gcovr -d -r . -e '^test\/' -p -o $(COVERDIR)/report.txt
	cat $(COVERDIR)/report.txt
	${MAKE} coverage_clean test_clean

