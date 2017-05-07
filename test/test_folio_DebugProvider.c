/*
 * test_folio_DebugProvider.c
 *
 *  Created on: May 1, 2017
 *      Author: marc
 */


// The source file being tested
#include <LongBow/unit-test.h>

#include "../src/folio_DebugProvider.c"

#include <Folio/private/folio_Pool.h>

LONGBOW_TEST_RUNNER(folio_DebugProvider)
{
    LONGBOW_RUN_TEST_FIXTURE(Local);
    LONGBOW_RUN_TEST_FIXTURE(CorruptMemory);
}

LONGBOW_TEST_RUNNER_SETUP(folio_DebugProvider)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(folio_DebugProvider)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Local)
{
    LONGBOW_RUN_TEST_CASE(Local, _allocate);
    LONGBOW_RUN_TEST_CASE(Local, _allocate_ZeroLength);
    LONGBOW_RUN_TEST_CASE(Local, _allocate_OutOfMemory);

    LONGBOW_RUN_TEST_CASE(Local, _allocateAndZero);
    LONGBOW_RUN_TEST_CASE(Local, _acquire);
    LONGBOW_RUN_TEST_CASE(Local, _report);
    LONGBOW_RUN_TEST_CASE(Local, _length);
    LONGBOW_RUN_TEST_CASE(Local, backtrace);
}

LONGBOW_TEST_FIXTURE_SETUP(Local)
{
	FolioMemoryProvider *debugProvider = folioDebugProvider_Create(SIZE_MAX);
	longBowTestCase_SetClipBoardData(testCase, debugProvider);

	return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Local)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	int status = LONGBOW_STATUS_SUCCEEDED;

	if (!folioMemoryProvider_TestRefCount(debugProvider, 0, stdout, "Memory leak in %s\n", longBowTestCase_GetFullName(testCase))) {
		_report(debugProvider, stdout);
		folioDebugProvider_DumpBacktraces(folio_GetProvider(), stdout);

		status = LONGBOW_STATUS_MEMORYLEAK;
	}

	_releaseProvider(&debugProvider);

	return status;
}

LONGBOW_TEST_CASE(Local, _allocate)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	const size_t allocSize = 9;
	void *memory = _allocate(debugProvider, allocSize, NULL);
	assertNotNull(memory, "Did not return memory pointer");

	size_t acquireCount = _acquireCount(debugProvider);
	assertTrue(acquireCount == 1, "Expected 1 allocation, got %zu", acquireCount) {
		folioDebugProvider_DumpBacktraces(debugProvider, stdout);
	}

	size_t allocationSize = _allocationSize(debugProvider);
	assertTrue(allocationSize == allocSize, "Expected %zu bytes, got %zu", allocSize, allocationSize) {
		folioDebugProvider_DumpBacktraces(debugProvider, stdout);
	}

	_validate(debugProvider, memory);
	_release(debugProvider, &memory);
}

LONGBOW_TEST_CASE(Local, _allocate_ZeroLength)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	const size_t allocSize = 0;
	void *memory = _allocate(debugProvider, allocSize, NULL);
	assertNotNull(memory, "Did not return memory pointer");

	size_t acquireCount = _acquireCount(debugProvider);
	assertTrue(acquireCount == 1, "Expected 1 allocation, got %zu", acquireCount);

	size_t allocationSize = _allocationSize(debugProvider);
	assertTrue(allocationSize == allocSize, "Expected %zu bytes, got %zu", allocSize, allocationSize);

	_validate(debugProvider, memory);
	_release(debugProvider, &memory);
}

LONGBOW_TEST_CASE(Local, _allocate_OutOfMemory)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	_setAvailableMemory(debugProvider, 64);

	const size_t allocSize = 128;

	// This will trap out of memory
	void * memory = _allocate(debugProvider, allocSize, NULL);
	assertNull(memory, "memory should have been NULL due to out of memory");
}


