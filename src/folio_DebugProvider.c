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

#include "folio_DebugProvider.h"

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>

#define DEBUG_ALLOC_LIST 0

static size_t _maximumMemory = SIZE_MAX;
static uint32_t _backtrace_depth = 10;
static uint32_t _backtrace_offset = 2;

static void * _allocate(const size_t length);
static void * _allocateAndZero(const size_t length);
static void * _acquire(const void *memory);
static size_t _length(const void *memory);
static void _release(void **memoryPtr);
static void _report(FILE *stream);
static void _validate(const void *memory);
static size_t _acquireCount(void);
static size_t _allocationSize(void);
static void _setFinalizer(void *memory, Finalizer fini);
static void _memory_Display(const void *memory, FILE *stream);


typedef struct internal_entry_t InternalEntry;
typedef struct internal_list_t InternalList;
static void _internalEntry_Display(const InternalEntry *entry, FILE *stream, const char *tag) __attribute__((unused));
static void _internalList_Display(const InternalList *list, FILE *stream, const char *tag) __attribute__((unused));

FolioMemoryProvider FolioDebugProvider = {
	.allocate = _allocate,
	.allocateAndZero = _allocateAndZero,
	.acquire = _acquire,
	.length = _length,
	.setFinalizer = _setFinalizer,
	.release = _release,
	.report = _report,
	.validate = _validate,
	.acquireCount = _acquireCount,
	.allocationSize = _allocationSize
};

// These are some random numbers
static const uint64_t _magic1 = 0x05fd095c69493bf8ULL;
static const uint32_t _magic2 = 0xf2beea1b;
static const uint64_t _magic3 = 0x05fd095c69493bf8ULL;

typedef struct stats_ {
	// The bytes allocated - bytes freed
	size_t currentAllocation;

	size_t outstandingAllocs;
	size_t outstandingAcquires;
} _Stats;

static _Stats _stats = { 0, 0, 0 };

/* ********************************************************************
 * Internal linked list for storing allocations
 *
 * Cannot use memory_LinkedList because of circular reference problems
 *
 * Doubly linked list.  This allows us to store a reference in the memory
 * allocation to it's location in the linked list and remove it in time O(1).
 */

struct internal_entry_t {
	void *data;
	InternalEntry *prev;
	InternalEntry *next;
};

struct internal_list_t {
	InternalEntry *head;
	InternalEntry *tail;
};

static InternalEntry *
_internalEntry_Create(void *memory)
{
	InternalEntry *entry = malloc(sizeof(InternalEntry));
	entry->data = memory;
	entry->next = NULL;
	entry->prev = NULL;
	return entry;
}

static void
_internalEntry_Release(InternalEntry **memoryPtr)
{
	free(*memoryPtr);
	*memoryPtr = NULL;
}

static void *
_internalEntry_GetData(const InternalEntry *entry)
{
	return entry->data;
}

static void
_internalEntry_Display(const InternalEntry *entry, FILE *stream, const char *tag)
{
	fprintf(stream, "%-10s: InternalEntry (%p): prev %p next %p data %p\n",
			tag,
			(void *) entry,
			(void *) entry->prev,
			(void *) entry->next,
			(void *) entry->data);

	fprintf(stream, "%-10s: ", tag);
	_memory_Display(entry->data, stream);
}

static void
_internalList_Display(const InternalList *list, FILE *stream, const char *tag)
{
	fprintf(stream, "%-10s: InternalList (%p): head %p tail %p\n",
			tag,
			(void *) list,
			(void *) list->head,
			(void *) list->tail);
}
static void *
_internalList_RemoveAt(InternalList *list, InternalEntry *entry)
{
#if DEBUG
	_internalList_Display(list, stdout, "++RemoveAt");
	_internalEntry_Display(entry, stdout, "++RemoveAt");
#endif

	if (list->head == entry) {
		list->head = list->head->next;
		if (list->head) {
			list->head->prev = NULL;
		} else {
			list->tail = NULL;
		}
	} else if (list->tail == entry) {
		list->tail = list->tail->prev;
		assertNotNull(list->tail, "Invalid state, if tail has no prev, it should have been the head");
		list->tail->next = NULL;
	} else {
		// it is in the middle somewhere
		InternalEntry *next = entry->next;
		InternalEntry *prev = entry->prev;

		prev->next = next;
		next->prev = prev;
	}

	void *data = _internalEntry_GetData(entry);
	_internalEntry_Release(&entry);

#if DEBUG
	_internalList_Display(list, stdout, "--");
#endif

	return data;
}

