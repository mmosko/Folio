# Folio Memory Provider for C

The Folio library is a replacement for `malloc()` and `free()`.  It allocates
reference countable memory blocks with detection of overrun and underrun of
memory allocations.  The debug memory provider also keeps a stack backtrace
of each allocation for simpler debugging of memory leaks.

Folio is built on the LongBow [1,2] xUnit tester for C and requires that it
be installed. See below for a full list of dependencies.

Folio is under active development and should not be used in anything important yet.

# Developer

More examples coming.  Right now, folio_LinkedList.c is the best
example of using the allocator, and the unit tests.

## Building

The install prefix is defined in the top-level Makefile as PREFIX.  You
should update this as desired or orverride it on the command line.

Just `make`. 

To use the default PREFIX=/usr/local
```
make
make check
sudo make install
```

To override the prefix either edit the top-level Makefile or
```
make PREFIX=~/folio all check install
```

At some point in the future, we'll switch to autoconf or cmake.

## Installing
Either edit the Makefile to udpate PREFIX or use the command line:

- make install
- make PREFIX=/tmp/folio install

## Dependencies
- LongBow [2]
- gcovr
- lcov
- pthreads

### LongBow dependency
The Makefile uses a new extention to LongBow for a TestAnythingProtocol
reporter, which is only available in the fork [2].  We are working to get
that merged in to the main branch [1] and get rid of [2].

# References

[1] https://wiki.fd.io/view/Cframework#LongBow
[2] https://github.com/mmosko/LongBow-TAP

