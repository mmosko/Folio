/*
 * folio_LinkedList.c
 *
 *  Created on: May 1, 2017
 *      Author: marc
 */

#include <LongBow/runtime.h>
#include "folio.h"
#include "../include/folio_LinkedList.h"

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
	Entry *entry = folio_AllocateAndZero(sizeof(Entry));
	assertNotNull(entry, "Got null from allocator");
	folio_SetFinalizer(entry, _entryFinalize);

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
	while (!folioLinkedList_IsEmpty(list)) {
		void *p = folioLinkedList_Remove(list);
		folio_Release(&p);
	}
}

FolioLinkedList *
folioLinkedList_Create(void)
{
	FolioLinkedList *list = folio_AllocateAndZero(sizeof(FolioLinkedList));
	assertNotNull(list, "Got null from allocator");
	folio_SetFinalizer(list, _finalize);

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
	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		assertNotNull(list->tail, "List tail should not be NULL if there's a head");
		assertNull(list->tail->next, "List tail's next should be null");

		list->tail->next = entry;
		list->tail = entry;
	}
}

void *
folioLinkedList_Remove(FolioLinkedList *list)
{
	void *data = NULL;
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
