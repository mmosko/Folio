.PHONY: all clean test check

all: test

#####
# unit tests

# COVER_FLAG is defined by the "make coverage" target to build
# with --coverage enabled.  You normally don't want that.

CFLAGS	= $(COVER_FLAG) -std=gnu11 -g -O0 -Wall -Wextra -I$(INCLUDEDIR) -I$(LONGBOW_DIR)/include
LDFLAGS	= -rdynamic -Wl,-rpath,$(BUILDABSDIR) -Wl,-rpath,$(LONGBOW_DIR)/lib
LIBS	= -L$(BUILDABSDIR) -L$(LONGBOW_DIR)/lib -lpthread -lfolio -llongbow -llongbow-tap


PRIV_SRC = $(wildcard private/*.c)
PRIV_EXE = $(addprefix $(BUILDDIR)/,$(basename $(PRIV_SRC)))

$(PRIV_EXE) : $(BUILDDIR)/private/test_% : private/test_%.c $(SOURCEDIR)/private/%.c $(HDR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

SRC = $(wildcard *.c)
HDR = $(wildcard $(INCLUDEDIR)/Folio/*.h) $(wildcard $(INCLUDEDIR)/Folio/private/*.h)
EXE = $(addprefix $(BUILDDIR)/,$(basename $(SRC)))

$(EXE) : $(BUILDDIR)/test_% : test_%.c $(SOURCEDIR)/%.c $(HDR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

test: $(EXE) $(PRIV_EXE)

clean:
	rm -rf $(EXE)

######
# execute tests
# You need to use liblongbow-tap for this to work

check: $(PRIV_EXE) $(EXE)
	prove --ignore-exit $^ 2> /dev/null

