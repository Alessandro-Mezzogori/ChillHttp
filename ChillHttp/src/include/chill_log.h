#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <varargs.h>
#include <time.h>

enum LOG_LEVEL {
	LL_NONE		= 0,
	LL_TRACE	= 1,
	LL_DEBUG	= 2,
	LL_INFO		= 3,
	LL_WARN		= 4,
	LL_ERROR	= 5,
	LL_FATAL	= 6,
};

extern const enum LOG_LEVEL log_runtime_level;
int logger(FILE* out, const enum LOG_LEVEL lvl, const char* func, const unsigned int line, const char* fmt, ...);
void flushLogger(FILE* out);

#ifndef LOG_BUILD_LEVEL
	#ifdef DEBUG
		#define LOG_BUILD_LEVEL LL_TRACE
	#else
		#define LOG_BUILD_LEVEL LL_TRACE
	#endif // DEBUG
#endif // !LOG_BUILD_LEVEL

#define SHOULD_LOG(lvl) ( lvl >= log_runtime_level && lvl >= LOG_BUILD_LEVEL)

#ifndef LOG_FP
	#ifdef stdout
		#define LOG_FP stdout
	#endif // stdout
#endif // !LOG_FP

#define LOG(level, fmt, ...) do { \
	if (SHOULD_LOG(level)) { \
		logger(LOG_FP, level, __func__, __LINE__, fmt, __VA_ARGS__); \
	} \
} while(0)

#define LOG_R(level, func, line, fmt, ...) do { \
	if (SHOULD_LOG(level)) { \
		logger(LOG_FP, level, func, line, fmt, __VA_ARGS__); \
	} \
} while(0)

#define LOG_TRACE(fmt, ...) LOG(LL_TRACE, fmt, __VA_ARGS__) 
#define LOG_DEBUG(fmt, ...) LOG(LL_DEBUG, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...)  LOG(LL_INFO, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...)  LOG(LL_WARN, fmt, __VA_ARGS__)
#define LOG_ERROR(fmt, ...) LOG(LL_ERROR, fmt, __VA_ARGS__)	
#define LOG_FATAL(fmt, ...) LOG(LL_FATAL, fmt, __VA_ARGS__)	


#define LOG_TRACE_R(fmt, func, line, ...) LOG_R(LL_TRACE, func, line, fmt, __VA_ARGS__) 
#define LOG_DEBUG_R(fmt, func, line, ...) LOG_R(LL_DEBUG, func, line, fmt, __VA_ARGS__)
#define LOG_INFO_R(fmt, func, line, ...)  LOG_R(LL_INFO , func, line, fmt, __VA_ARGS__)
#define LOG_WARN_R(fmt, func, line, ...)  LOG_R(LL_WARN , func, line, fmt, __VA_ARGS__)
#define LOG_ERROR_R(fmt, func, line, ...) LOG_R(LL_ERROR, func, line, fmt, __VA_ARGS__)	
#define LOG_FATAL_R(fmt, func, line, ...) LOG_R(LL_FATAL, func, line, fmt, __VA_ARGS__)	

#define LOG_FLUSH() flushLogger(LOG_FP)


