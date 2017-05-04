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

#ifndef SRC_PRIVATE_FOLIO_LOCK_H_
#define SRC_PRIVATE_FOLIO_LOCK_H_

typedef struct folio_lock FolioLock;

/**
 * Creates a spin lock that uses atomic operations.
 */
FolioLock *folioLock_Create(void);

/**
 * Obtains a reference count to the lock.
 */
FolioLock *folioLock_Acquire(const FolioLock *lock);

/**
 * Releases a reference to the lock.  It is allowed to
 * release the last reference to the lock even if the
 * lock is still in the locked state.
 */
void folioLock_Release(FolioLock **lockPtr);

/**
 * Obtains the lock.  Will busy-wait blocking.  No ordering
 * of contenders.  Will record the thread id of the locker.
 */
void folioLock_Lock(FolioLock *lock);

/**
 * Releases the lock.  Must be called by the same thread id
 * as the locker.  Will trap with LongBowTrapCannotObtainLockEvent
 * if the thread id is not the same.
 *
 * The thread that initially creates the lock may be able to
 * call folio_Unlock without first calling folio_Lock.  Likewise,
 * the locking thread may be able to call folio_Unlock multiple times
 * without re-obtaining the lock, assuming no one else has locked it.
 */
void folioLock_Unlock(FolioLock *lock);

#endif /* SRC_PRIVATE_FOLIO_LOCK_H_ */
