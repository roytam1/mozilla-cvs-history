/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is SniffURI.
 *
 * The Initial Developer of the Original Code is
 * Erik van der Poel <erik@vanderpoel.org>.
 * Portions created by the Initial Developer are Copyright (C) 1998-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _HASH_H_
#define _HASH_H_

typedef struct HashEntry
{
	unsigned char		*key;
	void			*value;
	struct HashEntry	*next;
} HashEntry;

typedef struct HashTable
{
	HashEntry	**buckets;
	int		count;
	void		(*free)(unsigned char *key, void *value);
	int		size;
} HashTable;

HashEntry *hashAdd(HashTable *table, unsigned char *key, void *value);
HashTable *hashAlloc(void (*func)(unsigned char *key, void *value));
void hashEnumerate(HashTable *table, void (*func)(HashEntry *));
void hashFree(HashTable *table);
HashEntry *hashLookup(HashTable *table, unsigned char *key);

#endif /* _HASH_H_ */
