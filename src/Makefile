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
TARGET := lib$(NAME).so
TARGET_MAJ := $(TARGET).$(MAJOR)
TARGET_VER := $(TARGET).$(VERSION)

CFLAGS       = -std=gnu11 -rdynamic -fPIC -g -O0 -Wall -Wextra -I$(HEADERDIR)
LDFLAGS      = -shared
LIBS         = -L$(BUILDDIR) -llongbow -llongbow-ansiterm
 
SRC = $(wildcard *.c)
HDR = $(wildcard $(INCLUDEDIR)/*.h)
OBJ = $(addprefix $(BUILDDIR),$(SRC:.c=.o))

lib: $(BUILDDIR)/$(TARGET)

$(OBJ): $(BUILDDIR)/%.o : %.c $(INCLUDEDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/$(TARGET): $(BUILDDIR)/$(TARGET_VER)
	ldconfig -v -n $(BUILDDIR)
	ln -fs $(BUILDDIR)/$(TARGET_MAJ) $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET_VER): $(LIB_OBJ)
	$(CC) -shared -Wl,-soname,$(BUILDDIR)/$(TARGET_MAJ) $^ -o $@

clean:
	$(RM) -rf ${OBJ} $(BUILDDIR)/$(TARGET)*


