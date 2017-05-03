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
Just `make`.

At some point in the future, we'll switch to autoconf or cmake.

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

