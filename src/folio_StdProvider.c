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

static void * _allocate(FolioMemoryProvider *provider, const size_t length);
static void * _allocateAndZero(FolioMemoryProvider *provider, const size_t length);
static void * _acquire(FolioMemoryProvider *provider, const void *memory);
static size_t _length(const FolioMemoryProvider *provider, const void *memory);
static void _release(FolioMemoryProvider *provider, void **memoryPtr);
static void _report(const FolioMemoryProvider *provider, FILE *stream);
static void _validate(const FolioMemoryProvider *provider, const void *memory);
static size_t _acquireCount(const FolioMemoryProvider *provider);
static size_t _allocationSize(const FolioMemoryProvider *provider);
static void _setFinalizer(FolioMemoryProvider *provider, void *memory, Finalizer fini);
static void _setAvailableMemory(FolioMemoryProvider *provider, size_t maximum);

static void _lock(FolioMemoryProvider *provider, void *memory);
static void _unlock(FolioMemoryProvider *provider, void *memory);

const FolioMemoryProvider FolioStdProviderTemplate = {
	.allocate = _allocate,
	.allocateAndZero = _allocateAndZero,
	.acquire = _acquire,
	.length = _length,
	.setFinalizer = _setFinalizer,
	.release = _release,
	.report = _report,
	.validate = _validate,
	.acquireCount = _acquireCount,
	.allocationSize = _allocationSize,
	.setAvailableMemory = _setAvailableMemory,
	.lock = _lock,
	.unlock = _unlock
};

typedef struct stats {
	FolioLock *spinLock;

	// The bytes allocated - bytes freed
	size_t currentAllocation;

	size_t outstandingAllocs;
	size_t outstandingAcquires;

	// number of allocations attempted but no memory available
	size_t outOfMemoryCount;
} _Stats;

FolioMemoryProvider *
folioStdProvider_Create(size_t poolSize)
{
	FolioMemoryProvider *pool = folioInternalProvider_Create(&FolioStdProviderTemplate, poolSize, sizeof(_Stats), 0);
	return pool;
}


static void *
_allocate(FolioMemoryProvider *provider, const size_t length)
{
	void *memory = folioInternalProvider_Allocate(provider, length);

	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(provider);

	folioLock_Lock(stats->spinLock);
	if (memory != NULL) {
		stats->outstandingAcquires++;
		stats->outstandingAllocs++;
	} else {
		stats->outOfMemoryCount++;
	}
	folioLock_Unlock(stats->spinLock);

	return memory;
}

static void *
_allocateAndZero(FolioMemoryProvider *provider, const size_t length)
{
	void * memory = _allocate(provider, length);
	if (memory) {
		memset(memory, 0, length);
	}
	return memory;
}

static void
_setFinalizer(FolioMemoryProvider *provider, void *memory, Finalizer fini)
{
	folioInternalProvider_SetFinalizer(provider, memory, fini);
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

	folioLock_Lock(stats->spinLock);
	stats->outstandingAcquires++;
	folioLock_Unlock(stats->spinLock);

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
		folioLock_Lock(stats->spinLock);
		stats->outstandingAcquires--;
		stats->outstandingAllocs--;
		folioLock_Unlock(stats->spinLock);
	} else {
		folioLock_Lock(stats->spinLock);
		stats->outstandingAcquires--;
		folioLock_Unlock(stats->spinLock);
	}
}

static void
_report(const FolioMemoryProvider *provider, FILE *stream)
{
	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(provider);

	_Stats copy;

	folioLock_Lock(stats->spinLock);
	memcpy(&copy, stats, sizeof(_Stats));
	folioLock_Unlock(stats->spinLock);

	fprintf(stream, "\nMemoryDebugAlloc: outstanding allocs %zu acquires %zu, currentAllocation %zu\n\n",
			copy.outstandingAllocs,
			copy.outstandingAcquires,
			folioInternalProvider_AllocationSize(provider));
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

	folioLock_Lock(stats->spinLock);
	size_t count = stats->outstandingAcquires;
	folioLock_Unlock(stats->spinLock);
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


