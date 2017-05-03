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
#include "folio_StdProvider.h"

static size_t _maximumMemory = SIZE_MAX;

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

FolioMemoryProvider FolioStdProvider = {
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
static const uint64_t _magic1 = 0x5c695da341472e96ULL;
static const uint32_t _magic2 = 0x2f059a66;
static const uint64_t _magic3 = 0xd4d44a5c6b617c2cULL;

typedef struct stats {
	// The bytes allocated - bytes freed
	size_t currentAllocation;

	size_t outstandingAllocs;
	size_t outstandingAcquires;
} _Stats;

static _Stats _stats = { 0, 0, 0 };

#define HEADER_BYTES ( \
				__SIZEOF_LONG_LONG__ + \
				__SIZEOF_SIZE_T__ + \
				__SIZEOF_POINTER__ + \
				__SIZEOF_INT__ + \
				__SIZEOF_INT__)

#define PAD_BYTES (32 - HEADER_BYTES)

typedef struct header_t {
	// This structure should always be a multiple of 8 bytes to keep
	// the user memory aligned.
	uint64_t magic1;

	// The requested allocation length
	size_t requestedLength;

	Finalizer fini;

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
		trapUnexpectedStateIf(header->refcount == 0, "Memory: refcount is zero");

		const Trailer *tail = (const Trailer *) (((const uint8_t *) header) + sizeof(Header) + header->requestedLength);
		if (tail->magic3 != _magic3) {
			trapUnexpectedState("Memory: invalid trailer (memory overrun)");
		}
	} else {
		trapUnexpectedState("Memory: invalid header (memory underrun)");
	}
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
	_increaseCurrentAllocation(length);

	_stats.outstandingAcquires++;
	_stats.outstandingAllocs++;

	size_t totalLength = sizeof(Header) + sizeof(Trailer) + length;
	void *memory = malloc(totalLength);
	Header *head = memory;
	void *user = memory + sizeof(Header);
	Trailer *tail = memory + sizeof(Header) + length;

	head->magic1 = _magic1;
	head->requestedLength = length;
	head->refcount = 1;
	head->fini = NULL;
	head->magic2 = _magic2;
	tail->magic3 = _magic3;

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

		_decreaseCurrentAllocation(header->requestedLength);
		// wipe out the header so it is no longer valid
		memset(header, 0, sizeof(Header));
		free(header);
		_stats.outstandingAllocs--;
	}

	*memoryPtr = NULL;
}

static void
_report(FILE *stream)
{
	fprintf(stream, "\nMemoryStdAlloc: outstanding allocs %zu acquires %zu, currentAllocation %zu\n\n",
			_stats.outstandingAllocs,
			_stats.outstandingAcquires,
			_stats.currentAllocation);
}

void
memoryStdAlloc_SetAvailableMemory(size_t availableMemory)
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

