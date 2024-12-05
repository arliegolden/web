#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

// Log levels
typedef enum {
    LOG_TRACE = 0,
    LOG_DEBUG = 1,
    LOG_INFO = 2,
    LOG_WARN = 3,
    LOG_ERROR = 4,
    LOG_FATAL = 5
} log_level_t;

// Color codes
#define COLOR_RESET   "\x1B[0m"
#define COLOR_RED     "\x1B[31m"
#define COLOR_GREEN   "\x1B[32m"
#define COLOR_YELLOW  "\x1B[33m"
#define COLOR_BLUE    "\x1B[34m"
#define COLOR_MAGENTA "\x1B[35m"
#define COLOR_CYAN    "\x1B[36m"
#define COLOR_WHITE   "\x1B[37m"

// Initialize the logger
int logger_init(const char *log_file_path);

// Cleanup logger resources
void logger_cleanup(void);

// Set log level
void logger_set_level(log_level_t level);

// Log functions
void log_trace(const char *file, int line, const char *fmt, ...);
void log_debug(const char *file, int line, const char *fmt, ...);
void log_info(const char *file, int line, const char *fmt, ...);
void log_warn(const char *file, int line, const char *fmt, ...);
void log_error(const char *file, int line, const char *fmt, ...);
void log_fatal(const char *file, int line, const char *fmt, ...);

// Convenience macros
#define TRACE(...) log_trace(__FILE__, __LINE__, __VA_ARGS__)
#define DEBUG(...) log_debug(__FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)  log_info(__FILE__, __LINE__, __VA_ARGS__)
#define WARN(...)  log_warn(__FILE__, __LINE__, __VA_ARGS__)
#define ERROR(...) log_error(__FILE__, __LINE__, __VA_ARGS__)
#define FATAL(...) log_fatal(__FILE__, __LINE__, __VA_ARGS__)

#endif