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

#ifndef SRC_PRIVATE_FOLIO_INTERNALPROVIDER_H_
#define SRC_PRIVATE_FOLIO_INTERNALPROVIDER_H_

#include <Folio/folio.h>

/**
 * Creates a memory pool with a maximum size.
 *
 * You can use all the functions from folioMemoryProvider_X() on the returned pool.
 *
 * Each pool created will have its own guard patterns so they are not compatible with each other.
 *
 * Memory is allocated like this:
 *
 * | magic1 | header data   | guard | user memory ............. | magic3 | guard      |
 * | -------- headerLength -------- | ---- requestedLength ---- | -- trailerLength -- |
 * | ---------------------------------- totalLength --------------------------------- |
 *
 * magic1 and magic2 are unique arrays for each memory pool.  Guard is a unique byte
 * pattern and its length will vary based on hardware platform to make sure the user memory
 * and totalLength are aligned.
 *
 * The header really looks like this:
 *
 * | InternalHeader ......  | providerHeader ..... | guard ..... |
 * | sizeof(InternalHeader) | providerHeaderLength | guardLength |
 * | .........headerLength aligned ((sizeof(void *))) .......... |
 *
 * headerLength and trailerLength are properties of the FolioInternalProvider.  requestedLength
 * and totalLength are properties of each memory allocation.
 *
 * The caller can provide two extra sizes used by the InternalProvider.  The first is
 * providerStateLength which is allocated as extra space in the FolioMemoryProvider, so is global to the
 * whole pool.  The second is providerHeaderLength which is space added to the header
 * of each memory allocation.
 *
 * @param template The function structure template to populate in the FolioMemoryProvider
 * @param memorySize The maximum number of bytes available from this pool.
 * @param providerDataLength The caller can ask for extra bytes to be allocated in the header for provider-specific storage
 * @param providerHeaderLength Extra space allocated in the header of every allocation for the caller
 */
FolioMemoryProvider *folioInternalProvider_Create(const FolioMemoryProvider *template, size_t memorySize,
		size_t providerStateLength, size_t providerHeaderLength);

/**
 * Returns a pointer to to the provider storage in the pool.  It will be of
 * length providerStateLength from the Create function.
 */
void *folioInternalProvider_GetProviderState(FolioMemoryProvider const *provider);

/**
 * Returns the size of folioInternalProvider_GetProviderState().  If the Internal Provider
 * was created via a call to folioInternalProvider_Create(), it will be the providerStateLength.
 * If using the static (global) Internal Provider, it is a fixed size that the user must
 * live within.
 */
size_t folioInternalProvider_GetProviderStateLength(const FolioMemoryProvider *provider);

/**
 * Returns a pointer to the provider storage in a memory allocation.  It will be
 * of length providerHeaderLength.
 */
void *folioInternalProvider_GetProviderHeader(FolioMemoryProvider const *provider, const void *memory);

/**
 * Returns the size of the Provider header storage.  If the provider is created via
 * a call to folioInternalProvider_Create(), it will be providerHeaderLength.  If using
 * the static (global) Internal Provider, it is a small fixed size.
 */
size_t folioInternalProvider_GetProviderHeaderLength(const FolioMemoryProvider *provider);


void folioInternalProvider_ReleaseProvider(FolioMemoryProvider **providerPtr);
void * folioInternalProvider_Allocate(FolioMemoryProvider *provider, const size_t length);
void * folioInternalProvider_AllocateAndZero(FolioMemoryProvider *provider, const size_t length);
void * folioInternalProvider_Acquire(FolioMemoryProvider *provider, const void *memory);
size_t folioInternalProvider_Length(const FolioMemoryProvider *provider, const void *memory);

/**
 * Release a reference to the memory.  If it is the final release, it will call the
 * finalizer, if set, then release the memory.  The provider header state is invalid at this point.
 *
 * @return true if this was the final release
 * @return false if there are still outstanding references
 */
bool folioInternalProvider_ReleaseMemory(FolioMemoryProvider *provider, void **memoryPtr);

void folioInternalProvider_Validate(const FolioMemoryProvider *provider, const void *memory);
size_t folioInternalProvider_AllocationSize(const FolioMemoryProvider *provider);
void folioInternalProvider_SetFinalizer(FolioMemoryProvider *provider, void *memory, Finalizer fini);
void folioInternalProvider_SetAvailableMemory(FolioMemoryProvider *provider, size_t maximum);
void folioInternalProvider_Lock(FolioMemoryProvider *provider, void *memory);
void folioInternalProvider_Unlock(FolioMemoryProvider *provider, void *memory);


#endif /* SRC_PRIVATE_FOLIO_INTERNALPROVIDER_H_ */
