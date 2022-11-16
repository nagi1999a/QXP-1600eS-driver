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

#if !defined(COMMON_LIST_H)
#define COMMON_LIST_H

#include "com_define.h"

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */


/*
 *
 *
 * Data Structure
 *
 *
 */
typedef struct _List_Head {
	struct _List_Head *prev, *next;
} List_Head, * PList_Head;

typedef struct _Counted_List_Head {
	struct _List_Head *prev, *next;
	MV_U32 node_count;
} Counted_List_Head, * PCounted_List_Head;

/*
 *
 *
 * Exposed Functions
 *
 *
 */
 
#define MV_LIST_HEAD(name) \
	List_Head name = { &(name), &(name) }

#define MV_LIST_HEAD_INIT(ptr) do { \
	(ptr)->next = (ptr)->prev = (ptr); \
} while (0)

#define MV_COUNTED_LIST_HEAD_INIT(ptr) do { \
	(ptr)->next = (ptr)->prev = (List_Head *)(ptr); \
	(ptr)->node_count = 0; \
} while (0)


#if defined( SUPPORT_ROC )&&defined( MV_ROC_IOP_TUNED )
#define MV_ROC_LIST_MUST_BE_MACRO								0

#if (MV_ROC_LIST_MUST_BE_MACRO==0)
static MV_INLINE void List_Add(List_Head *new_one, List_Head *head);

static MV_INLINE void List_AddTail(List_Head *new_one, List_Head *head);

static MV_INLINE void List_Del(List_Head *entry);

static MV_INLINE void List_DelInit(List_Head *entry);

static MV_INLINE void List_Move(List_Head *list, List_Head *head);

static MV_INLINE void List_MoveTail(List_Head *list,
				  List_Head *head);

static MV_INLINE int List_Empty(const List_Head *head);
#endif


#if (MV_ROC_LIST_MUST_BE_MACRO==1)
#define List_Empty(head)	(((head)->next)==(head))
#define __List_Add(new_one_,prev_,next_)							\
({                                                             \
	(next_)->prev = (new_one_);                                     \
	(new_one_)->next = (next_);                                     \
	(new_one_)->prev = (prev_);                                     \
	(prev_)->next = (new_one_);                                     \
})
                                                                
#define List_Add(new_one, head)                                 \
({                                                             \
	register List_Head *a = ((head)->next);					\
	__List_Add((new_one), (head), a);    					\
})
                                                                
#define List_AddTail(new_one, head)                             \
({                                                             \
	register List_Head *a = ((head)->prev);					\
	__List_Add((new_one), a, (head));    		\
})
                                                                
#define __List_Del( prev_,  next_)                              \
({                                                             \
	(next_)->prev = (prev_);                                      \
	(prev_)->next = (next_);                                      \
})                                          
                                                                
#define List_Del(entry)                                         \
({                                                             \
	__List_Del(((entry)->prev), ((entry)->next));                   \
	(entry)->next = (entry)->prev = NULL;                       \
})                                                   
#define List_DelInit(entry)                                    	\
({                                                             \
	List_Del(entry);											\
	MV_LIST_HEAD_INIT(entry);                                   \
})
                                                                
#define List_Move(list, head)                                   \
({                                                             \
	List_Del(entry);											\
    List_Add(list, head);                                   \
})                                                      
                                                                
#define List_MoveTail(list, head)                               \
({                                                             \
	List_Del(entry);											\
    List_AddTail(list, head);                               \
})                                                      
#define List_GetCount(head)               		                 \
({                                                              \
	register List_Head *pos;                                    \
	register int i=0;                                           \
	LIST_FOR_EACH(pos, head) {                                  \
		i++;                                                    \
	}                                                           \
	(i);                                                          \
})             
                                                                
#define List_GetFirst(head)                          			\
({                                                              \
	register List_Head *one = NULL;		                            \
	if ( !List_Empty(head) ) {			                        \
		one = (head)->next;                                     \
		List_Del(one);                                          \
	}											\
	(one);                                                        \
})                                                              
                                                                
#define List_GetLast(head)                           			\
({                                                              \
	register List_Head *one = NULL;      			                    \
	if ( !List_Empty(head) ) {                        			\
		one = (head)->prev;                                     \
		List_Del(one);                                          \
	}											\
	(one);                                                        \
})

static MV_INLINE void List_AddList(List_Head *list,
				 List_Head *head)
{
	if (!List_Empty(list)) {
		List_Head *first = list->next;
		List_Head *last = list->prev;
		List_Head *at = head->next;

		first->prev = head;
		head->next = first;

		last->next = at;
		at->prev = last;
		MV_LIST_HEAD_INIT( list );
	}
}

