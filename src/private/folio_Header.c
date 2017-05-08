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

#define _GNU_SOURCE
#include <LongBow/runtime.h>
#include <Folio/private/folio_Header.h>
#include <stdio.h>
#include <stdatomic.h>
#include <inttypes.h>

FolioHeader *
folioHeader_GetMemoryHeader(const void *memory, const FolioPool *pool)
{
	size_t length = pool->headerAlignedLength;
	trapUnexpectedStateIf(memory < (void *) length, "Invalid memory address (%p)", memory);
	return (FolioHeader *) ((uint8_t *) memory - pool->headerAlignedLength);
}


void
folioHeader_Initialize(FolioHeader *header, uint32_t magic, size_t requestedLength, const FolioLock *lock,
		int refCount, size_t providerDataLength, Finalizer fini, size_t headerGuardLength, size_t trailerGuardLength)
{
	header->xmagic1 = magic;
	header->xrequestedLength = requestedLength;
	header->xlock = folioLock_Acquire(lock);
	header->xreferenceCount = ATOMIC_VAR_INIT(refCount);
	header->xfini = fini;
	header->xproviderDataLength = providerDataLength;
	header->xheaderGuardLength = headerGuardLength;
	header->xtrailerGuardLength = trailerGuardLength;
	header->xmagic2 = magic;
}

void
folioHeader_Finalize(FolioHeader *header)
{
	folioLock_Release(&header->xlock);
}

size_t
folioHeader_GetRequestedLength(const FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	return header->xrequestedLength;
}

Finalizer
folioHeader_GetFinalizer(const FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	return header->xfini;
}

void
folioHeader_SetFinalizer(FolioHeader *header, Finalizer fini)
{
	assertNotNull(header, "header must be non-null");
	header->xfini = fini;
}

void
folioHeader_Lock(FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	folioLock_Lock(header->xlock);
}

void
folioHeader_Unlock(FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	folioLock_Unlock(header->xlock);
}

int
folioHeader_ReferenceCount(const FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	return atomic_load(&header->xreferenceCount);
}

size_t
folioHeader_ProviderDataLength(const FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	return header->xproviderDataLength;
}

bool
folioHeader_CompareMagic(const FolioHeader *header, const uint32_t magic)
{
	assertNotNull(header, "header must be non-null");
	return (header->xmagic1 == magic && header->xmagic2 == magic);
}

int
folioHeader_IncrementReferenceCount(FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	return atomic_fetch_add_explicit(&header->xreferenceCount, 1, memory_order_relaxed);
}

int
folioHeader_DecrementReferenceCount(FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	return atomic_fetch_sub_explicit(&header->xreferenceCount, 1, memory_order_relaxed);
}

void
folioHeader_ExecuteFinalizer(FolioHeader *header, void *memory)
{
	assertNotNull(header, "header must be non-null");
	if (header->xfini) {
		header->xfini(memory);
	}
}

void
folioHeader_Invalidate(FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	header->xmagic1 = ~header->xmagic1;
}

size_t
folioHeader_GetTrailerGuardLength(const FolioHeader *header)
{
	assertNotNull(header, "header must be non-null");
	return header->xtrailerGuardLength;
}

//static const uint8_t *
//_getGuard(const FolioHeader *header)
//{
//	size_t guardOffset = sizeof(FolioHeader) + header->xproviderDataLength;
//	const uint8_t *guard = (const uint8_t *) header + guardOffset;
//	return guard;
//}
//
char *
folioHeader_ToString(const FolioHeader *header)
{
	const uint8_t *guard = folioHeader_GetHeaderGuardAddress(header);
	char guardString[header->xheaderGuardLength * 2 + 1];
	guardString[0] = 0;

	for (unsigned i = 0; i < header->xheaderGuardLength; ++i) {
		sprintf(guardString + i*2, "%02x", guard[i]);
	}

	char *str;
	asprintf(&str, "{Header (%p) : mgk1 0x%08x, len %zu, fini %p, lock %p, refCount %d, "
			"pvdrLen %u, hdrgrdlen %u, trlgrdlen %u, mgk2 0x%08x, grd (%p) [0x%s]}",
			(void *) header,
			header->xmagic1,
			header->xrequestedLength,
			header->xfini,
			(void *) header->xlock,
			atomic_load(&header->xreferenceCount),
			header->xproviderDataLength,
			header->xheaderGuardLength,
			header->xtrailerGuardLength,
			header->xmagic2,
			(void *) guard,
			guardString);
	return str;
}

char *
folioTrailer_ToString(const FolioTrailer *trailer, size_t guardLength)
{
	const uint8_t *guard = NULL;
	char guardString[guardLength * 2 + 1];
	guardString[0] = 0;

	if (guardLength > 0) {
		guard = (const uint8_t *) trailer - guardLength;

		for (unsigned i = 0; i < guardLength; ++i) {
			sprintf(guardString + i*2, "%02x", guard[i]);
		}
	}

	char *str;
	asprintf(&str, "{Trailer (%p) : mgk3 0x%08x, grdLen %zu, grd (%p) [0x%s]}",
			(void *) trailer,
			trailer->magic3,
			guardLength,
			(void *) guard,
			guardString);

	return str;
}

const uint8_t *
folioHeader_GetHeaderGuardAddress(const FolioHeader *header)
{
	size_t guardOffset = sizeof(FolioHeader) + header->xproviderDataLength;
	const uint8_t *guard = (uint8_t *) header + guardOffset;
	return guard;
}

FolioTrailer *
folioHeader_GetTrailer(const FolioHeader *header, const FolioPool *pool)
{
	FolioTrailer *trailer = (FolioTrailer *)((uint8_t *) header
								+ pool->headerAlignedLength
								+ folioHeader_GetRequestedLength(header)
								+ folioHeader_GetTrailerGuardLength(header));
	return trailer;
}

const uint8_t *
folioHeader_GetTrailerGuardAddress(const FolioHeader *header, const FolioPool *pool)
{
	size_t guardOffset = pool->headerAlignedLength + folioHeader_GetRequestedLength(header);
	const uint8_t *guard = (uint8_t *) header + guardOffset;
	return guard;
}

