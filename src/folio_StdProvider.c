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
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

#include <Folio/folio_StdProvider.h>
#include <Folio/private/folio_Lock.h>
#include <Folio/private/folio_InternalProvider.h>
#include <Folio/private/folio_Pool.h>
#include <Folio/private/folio_Header.h>

static FolioMemoryProvider *_acquireProvider(const FolioMemoryProvider *provider);
static bool _releaseProvider(FolioMemoryProvider **providerPtr);

static void * _allocate(FolioMemoryProvider *provider, const size_t length, Finalizer fini);
static void * _allocateAndZero(FolioMemoryProvider *provider, const size_t length, Finalizer fini);
static void * _acquire(FolioMemoryProvider *provider, const void *memory);
static size_t _length(const FolioMemoryProvider *provider, const void *memory);
static void _release(FolioMemoryProvider *provider, void **memoryPtr);
static void _report(const FolioMemoryProvider *provider, FILE *stream);
static void _display(const FolioMemoryProvider *provider, const void *memory, FILE *stream);
static void _validate(const FolioMemoryProvider *provider, const void *memory);
static size_t _acquireCount(const FolioMemoryProvider *provider);
static size_t _allocationSize(const FolioMemoryProvider *provider);
static void _setAvailableMemory(FolioMemoryProvider *provider, size_t maximum);

static void _lock(FolioMemoryProvider *provider, void *memory);
static void _unlock(FolioMemoryProvider *provider, void *memory);

typedef struct stats {
	atomic_flag lock;

	size_t outstandingAllocs;
	size_t outstandingAcquires;

	// number of allocations attempted but no memory available
	size_t outOfMemoryCount;
} _Stats;


/*
 * Our local storage for the static provider
 */
typedef struct static_storage {
	FolioPool pool;
	_Stats stats;
} StaticStorage;

// Struct only used to compute its size
struct complete_provider {
	FolioMemoryProvider dummy1;
	StaticStorage dummy2;
} __attribute__((aligned));

struct aligned_header {
	FolioHeader dummy1;
} __attribute__((aligned));

#define StdHeaderMagic 0x69493bf8UL
#define GuardPattern 0xE0
#define GuardLength (sizeof(struct aligned_header) - sizeof(FolioHeader))

// Allocate the memory for the default static provider
static StaticStorage _storage = {
		.pool = {
				.internalMagic1 = _internalMagic,
				.headerMagic = StdHeaderMagic,
				.providerStateLength = sizeof(_Stats),
				.providerHeaderLength = 0,
				.headerGuardLength = GuardLength,
				.headerAlignedLength = sizeof(struct aligned_header),
				.trailerAlignedLength = sizeof(FolioTrailer),
				.guardPattern = GuardPattern,
				.allocationLock = ATOMIC_FLAG_INIT,
				.poolSize = SIZE_MAX,
				.currentAllocation = ATOMIC_VAR_INIT(0),
				.referenceCount = ATOMIC_VAR_INIT(1),
				.internalMagic2 = _internalMagic
		},
		.stats = {
				.lock = ATOMIC_FLAG_INIT,
				.outstandingAllocs = 0,
				.outstandingAcquires = 0,
				.outOfMemoryCount = 0
		}
};

FolioMemoryProvider FolioStdProvider = {
		.acquireProvider = _acquireProvider,
		.releaseProvider = _releaseProvider,
		.allocate = _allocate,
		.allocateAndZero = _allocateAndZero,
		.acquire = _acquire,
		.length = _length,
		.release = _release,
		.report = _report,
		.display = _display,
		.validate = _validate,
		.acquireCount = _acquireCount,
		.allocationSize = _allocationSize,
		.setAvailableMemory = _setAvailableMemory,
		.lock = _lock,
		.unlock = _unlock,
		.poolState = &_storage,
};

/* ********************************************************** */

FolioMemoryProvider *
folioStdProvider_Create(size_t poolSize)
{
	FolioMemoryProvider *pool = folioInternalProvider_Create(&FolioStdProvider, poolSize, sizeof(_Stats), 0);
	return pool;
}

