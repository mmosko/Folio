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

#include <LongBow/runtime.h>
#include <Folio/private/folio_InternalProvider.h>
#include <Folio/private/folio_Lock.h>

#include <stdio.h>
#include <stdlib.h>

#include <stdatomic.h>

#define _alignment_width sizeof(void *)

/*
 * Used to identify a folio_internal_provider memory block
 */
#define _internalMagic 0xa16588fb703b6f06ULL

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

#if 0

/*
 * Used to initialize the default pool
 */
extern const FolioMemoryProvider FolioStdProviderTemplate;


/* ******************************************************
 * A statically declared pool so we can have a default.
 *
 * It has a fixed size provider state space and provider header length
 */

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

const FolioStaticPool FolioInternalStaticPool = {
	.pool = {
		.provider = {
				.allocate = folioStdProvider_Allocate,
		},

		.internalMagic1 = _internalMagic,
		.headerMagic = 0x05fd095c69493bf8ULL,
		.providerStateLength = _staticProviderStateLength,
		.providerHeaderLength = _staticProviderHeaderLength,
		.headerGuardLength = _staticHeaderGuardLength,
//		.headerAlignedLength = sizeof(FolioHeader) + _staticProviderStateLength + _staticHeaderGuardLength,
		.headerAlignedLength = 328,
		.trailerAlignedLength = sizeof(FolioTrailer),
		.trailerGuardLength = 0,
		.guardPattern = 0xA0,
		.allocationLock = ATOMIC_FLAG_INIT,
		.poolSize = SIZE_MAX,
		.currentAllocation = ATOMIC_VAR_INIT(0),
		.internalMagic2 = _internalMagic
	},
	.providerState = { 0 },
	.guard = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7 },
};

//const FolioMemoryProvider *FolioInternalProvider = (const FolioMemoryProvider *) &_staticPool;
#endif


/* ****************************************************** */


static void
_fillGuard(uint8_t pattern, size_t length, uint8_t guard[length])
{
	memset(guard, pattern, length);
}

static bool
_verifyGuard(uint8_t pattern, size_t length, const uint8_t guard[length])
{
	size_t i = 0;
	for ( ; i < length && guard[i] == pattern; ++i);

	return i == length;
}

static int
_referenceCount(const FolioHeader *header)
{
	int refCount = atomic_load(&header->referenceCount);
	return refCount;
}

/**
 * @param alignment The length to align to (must be multiple of sizeof(void *))
 *
 * @return 0 on error (e.g. alignment is not multiple of sizeof(void *)
 * @return >=length on success
 *
 * Example:
 * <code>
 * size_t alignedLength = _calculateAlignedLength(sizeof(void *), 23);
 * </code>
 */
static size_t
_calculateAlignedLength(size_t length)
{
    const size_t mask = _alignment_width - 1;
    const size_t maskedLength = length & mask;
    const size_t extra = _alignment_width + ~maskedLength + 1;
    const size_t alignedLength = length + extra;
    return alignedLength;
}

static atomic_flag _needRandomInitialized = ATOMIC_FLAG_INIT;
static struct random_data _random_state;

static void
_initializeRandom(void)
{
	if (! atomic_flag_test_and_set(&_needRandomInitialized) ) {
		unsigned seed = 3842923;
		srandom_r(seed, & _random_state);
	}
}

static uint64_t
_getRandomMagic(void)
{
	int32_t r1, r2;
	random_r(&_random_state, &r1);
	random_r(&_random_state, &r2);
	uint64_t magic1 = r1 << sizeof(uint32_t) | r2;
	return magic1;
}

FolioMemoryProvider *
folioInternalProvider_Create(const FolioMemoryProvider *template, size_t memorySize, size_t providerStateLength, size_t providerHeaderLength)
{
	_initializeRandom();

	size_t length = sizeof(FolioPool) + providerStateLength;
	size_t alignedLength = _calculateAlignedLength(length);

	FolioPool *pool = calloc(1, alignedLength);
	memcpy(&pool->provider, template, sizeof(FolioMemoryProvider));

	pool->internalMagic1 = _internalMagic;
	pool->headerMagic = _getRandomMagic();
	pool->providerStateLength = providerStateLength;
	pool->providerHeaderLength = providerHeaderLength;

	size_t headerLengthNoGuard = sizeof(FolioHeader) + providerHeaderLength;
	pool->headerAlignedLength = _calculateAlignedLength(headerLengthNoGuard);
	if (pool->headerAlignedLength == headerLengthNoGuard) {
		pool->headerAlignedLength += _alignment_width;
	}

	pool->headerGuardLength = pool->headerAlignedLength - headerLengthNoGuard;
	pool->trailerAlignedLength = _calculateAlignedLength(sizeof(FolioTrailer));
	pool->trailerGuardLength = pool->trailerAlignedLength - sizeof(FolioTrailer);

	pool->guardPattern = (uint8_t)pool->headerMagic + 1;
	if (pool->guardPattern == 0) {
		pool->guardPattern++;
	}

	atomic_flag_clear(&pool->allocationLock);
	pool->poolSize = memorySize;
	pool->currentAllocation = ATOMIC_VAR_INIT(0);

	pool->internalMagic2 = _internalMagic;

	return (FolioMemoryProvider *) pool;
}

