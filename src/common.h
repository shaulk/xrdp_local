#ifndef COMMON_H
#define COMMON_H

// Common utilities

#define LOG_ERROR 1
#define LOG_WARN 2
#define LOG_INFO 3
#define LOG_DEBUG 4

void log(int level, const char *format, ...);

void set_log_level(int level);

#endif // COMMON_H
