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

// The source file being tested
#include "../src/private/folio_InternalProvider.c"

#include <LongBow/unit-test.h>
#include <Folio/folio.h>

/* **************************************** */
typedef struct mockup_stats {
	int foo;
} MockUpStats;


const FolioMemoryProvider MockupProviderTemplate = { 0 };

const size_t mockup_memory = 256;

/* **************************************** */

LONGBOW_TEST_RUNNER(folio_InternalProvider)
{
    LONGBOW_RUN_TEST_FIXTURE(Local);
    LONGBOW_RUN_TEST_FIXTURE(Global);
}

LONGBOW_TEST_RUNNER_SETUP(folio_InternalProvider)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(folio_InternalProvider)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Acquire);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_AcquireProvider);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Allocate);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_AllocateAndZero);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_AllocationSize);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Create_NoState);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Create_WithState);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Display);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_GetProviderHeader);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_GetProviderHeaderLength);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_GetProviderState);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_GetProviderStateLength);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Length);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Lock);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_ReleaseMemory);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_ReleaseProvider);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Report);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_SetAvailableMemory);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Unlock);
    LONGBOW_RUN_TEST_CASE(Global, folioInternalProvider_Validate);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
	return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
	int status = LONGBOW_STATUS_SUCCEEDED;

	if (!folio_TestRefCount(0, stdout, "Memory leak in %s\n", longBowTestCase_GetFullName(testCase))) {
		folio_Report(stdout);
		status = LONGBOW_STATUS_MEMORYLEAK;
	}

	return status;
}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Acquire)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_AcquireProvider)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Allocate)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_AllocateAndZero)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_AllocationSize)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Create_NoState)
{
	size_t stateLength = 0;
	size_t headerLength = 24;

	FolioMemoryProvider *provider = folioInternalProvider_Create(&MockupProviderTemplate,
			mockup_memory, stateLength, headerLength);
	assertNotNull(provider, "Could not allocate provider");

	FolioPool *pool = folioPool_GetFromProvider(provider);
	assertTrue(pool == provider->poolState, "wrong pool expected %p got %p", (void *) provider->poolState, pool);

	size_t expectedHeaderGuardLength = sizeof(void *);

	assertTrue(pool->headerGuardLength == expectedHeaderGuardLength, "Wrong header guard length expected %zu got %u",
			expectedHeaderGuardLength, pool->headerGuardLength);

	// with non-zero headerLength we need to account for the guard
	size_t expectedHeaderAlignedLength = sizeof(FolioHeader) + headerLength + expectedHeaderGuardLength;
	assertTrue(pool->headerAlignedLength == expectedHeaderAlignedLength,
			"wrong headerAlignedLength expected %zu got %u",
			expectedHeaderAlignedLength,
			pool->headerAlignedLength);

	assertTrue(pool->providerStateLength == stateLength, "Wrong provider state length, expected %zu got %u",
			stateLength, pool->providerStateLength);

	assertTrue(pool->providerHeaderLength == headerLength, "Wrong provider header length, expected %zu got %u",
			headerLength, pool->providerHeaderLength);

	folioInternalProvider_ReleaseProvider(&provider);
}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Create_WithState)
{
	size_t stateLength = 48;
	size_t headerLength = 24;

	FolioMemoryProvider *provider = folioInternalProvider_Create(&MockupProviderTemplate,
			mockup_memory, stateLength, headerLength);
	assertNotNull(provider, "Could not allocate provider");

	FolioPool *pool = folioPool_GetFromProvider(provider);
	assertTrue(pool == provider->poolState, "wrong pool expected %p got %p", (void *) provider->poolState, pool);

	size_t expectedHeaderGuardLength = sizeof(void *);

	assertTrue(pool->headerGuardLength == expectedHeaderGuardLength, "Wrong header guard length expected %zu got %u",
			expectedHeaderGuardLength, pool->headerGuardLength);


	size_t expectedHeaderAlignedLength = sizeof(FolioHeader) + headerLength + expectedHeaderGuardLength;
	assertTrue(pool->headerAlignedLength == expectedHeaderAlignedLength,
			"wrong headerAlignedLength expected %zu got %u",
			expectedHeaderAlignedLength,
			pool->headerAlignedLength);

	assertTrue(pool->providerStateLength == stateLength, "Wrong provider state length, expected %zu got %u",
			stateLength, pool->providerStateLength);

	assertTrue(pool->providerHeaderLength == headerLength, "Wrong provider header length, expected %zu got %u",
			headerLength, pool->providerHeaderLength);

	folioInternalProvider_ReleaseProvider(&provider);

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Display)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_GetProviderHeader)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_GetProviderHeaderLength)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_GetProviderState)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_GetProviderStateLength)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Length)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Lock)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_ReleaseMemory)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_ReleaseProvider)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Report)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_SetAvailableMemory)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Unlock)
{

}

LONGBOW_TEST_CASE(Global, folioInternalProvider_Validate)
{

}


/*****************************************************/

LONGBOW_TEST_FIXTURE(Local)
{
    LONGBOW_RUN_TEST_CASE(Local, _calculateAlignedLength);
    LONGBOW_RUN_TEST_CASE(Local, _computeTotalLength);
    LONGBOW_RUN_TEST_CASE(Local, _decreaseCurrentAllocation);
    LONGBOW_RUN_TEST_CASE(Local, _fillGuard);
    LONGBOW_RUN_TEST_CASE(Local, _getRandomMagic);
    LONGBOW_RUN_TEST_CASE(Local, _increaseCurrentAllocation);
    LONGBOW_RUN_TEST_CASE(Local, _validateInternal);
    LONGBOW_RUN_TEST_CASE(Local, _verifyGuard);
    LONGBOW_RUN_TEST_CASE(Local, _verifyHeader);
    LONGBOW_RUN_TEST_CASE(Local, _verifyInternalProvider);
    LONGBOW_RUN_TEST_CASE(Local, _verifyTrailer);
 }

LONGBOW_TEST_FIXTURE_SETUP(Local)
{
	return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Local)
{
	int status = LONGBOW_STATUS_SUCCEEDED;

	if (!folio_TestRefCount(0, stdout, "Memory leak in %s\n", longBowTestCase_GetFullName(testCase))) {
		folio_Report(stdout);
		status = LONGBOW_STATUS_MEMORYLEAK;
	}

	return status;
}

LONGBOW_TEST_CASE(Local, _calculateAlignedLength)
{

}

LONGBOW_TEST_CASE(Local, _computeTotalLength)
{

}

LONGBOW_TEST_CASE(Local, _decreaseCurrentAllocation)
{

}

LONGBOW_TEST_CASE(Local, _fillGuard)
{

}

LONGBOW_TEST_CASE(Local, _getRandomMagic)
{

}

LONGBOW_TEST_CASE(Local, _increaseCurrentAllocation)
{

}

LONGBOW_TEST_CASE(Local, _validateInternal)
{

}

LONGBOW_TEST_CASE(Local, _verifyGuard)
{

}

LONGBOW_TEST_CASE(Local, _verifyHeader)
{

}

LONGBOW_TEST_CASE(Local, _verifyInternalProvider)
{

}

LONGBOW_TEST_CASE(Local, _verifyTrailer)
{

}


/*****************************************************/

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(folio_InternalProvider);
    int exitStatus = LONGBOW_TEST_MAIN(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}


