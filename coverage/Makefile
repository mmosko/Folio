.PHONY: all clean coverage remove_gcov_files gcov

REPORTDIR := html

######
# execute tests

clean: remove_gcov_files
	$(MAKE) -C $(TOPDIR) BUILDDIR=$(TOPDIR)/coverage lib_clean test_clean
	rm -rf $(REPORTDIR) folio_test.info report.txt

remove_gcov_files:
	rm -f $(TESTDIR)/*.gcno $(TESTDIR)/*.gcda $(TESTDIR)/*.gcov

zerocounter: 
	lcov --base-directory . --directory $(TESTDIR) --zerocounters -q

build: zerocounter
	${MAKE} -C $(TOPDIR) COVER_FLAG="--coverage" BUILDDIR=$(TOPDIR)/coverage BUILDABSDIR=$(TOPDIR)/coverage lib test check

gcov:
	(cd $(TESTDIR); \
		gcov -s $(SOURCEDIR) *.gcno; \
		gcovr -r . -g -e '^test_' -o $(COVERDIR)/report.txt \
	)
	cat report.txt

coverage: build
	lcov --directory $(TESTDIR) -c -o folio_test.info
	lcov --remove folio_test.info "/usr*" -o folio_test.info
	genhtml -o $(REPORTDIR) -t "folio test coverage" --num-spaces 4 folio_test.info
	$(MAKE) gcov
	$(MAKE) remove_gcov_files

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
