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
#include <inttypes.h>

#include <Folio/private/folio_Header.h>
#include <Folio/private/folio_Pool.h>

//#define DEBUG 1

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

static uint64_t
_getRandomMagic(void)
{
	uint64_t r1, r2;
	r1 = random();
	r2 = random();
	uint64_t magic1 = r1 << 32 | r2;

	return magic1;
}

static FolioPool *
_getPool(const FolioMemoryProvider *provider)
{
	return (FolioPool *) provider->poolState;
}

/*
 * The memory will look like this:
 *
 * +-----------------------+
 * | FolioMemoryProvider   |
 * +-----------------------+
 * | FolioPool             | <-- _getPool(provider)  = pool->poolState pointer
 * +-----------------------+
 * | ProviderState         | <-- folioInternalProvider_GetProviderState(provider) = pool + sizeof(FolioPool)
 * +-----------------------+
 *
 */
FolioMemoryProvider *
folioInternalProvider_Create(const FolioMemoryProvider *template, size_t memorySize, size_t providerStateLength, size_t providerHeaderLength)
{
	size_t length = sizeof(FolioMemoryProvider) + sizeof(FolioPool) + providerStateLength;
	size_t alignedLength = _calculateAlignedLength(length);

	FolioMemoryProvider *provider = calloc(1, alignedLength);
	memcpy(provider, template, sizeof(FolioMemoryProvider));

	provider->poolState = (uint8_t *) provider + sizeof(FolioMemoryProvider);

	FolioPool *pool = _getPool(provider);

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
	pool->trailerAlignedLength = sizeof(FolioTrailer);

	pool->guardPattern = (uint8_t)pool->headerMagic + 1;
	if (pool->guardPattern == 0) {
		pool->guardPattern++;
	}

	atomic_flag_clear(&pool->allocationLock);
	pool->poolSize = memorySize;
	pool->currentAllocation = ATOMIC_VAR_INIT(0);
	pool->referenceCount = ATOMIC_VAR_INIT(1);

	pool->internalMagic2 = _internalMagic;

#if DEBUG
	folioInternalProvider_Report(provider, stderr);
#endif


	return provider;
}

static const uint8_t *
_getHeaderGuardAddress(const FolioPool *pool, const FolioHeader *header)
{
	size_t guardOffset = sizeof(FolioHeader) + pool->providerHeaderLength;
	const uint8_t *guard = (uint8_t *) header + guardOffset;
	return guard;
}

static const uint8_t *
_getTrailerGuardAddress(const FolioPool *pool, const FolioHeader *header)
{
	size_t guardOffset = pool->headerAlignedLength + folioHeader_GetRequestedLength(header);
	const uint8_t *guard = (uint8_t *) header + guardOffset;
	return guard;
}

static bool
_verifyInternalProvider(FolioPool const *internalProvider)
{
	assertNotNull(internalProvider, "internalProvider must be non-null");
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
	if (folioHeader_CompareMagic(header, pool->headerMagic)) {
		const uint8_t *guard = _getHeaderGuardAddress(pool, header);

		if (_verifyGuard(pool->guardPattern, pool->headerGuardLength, guard) ) {
			result = true;
		}
	}

	return result;
}

static bool
_verifyTrailer(const FolioPool *pool, const FolioTrailer *trailer, size_t trailerGuardLength)
{
	const uint8_t *guard = (const uint8_t *) trailer - trailerGuardLength;
	bool result = false;
	if (trailer->magic3 == pool->headerMagic) {
		if (_verifyGuard(pool->guardPattern, trailerGuardLength, guard)) {
			result = true;
		}
	}
	return result;
}

static FolioHeader *
_getHeader(const FolioPool *pool, const void *memory)
{
	size_t length = pool->headerAlignedLength;
	trapUnexpectedStateIf(memory < (void *) length, "Invalid memory address (%p)", memory);
	return (FolioHeader *) ((uint8_t *) memory - pool->headerAlignedLength);
}

static FolioTrailer *
_getTrailer(const FolioPool *pool, const FolioHeader *header)
{
	FolioTrailer *trailer = (FolioTrailer *)((uint8_t *) header
								+ pool->headerAlignedLength
								+ folioHeader_GetRequestedLength(header)
								+ folioHeader_GetTrailerGuardLength(header));
	return trailer;
}

void *
folioInternalProvider_GetProviderState(FolioMemoryProvider const *provider)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");
	return ((uint8_t *) pool) + sizeof(FolioPool);
}

void *
folioInternalProvider_GetProviderHeader(FolioMemoryProvider const *provider, const void *memory)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	trapUnexpectedStateIf( !_verifyHeader(pool, header), "memory corrupted");

	void *providerHeader = ((uint8_t *) header) + sizeof(FolioHeader);
	return providerHeader;
}

