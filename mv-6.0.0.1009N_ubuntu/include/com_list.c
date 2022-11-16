/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.
   
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "com_define.h"
#include "com_list.h"
#include "com_dbg.h"
/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
 #ifdef _OS_WINDOWS
static MV_INLINE void _List_AddLock(List_Head *new_one,
			      List_Head *head,
			      PKSPIN_LOCK SpinLock)
{
	register List_Head *next;
	KeAcquireSpinLockAtDpcLevel(SpinLock);
	next = head->next;
	next->prev = new_one;
	new_one->next = next;
	new_one->prev = head;
	head->next = new_one;
	KeReleaseSpinLockFromDpcLevel(SpinLock);
}
 static MV_INLINE void _List_AddTailLock(List_Head *new_one,
			      List_Head *head,
			      PKSPIN_LOCK SpinLock)
{
	register List_Head *prev;
	KeAcquireSpinLockAtDpcLevel(SpinLock);
	prev = head->prev;
	prev->next = new_one;
	new_one->next = head;
	new_one->prev = prev;
	head->prev = new_one;
	KeReleaseSpinLockFromDpcLevel(SpinLock);
}
 #endif
static MV_INLINE void __List_Add(List_Head *new_one,
			      List_Head *prev,
			      List_Head *next)
{
#ifdef MV_DEBUG
	if (unlikely(next->prev != prev)) {
			printk(KERN_ALERT "====== QNAP dump_stack() ======\n");
			dump_stack();
			printk(KERN_ALERT "==============================\n");
			MV_DPRINT(( "list_add corruption. next->prev should be "
				"prev (0x%p), but was 0x%p. (next=0x%p).\n",
				prev, next->prev, next));

			MV_DPRINT(( "prev=0x%p, prev->next=0x%p,  prev->prev=0x%p"
				"next=0x%p, next->next=0x%p, next->prev=0x%p.\n",
				prev, prev->next, prev->prev, next, next->next, next->prev));

		}
		if (unlikely(prev->next != next)) {
			printk(KERN_ALERT "====== QNAP dump_stack() ======\n");
			dump_stack();
			printk(KERN_ALERT "==============================\n");
			MV_DPRINT(( "list_add corruption. prev->next should be "
				"next (0x%p), but was 0x%p. (prev=0x%p).\n",
				next, prev->next, prev));

			MV_DPRINT(( "prev=0x%p, prev->next=0x%p,  prev->prev=0x%p,"
				"next=0x%p, next->next=0x%p, next->prev=0x%p.\n",
				prev, prev->next, prev->prev, next, next->next, next->prev));

		}
#endif

	next->prev = new_one;
	new_one->next = next;
	new_one->prev = prev;
	prev->next = new_one;
}

/**
 * List_Add - add a new entry
 * @new_one: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static MV_INLINE void List_Add(List_Head *new_one, List_Head *head)
{
	__List_Add(new_one, head, head->next);
}

static MV_INLINE void Counted_List_Add(List_Head *new_one, Counted_List_Head *head)
{
	List_Add(new_one, (List_Head *)head);
	head->node_count++;
}

/**
 * List_AddTail - add a new entry
 * @new_one: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static MV_INLINE void List_AddTail(List_Head *new_one, List_Head *head)
{
	__List_Add(new_one, head->prev, head);
}

static MV_INLINE void Counted_List_AddTail(List_Head *new_one, Counted_List_Head *head)
{
	List_AddTail(new_one, (List_Head *) head);
	head->node_count++;
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static MV_INLINE void __List_Del(List_Head * prev, List_Head * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * List_Del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: List_Empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
#ifdef MV_DEBUG
static void List_Del(List_Head *entry)
#else
static MV_INLINE void List_Del(List_Head *entry)
#endif
{
#ifdef MV_DEBUG
	if (entry->prev) {
		if (unlikely(entry->prev->next != entry)) {
                        MV_DASSERT(MV_FALSE);
			MV_DPRINT(("list_del corruption. prev->next should be %p, "
					"but was %p\n", entry, entry->prev->next));
			MV_DPRINT(( "entry->next=%p, entry->next->next=%p,	entry->next->prev=%p.\n"
					"entry->prev=%p, entry->prev->next=%p, entry->prev->prev=%p.\n",
				entry->prev, entry->next->next, entry->next->prev, entry->prev, entry->prev->next, entry->prev->prev));

		}
		if (unlikely(entry->next->prev != entry)) {
                        MV_DASSERT(MV_FALSE);
			MV_DPRINT(( "list_del corruption. next->prev should be %p, "
					"but was %p\n", entry, entry->next->prev));

			MV_DPRINT(("entry->next=%p, entry->next->next=%p,  entry->next->prev=%p.\n"
					"entry->prev=%p, entry->prev->next=%p, entry->prev->prev=%p.\n",
				entry->next, entry->next->next, entry->next->prev, entry->prev, entry->prev->next, entry->prev->prev));

		}
	}
#endif

	__List_Del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

static MV_INLINE void Counted_List_Del(List_Head *entry, Counted_List_Head *head)
{
	List_Del(entry);
	head->node_count--;
}


/**
 * List_DelInit - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static MV_INLINE void List_DelInit(List_Head *entry)
{
	__List_Del(entry->prev, entry->next);
	MV_LIST_HEAD_INIT(entry);
}

/**
 * List_Move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static MV_INLINE void List_Move(List_Head *list, List_Head *head)
{
        __List_Del(list->prev, list->next);
        List_Add(list, head);
}

/**
 * List_MoveTail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static MV_INLINE void List_MoveTail(List_Head *list,
				  List_Head *head)
{
        __List_Del(list->prev, list->next);
        List_AddTail(list, head);
}

/**
 * List_Empty - tests whether a list is empty
 * @head: the list to test.
 */
