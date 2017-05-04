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

#include <LongBow/runtime.h>
#include <pthread.h>
#include <stdatomic.h>

#include <Folio/folio.h>
#include <Folio/private/folio_Lock.h>

struct folio_lock {
	atomic_int referenceCount;
	atomic_flag spinLock;
	pthread_t lockingThreadId;
};

FolioLock *
folioLock_Create(void)
{
	FolioLock *lock = calloc(1, sizeof(FolioLock));
	if (lock) {
		atomic_flag_clear(&lock->spinLock);
		lock->referenceCount = ATOMIC_VAR_INIT(1);

		// note that this thread does not really hold the lock, but
		// we need to initialize it with something and as it is an
		// opaque value this is all we have.
		lock->lockingThreadId = pthread_self();
	}

	return lock;
}

FolioLock *
folioLock_Acquire(const FolioLock *lock)
{
	assertNotNull(lock, "lock must be non-null");
	FolioLock *copy = (FolioLock *) lock;
	int prior = atomic_fetch_add(&copy->referenceCount, 1);
	trapUnexpectedStateIf(prior == 0, "Acquire on memory with zero reference count");
	return copy;
}

void
folioLock_Release(FolioLock **lockPtr)
{
	assertNotNull(lockPtr, "lockPtr must be non-null");
	assertNotNull(*lockPtr, "lockPtr must dereference to non-null");
	FolioLock *lock = *lockPtr;
	int prior = atomic_fetch_sub(&lock->referenceCount, 1);
	trapUnexpectedStateIf(prior < 1, "Releasing memory with a refcount %d < 1", prior)
	if (prior == 1) {
		free(lock);
	}
	*lockPtr = NULL;
}

void
folioLock_Lock(FolioLock *lock)
{
	assertNotNull(lock, "lock must be non-null");

	// spin until the prior value is FALSE, which indicates that
	// no one else had the lock.
	bool prior;
	do {
		prior = atomic_flag_test_and_set(&lock->spinLock);
	} while (!prior);

	lock->lockingThreadId = pthread_self();
}

void
folioLock_Unlock(FolioLock *lock)
{
	if (pthread_equal(pthread_self(), lock->lockingThreadId)) {
		atomic_flag_clear(&lock->spinLock);
	} else {
		trapCannotObtainLock("Caller thread id not equal to locker thread id");
	}
}