size_t
folioInternalProvider_GetProviderStateLength(const FolioMemoryProvider *provider)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");
	return pool->providerStateLength;
}

size_t
folioInternalProvider_GetProviderHeaderLength(const FolioMemoryProvider *provider)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");
	return pool->providerHeaderLength;
}

/* **************************************************************
 * Implementation of FolioMemoryProvider
 *
 */

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

static size_t
_computeTotalLength(FolioPool *pool, size_t requestLength, size_t trailerGuardLength)
{
	size_t totalLength = pool->headerAlignedLength + pool->headerGuardLength + requestLength +
							trailerGuardLength + pool->trailerAlignedLength;

	return totalLength;
}

/*
 * The memory will look like this:
 *
 * +-----------------------+
 * | FolioHeader           |
 * +-----------------------+  <- aligned
 * | ProviderHeader        |
 * +-----------------------+
 * | header guard[]        |
 * +-----------------------+  <- aligned
 * | user memory           |
 * +-----------------------+
 * | trailer guard[]       |
 * +-----------------------+  <- aligned
 * | FolioTrailer          |
 * +-----------------------+  <- aligned
 *
 */

void *
folioInternalProvider_Allocate(FolioMemoryProvider *provider, const size_t length, Finalizer fini)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	void *user = NULL;

	bool memoryIsAvailable = _increaseCurrentAllocation(pool, length);

	if (memoryIsAvailable) {
		size_t alignedLength = _calculateAlignedLength(length);
		size_t trailerGuardLength = alignedLength - length;

		size_t totalLength = _computeTotalLength(pool, length, trailerGuardLength);

		void *memory = malloc(totalLength);
		FolioHeader *header = memory;

		FolioLock *lock = folioLock_Create();
		folioHeader_Initialize(header, pool->headerMagic, length, lock, 1, pool->providerHeaderLength,
								fini, pool->headerGuardLength, trailerGuardLength);
		folioLock_Release(&lock);

		user = memory + pool->headerAlignedLength;

		uint8_t *headerGuard = (uint8_t *) _getHeaderGuardAddress(pool, header);
		_fillGuard(pool->guardPattern, pool->headerGuardLength, headerGuard);

		FolioTrailer *trailer = _getTrailer(pool, header);
		trailer->magic3 = pool->headerMagic;

		uint8_t *trailerGuard = (uint8_t *) _getTrailerGuardAddress(pool, header);
		_fillGuard(pool->guardPattern, trailerGuardLength, trailerGuard);

		folioInternalProvider_Validate(provider, user);

#if DEBUG
		fprintf(stderr, "Header %p Guard %p User %p Trailer %p Guard %p TotalLength %zu\n",
				(void *) header,
				(void *) headerGuard,
				(void *) user,
				(void *) trailer,
				(void *) trailerGuard,
				totalLength);

		folioInternalProvider_Display(provider, user, stderr);

		longBowDebug_MemoryDump((const char *) header, totalLength);
#endif
	}

	return user;
}

void *
folioInternalProvider_AllocateAndZero(FolioMemoryProvider *provider, const size_t length, Finalizer fini)
{
	void * memory = folioInternalProvider_Allocate(provider, length, fini);
	if (memory) {
		memset(memory, 0, length);
	}
	return memory;
}

static void
_validateInternal(FolioPool *pool, const FolioHeader *header)
{
	if (folioHeader_CompareMagic(header, pool->headerMagic)) {
		int refcount = folioHeader_ReferenceCount(header);
		if (refcount == 0) {
			char *headerString = folioHeader_ToString(header);
			printf("\n\nHeader: %s\n", headerString);
			free(headerString);

			longBowDebug_MemoryDump((const char *) header, pool->headerAlignedLength);
			trapUnexpectedState("Memory: refcount is zero");
		}

		const FolioTrailer *trailer = _getTrailer(pool, header);
		if (!_verifyTrailer(pool, trailer, folioHeader_GetTrailerGuardLength(header))) {
			char *trailerString = folioTrailer_ToString(trailer, folioHeader_GetTrailerGuardLength(header));
			printf("\n\nTrailer: %s\n", trailerString);
			free(trailerString);

			size_t totalLength = _computeTotalLength(pool, folioHeader_GetRequestedLength(header), folioHeader_GetTrailerGuardLength(header));


			longBowDebug_MemoryDump((const char *) header, totalLength);
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
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);
}

FolioMemoryProvider *
folioInternalProvider_AcquireProvider(const FolioMemoryProvider *provider)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	// prior value must be greater than 0.  If it is not positive it means someone
	// freed the memory during the time between _validateInternal and now.
	int prior = atomic_fetch_add_explicit(&pool->referenceCount, 1, memory_order_relaxed);
	if (prior < 1) {
		trapUnrecoverableState("The provider %p was freed during acquire", (void *) provider);
	}

	return (FolioMemoryProvider *) provider;
}

