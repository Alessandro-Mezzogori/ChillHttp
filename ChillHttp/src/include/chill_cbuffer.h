#pragma once

#include <chill_log.h>

#define DEFAULT_BUFFER_SIZE 4096

typedef struct ringbuffer_t {
	char buffer[DEFAULT_BUFFER_SIZE];
	size_t read;
	size_t written;
} ringbuffer_t;

errno_t write(ringbuffer_t* buffer, const char* data, size_t size);
errno_t read(ringbuffer_t* buffer, char* data, size_t size);
