#include <chill_hashtable.h>

unsigned hashtableHash(char* s) {
	unsigned hashval;
	for (hashval = 0; *s != '\0'; s++) {
		hashval = *s + 31 * hashval;
	}
	return hashval % HASHSIZE;
}

HashEntry* hashtableLookup(HashTable* hashtable, char* s) {
	if(hashtable == NULL) {
		return NULL;
	}

	unsigned hash = hashtableHash(s);
	HashEntry* entry = hashtable->entries[hash];
	while (entry != NULL) {
		if(strcmp(s, entry->name) == 0) {
			return entry;
		}

		entry = entry->next;
	}

	return NULL;
}

HashEntry* createHashEntry(char* name, char* value) {
	HashEntry* entry = (HashEntry*)malloc(sizeof(HashEntry));
	if (entry == NULL) {
		goto _failure;
	}

	entry->name = strdup(name);
	if (entry->name == NULL) {
		goto _cleanup_entry;
	}

	entry->value = strdup(value);
	if (entry->value == NULL) {
		goto _cleanup_entry;
	}

	entry->next = NULL;
	return entry;

_cleanup_entry:
	free(entry->name);
	free(entry->value);
	free(entry);
_failure:
	return NULL;
}

HashEntry* hashtableAdd(HashTable* hashtable, char* name, char* value) {
	if(hashtable == NULL) {
		return NULL;
	}

	unsigned int hashval = hashtableHash(name);
	HashEntry* entry = hashtable->entries[hashval];
	if (entry != NULL) {
		while (entry->next != NULL && strcmp(name, entry->name) != 0) {
			entry = entry->next;
		}

		if (strcmp(name, entry->name) == 0) {
			free(entry->value);
			entry->value = strdup(value);
			return entry;
		}

		HashEntry* newEntry = createHashEntry(name, value);
		if (newEntry == NULL) {
			return NULL;
		}

		entry->next = newEntry;
		hashtable->size += 1;
		return newEntry;
	}
	else {
		entry = createHashEntry(name, value);
		if(entry == NULL) {
			return NULL;
		}

		hashtable->entries[hashval] = entry;
		hashtable->size += 1;
		return entry;
	}

	return NULL;
}

int hashtableRemove(HashTable* hashtable, char* name) {
	if (hashtable == NULL) {
		return -1;
	}

	unsigned hashval = hashtableHash(name);
	HashEntry* entry = hashtable->entries[hashval];
	HashEntry* prev = NULL;

	// If the entry is the only one in the bucket => clear the bucket
	if (entry != NULL && entry->next == NULL) {
		hashentryFree(entry);
		hashtable->entries[hashval] = NULL;
		hashtable->size -= 1;
		return 0;
	}

	while (entry != NULL) {
		if (strcmp(name, entry->name) == 0) {
			if (prev == NULL) {
				hashtable->entries[hashval] = entry->next;
			}
			else {
				prev->next = entry->next;
			}

			hashentryFree(entry);
			hashtable->size -= 1;
			return 0;
		}

		prev = entry;
		entry = entry->next;
	}

	return -1;
}

HashTable* hashtableCreate() {
	HashEntry** entries = (HashEntry**) calloc(HASHSIZE, sizeof(HashEntry*));

	if (entries == NULL) {
		goto _failure;
	}

	HashTable* hashtable = (HashTable*) malloc(sizeof(HashTable));
	if (hashtable == NULL) {
		goto _clean_entries;
	}

	hashtable->entries = entries;
	hashtable->hashsize = HASHSIZE;
	hashtable->size = 0;

	return hashtable;

_clean_entries:
	free(entries);

_failure:
	return NULL;
}

void hashtableFree(HashTable* hashtable) {
	if(hashtable == NULL) {
		return;
	}

	if (hashtable->entries != NULL) {
		for (int i = 0; i < HASHSIZE; i++) {
			HashEntry* entry = hashtable->entries[i];
			hashentryFullFree(entry);
		}
	}

	free(hashtable->entries);
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

void hashtablePrint(FILE* out, HashTable* ht) {
	if(ht == NULL || ht->entries == NULL) {
		return;
	}

	for (int i = 0; i < HASHSIZE; i++) {
		HashEntry* entry = ht->entries[i];
		while (entry != NULL) {
			fprintf(out, "\t%s: %s\n", entry->name, entry->value);
			entry = entry->next;
		}
	}
}
