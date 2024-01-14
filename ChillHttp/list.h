#pragma once

typedef struct DynamicString {
	char *data;
	size_t length;
	size_t capacity;
} DynamicString;

DynamicString* createDynamicString(size_t capacity);
void freeDynamicString(DynamicString* string);
void appendDynamicString(DynamicString* string, char c);
void appendDynamicStringN(DynamicString* string, char* c, size_t n);
