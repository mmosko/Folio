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

#ifndef FOLIO_DEBUGPROVIDER_H
#define FOLIO_DEBUGPROVIDER_H

#include <stdio.h>
#include "folio.h"

/**
 * Debug memory allocator with checks for underflow & overflow
 * plus stack backtraces.  Release the reference with
 * folioMemoryProvider_ReleaseProvider().
 */
FolioMemoryProvider * folioDebugProvider_Create(size_t poolSize);

/**
 * Display the backtrace for a specific memory allocation
 */
void folioDebugProvider_Backtrace(FolioMemoryProvider *provider, const void *memory, FILE *stream);

/**
 * Display all backtraces
 */
void folioDebugProvider_DumpBacktraces(FolioMemoryProvider *provider, FILE *stream);

/**
 * Validates every allocation in the backtrace list.  This will be slow,
 * do not do it if you do not need to.
 */
void folioDebugProvider_ValidateAll(FolioMemoryProvider *provider);

#endif /* FOLIO_DEBUGPROVIDER_H */
