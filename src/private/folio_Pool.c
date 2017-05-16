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
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <Folio/private/folio_Pool.h>

char *
folioPool_ToString(const FolioPool *pool)
{
	char *str = NULL;

	bool locked = atomic_flag_test_and_set(&((FolioPool *)pool)->allocationLock);
	if (!locked) {
		atomic_flag_clear(&((FolioPool *)pool)->allocationLock);
	}

	asprintf(&str, "Pool (%p) : hdrMagic 0x%08x provStateLen %u provHdrLen %u hdrAlgnLen %u hdrGrdLen %u "
			" trlAlgnLen %u GrdByte 0x%02x IsLocked %d poolSize %zu alloc'd %d refCount %d\n",
			(void *) pool,
			pool->headerMagic,
			pool->providerStateLength,
			pool->providerHeaderLength,
			pool->headerAlignedLength,
			pool->headerGuardLength,
			pool->trailerAlignedLength,
			pool->guardPattern,
			locked,
			pool->poolSize,
			pool->currentAllocation,
			atomic_load(&((FolioPool *)pool)->referenceCount));

	return str;
}
