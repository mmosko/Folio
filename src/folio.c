/*
 * folio.c
 *
 *  Created on: May 1, 2017
 *      Author: marc
 */

#include <LongBow/runtime.h>
#include "folio.h"
#include "folio_StdProvider.h"
#include <stdarg.h>

static FolioMemoryProvider const *_allocator = &FolioStdProvider;

void
folio_SetAllocator(FolioMemoryProvider *allocator)
{
	_allocator = allocator;
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

