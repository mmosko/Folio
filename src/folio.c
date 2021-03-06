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

#include <Folio/private/folio_Pool.h>
#include <LongBow/runtime.h>
#include "Folio/folio.h"
#include "Folio/folio_StdProvider.h"
#include <stdarg.h>


// Because it is a static allocation, we cannot get a reference count.
// In any case, it is statically allocated and we cannot release it.
static FolioMemoryProvider *_defaultProvider = &FolioStdProvider;
static FolioMemoryProvider *_provider = &FolioStdProvider;

void
folio_SetProvider(FolioMemoryProvider *provider)
{
	if (_provider != _defaultProvider) {
		folioMemoryProvider_ReleaseProvider(&_provider);
	}

	if (provider != NULL) {
		_provider = folioMemoryProvider_AcquireProvider(provider);
	} else {
		_provider = NULL;
	}
}

void
folio_ReleaseProvider(void)
{
	if (_provider != _defaultProvider) {
		folioMemoryProvider_ReleaseProvider(&_provider);
	} else {
		// If trying to release the default provider, don't do it!  Just null it.
		// The default provider is statically allocated and cannot be released.
		_provider = NULL;
	}
}

void
folio_SetAvailableMemory(size_t maximum) {
	folioMemoryProvider_SetAvailableMemory(_provider, maximum);
}

FolioMemoryProvider *
folio_GetProvider(void)
{
	return _provider;
}

void *
folio_Allocate(size_t length)
{
	return folioMemoryProvider_Allocate(_provider, length, NULL);
}

void *
folio_AllocateAndZero(size_t length, Finalizer fini)
{
	return folioMemoryProvider_AllocateAndZero(_provider, length, fini);
}

void *
folio_Acquire(const void *memory)
{
	return folioMemoryProvider_Acquire(_provider, memory);
}

void
folio_Release(void **memoryPtr)
{
	folioMemoryProvider_Release(_provider, memoryPtr);
}

size_t
folio_Length(const void *memory) {
	return folioMemoryProvider_Length(_provider, memory);
}

void
folio_Report(FILE *stream)
{
	folioMemoryProvider_Report(_provider, stream);
}

size_t
folio_OustandingReferences(void)
{
	return folioMemoryProvider_OustandingReferences(_provider);
}

size_t
folio_AllocatedBytes(void)
{
	return folioMemoryProvider_AllocatedBytes(_provider);
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

void
folio_Validate(const void *memory)
{
	folioMemoryProvider_Validate(_provider, memory);
}

void
folio_Lock(void *memory)
{
	folioMemoryProvider_Lock(_provider, memory);
}

void
folio_Unlock(void *memory)
{
	folioMemoryProvider_Unlock(_provider, memory);
}

