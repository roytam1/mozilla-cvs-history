/* 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is the Netscape security libraries.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are 
 * Copyright (C) 1994-2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable 
 * instead of those above.  If you wish to allow use of your 
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile$ $Revision$ $Date$ $Name$";
#endif /* DEBUG */

/*
 * list.c
 *
 * This contains the implementation of NSS's thread-safe linked list.
 */

#ifndef BASE_H
#include "base.h"
#endif /* BASE_H */

struct nssListElementStr {
    PRCList  link;
    void    *data;
};

typedef struct nssListElementStr nssListElement;

struct nssListStr {
    NSSArena       *arena;
    PZLock         *lock;
    nssListElement *head;
    PRUint32        count;
    nssListCompareFunc compareFunc;
};

struct nssListIteratorStr {
    nssList *list;
    nssListElement *current;
};

#define NSSLIST_LOCK_IF(list) \
    if ((list)->lock) PZ_Lock((list)->lock)

#define NSSLIST_UNLOCK_IF(list) \
    if ((list)->lock) PZ_Unlock((list)->lock)

static PRBool
pointer_compare(void *a, void *b)
{
    return (PRBool)(a == b);
}

static nssListElement *
nsslist_get_matching_element(nssList *list, void *data)
{
    PRCList *link;
    nssListElement *node;
    node = list->head;
    link = &node->link;
    while (node) {
	/* using a callback slows things down when it's just compare ... */
	if (list->compareFunc(node->data, data)) {
	    break;
	}
	link = &node->link;
	if (link == PR_LIST_TAIL(&list->head->link)) break;
	node = (nssListElement *)PR_NEXT_LINK(&node->link);
    }
    return node;
}

NSS_IMPLEMENT nssList *
nssList_Create
(
  NSSArena *arenaOpt,
  PRBool threadSafe
)
{
    NSSArena *arena;
    nssList *list;
    arena = (arenaOpt) ? arenaOpt : nssArena_Create();
    if (!arena) {
	return (nssList *)NULL;
    }
    list = nss_ZNEW(arena, nssList);
    if (!list) {
	return (nssList *)NULL;
    }
    if (threadSafe) {
	list->lock = PZ_NewLock(nssILockOther);
	if (!list->lock) {
	    if (arenaOpt) {
		nss_ZFreeIf(list);
	    } else {
		NSSArena_Destroy(arena);
	    }
	    return (nssList *)NULL;
	}
    }
    if (!arenaOpt) {
	list->arena = arena;
    }
    list->compareFunc = pointer_compare;
    return list;
}

NSS_IMPLEMENT PRStatus
nssList_Destroy(nssList *list)
{
    if (list->lock) PZ_DestroyLock(list->lock);
    NSSArena_Destroy(list->arena);
    return PR_SUCCESS;
}

NSS_IMPLEMENT void
nssList_SetCompareFunction(nssList *list, nssListCompareFunc compareFunc)
{
    list->compareFunc = compareFunc;
}

NSS_IMPLEMENT nssListCompareFunc
nssList_GetCompareFunction(nssList *list)
{
    return list->compareFunc;
}

NSS_IMPLEMENT PRStatus
nssList_DestroyElements(nssList *list, nssListElementDestructorFunc destructor)
{
    PRCList *link;
    nssListElement *node;
    NSSLIST_LOCK_IF(list);
    node = list->head;
    while (node && list->count > 0) {
	(*destructor)(node->data);
	link = &node->link;
	node = (nssListElement *)PR_NEXT_LINK(link);
	PR_REMOVE_LINK(link);
	--list->count;
    }
    NSSLIST_UNLOCK_IF(list);
    return nssList_Destroy(list);
}

