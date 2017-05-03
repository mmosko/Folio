.PHONY: all clean coverage remove_gcov_files gcov

REPORTDIR := html

######
# execute tests

clean: remove_gcov_files
	$(MAKE) -C $(TOPDIR) BUILDDIR=$(TOPDIR)/coverage lib_clean test_clean
	rm -rf $(REPORTDIR)

remove_gcov_files:
	rm -f $(TESTDIR)/*.gcno $(TESTDIR)/*.gcda $(TESTDIR)/*.gcov folio_test.info report.txt

zerocounter: 
	lcov --base-directory . --directory $(TESTDIR) --zerocounters -q

build: zerocounter
	${MAKE} -C $(TOPDIR) COVER_FLAG="--coverage" BUILDDIR=$(TOPDIR)/coverage BUILDABSDIR=$(TOPDIR)/coverage lib test check

gcov:
	(cd $(TESTDIR); \
		gcov -s $(SOURCEDIR) *.gcno; \
		gcovr -r . -g -o $(COVERDIR)/report.txt \
	)

coverage: build
	lcov --directory $(TESTDIR) -c -o folio_test.info
	lcov --remove folio_test.info "/usr*" -o folio_test.info
	genhtml -o $(REPORTDIR) -t "folio test coverage" --num-spaces 4 folio_test.info
	$(MAKE) gcov

