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
#include "../src/private/folio_Header.c"

#include <LongBow/unit-test.h>
#include <Folio/folio.h>

LONGBOW_TEST_RUNNER(folio_Header)
{
    LONGBOW_RUN_TEST_FIXTURE(Global);
}

LONGBOW_TEST_RUNNER_SETUP(folio_Header)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_RUNNER_TEARDOWN(folio_Header)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_CompareMagic);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_DecrementReferenceCount);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_ExecuteFinalizer);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_Finalize);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_GetFinalizer);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_GetHeaderGuardAddress);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_GetMemoryHeader);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_GetRequestedLength);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_GetTrailer);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_GetTrailerGuardAddress);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_GetTrailerGuardLength);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_IncrementReferenceCount);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_Initialize);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_Invalidate);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_Lock);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_ProviderDataLength);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_ReferenceCount);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_SetFinalizer);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_ToString);
    LONGBOW_RUN_TEST_CASE(Global, folioHeader_Unlock);
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

LONGBOW_TEST_CASE(Global, folioHeader_CompareMagic)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_DecrementReferenceCount)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_ExecuteFinalizer)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_Finalize)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_GetFinalizer)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_GetHeaderGuardAddress)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_GetMemoryHeader)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_GetRequestedLength)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_GetTrailer)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_GetTrailerGuardAddress)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_GetTrailerGuardLength)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_IncrementReferenceCount)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_Initialize)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_Invalidate)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_Lock)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_ProviderDataLength)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_ReferenceCount)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_SetFinalizer)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_ToString)
{

}

LONGBOW_TEST_CASE(Global, folioHeader_Unlock)
{

}

/*****************************************************/

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(folio_Header);
    int exitStatus = LONGBOW_TEST_MAIN(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}


