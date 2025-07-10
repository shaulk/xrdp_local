#include <stdio.h>
#include <stdarg.h>

#include "common.h"

void log(int level, const char *format, ...) {
	if (level < LOG_DEBUG) {
		return;
	}

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}