static PRStatus
nsslist_add_element(nssList *list, void *data)
{
    nssListElement *node = nss_ZNEW(list->arena, nssListElement);
    PR_INIT_CLIST(&node->link);
    node->data = data;
    if (list->head) {
	PR_APPEND_LINK(&node->link, &list->head->link);
    } else {
	list->head = node;
    }
    ++list->count;
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
nssList_AddElement(nssList *list, void *data)
{
    PRStatus nssrv;
    NSSLIST_LOCK_IF(list);
    nssrv = nsslist_add_element(list, data);
    NSSLIST_UNLOCK_IF(list);
    return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
nssList_AddElementUnique(nssList *list, void *data)
{
    PRStatus nssrv;
    nssListElement *node;
    NSSLIST_LOCK_IF(list);
    node = nsslist_get_matching_element(list, data);
    if (node) {
	/* already in, finish */
	NSSLIST_UNLOCK_IF(list);
	return PR_SUCCESS;
    }
    nssrv = nsslist_add_element(list, data);
    NSSLIST_UNLOCK_IF(list);
    return nssrv;
}

NSS_IMPLEMENT PRStatus
nssList_RemoveElement(nssList *list, void *data)
{
    nssListElement *node;
    NSSLIST_LOCK_IF(list);
    node = nsslist_get_matching_element(list, data);
    if (node) {
	if (node == list->head) {
	    list->head = (nssListElement *)PR_NEXT_LINK(&node->link);
	}
	PR_REMOVE_LINK(&node->link);
	nss_ZFreeIf(node);
	--list->count;
    }
    NSSLIST_UNLOCK_IF(list);
    return PR_SUCCESS;
}

NSS_IMPLEMENT void *
nssList_GetElement(nssList *list, void *data)
{
    nssListElement *node;
    NSSLIST_LOCK_IF(list);
    node = nsslist_get_matching_element(list, data);
    NSSLIST_UNLOCK_IF(list);
    return node->data;
}

NSS_IMPLEMENT PRUint32
nssList_Count(nssList *list)
{
    return list->count;
}

NSS_IMPLEMENT PRStatus
nssList_GetArray(nssList *list, void **rvArray, PRUint32 maxElements)
{
    nssListIterator *iter;
    void *el;
    int i = 0;
    iter = nssList_CreateIterator(list);
    for (el = nssListIterator_Start(iter); el != NULL && i < maxElements; 
         el = nssListIterator_Next(iter)) 
    {
	rvArray[i++] = el;
    }
    rvArray[i] = NULL;
    nssListIterator_Finish(iter);
    nssListIterator_Destroy(iter);
    return PR_SUCCESS;
}

NSS_IMPLEMENT nssListIterator *
nssList_CreateIterator(nssList *list)
{
    nssListIterator *rvIterator;
    rvIterator = nss_ZNEW(list->arena, nssListIterator);
    rvIterator->list = list;
    rvIterator->current = list->head;
    return rvIterator;
}

NSS_IMPLEMENT void
nssListIterator_Destroy(nssListIterator *iter)
{
    nss_ZFreeIf(iter);
}

NSS_IMPLEMENT void *
nssListIterator_Start(nssListIterator *iter)
{
    NSSLIST_LOCK_IF(iter->list);
    iter->current = iter->list->head;
    return iter->current->data;
}

NSS_IMPLEMENT void *
nssListIterator_Next(nssListIterator *iter)
{
    nssListElement *node;
    PRCList *link;
    if (iter->list->count == 1 || iter->current == NULL) {
	/* Reached the end of the list.  Don't change the state, force to
	 * user to call nssList_Finish to clean up.
	 */
	return NULL;
    }
    node = (nssListElement *)PR_NEXT_LINK(&iter->current->link);
    link = &node->link;
    if (link == PR_LIST_TAIL(&iter->list->head->link)) {
	/* Signal the end of the list. */
	iter->current = NULL;
	return node->data;
    }
    iter->current = node;
    return node->data;
}

NSS_IMPLEMENT PRStatus
nssListIterator_Finish(nssListIterator *iter)
{
    iter->current = iter->list->head;
    return (iter->list->lock) ? PZ_Unlock(iter->list->lock) : PR_SUCCESS;
}