static MV_INLINE int List_Empty(const List_Head *head)
{
	return head->next == head;
}

static MV_INLINE int Counted_List_Empty(const Counted_List_Head *head)
{
	return (head->node_count == 0);
}

static MV_INLINE int List_GetCount(const List_Head *head)
{
	int i=0;
	List_Head *pos;
	LIST_FOR_EACH(pos, head) {
		i++;
	}
	return i;
}

static MV_INLINE int Counted_List_GetCount(const Counted_List_Head *head, const MV_BOOLEAN traverse_list)
{
	int i=0;
	List_Head *pos;

	if (traverse_list) {
		LIST_FOR_EACH(pos, head) {
			i++;
		}
		return i;
	}
	else {
		return head->node_count;
	}
}
#ifdef _OS_WINDOWS
static MV_INLINE int Counted_List_GetCount_Lock(const Counted_List_Head *head, PKSPIN_LOCK SpinLock, const MV_BOOLEAN traverse_list)
{
	int i=0;
	List_Head *pos;
	KeAcquireSpinLockAtDpcLevel(SpinLock);
	if (traverse_list) {
		LIST_FOR_EACH(pos, head) {
			i++;
		}
	}
	else {
		i = head->node_count;
	}
	KeReleaseSpinLockFromDpcLevel(SpinLock);
	return i;
}
static MV_INLINE List_Head* List_GetFirstLock(List_Head *head, PKSPIN_LOCK SpinLock)
{
	List_Head * one = NULL;
	if ( List_Empty(head) ) return NULL;
	KeAcquireSpinLockAtDpcLevel(SpinLock);
	one = head->next;
	List_Del(one);
	KeReleaseSpinLockFromDpcLevel(SpinLock);
	return one;
}
static MV_INLINE List_Head* List_GetLastLock(List_Head *head, PKSPIN_LOCK SpinLock)
{
	List_Head * one = NULL;
	if ( List_Empty(head) ) return NULL;
	KeAcquireSpinLockAtDpcLevel(SpinLock);
	one = head->prev;
	List_Del(one);
	KeReleaseSpinLockFromDpcLevel(SpinLock);
	return one;
}
#endif
static MV_INLINE List_Head* List_GetFirst(List_Head *head)
{
	List_Head * one = NULL;
	if ( List_Empty(head) ) return NULL;

	one = head->next;
	List_Del(one);
	return one;
}

static MV_INLINE List_Head* Counted_List_GetFirst(Counted_List_Head *head)
{
	List_Head *one = NULL;
	if ( Counted_List_Empty(head) ) return NULL;
	one = head->next;
	Counted_List_Del(one, head);
	return one;
}

static MV_INLINE List_Head* List_GetLast(List_Head *head)
{
	List_Head * one = NULL;
	if ( List_Empty(head) ) return NULL;

	one = head->prev;
	List_Del(one);
	return one;
}

static MV_INLINE List_Head* Counted_List_GetLast(Counted_List_Head *head)
{
	List_Head * one = NULL;
	if ( Counted_List_Empty(head) ) return NULL;

	one = head->prev;
	Counted_List_Del(one, head);
	return one;
}

static MV_INLINE void __List_Splice(List_Head *list,
	List_Head *head)
{
	List_Head *first = list->next;
	List_Head *last = list->prev;
	List_Head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

static MV_INLINE void __List_SpliceTail(List_Head *list,
	List_Head *head)
{
	List_Head *first = list->next;
	List_Head *last = list->prev;
	List_Head *at = head->prev;

	first->prev = at;
	at->next = first;

	last->next = head;
	head->prev = last;
}

/**
 * List_Splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static MV_INLINE void List_Splice(List_Head *list, List_Head *head)
{
	if (!List_Empty(list))
		__List_Splice(list, head);
}

/**
 * List_AddList - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */

static MV_INLINE void List_AddList(List_Head *list,
				 List_Head *head)
{
	if (!List_Empty(list)) {
		__List_Splice(list, head);
		MV_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void List_AddCountedList(Counted_List_Head *list, List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		__List_Splice((List_Head *)list, head);
		MV_COUNTED_LIST_HEAD_INIT( list );
	}

}

static MV_INLINE void Counted_List_AddList(List_Head *list, Counted_List_Head *head)
{
	if (!List_Empty(list)) {
		head->node_count += List_GetCount(list);
		__List_Splice(list, (List_Head *) head);
		MV_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void Counted_List_AddCountedList(Counted_List_Head *list, Counted_List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		head->node_count += list->node_count;
		__List_Splice((List_Head *)list, (List_Head *) head);
		MV_COUNTED_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void List_AddListTail(List_Head *list, List_Head *head)
{
	if (!List_Empty(list)) {
		__List_SpliceTail(list, head);
		MV_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void List_AddCountedList_Tail(Counted_List_Head *list, List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		__List_SpliceTail((List_Head *) list, head);
		MV_COUNTED_LIST_HEAD_INIT(list);
	}
}

static MV_INLINE void Counted_List_AddList_Tail(List_Head *list, Counted_List_Head *head)
{
	if (!List_Empty(list)) {
		head->node_count += List_GetCount(list);
		__List_SpliceTail(list, (List_Head *) head);
		MV_LIST_HEAD_INIT(list);
	}
}

static MV_INLINE void Counted_List_AddCountedList_Tail(Counted_List_Head *list, Counted_List_Head *head)
{
	if (!Counted_List_Empty(list)) {
		head->node_count += list->node_count;
		__List_SpliceTail((List_Head *) list, (List_Head *) head);
		MV_COUNTED_LIST_HEAD_INIT(list);
	}
}