static FolioMemoryProvider *
_acquireProvider(const FolioMemoryProvider *provider)
{
	return folioInternalProvider_AcquireProvider(provider);
}

static bool
_releaseProvider(FolioMemoryProvider **providerPtr)
{
	return folioInternalProvider_ReleaseProvider(providerPtr);
}

static void *
_allocate(FolioMemoryProvider *provider, const size_t length, Finalizer fini)
{
	void *memory = folioInternalProvider_Allocate(provider, length, fini);

	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(provider);

	folioLock_FlagLock(&stats->lock);
	if (memory != NULL) {
		stats->outstandingAcquires++;
		stats->outstandingAllocs++;
	} else {
		stats->outOfMemoryCount++;
	}
	folioLock_FlagUnlock(&stats->lock);

	return memory;
}

static void *
_allocateAndZero(FolioMemoryProvider *provider, const size_t length, Finalizer fini)
{
	void * memory = _allocate(provider, length, fini);
	if (memory) {
		memset(memory, 0, length);
	}
	return memory;
}

static void
_validate(const FolioMemoryProvider *provider, const void *memory)
{
	folioInternalProvider_Validate(provider, memory);
}

static void *
_acquire(FolioMemoryProvider *provider, const void *memory)
{
	folioInternalProvider_Acquire(provider, memory);

	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(provider);

	folioLock_FlagLock(&stats->lock);
	stats->outstandingAcquires++;
	folioLock_FlagUnlock(&stats->lock);

	return (void *) memory;
}

static size_t
_length(const FolioMemoryProvider *provider, const void *memory)
{
	return folioInternalProvider_Length(provider, memory);
}

static void
_release(FolioMemoryProvider *provider, void **memoryPtr)
{
	assertNotNull(memoryPtr, "memoryPtr must be non-null");
	assertNotNull(*memoryPtr, "memoryPtr must dereference to non-null");

	bool finalRelease = folioInternalProvider_ReleaseMemory(provider, memoryPtr);

	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(provider);

	if (finalRelease) {
		folioLock_FlagLock(&stats->lock);
		stats->outstandingAcquires--;
		stats->outstandingAllocs--;
		folioLock_FlagUnlock(&stats->lock);
	} else {
		folioLock_FlagLock(&stats->lock);
		stats->outstandingAcquires--;
		folioLock_FlagUnlock(&stats->lock);
	}
}

static void
_report(const FolioMemoryProvider *provider, FILE *stream)
{
	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(provider);

	_Stats copy;

	folioLock_FlagLock(&stats->lock);
	memcpy(&copy, stats, sizeof(_Stats));
	folioLock_FlagUnlock(&stats->lock);

	fprintf(stream, "\nFolioStdProvider: outstanding allocs %zu acquires %zu, currentAllocation %zu\n\n",
			copy.outstandingAllocs,
			copy.outstandingAcquires,
			folioInternalProvider_AllocationSize(provider));

	folioInternalProvider_Report(provider, stream);
}

static void
_display(const FolioMemoryProvider *provider, const void *memory, FILE *stream)
{
	folioInternalProvider_Display(provider, memory, stream);
}

static void
_setAvailableMemory(FolioMemoryProvider *provider, size_t availableMemory)
{
	folioInternalProvider_SetAvailableMemory(provider, availableMemory);
}

static size_t
_acquireCount(const FolioMemoryProvider *provider)
{
	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(provider);

	folioLock_FlagLock(&stats->lock);
	size_t count = stats->outstandingAcquires;
	folioLock_FlagUnlock(&stats->lock);
	return count;
}

static size_t
_allocationSize(const FolioMemoryProvider *provider)
{
	return folioInternalProvider_AllocationSize(provider);
}

static void
_lock(FolioMemoryProvider *provider, void *memory)
{
	folioInternalProvider_Lock(provider, memory);
}

static void
_unlock(FolioMemoryProvider *provider, void *memory)
{
	folioInternalProvider_Unlock(provider, memory);
}


