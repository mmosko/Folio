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

CFLAGS       = -std=gnu11 -rdynamic -fPIC -g -O0 -Wall -Wextra -I$(INCLUDEDIR)
LDFLAGS      = 
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
	$(CC) -shared -Wl,-soname,$(TARGET_MAJ) $^ -o $@

$(BUILDDIR)/$(STATIC): $(OBJ)
	$(AR) rcs $@ $^

clean:
	$(RM) -rf ${OBJ} $(BUILDDIR)/$(TARGET)* $(BUILDDIR)/$(STATIC)

install: lib
	install -d $(PREFIX) $(PREFIX)/include/Folio $(PREFIX)/lib
	install -p -t $(PREFIX)/include/Folio $(INCLUDEDIR)/Folio/*.h
	install -p -t $(PREFIX)/lib $(BUILDDIR)/$(STATIC) $(BUILDDIR)/$(TARGET_VER)
	