static MV_INLINE void List_AddListTail(List_Head *list, List_Head *head)
{
	if (!List_Empty(list)) {
		List_Head *first = list->next;
		List_Head *last = list->prev;
		List_Head *at = head->prev;

		first->prev = at;
		at->next = first;

		last->next = head;
		head->prev = last;
		MV_LIST_HEAD_INIT( list );
	}
}
#endif
#endif

/**
 * LIST_ENTRY - get the struct for this entry
 * @ptr:	the &List_Head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
//TBD
/*#define CONTAINER_OF(ptr, type, member) ({			\
*        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
*        (type *)( (char *)__mptr - OFFSET_OF(type,member) );})
*/

#define CONTAINER_OF(ptr, type, member) 			\
        ( (type *)( (char *)(ptr) - OFFSET_OF(type,member) ) )

#define LIST_ENTRY(ptr, type, member) \
	CONTAINER_OF(ptr, type, member)


/**
 * LIST_FOR_EACH	-	iterate over a list
 * @pos:	the &List_Head to use as a loop counter.
 * @head:	the head for your list.
 */
#define LIST_FOR_EACH(pos, head) \
	for (pos = (head)->next; pos != (List_Head *)(head); pos = pos->next)

/**
 * LIST_FOR_EACH_PREV	-	iterate over a list backwards
 * @pos:	the &List_Head to use as a loop counter.
 * @head:	the head for your list.
 */
#define LIST_FOR_EACH_PREV(pos, head) \
	for (pos = (head)->prev; pos != (List_Head *)(head); pos = pos->prev)

/**
 * LIST_FOR_EACH_ENTRY	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY(pos, head, member)				\
	for (pos = LIST_ENTRY((head)->next, typeof(*pos), member);	\
	     &pos->member != (List_Head *)(head); 	\
	     pos = LIST_ENTRY(pos->member.next, typeof(*pos), member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY_SAFE(pos, n, head, member)			\
			for (pos = LIST_ENTRY((head)->next, typeof(*pos), member),	\
				n = LIST_ENTRY(pos->member.next, typeof(*pos), member); \
				 &pos->member != (List_Head *) (head);					\
				 pos = n, n = LIST_ENTRY(n->member.next, typeof(*n), member))
	


/**
 * LIST_FOR_EACH_ENTRY_TYPE	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 * @type:	the type of the struct this is embedded in.
*/
#define LIST_FOR_EACH_ENTRY_TYPE(pos, head, type, member)       \
	for (pos = LIST_ENTRY((head)->next, type, member);	\
	     &pos->member != (List_Head *) (head); 	                        \
	     pos = LIST_ENTRY(pos->member.next, type, member))

/**
 * LIST_FOR_EACH_ENTRY_PREV - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define LIST_FOR_EACH_ENTRY_PREV(pos, head, member)			\
	for (pos = LIST_ENTRY((head)->prev, typeof(*pos), member);	\
	     &pos->member != (List_Head *)(head); 	\
	     pos = LIST_ENTRY(pos->member.prev, typeof(*pos), member))



#if !defined(SUPPORT_ROC)
	#if !defined( _OS_BIOS )
	#include "com_list.c"
	#endif
#else
	#if (MV_ROC_LIST_MUST_BE_MACRO==0)
       #include "com_list.c"
	#endif
#endif


#define List_GetFirstEntry(head, type, member)	\
	LIST_ENTRY(List_GetFirst(head), type, member)

#define Counted_List_GetFirstEntry(head, type, member) \
	LIST_ENTRY(Counted_List_GetFirst(head), type, member)

#define List_GetLastEntry(head, type, member)	\
	LIST_ENTRY(List_GetLast(head), type, member)

#define Counted_List_GetLastEntry(head, type, member) \
	LIST_ENTRY(Counted_List_GetLast(head), type, member)
#ifdef _OS_WINDOWS
#define List_GetFirstEntryLock(first, head, lock, type, member)	do{ \
	KeAcquireSpinLockAtDpcLevel(lock);					\
	first=(type *)LIST_ENTRY(List_GetFirst(head), type, member);\
	KeReleaseSpinLockFromDpcLevel(lock);	\
	}while(0)
#define List_GetLastEntryLock(last, head, lock, type, member)	do{\
	KeAcquireSpinLockAtDpcLevel(lock);					\
	last=(type *)LIST_ENTRY(List_GetLast(head, lock), type, member);\
	KeReleaseSpinLockFromDpcLevel(lock);	\
	}while(0)
#define List_AddLock(new_one, head, lock)  do{\
	KeAcquireSpinLockAtDpcLevel(lock);	\
	List_Add(new_one, head);	\
	KeReleaseSpinLockFromDpcLevel(lock);	\
	}while(0)
#define List_AddTailLock(new_one, head, lock)   do{\
	KeAcquireSpinLockAtDpcLevel(lock);	\
	List_AddTail(new_one, head);	\
	KeReleaseSpinLockFromDpcLevel(lock);	\
	}while(0)
#endif
#endif /* COMMON_LIST_H */