static InternalEntry *
_internalList_Append(InternalList *list, void *data)
{
	InternalEntry *entry = _internalEntry_Create(data);
#if DEBUG
	_internalList_Display(list, stdout, "++Append");
	_internalEntry_Display(entry, stdout, "++Append");
#endif

	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		list->tail->next = entry;
		entry->prev = list->tail;
		list->tail = entry;
	}

#if DEBUG
	_internalList_Display(list, stdout, "--");
	_internalEntry_Display(entry, stdout, "--");
#endif
	return entry;
}

static void
_internalList_ForEach(InternalList *list, void (callback)(const void *memory, void *closure), void *closure)
{
#if DEBUG
	_internalList_Display(list, stdout, "++ForEach");
#endif

	InternalEntry *entry = list->head;
	while (entry) {
		#if DEBUG
			_internalEntry_Display(entry, stdout, "==");
		#endif
		callback(_internalEntry_GetData(entry), closure);
		entry = entry->next;
	}

#if DEBUG
	_internalList_Display(list, stdout, "--");
#endif
}

static InternalList _privateAllocList = {
		.head = NULL,
		.tail = NULL
};

static InternalList *_allocList = &_privateAllocList;
static pthread_mutex_t _allocListLock = PTHREAD_MUTEX_INITIALIZER;

/* ********************************************************************/

#define HEADER_BYTES ( \
				__SIZEOF_LONG_LONG__ + \
				__SIZEOF_SIZE_T__ + \
				__SIZEOF_POINTER__ + \
				__SIZEOF_POINTER__ + \
				__SIZEOF_POINTER__ + \
				__SIZEOF_INT__ + \
				__SIZEOF_INT__)

#define PAD_BYTES (48 - HEADER_BYTES)

typedef struct header_t {
	// This structure should always be a multiple of 8 bytes to keep
	// the user memory aligned.
	uint64_t magic1;

	// The requested allocation length
	size_t requestedLength;

	Finalizer fini;

	LongBowBacktrace *backtrace;

	InternalEntry *allocListEntry;

	// pad out to alignment size bytes
	// This ensures that magic2 will be adjacent to the user space
#if PAD_BYTES > 0
	uint8_t  pad[PAD_BYTES];
#endif

	uint32_t refcount;
	uint32_t magic2;
} __attribute__ ((aligned)) Header;

typedef struct trailer_t {
	uint64_t magic3;
} Trailer;



static Header *
_getHeader(const void *memory)
{
	return (Header *) (memory - sizeof(Header));
}

static Trailer *
_getTrailer(const Header *header)
{
	Trailer *tail = (Trailer *) (((uint8_t *) header) + sizeof(Header) + header->requestedLength);
	return tail;
}

static void
_validateMemoryLocation(const void *memory)
{
	// the smallest possible memory location we could have
	const void *bottom = (const void *) sizeof(Header);
	trapIllegalValueIf(memory < bottom, "Invalid memory location");
}

static void
_validateInternal(const Header *header)
{
	if (header->magic1 == _magic1 && header->magic2 == _magic2) {
		if (header->refcount == 0) {
			printf("\n\nHeader:\n");
			longBowDebug_MemoryDump((const char *) header, sizeof(Header));
			trapUnexpectedState("Memory: refcount is zero");
		}

		const Trailer *tail = _getTrailer(header);
		if (tail->magic3 != _magic3) {
			printf("\n\nTrailer:\n");
			longBowDebug_MemoryDump((const char *) tail, sizeof(Trailer));
			trapUnexpectedState("Memory: invalid trailer (memory overrun)");
		}
	} else {
		printf("\n\nHeader:\n");
		longBowDebug_MemoryDump((const char *) header, sizeof(Header));
		trapUnexpectedState("Memory: invalid header (memory underrun)");
	}
}

static size_t
_calculateTotalAllocation(size_t requestedLength)
{
	return sizeof(Header) + sizeof(Trailer) + requestedLength;
}

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
			header->refcount,
			(void *) header,
			(void *) _getTrailer(header)
			);
}

static void
_increaseCurrentAllocation(const size_t length)
{
	size_t newAllocation = _stats.currentAllocation + length;

	// Because it is unsigned, if the new value is less than _stats.currentAllocation we had overflow
	trapOutOfMemoryIf(newAllocation < _stats.currentAllocation, "Allocation overflows bounds");

	trapOutOfMemoryIf(_maximumMemory < newAllocation, "Allocation exceeds maximum memory");

	_stats.currentAllocation = newAllocation;
}

static void
_decreaseCurrentAllocation(const size_t length)
{
	trapUnexpectedStateIf(_stats.currentAllocation < length, "current allocation less than length");
	_stats.currentAllocation -= length;
}