LONGBOW_TEST_CASE(Local, _allocateAndZero)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	const size_t length = 128;
	uint8_t truth[length];
	memset(truth, 0, length);

	void *memory = _allocateAndZero(debugProvider, length, NULL);
	int result = memcmp(truth, memory, length);
	assertTrue(result == 0, "Memory was not set to zero");
	_release(debugProvider, &memory);
}

LONGBOW_TEST_CASE(Local, _acquire)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	const size_t length = 128;
	void *memory = _allocate(debugProvider, length, NULL);
	void *mem2 = _acquire(debugProvider, memory);
	_validate(debugProvider, memory);
	_validate(debugProvider, mem2);

	size_t acquireCount = _acquireCount(debugProvider);
	assertTrue(acquireCount == 2, "Expected 2 allocation, got %zu", acquireCount);

	size_t allocationSize = _allocationSize(debugProvider);
	assertTrue(allocationSize == length, "Expected %zu bytes, got %zu", length, allocationSize);

	_release(debugProvider, &memory);

	acquireCount = _acquireCount(debugProvider);
	assertTrue(acquireCount == 1, "Expected 1 allocation, got %zu", acquireCount);

	allocationSize = _allocationSize(debugProvider);
	assertTrue(allocationSize == length, "Expected %zu bytes, got %zu", length, allocationSize);

	_release(debugProvider, &mem2);
}

LONGBOW_TEST_CASE(Local, _report)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	const size_t length = 128;
	void *memory = _allocate(debugProvider, length, NULL);
	void *mem2 = _acquire(debugProvider, memory);

	_report(debugProvider, stdout);

	_release(debugProvider, &mem2);
	_release(debugProvider, &memory);
}

LONGBOW_TEST_CASE(Local, _length)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	const size_t length = 128;
	void *memory = _allocate(debugProvider, length, NULL);
	size_t test = _length(debugProvider, memory);
	assertTrue(length == test, "Wrong length, expected %zu got %zu", length, test);
	_release(debugProvider, &memory);
}

LONGBOW_TEST_CASE(Local, backtrace)
{
	FolioMemoryProvider *debugProvider = longBowTestCase_GetClipBoardData(testCase);

	const size_t length = 128;
	void *memory = _allocate(debugProvider, length, NULL);
	folioDebugAlloc_Backtrace(debugProvider, memory, stdout);

	_release(debugProvider, &memory);
}

/*****************************************************/

typedef struct corrupt_data {
	FolioMemoryProvider *provider;
	void *memory;
} CorruptData;

static CorruptData _data;

LONGBOW_TEST_FIXTURE(CorruptMemory)
{
    LONGBOW_RUN_TEST_CASE(CorruptMemory, overrun);
    LONGBOW_RUN_TEST_CASE(CorruptMemory, underrun);
}

LONGBOW_TEST_FIXTURE_SETUP(CorruptMemory)
{
	_data.provider = folioDebugProvider_Create(SIZE_MAX);
	_data.memory = _allocate(_data.provider, 64, NULL);

	return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(CorruptMemory)
{
	int status = LONGBOW_STATUS_SUCCEEDED;

	// Because we intentionally corrupt the memory, we cannot use
	// normal release.  We cleanup here manually

	FolioPool *pool = (FolioPool *) _data.provider;
	FolioHeader *h = (FolioHeader *) ((uint8_t *) _data.memory - pool->headerAlignedLength);
	size_t requestLength = folioHeader_GetRequestedLength(h);

	DebugState *state = (DebugState *) folioInternalProvider_GetProviderState(_data.provider);

	state->stats.outstandingAllocs--;
	state->stats.outstandingAcquires--;
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
	size_t length = _length(_data.provider, _data.memory);

	memset(_data.memory, 0, length + 1);
	_validate(_data.provider, _data.memory);
}

LONGBOW_TEST_CASE_EXPECTS(CorruptMemory, underrun, .event = &LongBowTrapUnexpectedStateEvent)
{
	// wipe out part of magic2, but be sure to not corrupt the length
	void *p2 = _data.memory - 1;
	memset(p2, 0, 1);
	_validate(_data.provider, _data.memory);
}

/*****************************************************/

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(folio_DebugProvider);
    int exitStatus = LONGBOW_TEST_MAIN(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
