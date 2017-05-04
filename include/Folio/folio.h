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

#include <Folio/folio_MemoryProvider.h>
#include <stdio.h>
#include <stdbool.h>


/**
 * Should be done once (and only once) at program startup.
 *
 * Defaults to FolioStdProvider if not otherwise set.
 */
void folio_SetProvider(FolioMemoryProvider *provider);

/**
 * Sets the maximum amount of memory that can be allocated
 */
void folio_SetAvailableMemory(size_t maximum);

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
