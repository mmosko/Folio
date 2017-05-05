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
#include <LongBow/longBow_Backtrace.h>

#include <Folio/folio_DebugProvider.h>
#include <Folio/private/folio_InternalList.h>
#include <Folio/private/folio_InternalProvider.h>
#include <Folio/private/folio_Lock.h>

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <stdatomic.h>

#define DEBUG_ALLOC_LIST 0

static uint32_t _backtrace_depth = 10;
static uint32_t _backtrace_offset = 2;

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

const FolioMemoryProvider FolioDebugProviderTemplate = {
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
	atomic_flag lock;

	// The bytes allocated - bytes freed
	size_t currentAllocation;

	size_t outstandingAllocs;
	size_t outstandingAcquires;

	// number of allocations attempted but no memory available
	size_t outOfMemoryCount;
} Stats;

typedef struct debug_state {
	Stats stats;
	FolioInternalList *allocationList;
} DebugState;

typedef struct debug_header_t {
	LongBowBacktrace *backtrace;
	FolioInternalEntry *allocListHandle;

} __attribute__ ((aligned)) DebugHeader;

/*
static void
_memory_Display(const void *memory, FILE *stream)
{
	_validateMemoryLocation(memory);
	const Header *header = _getHeader(memory);

	fprintf(stream, "memory (%p): len %zu fini %p btrace %p listEntry %p refcount %u (Header %p Trailer %p)\n",
			(void *) memory,
			header->requestedLength,
			(void *) header->fini,
			(void *) header->backtrace,
			(void *) header->allocListEntry,
			_referenceCount(memory),
			(void *) header,
			(void *) _getTrailer(header)
			);
}
*/

FolioMemoryProvider *
folioStdProvider_Create(size_t poolSize)
{
	FolioMemoryProvider *pool = folioInternalProvider_Create(&FolioDebugProviderTemplate, poolSize,
									sizeof(DebugState), sizeof(DebugHeader));

	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(pool);
	state->allocationList = folioInternalList_Create();

	memset(&state->stats, 0, sizeof(Stats));

	return pool;
}


static void *
_allocate(FolioMemoryProvider *provider, const size_t length)
{
	void *memory = folioInternalProvider_Allocate(provider, length);

	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(provider);
	DebugHeader *debug = (DebugHeader *) folioInternalProvider_GetProviderHeader(provider, memory);

	folioLock_FlagLock(&state->stats.lock);
	if (memory != NULL) {

		debug->backtrace = longBowBacktrace_Create(_backtrace_depth, _backtrace_offset);

		folioInternalList_Lock(state->allocationList);
		debug->allocListHandle = folioInternalList_Append(state->allocationList, memory);
		folioInternalList_Unlock(state->allocationList);

		state->stats.outstandingAcquires++;
		state->stats.outstandingAllocs++;
	} else {
		state->stats.outOfMemoryCount++;
	}
	folioLock_FlagUnlock(&state->stats.lock);

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

	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(provider);

	folioLock_FlagLock(&state->stats.lock);
	state->stats.outstandingAcquires++;
	folioLock_FlagUnlock(&state->stats.lock);

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

	void *memory = *memoryPtr;

	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(provider);

	// must extract the header before release otherwise it might go away and copy it to local
	DebugHeader *debug = (DebugHeader *) folioInternalProvider_GetProviderHeader(provider, memory);
	DebugHeader debugCopy = *debug;

	bool finalRelease = folioInternalProvider_ReleaseMemory(provider, memoryPtr);
	if (finalRelease) {

		if (debugCopy.allocListHandle != NULL) {
			folioInternalList_Lock(state->allocationList);
			folioInternalList_RemoveAt(state->allocationList, debugCopy.allocListHandle);
			folioInternalList_Unlock(state->allocationList);
		}

		longBowBacktrace_Destroy(&debugCopy.backtrace);

		folioLock_FlagLock(&state->stats.lock);
		state->stats.outstandingAcquires--;
		state->stats.outstandingAllocs--;
		folioLock_FlagUnlock(&state->stats.lock);

	} else {
		folioLock_FlagLock(&state->stats.lock);
		state->stats.outstandingAcquires--;
		folioLock_FlagUnlock(&state->stats.lock);
	}
}

static void
_report(const FolioMemoryProvider *provider, FILE *stream)
{
	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(provider);

	Stats copy;

	folioLock_FlagLock(&state->stats.lock);
	memcpy(&copy, &state->stats, sizeof(Stats));
	folioLock_FlagUnlock(&state->stats.lock);

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
	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(provider);

	folioLock_FlagLock(&state->stats.lock);
	size_t count = state->stats.outstandingAcquires;
	folioLock_FlagUnlock(&state->stats.lock);
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

/* **********
 * Debug specific functions
 */

// This is a private function of longbow
extern void longBowMemory_Deallocate(void **pointerPointer);

struct backtrace_arg {
	FILE *stream;
	FolioMemoryProvider *provider;
};

static void
_internalBacktrace(const void *memory, void *arg)
{
	struct backtrace_arg *a = arg;

	DebugHeader *debug = (DebugHeader *) folioInternalProvider_GetProviderHeader(a->provider, memory);

	char *str = longBowBacktrace_ToString(debug->backtrace);
	fprintf(a->stream, "\nMemory Backtrace (%p)\n%s\n\n", memory, str);

	longBowMemory_Deallocate((void **) &str);
}

void
folioDebugAlloc_Backtrace(FolioMemoryProvider *provider, const void *memory, FILE *stream)
{
	struct backtrace_arg arg = {
			.stream = stream,
			.provider = provider
	};

	_internalBacktrace(memory, &arg);
}

void
folioDebugProvider_DumpBacktraces(FolioMemoryProvider *provider, FILE *stream)
{
	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(provider);

	struct backtrace_arg arg = {
			.stream = stream,
			.provider = provider
	};

	folioInternalList_Lock(state->allocationList);
	folioInternalList_ForEach(state->allocationList, _internalBacktrace, &arg);
	folioInternalList_Unlock(state->allocationList);
}

static void
_internalValidateCallback(const void *memory, void *arg)
{
	FolioMemoryProvider *provider = arg;
	folioInternalProvider_Validate(provider, memory);
}

void
folioDebugProvider_ValidateAll(FolioMemoryProvider *provider)
{
	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(provider);

	folioInternalList_Lock(state->allocationList);
	folioInternalList_ForEach(state->allocationList, _internalValidateCallback, provider);
	folioInternalList_Unlock(state->allocationList);
}

