.PHONY: all lib clean

SHELL = /bin/sh


CC    = gcc

all: lib

###########
# Library setup

MAJOR := 0
MINOR := 1
NAME := folio

CFLAGS       = -std=gnu11 -g -O0 -fPIC -Wall -Wextra -I$(INCLUDEDIR) -I$(LONGBOW_DIR)/include
LDFLAGS      = -rdynamic 
LIBS         = -L$(BUILDDIR) -L$(LONGBOW_DIR)/lib -llongbow -llongbow-ansiterm
 
SRC = $(wildcard *.c) $(wildcard private/*.c)
HDR = $(wildcard $(INCLUDEDIR)/Folio/*.h) $(wildcard $(INCLUDEDIR)/Folio/private/*.h)
OBJ = $(addprefix $(BUILDDIR)/,$(SRC:.c=.o))

UNAME = $(shell uname -s)
ifeq ($(UNAME),Linux)
	include Makefile.linux
endif
ifeq ($(UNAME),Darwin)
	include Makefile.darwin
endif

lib: $(BUILDDIR)/$(TARGET) $(BUILDDIR)/$(STATIC)

$(OBJ): $(BUILDDIR)/%.o : %.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(STATIC): $(OBJ)
	$(AR) rcs $@ $^

clean:
	$(RM) -rf ${OBJ} $(BUILDDIR)/$(TARGET)* $(BUILDDIR)/$(STATIC)

remove:
	rm -rf $(PREFIX)/include/Folio
	rm -f $(PREFIX)/lib/libfolio.a $(PREFIX)/lib/libfolio.so*

depend:
	makedepend -- $(CFLAGS) -- $(SRC) -f -
	
