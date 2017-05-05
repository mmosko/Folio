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

#ifndef INCLUDE_FOLIO_PRIVATE_FOLIO_INTERNALLIST_H_
#define INCLUDE_FOLIO_PRIVATE_FOLIO_INTERNALLIST_H_

#include <stdio.h>

typedef struct folio_internal_entry FolioInternalEntry;
typedef struct folio_internal_list FolioInternalList;

FolioInternalList * folioInternalList_Create(void);
void folioInternalList_Release(FolioInternalList **listPtr);

void folioInternalList_Display(const FolioInternalList *list, FILE *stream, const char *tag);

/**
 * Appends the data to the list and returns a handle to the list entry
 * that can be used by folio_InternalList_RemoveAt().
 */
FolioInternalEntry * folioInternalList_Append(FolioInternalList *list, void *data);

void * folioInternalList_RemoveAt(FolioInternalList *list, FolioInternalEntry *entry);

/**
 * Execute a callback on each list entry in order.
 */
void folioInternalList_ForEach(FolioInternalList *list, void (callback)(const void *memory, void *closure),
		void *closure);

void folioInternalList_Lock(FolioInternalList *list);

void folioInternalList_Unlock(FolioInternalList *list);


#endif /* INCLUDE_FOLIO_PRIVATE_FOLIO_INTERNALLIST_H_ */
