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

#ifndef INCLUDE_FOLIO_PRIVATE_FOLIO_HEADER_H_
#define INCLUDE_FOLIO_PRIVATE_FOLIO_HEADER_H_

#include <Folio/folio_Finalizer.h>
#include <Folio/private/folio_Lock.h>
#include <Folio/private/folio_Pool.h>
#include <stdatomic.h>
#include <stdbool.h>

/**
 * The header of every memory allocation.
 *
 * It is defined here in the clear because FolioStdProvider offers a statically
 * allocated memory provider, so we need to know all the lengths of the structs.
 *
 * FolioHeader is an aligned structure.  providerHeaderLength is not necessarily
 * and aligned value, so the guard could start on a non-aligned byte.  guardLength
 * will always bring the overall header to an aligned length so the user memory
 * starts on an aligned boundary.
 *
 *
 * The header really looks like this:
 *
 * | FolioHeader ......  | providerHeader ..... | guard ..... | user memory .... | guard ... | trailer |
 * | sizeof(FolioHeader) | providerHeaderLength | guardLength |
 * | ................ aligned ((sizeof(void *))) ............ |
 */
typedef struct folio_header {
	uint32_t xmagic1;
	atomic_int xreferenceCount;

	// The requested allocation length
	size_t xrequestedLength;

	Finalizer xfini;

	FolioLock *xlock;

	uint8_t xheaderGuardLength;
	uint8_t xtrailerGuardLength;
	uint16_t xproviderDataLength;

	uint32_t xmagic2;
} FolioHeader __attribute__((aligned));

typedef struct folio_trailer {
	uint32_t magic3;
} FolioTrailer __attribute__((aligned));

FolioHeader *folioHeader_GetMemoryHeader(const void *memory, const FolioPool *pool);

/**
 * Initialize a header.  The header must exist in allocated memory.
 *
 * You must call folioHeader_Finalize() when done with the header (to free
 * the reference to the lock).
 *
 * Example
 * <code>
 * void foo() {
 *    FolioHeader header;
 *    FolioLock *lock = folioLock_Create();
 *    folioHeader_Initialize(&header, 123, 500, lock, 1, NULL, 0);
 *    folioLock_Release(&lock);
 * }
 * </code>
 *
 * @param header The header structure to initialize
 * @param magic The value to use in the two magic fields that protect the front and rear of the header
 * @param requestedLength The amount of memory requested by the user
 * @param lock The FolioLock to use (stores a reference to the lock, must not be NULL)
 * @param refCount The value to set the refcount to (usually 1)
 * @param providerDataLength the additional bytes for a provider's storage
 * @param fini The finalizer for the memory (may be NULL)
 * @param guardLength The number of guard bytes past magic2 (may be 0)
 */
void folioHeader_Initialize(FolioHeader *header, uint32_t magic, size_t requestedLength,
		const FolioLock *lock, int refCount, size_t providerHeaderLength, Finalizer fini,
		size_t headerGuardLength, size_t trailerGuardLength);

void folioHeader_Finalize(FolioHeader *header);

/**
 * Increments the reference count stored in the header.  This is an atomic operation.
 *
 * @return The prior reference count before increment
 */
int folioHeader_IncrementReferenceCount(FolioHeader *header);

/**
 * Subtracts one from the reference count stored in the header.  This is an atomic operation.
 *
 * @return The prior reference count before decrement.
 */
int folioHeader_DecrementReferenceCount(FolioHeader *header);

/**
 * The number of bytes requested by the user and available in this allocation.
 */
size_t folioHeader_GetRequestedLength(const FolioHeader *header);

/**
 * If the finalizer is non-null, execute it with the provided memory pointer
 */
void folioHeader_ExecuteFinalizer(FolioHeader *header, void *memory);

/**
 * Locks the memory allocation.  This is callable (indirectly) by the user
 * to lock a given memory allocation.
 *
 * This uses a spinlock, so it is not meant for long locks.
 */
void folioHeader_Lock(FolioHeader *header);

/**
 * Unlocks the memory allocation.  Must be called by the lock holder (same thread id).
 */
void folioHeader_Unlock(FolioHeader *header);

/**
 * The number of outstanding acquires on the memory allocation.
 */
int folioHeader_ReferenceCount(const FolioHeader *header);

/**
 * The FolioMemoryProvider implementation class (e.g. FolioStdProvider) can store its own
 * data in the header block.  This returns the number of bytes reserved for that header.
 */
size_t folioHeader_ProviderDataLength(const FolioHeader *header);

/**
 * The number of bytes between the end of the user memory and the trailer
 */
size_t folioHeader_GetTrailerGuardLength(const FolioHeader *header);

FolioTrailer *folioHeader_GetTrailer(const FolioHeader *header, const FolioPool *pool);

const uint8_t *folioHeader_GetHeaderGuardAddress(const FolioHeader *header);

const uint8_t *folioHeader_GetTrailerGuardAddress(const FolioHeader *header, const FolioPool *pool);

/**
 * Compare the two magic fields in the header with the given value.
 *
 * @return true If both fields match
 * @return false If one or both mismatch
 */
bool folioHeader_CompareMagic(const FolioHeader *header, const uint32_t magic);

/**
 * Invalidate the header so it will not pass the folioHeader_CompareMagic() test with
 * the expected magic.  This is typically done just before freeing the memory associated
 * with the header so erroneous use of the memory later will fail.
 */
void folioHeader_Invalidate(FolioHeader *header);

/**
 * Represents the header as a string.  You must free the returned pointer
 * with free().
 */
char *folioHeader_ToString(const FolioHeader *header);

/**
 * Represents the trailer as a string.  You must free the returned pointer with free().
 *
 * @param trailer The trailer to convert to string
 * @param guardLength The number of bytes allocated to the trailer's guard (a property of the FolioPool)
 */
char *folioTrailer_ToString(const FolioTrailer *trailer, size_t guardLength);

#endif /* INCLUDE_FOLIO_PRIVATE_FOLIO_HEADER_H_ */
