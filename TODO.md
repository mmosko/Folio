
Things we know need to be done:

- realloc
- memalign
- The way the Header and Trailer work might not result in aligned user memory address.
  Need to verify how its being done now and if that is correct.
- No testing on ARM or x32 or big endian

- Need to create a single internal provider that can store provider-specific data in
  the header to avoid so much code duplication between allocators.  Also, private
  classes like folio_Lock should use the internal provider to avoid circular references
  to user-level providers.
- folio_Lock uses raw malloc/free.  Needs private internal memory provider.
