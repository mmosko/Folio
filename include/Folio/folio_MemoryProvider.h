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

#ifndef INCLUDE_FOLIO_FOLIO_MEMORYPROVIDER_H_
#define INCLUDE_FOLIO_FOLIO_MEMORYPROVIDER_H_

#include <stdio.h> // for FILE *
#include <stdint.h>
#include <stdbool.h>

/**
 * The finalizer, if set, is called when the last reference to
 * the memory is released.
 */
typedef void (*Finalizer)(void *memory);

typedef struct folioMemoryProvider_memory_provider {
	void * (*allocate)(const size_t length);
	void * (*allocateAndZero)(const size_t length);
	void * (*acquire)(const void * memory);
	size_t (*length)(const void *memory);
	void (*setFinalizer)(void *memory, Finalizer fini);
	void (*release)(void **memoryPtr);

	/**
	 * A spin-lock on the memory using atomic operations.  Contenting
	 * threads will busy-wait on the lock.  No ordering of threads and
	 * starvation is possible.
	 */
	void (*lock)(void *memory);

	/**
	 * Release the lock.  Will trap with LongBowTrapCannotObtainLockEvent
	 * if the thread id of the locker is not the same as the unlocker.
	 */
	void (*unlock)(void *memory);

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

#define folioMemoryProvider_SetAvailableMemory(provider, maximum) (provider)->setAvailableMemory(maximum)
#define folioMemoryProvider_Allocate(provider, length) (provider)->allocate(length)
#define folioMemoryProvider_AllocateAndZero(provider, length) (provider)->allocateAndZero(length);
#define folioMemoryProvider_SetFinalizer(provider, memory, fini) (provider)->setFinalizer(memory, fini)
#define folioMemoryProvider_Acquire(provider, memory) (provider)->acquire(memory)
#define folioMemoryProvider_Release(provider, memoryPtr) (provider)->release(memoryPtr)
#define folioMemoryProvider_Length(provider, memory) (provider)->length(memory)
#define folioMemoryProvider_Report(provider, stream) (provider)->report(stream)
#define folioMemoryProvider_Validate(provider, memory) (provider)->validate(memory)
#define folioMemoryProvider_OustandingReferences(provider) (provider)->acquireCount()
#define folioMemoryProvider_AllocatedBytes(provider) (provider)->allocationSize()
#define folioMemoryProvider_Lock(provider, memory) (provider)->lock(memory)
#define folioMemoryProvider_Unlock(provider, memory) (provider)->unlock(memory)

/**
 * Tests if the current number of Acquires is equal to the expected reference count.
 * If it is not, the function will display the provided message and return false.
 *
 * @return true The current Acquire count is equal to the expected reference count
 * @return false outstanding acquires not equal to execpted reference count
 */
bool folioMemoryProvider_TestRefCount(FolioMemoryProvider const *provider, size_t expectedRefCount, FILE *stream, const char *format, ...);

#endif /* INCLUDE_FOLIO_FOLIO_MEMORYPROVIDER_H_ */
