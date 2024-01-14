#include "hashtable.h"

unsigned hashtableHash(char* s) {
	unsigned hashval;
	for (hashval = 0; *s != '\0'; s++) {
		hashval = *s + 31 * hashval;
	}
	return hashval % HASHSIZE;
}

HashEntry* hashtableLookup(HashTable hashtable, char* s) {
	HashEntry* entry = hashtable[hashtableHash(s)];
	while (entry != NULL) {
		if(strcmp(s, entry->name) == 0) {
			return entry;
		}

		entry = entry->next;
	}

	return NULL;
}

HashEntry* hashtableAdd(HashTable hashtable, char* name, char* value) {
	unsigned hashval;

	HashEntry* np = hashtableLookup(hashtable, name);
	if (np == NULL) {
		np = (HashEntry*) malloc(sizeof(*np));
		if (np == NULL) {
			return NULL;
		}

		np->name = strdup(name);
		if (np->name == NULL) {
			free(np);
			return NULL;
		}

		hashval = hashtableHash(name);
		np->next = hashtable[hashval];
		hashtable[hashval] = np;
	}
	else {
		free(np->value);
	}

	np->value = strdup(value);
	if(np->value == NULL) {
		free(np->name);
		free(np);
		return NULL;
	}

	return np;
}

int hashtableRemove(HashTable hashtable, char* name) {
	unsigned hashval = hashtableHash(name);
	HashEntry* entry = hashtable[hashval];
	HashEntry* prev = NULL;

	// If the entry is the only one in the bucket => clear the bucket
	if (entry != NULL && entry->next == NULL) {
		hashentryFree(entry);
		hashtable[hashval] = NULL;
		return 0;
	}

	while (entry != NULL) {
		if (strcmp(name, entry->name) == 0) {
			if (prev == NULL) {
				hashtable[hashval] = entry->next;
			}
			else {
				prev->next = entry->next;
			}

			hashentryFree(entry);
			return 0;
		}

		prev = entry;
		entry = entry->next;
	}

	return -1;
}

HashTable hashtableCreate() {
	HashTable hashtable = (HashTable) calloc(HASHSIZE, sizeof(HashEntry*));
	if (hashtable == NULL) {
		return NULL;
	}

	return hashtable;
}

void hashtableFree(HashTable hashtable) {
	if(hashtable == NULL) {
		return;
	}

	for (int i = 0; i < HASHSIZE; i++) {
		HashEntry* entry = hashtable[i];
		hashentryFullFree(entry);
	}

	free(hashtable);
}

void hashentryFullFree(HashEntry* entry) {
	while (entry != NULL) {
		HashEntry* next = entry->next;
		hashentryFree(entry);
		entry = next;
	}
}

void hashentryFree(HashEntry* entry) {
	if(entry == NULL) {
		return;
	}

	free(entry->name);
	free(entry->value);
	free(entry);
}

void hashtablePrint(FILE* out, HashTable ht) {
	if(ht == NULL) {
		return;
	}

	for (int i = 0; i < HASHSIZE; i++) {
		HashEntry* entry = ht[i];
		while (entry != NULL) {
			fprintf(out, "\t%s: %s\n", entry->name, entry->value);
			entry = entry->next;
		}
	}
}
