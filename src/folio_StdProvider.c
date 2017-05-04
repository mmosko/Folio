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
static void _setAvailableMemory(size_t maximum);

static void _lock(void *memory);
static void _unlock(void *memory);

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
	.allocationSize = _allocationSize,
	.setAvailableMemory = _setAvailableMemory
};

// These are some random numbers
static const uint64_t _magic1 = 0x5c695da341472e96ULL;
static const uint32_t _magic2 = 0x2f059a66;
static const uint64_t _magic3 = 0xd4d44a5c6b617c2cULL;

typedef struct stats {
	atomic_flag spinLock;

	// The bytes allocated - bytes freed
	size_t currentAllocation;

	size_t outstandingAllocs;
	size_t outstandingAcquires;

	// number of allocations attempted but no memory available
	size_t outOfMemoryCount;
} _Stats;

static _Stats _stats = {
		.spinLock = ATOMIC_FLAG_INIT,  // clear state
		.currentAllocation = 0,
		.outstandingAllocs = 0,
		.outstandingAcquires = 0,
		.outOfMemoryCount = 0
};

/*
 * blocks in a busy loop until we acquire the spinlock.
 */
static void
_statsLock(void) {
	bool prior;
	do {
		// set the spin lock to TRUE.
		// We acquire the lock iff the prior value was FALSE
		prior = atomic_flag_test_and_set(&_stats.spinLock);
	} while (prior);
}

/*
 * Clears the spinlock.
 * precondition: you are holding the lock
 */
static void
_statsUnlock(void) {
	atomic_flag_clear(&_stats.spinLock);
}

#if 0
#define HEADER_BYTES ( \
				__SIZEOF_LONG_LONG__ + \
				__SIZEOF_SIZE_T__ + \
				__SIZEOF_POINTER__ + \
				__SIZEOF_ATOMIC_INT__ + \
				__SIZEOF_INT__)
#endif

#define HEADER_BYTES ( 40 )

#define PAD_BYTES (64 - HEADER_BYTES)

typedef struct header_t {
	// This structure should always be a multiple of 8 bytes to keep
	// the user memory aligned.
	uint64_t magic1;

	// The requested allocation length
	size_t requestedLength;

	Finalizer fini;

	FolioLock *lock;

	atomic_int referenceCount;

	// TODO: Should restructure this to remove magic2 and make sure
	// we have atleast 4 PAD_BYTES so we can fill those with a guard value.

	// pad out to alignment size bytes
	// This ensures that magic2 will be adjacent to the user space
#if PAD_BYTES > 0
	uint8_t  pad[PAD_BYTES];
#endif

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

static int
_referenceCount(const Header *header)
{
	return atomic_load(&header->referenceCount);
}

static void
_validateInternal(const Header *header)
{
	if (header->magic1 == _magic1 && header->magic2 == _magic2) {
		int refcount = _referenceCount(header);
		if (refcount == 0) {
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

/*
 * precondition: you hold the stats lock
 *
 * @return true if allocation of length bytes is ok
 * @return false if out of memory
 */
static bool
_increaseCurrentAllocation(const size_t length)
{
	bool memoryIsAvailable = false;

	if (_maximumMemory >= _stats.currentAllocation) {
		size_t remainingMemory = _maximumMemory - _stats.currentAllocation;

		if (length <= remainingMemory) {
			memoryIsAvailable = true;
			_stats.currentAllocation = _stats.currentAllocation + length;
		}
	}
	return memoryIsAvailable;
}

/*
 * precondition: you hold the stats lock
 */
static void
_decreaseCurrentAllocation(const size_t length)
{
	trapIllegalValueIf(_stats.currentAllocation < length, "current allocation less than length");
	_stats.currentAllocation -= length;
}

static void *
_allocate(const size_t length)
{
	void *user = NULL;

	_statsLock();
	bool memoryIsAvailable = _increaseCurrentAllocation(length);
	if (memoryIsAvailable) {
		_stats.outstandingAcquires++;
		_stats.outstandingAllocs++;
	} else {
		_stats.outOfMemoryCount++;
	}
	_statsUnlock();

	if (memoryIsAvailable) {
		size_t totalLength = sizeof(Header) + sizeof(Trailer) + length;
		void *memory = malloc(totalLength);
		Header *head = memory;

		head->magic1 = _magic1;
		head->requestedLength = length;
		head->referenceCount = ATOMIC_VAR_INIT(1);
		head->fini = NULL;
		head->magic2 = _magic2;

		user = memory + sizeof(Header);

		Trailer *tail = _getTrailer(head);
		tail->magic3 = _magic3;
	}

	return user;
}

static void *
_allocateAndZero(const size_t length)
{
	void * memory = _allocate(length);
	if (memory) {
		memset(memory, 0, length);
	}
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

	// prior value must be greater than 0.  If it is not positive it means someone
	// freed the memory during the time between _validateInternal and now.
	int prior = atomic_fetch_add_explicit(&header->referenceCount, 1, memory_order_relaxed);
	if (prior < 1) {
		trapUnrecoverableState("The memory %p was freed during acquire", (void *) memory);
	}

	_statsLock();
	_stats.outstandingAcquires++;
	_statsUnlock();

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

	int prior = atomic_fetch_sub(&header->referenceCount, 1);
	trapIllegalValueIf(prior < 1, "Reference count was %d < 1 when trying to release", prior);

	if (prior == 1) {
		if (header->fini != NULL) {
			header->fini(memory);
		}

		_statsLock();
		_decreaseCurrentAllocation(header->requestedLength);
		_stats.outstandingAcquires--;
		_stats.outstandingAllocs--;
		_statsUnlock();

		// Write over magic1 so it invalidates the block
		memset(header, 0, sizeof(uint64_t));
		free(header);
	} else {
		_statsLock();
		_stats.outstandingAcquires--;
		_statsUnlock();
	}

	*memoryPtr = NULL;
}

static void
_report(FILE *stream)
{
	_Stats copy;

	_statsLock();
	memcpy(&copy, &_stats, sizeof(_Stats));
	_statsUnlock();

	fprintf(stream, "\nMemoryDebugAlloc: outstanding allocs %zu acquires %zu, currentAllocation %zu\n\n",
			copy.outstandingAllocs,
			copy.outstandingAcquires,
			copy.currentAllocation);
}

static void
_setAvailableMemory(size_t availableMemory)
{
	_maximumMemory = availableMemory;
}

static size_t
_acquireCount(void)
{
	_statsLock();
	size_t count = _stats.outstandingAcquires;
	_statsUnlock();
	return count;
}

static size_t
_allocationSize(void)
{
	_statsLock();
	size_t count = _stats.currentAllocation;
	_statsUnlock();
	return count;
}

static void
_lock(void *memory)
{

}

static void
_unlock(void *memory)
{

}


