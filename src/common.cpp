#include <stdio.h>
#include <stdarg.h>

#include "common.h"

static int log_level = LOG_INFO;

void log(int level, const char *format, ...) {
	if (level > log_level) {
		return;
	}

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fflush(stderr);
}

void set_log_level(int level) {
	log_level = level;
}
