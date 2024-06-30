#include <chill_log.h>

//const enum LOG_LEVEL log_runtime_level = LL_TRACE;
const enum LOG_LEVEL log_runtime_level = LL_INFO;
const char* log_level_strings[] = {
	"NONE",
	"TRA",
	"DBG",
	"INF",
	"WRN",
	"ERR",
	"FTL"
};

int logger(FILE* out, const enum LOG_LEVEL lvl, char* const func, const unsigned int line, char* const fmt, ...) {
	// Building loggin timestamp
	time_t t = time(NULL);
	struct tm* tm = localtime(&t);
	char time[64];
	strftime(time, sizeof(time), "%c", tm);

	// building passed format with appropriate variadic params
	char buffer[2048];
	memset(buffer, 0, sizeof(buffer));

	va_list argptr;
	va_start(argptr, fmt);
	// TODO handle buffer overflow
	vsprintf_s(buffer, sizeof(buffer), fmt, argptr);
	va_end(argptr);

	// Printing actual log
	fprintf_s(out, "%s | %s | %-3d:%-25s | %s\n", time, log_level_strings[lvl], line, func, buffer);

	// Flushing log file
	// Not super efficient, but it's for ease of use 
	flushLogger(out);

	return 0;
}

void flushLogger(FILE* out) {
	fflush(out);
}
