/*
 * folio_InternalHeader.h
 *
 *  Created on: May 5, 2017
 *      Author: marc
 */

#ifndef INCLUDE_FOLIO_PRIVATE_FOLIO_INTERNALHEADER_H_
#define INCLUDE_FOLIO_PRIVATE_FOLIO_INTERNALHEADER_H_

#include <stdatomic.h>
#include <Folio/private/folio_Lock.h>

#define _alignment_width sizeof(void *)

/*
 * Used to identify a folio_internal_provider memory block
 */
#define _internalMagic 0xa16588fb703b6f06ULL

typedef struct folio_pool {
	// Bit pattern to make sure we are really working with this data structure.
	uint64_t internalMagic1;

	// per-instance magics to guard memory locations
	uint64_t headerMagic;

	// The size of extra bytes at the end of FolioPool for
	// the user to store data.
	size_t providerStateLength;

	// The amount of extra bytes to allocate in each memory allocation for
	// the user to store extra data.
	size_t providerHeaderLength;

	// The number of extra bytes in the header for the guard pattern
	size_t headerGuardLength;

	// The total length of the header, including our data, providerStateLength,
	// and the header guard
	size_t headerAlignedLength;

	// The size of the trailer including the guard length.
	size_t trailerAlignedLength;

	// The number of bytes allocated for the guard in the trailer
	size_t trailerGuardLength;

	// Used to start a guard byte array pattern.  Varries for each pool.
	uint8_t guardPattern;

	atomic_flag allocationLock;

	// amount of memory in the pool
	size_t poolSize;

	atomic_uint currentAllocation;

	// Bit pattern to make sure we are really working with this data structure.
	uint64_t internalMagic2;
} FolioPool __attribute__((aligned));


#if 0
typedef struct folio_pool {
	FolioMemoryProvider provider;

	// Bit pattern to make sure we are really working with this data structure.
	uint64_t internalMagic1;

	// per-instance magics to guard memory locations
	uint64_t headerMagic;

	// The size of extra bytes at the end of FolioPool for
	// the user to store data.
	size_t providerStateLength;

	// The amount of extra bytes to allocate in each memory allocation for
	// the user to store extra data.
	size_t providerHeaderLength;

	// The number of extra bytes in the header for the guard pattern
	size_t headerGuardLength;

	// The total length of the header, including our data, providerStateLength,
	// and the header guard
	size_t headerAlignedLength;

	// The size of the trailer including the guard length.
	size_t trailerAlignedLength;

	// The number of bytes allocated for the guard in the trailer
	size_t trailerGuardLength;

	// Used to start a guard byte array pattern.  Varries for each pool.
	uint8_t guardPattern;

	atomic_flag allocationLock;

	// amount of memory in the pool
	size_t poolSize;

	atomic_uint currentAllocation;

	// Bit pattern to make sure we are really working with this data structure.
	uint64_t internalMagic2;
} FolioPool __attribute__((aligned));
#endif

/**
 * The header really looks like this:
 *
 * | FolioHeader ......  | providerHeader ..... | guard ..... |
 * | sizeof(FolioHeader) | providerHeaderLength | guardLength |
 * | ................ aligned ((sizeof(void *))) ............ |
 */
typedef struct folio_header {
	uint64_t magic1;

	// The requested allocation length
	size_t requestedLength;

	Finalizer fini;

	FolioLock *lock;

	atomic_int referenceCount;

	size_t providerDataLength;
	size_t guardLength;
	uint64_t magic2;
} FolioHeader __attribute__((aligned));

typedef struct folio_trailer {
	uint64_t magic3;
	uint8_t guard[0];
} FolioTrailer;


/* ******************************************************
 * A statically declared pool so we can have a default.
 *
 * It has a fixed size provider state space and provider header length
 */

#if 0
#define _staticProviderStateLength 256
#define _staticProviderHeaderLength 16
#define _static_alignment_width 8
#define _staticHeaderGuardLength 8

#include <Folio/folio_StdProvider.h>

typedef struct folio_static_pool {
	FolioPool pool;
	uint8_t providerState[_staticProviderStateLength];
	uint8_t guard[_static_alignment_width];
} FolioStaticPool;

#define STATIC_NAME(name) _static_ ## name

#define STATIC_POOL_INIT(name, template) \
	FolioStaticPool STATIC_NAME(name) = { \
	.pool = { \
		.provider = template, \
		.internalMagic1 = _internalMagic, \
		.headerMagic = 0x05fd095c69493bf8ULL, \
		.providerStateLength = _staticProviderStateLength, \
		.providerHeaderLength = _staticProviderHeaderLength, \
		.headerGuardLength = _staticHeaderGuardLength, \
		.headerAlignedLength = 328, \
		.trailerAlignedLength = sizeof(FolioTrailer), \
		.trailerGuardLength = 0, \
		.guardPattern = 0xA0, \
		.allocationLock = ATOMIC_FLAG_INIT, \
		.poolSize = SIZE_MAX, \
		.currentAllocation = ATOMIC_VAR_INIT(0), \
		.internalMagic2 = _internalMagic \
		}, \
	.providerState = { 0 }, \
	.guard = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7 }, \
	}; \
	FolioMemoryProvider *name = (FolioMemoryProvider *) &STATIC_NAME(name);

 .headerAlignedLength = sizeof(FolioHeader) + _staticProviderStateLength + _staticHeaderGuardLength

#define POOL_INIT(function_template, header_magic, alignedLength) { \
		.provider = function_template, \
		.internalMagic1 = _internalMagic, \
		.headerMagic = header_magic, \
		.providerStateLength = _staticProviderStateLength, \
		.providerHeaderLength = _staticProviderHeaderLength, \
		.headerGuardLength = _staticHeaderGuardLength, \
		.headerAlignedLength = alignedLength, \
		.trailerAlignedLength = sizeof(FolioTrailer), \
		.trailerGuardLength = 0, \
		.guardPattern = 0xA0, \
		.allocationLock = ATOMIC_FLAG_INIT, \
		.poolSize = SIZE_MAX, \
		.currentAllocation = ATOMIC_VAR_INIT(0), \
		.internalMagic2 = _internalMagic \
		}
#endif

#endif /* INCLUDE_FOLIO_PRIVATE_FOLIO_INTERNALHEADER_H_ */
