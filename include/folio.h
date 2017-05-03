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

#ifndef FOLIO_H_
#define FOLIO_H_

#include <stdio.h>
#include <stdbool.h>

/**
 * The finalizer, if set, is called when the last reference to
 * the memory is released.
 */
typedef void (*Finalizer)(void *memory);

typedef struct folio_memory_provider {
	void * (*allocate)(const size_t length);
	void * (*allocateAndZero)(const size_t length);
	void * (*acquire)(const void * memory);
	size_t (*length)(const void *memory);
	void (*setFinalizer)(void *memory, Finalizer fini);
	void (*release)(void **memoryPtr);

	/**
	 * Report statistics about the allocator
	 */
	void (*report)(FILE *stream);

	/**
	 * Sanity checks on the allocated memory
	 */
	void (*validate)(const void *memory);

	/**
	 * The current number of outstanding Acquires
	 */
	size_t (*acquireCount)(void);

	/**
	 * The total bytes of memory requested by the current
	 * allocations.  Not that multiple references to the same
	 * memory do not increase this number.
	 *
	 * The allocation size is the amount of user memory requested.
	 * Actual memory used will be approximately 40 bytes more per
	 * allocation (not acquire).
	 */
	size_t (*allocationSize)(void);

	/**
	 * Set the maximum amount of user memory available to the allocator.
	 *
	 * Allocations beyond this value will result in a trapOutOfMemory().
	 *
	 * The allocator will use more physical memory than the value set because
	 * there is some fixed overhead per allocation (40 bytes or so, depending
	 * on how the compiler aligns structures and if using a 64-bit machine).
	 *
	 * You may set this at any time, though if set after some allocations
	 * have taken place it will not trap until the next allocation.
	 */
	void (*setAvailableMemory)(size_t bytes);
} FolioMemoryProvider;

/**
 * Should be done once (and only once) at program startup.
 *
 * Defaults to FolioStdProvider if not otherwise set.
 */
void folio_SetProvider(FolioMemoryProvider *provider);

/**
 * Returns the active allocator.
 */
const FolioMemoryProvider *folio_GetProvider(void);

void * folio_Allocate(const size_t length);

/**
 * Allocates the memory and zeros it
 */
void * folio_AllocateAndZero(const size_t length);

/**
 * Associates a finalizer with the memory.  The allocator will call
 * the finalizer when it is about to release the last reference to the memory.
 *
 * Example
 * <code>
 * static void
 * _finalizer(void *memory)
 * {
 *    QueueReader *reader = memory;
 *    packetQueue_Release(&reader->pqueue);
 * }
 *
 * QueueReader *
 * queueReader_Create(int queueNumber)
 * {
 *    QueueReader *reader = folio_AllocateAndZero(sizeof(QueueReader));
 *    folio_SetFinalizer(reader, _finalizer);
 *    reader->pqueue = packetQueue_Create(128, 1024);
 *   return reader;
 * }
 * </code>
 */
void folio_SetFinalizer(void *memory, Finalizer fini);

/**
 * Obtain a reference count to the memory.  Use folio_Release() to free it.
 */
void * folio_Acquire(const void *memory);

/**
 * Releases a reference to the memory.
 *
 * When the last reference is released, the allocator will call the
 * finalizer (if set via folio_SetFinalizer()) before the memory is freed.
 */
void folio_Release(void **memoryPtr);

/**
 * The number of bytes in this allocation.
 */
size_t folio_Length(const void *memory);

/**
 * Report memory statistics
 */
void folio_Report(FILE *stream);

/**
 * Sanity checks on the memory allocation.
 */
void folio_Validate(const void *memory);

/**
 * The total number of outstanding references in the allocator
 */
size_t folio_OustandingReferences(void);

/**
 * The total number of outstanding bytes allocated to the user.
 *
 * The actual number of bytes allocated will be a small constant above
 * this number due to overhead in the allocator (approximately 40 bytes per
 * allocation).
 */
size_t folio_AllocatedBytes(void);

/**
 * Tests if the current number of Acquires is equal to the expected reference count.
 * If it is not, the function will display the provided message and return false.
 *
 * @return true The current Acquire count is equal to the expected reference count
 * @return false outstanding acquires not equal to execpted reference count
 */
bool folio_TestRefCount(size_t expectedRefCount, FILE *stream, const char *format, ...);

#endif /* FOLIO_H_ */
