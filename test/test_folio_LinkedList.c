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

#include "../src/folio_LinkedList.c"

LONGBOW_TEST_RUNNER(memory_LinkedList)
{
    LONGBOW_RUN_TEST_FIXTURE(Global);
}

LONGBOW_TEST_RUNNER_SETUP(memory_LinkedList)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(memory_LinkedList)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, folioLinkedList_Create);
    LONGBOW_RUN_TEST_CASE(Global, folioLinkedList_Append);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
	return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
	int status = LONGBOW_STATUS_SUCCEEDED;

	if (!memory_TestRefCount(0, stdout, "Memory leak in %s\n", longBowTestCase_GetFullName(testCase))) {
		memory_Report(stdout);
		status = LONGBOW_STATUS_MEMORYLEAK;
	}

	return status;
}

LONGBOW_TEST_CASE(Global, folioLinkedList_Create)
{
	FolioLinkedList *list = folioLinkedList_Create();
	assertNotNull(list, "Got null from create");
	folioLinkedList_Release(&list);
}

typedef struct integer_t {
	int value;
} Integer;

LONGBOW_TEST_CASE(Global, folioLinkedList_Append)
{
	FolioLinkedList *list = folioLinkedList_Create();
	assertNotNull(list, "Got null from create");

	const size_t count = 4;
	Integer *truthArray[count];
	for (unsigned i = 0; i < count; ++i) {
		Integer *integer = memory_Allocate(sizeof(Integer));
		integer->value = i;
		truthArray[i] = integer;
		folioLinkedList_Append(list, integer);
	}

	for (unsigned i = 0; i < count; ++i) {
		Integer *truth = truthArray[i];
		Integer *test = folioLinkedList_Remove(list);
		assertTrue(truth->value == test->value, "Wrong element, expected %d actual %d",
				truth->value, test->value);
		memory_Release((void **) &truth);
		memory_Release((void **) &test);
	}

	folioLinkedList_Release(&list);
}

/*****************************************************/


/*****************************************************/

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(memory_LinkedList);
    int exitStatus = LONGBOW_TEST_MAIN(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
