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
#include <Folio/folio.h>
#include <Folio/folio_LinkedList.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct entry_t Entry;

struct entry_t {
	void *data;
	Entry *next;
};

struct folio_linkedlist {
	Entry *head;
	Entry *tail;
};

static void
_entryFinalize(void *memory)
{
	Entry *entry = memory;
	folio_Release(&entry->data);
}

Entry *
entry_Create(const void *data)
{
	Entry *entry = folio_AllocateAndZero(sizeof(Entry), _entryFinalize);
	assertNotNull(entry, "Got null from allocator");

	entry->data = folio_Acquire(data);
	return entry;
}

void *
entry_GetData(const Entry *entry)
{
	return entry->data;
}

void
entry_Release(Entry **entryPtr)
{
	folio_Release((void **) entryPtr);
}

static void
_finalize(void *memory)
{
	FolioLinkedList *list = memory;

	folio_Lock(list);
	while (!folioLinkedList_IsEmpty(list)) {
		void *p = folioLinkedList_Remove(list);
		folio_Release(&p);
	}
	folio_Unlock(list);
}

FolioLinkedList *
folioLinkedList_Create(void)
{
	FolioLinkedList *list = folio_AllocateAndZero(sizeof(FolioLinkedList), _finalize);
	assertNotNull(list, "Got null from allocator");

	return list;
}

FolioLinkedList *
folioLinkedList_Acquire(const FolioLinkedList *list)
{
	return folio_Acquire(list);
}

void
folioLinkedList_Release(FolioLinkedList **listPtr)
{
	folio_Release((void **) listPtr);
}

void
folioLinkedList_Append(FolioLinkedList *list, const void *data)
{
	trapIllegalValueIf(data == NULL, "Cannot store NULL in the list");

	Entry *entry = entry_Create(data);

	folio_Lock(list);
	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		assertNotNull(list->tail, "List tail should not be NULL if there's a head");
		assertNull(list->tail->next, "List tail's next should be null");

		list->tail->next = entry;
		list->tail = entry;
	}
	folio_Unlock(list);
}

void *
folioLinkedList_Remove(FolioLinkedList *list)
{
	void *data = NULL;

	folio_Lock(list);
	if (list->head != NULL) {
		Entry *entry = list->head;
		list->head = list->head->next;

		// If we removed the tail element, reset tail
		if (list->head == NULL) {
			list->tail = NULL;
		}

		data = folio_Acquire(entry_GetData(entry));
		entry_Release(&entry);
	}
	folio_Unlock(list);

	return data;
}

bool
folioLinkedList_IsEmpty(const FolioLinkedList *list)
{
	return list->head == NULL;
}

void
folioLinkedList_Display(const FolioLinkedList *list, FILE *stream)
{
	fprintf(stream, "List (%p): head %p tail %p\n",
			(void *) list,
			(void *) list->head,
			(void *) list->tail);
}
