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

#ifndef FOLIO_LINKEDLIST_H
#define FOLIO_LINKEDLIST_H

typedef struct folio_linkedlist FolioLinkedList;

#include <stdio.h>
#include <stdbool.h>

#include "folio.h"

/**
 * A linked list of Memory objects.  The elements of the
 * list must be created via memory_Allocate() (or memory_AllocateAndZero()).
 */
FolioLinkedList * folioLinkedList_Create(void);

FolioLinkedList * folioLinkedList_Acquire(const FolioLinkedList *list);

void folioLinkedList_Release(FolioLinkedList **listPtr);

/**
 * Stores a reference to the memory on the list.  The caller still owns their reference
 * to the memory.
 */
void folioLinkedList_Append(FolioLinkedList *list, const void *data);

/**
 * Returns the head of the list and removes it.
 *
 * @return null if list empty
 * @return non-null The head of the list
 */
void * folioLinkedList_Remove(FolioLinkedList *list);

bool folioLinkedList_IsEmpty(const FolioLinkedList *list);

void folioLinkedList_Display(const FolioLinkedList *list, FILE *stream);

#endif /* FOLIO_LINKEDLIST_H */
