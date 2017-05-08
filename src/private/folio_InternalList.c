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

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <Folio/private/folio_InternalList.h>

//#define DEBUG 1

/* ********************************************************************
 * Internal linked list for storing allocations
 *
 * Cannot use memory_LinkedList because of circular reference problems
 *
 * Doubly linked list.  This allows us to store a reference in the memory
 * allocation to it's location in the linked list and remove it in time O(1).
 *
 * We a pthreads mutex on this.  Could be improved later.
 */

struct folio_internal_entry  {
	void *data;
	struct folio_internal_entry *prev;
	struct folio_internal_entry *next;
};

static void _internalEntry_Display(const FolioInternalEntry *entry, FILE *stream, const char *tag) __attribute__((unused));


static FolioInternalEntry *
_internalEntry_Create(void *memory)
{
	FolioInternalEntry *entry = malloc(sizeof(FolioInternalEntry));
	entry->data = memory;
	entry->next = NULL;
	entry->prev = NULL;
	return entry;
}

static void
_internalEntry_Release(FolioInternalEntry **memoryPtr)
{
	free(*memoryPtr);
	*memoryPtr = NULL;
}

static void *
_internalEntry_GetData(const FolioInternalEntry *entry)
{
	return entry->data;
}

static void
_internalEntry_Display(const FolioInternalEntry *entry, FILE *stream, const char *tag)
{
	fprintf(stream, "%-10s: FolioInternalEntry (%p): prev %p next %p data %p\n",
			tag,
			(void *) entry,
			(void *) entry->prev,
			(void *) entry->next,
			(void *) entry->data);

	fprintf(stream, "%-10s: FIXME\n", tag);
	//_memory_Display(entry->data, stream);
}

/* **********************************
 * Internal List implementation
 */

struct folio_internal_list {
	pthread_mutex_t lock;
	FolioInternalEntry *head;
	FolioInternalEntry *tail;
};

FolioInternalList *
folioInternalList_Create(void)
{
	FolioInternalList *list = calloc(1, sizeof(FolioInternalList));
	pthread_mutex_init(&list->lock, NULL);
	return list;
}

void
folioInternalList_Release(FolioInternalList **listPtr)
{
	FolioInternalList *list = *listPtr;
	free(list);
	*listPtr = NULL;
}

void
folioInternalList_Display(const FolioInternalList *list, FILE *stream, const char *tag)
{
	fprintf(stream, "%-10s: FolioInternalList (%p): head %p tail %p\n",
			tag,
			(void *) list,
			(void *) list->head,
			(void *) list->tail);
}

void *
folioInternalList_RemoveAt(FolioInternalList *list, FolioInternalEntry *entry)
{
#if DEBUG
	folioInternalList_Display(list, stdout, "++RemoveAt");
	_internalEntry_Display(entry, stdout, "++RemoveAt");
#endif

	if (list->head == entry) {
		list->head = list->head->next;
		if (list->head) {
			list->head->prev = NULL;
		} else {
			list->tail = NULL;
		}
	} else if (list->tail == entry) {
		list->tail = list->tail->prev;
		assertNotNull(list->tail, "Invalid state, if tail has no prev, it should have been the head");
		list->tail->next = NULL;
	} else {
		// it is in the middle somewhere
		FolioInternalEntry *next = entry->next;
		FolioInternalEntry *prev = entry->prev;

		prev->next = next;
		next->prev = prev;
	}

	void *data = _internalEntry_GetData(entry);
	_internalEntry_Release(&entry);

#if DEBUG
	folioInternalList_Display(list, stdout, "--");
#endif

	return data;
}

FolioInternalEntry *
folioInternalList_Append(FolioInternalList *list, void *data)
{
	FolioInternalEntry *entry = _internalEntry_Create(data);
#if DEBUG
	folioInternalList_Display(list, stdout, "++Append");
	_internalEntry_Display(entry, stdout, "++Append");
#endif

	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		list->tail->next = entry;
		entry->prev = list->tail;
		list->tail = entry;
	}

#if DEBUG
	folioInternalList_Display(list, stdout, "--");
	_internalEntry_Display(entry, stdout, "--");
#endif
	return entry;
}

void
folioInternalList_ForEach(FolioInternalList *list, void (callback)(const void *memory, void *closure), void *closure)
{
#if DEBUG
	folioInternalList_Display(list, stdout, "++ForEach");
#endif

	FolioInternalEntry *entry = list->head;
	while (entry) {
		#if DEBUG
			_internalEntry_Display(entry, stdout, "==");
		#endif
		callback(_internalEntry_GetData(entry), closure);
		entry = entry->next;
	}

#if DEBUG
	folioInternalList_Display(list, stdout, "--");
#endif
}

void
folioInternalList_Lock(FolioInternalList *list)
{
	pthread_mutex_lock(&list->lock);
}

void
folioInternalList_Unlock(FolioInternalList *list)
{
	pthread_mutex_unlock(&list->lock);
}

//static FolioInternalList _privateAllocList = {
//		.lock = PTHREAD_MUTEX_INITIALIZER,
//		.head = NULL,
//		.tail = NULL
//};
//
//static FolioInternalList *_allocList = &_privateAllocList;