static void *
_allocate(const size_t length)
{
	size_t totalLength = _calculateTotalAllocation(length);
	void *memory = malloc(totalLength);
	Header *head = memory;
	void *user = memory + sizeof(Header);

	head->magic1 = _magic1;
	head->requestedLength = length;
	head->refcount = 1;
	head->fini = NULL;
	head->backtrace = longBowBacktrace_Create(_backtrace_depth, _backtrace_offset);
	head->magic2 = _magic2;

	Trailer *tail = _getTrailer(head);
	tail->magic3 = _magic3;

	pthread_mutex_lock(&_allocListLock);
	head->allocListEntry = _internalList_Append(_allocList, user);
	pthread_mutex_unlock(&_allocListLock);

	_increaseCurrentAllocation(length);
	_stats.outstandingAcquires++;
	_stats.outstandingAllocs++;

	return user;
}

static void *
_allocateAndZero(const size_t length)
{
	void * memory = _allocate(length);
	memset(memory, 0, length);
	return memory;
}

static void
_setFinalizer(void *memory, Finalizer fini)
{
	_validateMemoryLocation(memory);
	Header *header = _getHeader(memory);
	header->fini = fini;
}

static void
_validate(const void *memory)
{
	_validateMemoryLocation(memory);
	const Header *header = _getHeader(memory);

	_validateInternal(header);
}

static void *
_acquire(const void *memory)
{
	_validateMemoryLocation(memory);
	Header *header = _getHeader(memory);

	_validateInternal(header);

	_stats.outstandingAcquires++;
	header->refcount++;
	return (void *) memory;
}

static size_t
_length(const void *memory)
{
	_validateMemoryLocation(memory);
	Header *header = _getHeader(memory);

	_validateInternal(header);

	return header->requestedLength;
}

static void
_release(void **memoryPtr)
{
	trapIllegalValueIf(memoryPtr == NULL, "Null memory pointer");

	void *memory = *memoryPtr;
	_validateMemoryLocation(memory);

	Header *header = _getHeader(memory);

	_validateInternal(header);

	header->refcount--;

	_stats.outstandingAcquires--;

	if (header->refcount == 0) {
		if (header->fini != NULL) {
			header->fini(memory);
		}

		if (header->allocListEntry != NULL) {
			pthread_mutex_lock(&_allocListLock);
			_internalList_RemoveAt(_allocList, header->allocListEntry);
			pthread_mutex_unlock(&_allocListLock);
		}

		_decreaseCurrentAllocation(header->requestedLength);
		_stats.outstandingAllocs--;

		longBowBacktrace_Destroy(&header->backtrace);

		// wipe out the whole allocation so not useful anymore
		memset(header, 1, _calculateTotalAllocation(header->requestedLength));
		free(header);
	}

	*memoryPtr = NULL;
}

static void
_report(FILE *stream)
{
	fprintf(stream, "\nMemoryDebugAlloc: outstanding allocs %zu acquires %zu, currentAllocation %zu\n\n",
			_stats.outstandingAllocs,
			_stats.outstandingAcquires,
			_stats.currentAllocation);
}

void
memoryDebugAlloc_SetAvailableMemory(size_t availableMemory)
{
	_maximumMemory = availableMemory;
}

static size_t
_acquireCount(void)
{
	return _stats.outstandingAcquires;
}

static size_t
_allocationSize(void)
{
	return _stats.currentAllocation;
}

// This is a private function of longbow
extern void longBowMemory_Deallocate(void **pointerPointer);

static void
_internalBacktrace(const void *memory, void *voidStream)
{
	FILE *stream = voidStream;

	_validateMemoryLocation(memory);
	Header *header = _getHeader(memory);
	_validateInternal(header);

	char *str = longBowBacktrace_ToString(header->backtrace);
	fprintf(stream, "\nMemory Backtrace (%p)\n%s\n\n", memory, str);

	longBowMemory_Deallocate((void **) &str);
}

void
folioDebugAlloc_Backtrace(const void *memory, FILE *stream)
{
	_internalBacktrace(memory, stream);
}

void
folioDebugProvider_DumpBacktraces(FILE *stream)
{
	pthread_mutex_lock(&_allocListLock);
	_internalList_ForEach(_allocList, _internalBacktrace, stream);
	pthread_mutex_unlock(&_allocListLock);
}

static void
_internalValidateCallback(const void *memory, __attribute__((unused)) void *dummy)
{
	_validate(memory);
}

void
folioDebugProvider_ValidateAll(void)
{
	pthread_mutex_lock(&_allocListLock);
	_internalList_ForEach(_allocList, _internalValidateCallback, NULL);
	pthread_mutex_unlock(&_allocListLock);
}