/**
 * Release a reference to the provider.
 */
bool
folioInternalProvider_ReleaseProvider(FolioMemoryProvider **providerPtr)
{
	trapIllegalValueIf(providerPtr == NULL, "providerPtr must be non-null");

	FolioMemoryProvider *provider = *providerPtr;

	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	int prior = atomic_fetch_sub(&pool->referenceCount, 1);
	trapIllegalValueIf(prior < 1, "Reference count was %d < 1 when trying to release", prior);

	bool finalRelease = false;
	if (prior == 1) {
		finalRelease = true;

		// Write over magic1 so it invalidates the block
		pool->internalMagic1 = 0;
		free(provider);
	}

	*providerPtr = NULL;
	return finalRelease;

}


void *
folioInternalProvider_Acquire(FolioMemoryProvider *provider, const void *memory)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);

	_validateInternal(pool, header);

	// prior value must be greater than 0.  If it is not positive it means someone
	// freed the memory during the time between _validateInternal and now.
	int prior = folioHeader_IncrementReferenceCount(header);
	if (prior < 1) {
		trapUnrecoverableState("The memory %p was freed during acquire", (void *) memory);
	}

	return (void *) memory;
}

size_t
folioInternalProvider_Length(const FolioMemoryProvider *provider, const void *memory)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	return folioHeader_GetRequestedLength(header);
}

bool
folioInternalProvider_ReleaseMemory(FolioMemoryProvider *provider, void **memoryPtr)
{
	trapIllegalValueIf(memoryPtr == NULL, "Null memory pointer");

	void *memory = *memoryPtr;

	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	int prior = folioHeader_DecrementReferenceCount(header);
	trapIllegalValueIf(prior < 1, "Reference count was %d < 1 when trying to release", prior);

	bool finalRelease = false;
	if (prior == 1) {
		finalRelease = true;
		folioHeader_ExecuteFinalizer(header, memory);

		_decreaseCurrentAllocation(pool, folioHeader_GetRequestedLength(header));

		folioHeader_Finalize(header);

		// Write over magic1 so it invalidates the block
		folioHeader_Invalidate(header);
		free(header);
	}

	*memoryPtr = NULL;
	return finalRelease;
}

void
folioInternalProvider_SetAvailableMemory(FolioMemoryProvider *provider, size_t availableMemory)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	pool->poolSize = availableMemory;
}

size_t
folioInternalProvider_AllocationSize(const FolioMemoryProvider *provider)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	folioLock_FlagLock(&pool->allocationLock);
	size_t size = pool->currentAllocation;
	folioLock_FlagUnlock(&pool->allocationLock);
	return size;
}

void
folioInternalProvider_Lock(FolioMemoryProvider *provider, void *memory)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	folioHeader_Lock(header);
}

void
folioInternalProvider_Unlock(FolioMemoryProvider *provider, void *memory)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	_validateInternal(pool, header);

	folioHeader_Unlock(header);
}

void
folioInternalProvider_Report(const FolioMemoryProvider *provider, FILE *stream)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	bool locked = atomic_flag_test_and_set(&pool->allocationLock);
	if (!locked) {
		atomic_flag_clear(&pool->allocationLock);
	}

	fprintf(stream, "Pool (%p) : hdrMagic 0x%016" PRIx64 " provStateLen %u provHdrLen %u hdrAlgnLen %u hdrGrdLen %u "
			" trlAlgnLen %u GrdByte 0x%02x IsLocked %d poolSize %zu alloc'd %d refCount %d\n",
			(void *) pool,
			pool->headerMagic,
			pool->providerStateLength,
			pool->providerHeaderLength,
			pool->headerAlignedLength,
			pool->headerGuardLength,
			pool->trailerAlignedLength,
			pool->guardPattern,
			locked,
			pool->poolSize,
			pool->currentAllocation,
			atomic_load(&pool->referenceCount));
}

void
folioInternalProvider_Display(const FolioMemoryProvider *provider, const void *memory, FILE *stream)
{
	FolioPool *pool = _getPool(provider);
	trapUnexpectedStateIf( !_verifyInternalProvider(pool), "provider pointer is not a FolioPool");

	FolioHeader *header = _getHeader(pool, memory);
	char *headerString = folioHeader_ToString(header);

	FolioTrailer *trailer = _getTrailer(pool, header);
	char *trailerString = folioTrailer_ToString(trailer, folioHeader_GetTrailerGuardLength(header));

	fprintf(stream, "Memory (%p) : %s, %s\n",
			(void *) memory,
			headerString,
			trailerString);

	free(trailerString);
	free(headerString);
}

