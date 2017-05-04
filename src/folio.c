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
#include "Folio/folio.h"
#include "Folio/folio_StdProvider.h"
#include <stdarg.h>

static FolioMemoryProvider const *_allocator = &FolioStdProvider;

void
folio_SetAllocator(FolioMemoryProvider *allocator)
{
	_allocator = allocator;
}

void
folio_SetAvailableMemory(size_t maximum) {
	_allocator->setAvailableMemory(maximum);
}

const
FolioMemoryProvider *folio_GetAllocator(void)
{
	return _allocator;
}

void *
folio_Allocate(size_t length)
{
	return _allocator->allocate(length);
}

void *
folio_AllocateAndZero(size_t length)
{
	return _allocator->allocateAndZero(length);
}

void
folio_SetFinalizer(void *memory, Finalizer fini)
{
	_allocator->setFinalizer(memory, fini);
}

void *
folio_Acquire(const void *memory)
{
	return _allocator->acquire(memory);
}

void
folio_Release(void **memoryPtr)
{
	_allocator->release(memoryPtr);
}

size_t
folio_Length(const void *memory) {
	return _allocator->length(memory);
}

void
folio_Report(FILE *stream)
{
	_allocator->report(stream);
}

size_t
folio_OustandingReferences(void)
{
	return _allocator->acquireCount();
}

size_t
folio_AllocatedBytes(void)
{
	return _allocator->allocationSize();
}


bool
folio_TestRefCount(size_t expectedRefCount, FILE *stream, const char *format, ...)
{
	bool result = true;
	if (folio_OustandingReferences() != expectedRefCount) {
		va_list va;
		va_start(va, format);

		vfprintf(stream, format, va);
		va_end(va);
		result = false;
	}
	return result;
}

