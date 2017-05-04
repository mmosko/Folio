.PHONY: all clean test check

all: test

#####
# unit tests

# COVER_FLAG is defined by the "make coverage" target to build
# with --coverage enabled.  You normally don't want that.

CFLAGS	= $(COVER_FLAG) -std=gnu11 -g -O0 -Wall -Wextra -I$(INCLUDEDIR) -I$(LONGBOW_DIR)/include
LDFLAGS	= -rdynamic -Wl,-rpath,$(BUILDABSDIR) -Wl,-rpath,$(LONGBOW_DIR)/lib
#LDFLAGS	= -rdynamic
LIBS	= -L$(BUILDABSDIR) -L$(LONGBOW_DIR)/lib -lpthread -lfolio -llongbow -llongbow-tap

SRC = $(wildcard *.c)
EXE = $(addprefix $(BUILDDIR)/,$(basename $(SRC)))

$(EXE) : $(BUILDDIR)/test_% : test_%.c $(SOURCEDIR)/%.c $(INCLUDEDIR)/Folio/%.h
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

test: $(info $$EXE is [$(EXE)]) $(EXE)

clean:
	rm -rf $(EXE)

######
# execute tests
# You need to use liblongbow-tap for this to work

check: $(info $$EXE = [$(EXE)]) $(EXE)
	prove --ignore-exit $^ 2> /dev/null

