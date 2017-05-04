
Things we know need to be done:

- realloc
- memalign
- The way the Header and Trailer work might not result in aligned user memory address.
  Need to verify how its being done now and if that is correct.
- No testing on ARM or x32 or big endian

