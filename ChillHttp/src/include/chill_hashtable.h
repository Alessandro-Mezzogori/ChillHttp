#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// workaround for strdup deprecation error
#define strdup _strdup

typedef struct HashEntry {
	struct HashEntry *next;
	char *name;
	char *value;
} HashEntry;

typedef struct HashTable {
	HashEntry** entries;
	size_t hashsize;
	size_t size;
} HashTable;

#define HASHSIZE 101	

unsigned hashtableHash(const char *s);
HashEntry* hashtableLookup(HashTable* hashtable, const char* s);
HashEntry* hashtableAdd(HashTable* hashtable, const char* name, const char* value);
HashTable* hashtableCreate();
int hashtableRemove(HashTable* hashtable, const char* name);
void hashtableFree(HashTable* hashtable);
void hashentryFullFree(HashEntry* hashtable);
void hashentryFree(HashEntry* hashtable);
void hashtablePrint(FILE* out, HashTable* hashtable);
