.PHONY: all lib clean

SHELL = /bin/sh
CC    = gcc

all: lib

###########
# Library setup

MAJOR := 0
MINOR := 1
NAME := folio
VERSION := $(MAJOR).$(MINOR)
STATIC := lib$(NAME).a
TARGET := lib$(NAME).so
TARGET_MAJ := $(TARGET).$(MAJOR)
TARGET_VER := $(TARGET).$(VERSION)

CFLAGS       = -std=gnu11 -g -O0 -rdynamic -fPIC -Wall -Wextra -I$(INCLUDEDIR)
LDFLAGS      = -rdynamic 
LIBS         = -L$(BUILDDIR) -llongbow -llongbow-ansiterm
 
SRC = $(wildcard *.c)
HDR = $(wildcard $(INCLUDEDIR)/Folio/*.h)
OBJ = $(addprefix $(BUILDDIR)/,$(SRC:.c=.o))

lib: $(BUILDDIR)/$(TARGET) $(BUILDDIR)/$(STATIC)

$(OBJ): $(BUILDDIR)/%.o : %.c $(INCLUDEDIR)/Folio/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(TARGET): $(BUILDDIR)/$(TARGET_VER)
	ldconfig -v -n -r $(BUILDDIR) .
	ln -fs $(TARGET_MAJ) $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET_VER): $(OBJ)
	$(CC) -shared $(LDFLAGS) -Wl,-soname,$(TARGET_MAJ) $^ -o $@

$(BUILDDIR)/$(STATIC): $(OBJ)
	$(AR) rcs $@ $^

clean:
	$(RM) -rf ${OBJ} $(BUILDDIR)/$(TARGET)* $(BUILDDIR)/$(STATIC)

install: lib
	install -d $(PREFIX) $(PREFIX)/include/Folio $(PREFIX)/lib
	install -p -t $(PREFIX)/include/Folio $(INCLUDEDIR)/Folio/*.h
	install -p -t $(PREFIX)/lib $(BUILDDIR)/$(STATIC) $(BUILDDIR)/$(TARGET_VER)
	ln -fs $(TARGET_VER) $(PREFIX)/lib/$(TARGET_MAJ)
	ln -fs $(TARGET_MAJ) $(PREFIX)/lib/$(TARGET)

remove:
	rm -rf $(PREFIX)/include/Folio
	rm -f $(PREFIX)/lib/libfolio.a $(PREFIX)/lib/libfolio.so*

	
