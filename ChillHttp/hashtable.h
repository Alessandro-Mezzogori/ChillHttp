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
} HashEntry, **HashTable;

#define HASHSIZE 101	

unsigned hashtableHash(char *s);
HashEntry* hashtableLookup(HashTable hashtable, char* s);
HashEntry* hashtableAdd(HashTable hashtable, char* name, char* value);
HashTable hashtableCreate();
int hashtableRemove(HashTable hashtable, char* name);
void hashtableFree(HashTable hashtable);
void hashentryFullFree(HashEntry* hashtable);
void hashentryFree(HashEntry* hashtable);
void hashtablePrint(FILE* out, HashTable hashtable);
