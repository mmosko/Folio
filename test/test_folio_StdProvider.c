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
#include <LongBow/unit-test.h>

#include "../src/folio_StdProvider.c"
#include <Folio/folio.h>

LONGBOW_TEST_RUNNER(folio_StdProvider)
{
    LONGBOW_RUN_TEST_FIXTURE(Local);
    LONGBOW_RUN_TEST_FIXTURE(CorruptMemory);
}

LONGBOW_TEST_RUNNER_SETUP(folio_StdProvider)
{
//	folio_Initialize();

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(folio_StdProvider)
{
//	folio_Finalize();

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Local)
{
	LONGBOW_RUN_TEST_CASE(Local, folioStdProvider_Create);
	LONGBOW_RUN_TEST_CASE(Local, _allocate);
    LONGBOW_RUN_TEST_CASE(Local, _allocate_ZeroLength);
    LONGBOW_RUN_TEST_CASE(Local, _allocate_OutOfMemory);

    LONGBOW_RUN_TEST_CASE(Local, _allocateAndZero);
    LONGBOW_RUN_TEST_CASE(Local, _acquire);
    LONGBOW_RUN_TEST_CASE(Local, _report);
    LONGBOW_RUN_TEST_CASE(Local, _length);
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

	folio_SetAvailableMemory(SIZE_MAX);

	return status;
}

LONGBOW_TEST_CASE(Local, folioStdProvider_Create)
{
	FolioMemoryProvider *provider = folioStdProvider_Create(SIZE_MAX);
	folioMemoryProvider_Report(provider, stdout);
	folioMemoryProvider_ReleaseProvider(&provider);
}

LONGBOW_TEST_CASE(Local, _allocate)
{
	const size_t allocSize = 9;
	void *memory = folio_Allocate(allocSize);
	assertNotNull(memory, "Did not return memory pointer");

	size_t acquireCount = folio_OustandingReferences();
	assertTrue(acquireCount == 1, "Expected 1 allocation, got %zu", acquireCount);

	size_t allocationSize = folio_AllocatedBytes();
	assertTrue(allocationSize == allocSize, "Expected %zu bytes, got %zu", allocSize, allocationSize);

	folio_Validate(memory);
	folio_Release(&memory);
}

LONGBOW_TEST_CASE(Local, _allocate_ZeroLength)
{
	const size_t allocSize = 0;
	void *memory = folio_Allocate(allocSize);
	assertNotNull(memory, "Did not return memory pointer");

	size_t acquireCount = folio_OustandingReferences();
	assertTrue(acquireCount == 1, "Expected 1 allocation, got %zu", acquireCount);

	size_t allocationSize = folio_AllocatedBytes();
	assertTrue(allocationSize == allocSize, "Expected %zu bytes, got %zu", allocSize, allocationSize);

	folio_Validate(memory);
	folio_Release(&memory);
}

LONGBOW_TEST_CASE(Local, _allocate_OutOfMemory)
{
	folio_SetAvailableMemory(64);

	const size_t allocSize = 128;

	// This will trap out of memory
	void * memory = folio_Allocate(allocSize);
	assertNull(memory, "memory should have been NULL due to out of memory");
}


LONGBOW_TEST_CASE(Local, _allocateAndZero)
{
	const size_t length = 128;
	uint8_t truth[length];
	memset(truth, 0, length);

	void *memory = folio_AllocateAndZero(length, NULL);
	int result = memcmp(truth, memory, length);
	assertTrue(result == 0, "Memory was not set to zero");
	folio_Release(&memory);
}

LONGBOW_TEST_CASE(Local, _acquire)
{
	const size_t length = 128;
	void *memory = folio_Allocate(length);
	void *mem2 = folio_Acquire(memory);
	folio_Validate(memory);
	folio_Validate(mem2);

	size_t acquireCount = folio_OustandingReferences();
	assertTrue(acquireCount == 2, "Expected 2 allocation, got %zu", acquireCount);

	size_t allocationSize = folio_AllocatedBytes();
	assertTrue(allocationSize == length, "Expected %zu bytes, got %zu", length, allocationSize);

	folio_Release(&memory);

	acquireCount = folio_OustandingReferences();
	assertTrue(acquireCount == 1, "Expected 1 allocation, got %zu", acquireCount);

	allocationSize = folio_AllocatedBytes();
	assertTrue(allocationSize == length, "Expected %zu bytes, got %zu", length, allocationSize);

	folio_Release(&mem2);
}

LONGBOW_TEST_CASE(Local, _report)
{
	const size_t length = 128;
	void *memory = folio_Allocate(length);
	void *mem2 = folio_Acquire(memory);

	folio_Report(stdout);

	folio_Release(&mem2);
	folio_Release(&memory);
}

LONGBOW_TEST_CASE(Local, _length)
{
	const size_t length = 128;
	void *memory = folio_Allocate(length);
	size_t test = folio_Length(memory);
	assertTrue(length == test, "Wrong length, expected %zu got %zu", length, test);
	folio_Release(&memory);
}

/*****************************************************/

LONGBOW_TEST_FIXTURE(CorruptMemory)
{
    LONGBOW_RUN_TEST_CASE(CorruptMemory, overrun);
    LONGBOW_RUN_TEST_CASE(CorruptMemory, underrun);
}

LONGBOW_TEST_FIXTURE_SETUP(CorruptMemory)
{
	void *p = folio_Allocate(64);
	longBowTestCase_SetClipBoardData(testCase, p);

	return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(CorruptMemory)
{
	int status = LONGBOW_STATUS_SUCCEEDED;

	// Because we intentionally corrupt the memory, we cannot use
	// normal release.  We cleanup here manually

	void *memory = longBowTestCase_GetClipBoardData(testCase);
	FolioPool *pool = (FolioPool *) FolioStdProvider.poolState;
	FolioHeader *h = (FolioHeader *) ((uint8_t *) memory - pool->headerAlignedLength);
	size_t requestLength = folioHeader_GetRequestedLength(h);
	free(h);

	_Stats *stats = (_Stats *) folioInternalProvider_GetProviderState(&FolioStdProvider);
	stats->outstandingAllocs--;
	stats->outstandingAcquires--;
	pool->currentAllocation -= requestLength;

	if (!folio_TestRefCount(0, stdout, "Memory leak in %s\n", longBowTestCase_GetFullName(testCase))) {
		folio_Report(stdout);
		status = LONGBOW_STATUS_MEMORYLEAK;
	}

	folio_SetAvailableMemory(SIZE_MAX);

	return status;
}

LONGBOW_TEST_CASE_EXPECTS(CorruptMemory, overrun, .event = &LongBowTrapUnexpectedStateEvent)
{
	void *p = longBowTestCase_GetClipBoardData(testCase);
	size_t length = folio_Length(p);

	memset(p, 0, length + 1);
	folio_Validate(p);
}

LONGBOW_TEST_CASE_EXPECTS(CorruptMemory, underrun, .event = &LongBowTrapUnexpectedStateEvent)
{
	uint8_t *p = longBowTestCase_GetClipBoardData(testCase);

	// wipe out part of magic2, but be sure to not corrupt the length
	void *p2 = p - 1;
	printf("memory (%p) overwriting %p\n", (void *) p, (void *) p2);

	memset(p2, 0, 1);
	folio_Validate(p);
}

/*****************************************************/

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(folio_StdProvider);
    int exitStatus = LONGBOW_TEST_MAIN(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
