/*
   Copyright (c) 2017, Palo Alto Research Center
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef INCLUDE_FOLIO_PRIVATE_FOLIO_POOL_H_
#define INCLUDE_FOLIO_PRIVATE_FOLIO_POOL_H_

#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>
#include <Folio/private/folio_Lock.h>

#define _alignment_width sizeof(void *)

/*
 * Used to identify a folio_internal_provider memory block
 */
#define _internalMagic 0xa16588fb703b6f06ULL

/**
 * +-----------------------+
 * | FolioMemoryProvider   |
 * +-----------------------+
 * | FolioPool             | <-- _getPool(provider)  = pool->poolState pointer
 * +-----------------------+
 * | ProviderState         | <-- folioInternalProvider_GetProviderState(provider) = pool + sizeof(FolioPool)
 * +-----------------------+
 */
typedef struct folio_pool {
	// Bit pattern to make sure we are really working with this data structure.
	uint64_t internalMagic1;

	// per-instance magics to guard memory locations
	uint32_t headerMagic;

	// The size of extra bytes at the end of FolioPool for
	// the user to store data.
	uint32_t providerStateLength;

	// The amount of extra bytes to allocate in each memory allocation for
	// the user to store extra data.
	uint32_t providerHeaderLength;

	// The number of extra bytes in the header for the guard pattern
	uint32_t headerGuardLength;

	// The total length of the header, including our data, providerStateLength,
	// and the header guard
	uint32_t headerAlignedLength;

	// The size of the trailer including the guard length.
	uint32_t trailerAlignedLength;

	atomic_uint currentAllocation;

	atomic_int referenceCount;

	atomic_flag allocationLock;

	// amount of memory in the pool
	size_t poolSize;

	// Used to start a guard byte array pattern.  Varries for each pool.
	uint8_t guardPattern;

	// Bit pattern to make sure we are really working with this data structure.
	uint64_t internalMagic2;
} FolioPool __attribute__((aligned));

/**
 * Creates a string report of the pool.
 *
 * You must call free() on the returned string.
 */
char *folioPool_ToString(const FolioPool *pool);

#endif /* INCLUDE_FOLIO_PRIVATE_FOLIO_POOL_H_ */