static const uint8_t *
_getGuardAddress(const FolioPool *pool, const FolioHeader *header)
{
	size_t guardOffset = sizeof(FolioHeader) + pool->providerHeaderLength;
	const uint8_t *guard = (uint8_t *) header + guardOffset;
	return guard;
}

static bool
_verifyInternalProvider(FolioPool const *internalProvider)
{
	bool result = false;
	if (internalProvider->internalMagic1 == _internalMagic && internalProvider->internalMagic2 == _internalMagic) {
		result = true;
	}
	return result;
}

static bool
_verifyHeader(FolioPool const *pool, const FolioHeader *header)
{
	bool result = false;
	if ((header->magic1 == pool->headerMagic) && (header->magic2 == pool->headerMagic)) {
		const uint8_t *guard = _getGuardAddress(pool, header);

		if (_verifyGuard(pool->guardPattern, pool->headerGuardLength, guard) ) {
			result = true;
		}
	}

	return result;
}

static bool
_verifyTrailer(const FolioPool *pool, const FolioTrailer *trailer)
{
	bool result = false;
	if (trailer->magic3 == pool->headerMagic) {
		if (_verifyGuard(pool->guardPattern, pool->headerGuardLength, trailer->guard)) {
			result = true;
		}
	}
	return result;
}

static FolioHeader *
_getHeader(const FolioPool *pool, const void *memory)
{
	trapUnexpectedStateIf(memory < (void *) pool->headerAlignedLength, "Invalid memory address (%p)", memory);
	return (FolioHeader *) ((uint8_t *) memory - pool->headerAlignedLength);
}

static FolioTrailer *
_getTrailer(const FolioPool *pool, const FolioHeader *header)
{
	FolioTrailer *trailer = (FolioTrailer *)((uint8_t *) header + pool->headerAlignedLength + header->requestedLength);
	return trailer;
}

void *
folioInternalProvider_GetProviderState(FolioMemoryProvider const *provider)
{
	FolioPool const * pool = (FolioPool const *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");
	return ((uint8_t *) pool) + sizeof(FolioPool);
}

void *
folioInternalProvider_GetProviderHeader(FolioMemoryProvider const *provider, const void *memory)
{
	const FolioPool * pool = (const FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	trapUnexpectedStateIf( !_verifyHeader(pool, header), "memory corrupted");

	void *providerHeader = ((uint8_t *) header) + pool->headerAlignedLength;
	return providerHeader;
}

size_t
folioInternalProvider_GetProviderStateLength(const FolioMemoryProvider *provider)
{
	FolioPool const * pool = (FolioPool const *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");
	return pool->providerStateLength;
}

size_t
folioInternalProvider_GetProviderHeaderLength(const FolioMemoryProvider *provider)
{
	FolioPool const * pool = (FolioPool const *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");
	return pool->providerHeaderLength;
}

/* **************************************************************
 * Implementation of FolioMemoryProvider
 *
 */

void
folioInternalProvider_Allocate_ReleaseProvider(FolioMemoryProvider **providerPtr)
{
	FolioPool* pool = (FolioPool *) *providerPtr;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	free(pool);
	*providerPtr = NULL;
}

/*
 * Tests for available memory and if so subtracts the allocation length.
 *
 * @return true if allocation of length bytes is ok
 * @return false if out of memory
 */
static bool
_increaseCurrentAllocation(FolioPool *pool, const size_t length)
{
	bool memoryIsAvailable = false;

	folioLock_FlagLock(&pool->allocationLock);

	if (pool->poolSize >= pool->currentAllocation) {
		size_t remainingMemory = pool->poolSize - pool->currentAllocation;

		if (length <= remainingMemory) {
			memoryIsAvailable = true;
			pool->currentAllocation += length;
		}
	}

	folioLock_FlagUnlock(&pool->allocationLock);

	return memoryIsAvailable;
}

/*
 * precondition: you hold the stats lock
 */
static void
_decreaseCurrentAllocation(FolioPool *pool, const size_t length)
{
	folioLock_FlagLock(&pool->allocationLock);

	trapIllegalValueIf(pool->currentAllocation < length, "current allocation less than length");
	pool->currentAllocation -= length;

	folioLock_FlagUnlock(&pool->allocationLock);
}

void *
folioInternalProvider_Allocate(FolioMemoryProvider *provider, const size_t length)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	void *user = NULL;

	bool memoryIsAvailable = _increaseCurrentAllocation(pool, length);

	if (memoryIsAvailable) {
		size_t totalLength = pool->headerAlignedLength + length + pool->trailerAlignedLength;
		void *memory = malloc(totalLength);
		FolioHeader *header = memory;

		header->magic1 = pool->headerMagic;
		header->requestedLength = length;
		header->referenceCount = ATOMIC_VAR_INIT(1);
		header->fini = NULL;
		header->magic2 = pool->headerMagic;

		user = memory + pool->headerAlignedLength;

		uint8_t *guard = (uint8_t *) _getGuardAddress(pool, header);
		_fillGuard(pool->guardPattern, pool->headerGuardLength, guard);

		FolioTrailer *trailer = _getTrailer(pool, header);
		trailer->magic3 = pool->headerMagic;
		_fillGuard(pool->guardPattern, pool->trailerGuardLength, trailer->guard);
	}

	return user;
}

void *
folioInternalProvider_AllocateAndZero(FolioMemoryProvider *provider, const size_t length)
{
	void * memory = folioInternalProvider_Allocate(provider, length);
	if (memory) {
		memset(memory, 0, length);
	}
	return memory;
}

void
folioInternalProvider_SetFinalizer(FolioMemoryProvider *provider, void *memory, Finalizer fini)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	trapUnexpectedStateIf( !_verifyHeader(pool, header), "memory corrupted");

	header->fini = fini;
}

static void
_validateInternal(FolioPool *pool, const FolioHeader *header)
{
	if (header->magic1 == pool->headerMagic && header->magic2 == pool->headerMagic) {
		int refcount = _referenceCount(header);
		if (refcount == 0) {
			printf("\n\nHeader:\n");
			longBowDebug_MemoryDump((const char *) header, pool->headerAlignedLength);
			trapUnexpectedState("Memory: refcount is zero");
		}

		const FolioTrailer *trailer = _getTrailer(pool, header);
		if (!_verifyTrailer(pool, trailer)) {
			printf("\n\nTrailer:\n");
			longBowDebug_MemoryDump((const char *) trailer, pool->trailerAlignedLength);
			trapUnexpectedState("Memory: invalid trailer (memory overrun)");
		}
	} else {
		printf("\n\nHeader:\n");
		longBowDebug_MemoryDump((const char *) header, pool->headerAlignedLength);
		trapUnexpectedState("Memory: invalid header (memory underrun)");
	}
}

void
folioInternalProvider_Validate(const FolioMemoryProvider *provider, const void *memory)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);
}

void *
folioInternalProvider_Acquire(FolioMemoryProvider *provider, const void *memory)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);

	_validateInternal(pool, header);

	// prior value must be greater than 0.  If it is not positive it means someone
	// freed the memory during the time between _validateInternal and now.
	int prior = atomic_fetch_add_explicit(&header->referenceCount, 1, memory_order_relaxed);
	if (prior < 1) {
		trapUnrecoverableState("The memory %p was freed during acquire", (void *) memory);
	}

	return (void *) memory;
}

size_t
folioInternalProvider_Length(const FolioMemoryProvider *provider, const void *memory)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	return header->requestedLength;
}

bool
folioInternalProvider_ReleaseMemory(FolioMemoryProvider *provider, void **memoryPtr)
{
	trapIllegalValueIf(memoryPtr == NULL, "Null memory pointer");

	void *memory = *memoryPtr;

	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	int prior = atomic_fetch_sub(&header->referenceCount, 1);
	trapIllegalValueIf(prior < 1, "Reference count was %d < 1 when trying to release", prior);

	bool finalRelease = false;
	if (prior == 1) {
		finalRelease = true;
		if (header->fini != NULL) {
			header->fini(memory);
		}

		_decreaseCurrentAllocation(pool, header->requestedLength);

		// Write over magic1 so it invalidates the block
		memset(header, 0, sizeof(uint64_t));
		free(header);
	}

	*memoryPtr = NULL;
	return finalRelease;
}

void
folioInternalProvider_SetAvailableMemory(FolioMemoryProvider *provider, size_t availableMemory)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	pool->poolSize = availableMemory;
}

size_t
folioInternalProvider_AllocationSize(const FolioMemoryProvider *provider)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	folioLock_FlagLock(&pool->allocationLock);
	size_t size = pool->currentAllocation;
	folioLock_FlagUnlock(&pool->allocationLock);
	return size;
}

void
folioInternalProvider_Lock(FolioMemoryProvider *provider, void *memory)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	folioLock_Lock(header->lock);
}

void
folioInternalProvider_Unlock(FolioMemoryProvider *provider, void *memory)
{
	FolioPool * pool = (FolioPool *) provider;
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	folioLock_Unlock(header->lock);
}

