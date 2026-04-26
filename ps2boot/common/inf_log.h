#ifndef INF_LOG_H
#define INF_LOG_H

/*
 * Centralised compile-time log level for InfinityStation runtime code.
 *
 *   INF_LOG_LEVEL = 0   -> silent (default)
 *   INF_LOG_LEVEL = 1   -> informational logs
 *   INF_LOG_LEVEL = 2   -> debug logs
 *
 * Override from the command line, e.g.:
 *     make EE_CFLAGS+=-DINF_LOG_LEVEL=2
 *
 * Files that include this header should use INF_LOG_INFO / INF_LOG_DBG
 * for new logging. Plain printf() calls in modules that include this
 * header are silenced when INF_LOG_LEVEL == 0 (matches the previous
 * QUIET_RUNTIME_LOGS behaviour, but without each .c file redefining it).
 */

#include <stdio.h>

#ifndef INF_LOG_LEVEL
#define INF_LOG_LEVEL 0
#endif

#if INF_LOG_LEVEL >= 2
#  define INF_LOG_DBG(...)  do { printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#  define INF_LOG_DBG(...)  ((void)0)
#endif

#if INF_LOG_LEVEL >= 1
#  define INF_LOG_INFO(...) do { printf(__VA_ARGS__); fflush(stdout); } while (0)
#else
#  define INF_LOG_INFO(...) ((void)0)
#endif

#if INF_LOG_LEVEL == 0
#  ifdef printf
#    undef printf
#  endif
#  define printf(...) ((void)0)
#endif

/*
 * INF_SNPRINTF_OK(buf, size, fmt, ...)
 *
 * Wrap snprintf() with an explicit truncation/error check. Evaluates
 * to 1 if the formatted string fit fully in 'buf' (NUL terminator
 * included), or 0 if snprintf returned a negative error, the result
 * was truncated, or 'size' was zero.
 *
 * Use at sites where silent truncation would produce a wrong path
 * or wrong identifier (save paths, ROM candidates, env keys, etc).
 * Display-only sites can stay on plain snprintf.
 *
 * 'size' is evaluated once. Uses GCC statement-expressions, which
 * the rest of the codebase already relies on (mfc0 asm).
 */
#define INF_SNPRINTF_OK(buf, size, ...) (__extension__ ({               \
    size_t _inf_n_size = (size);                                         \
    int _inf_n_written = (_inf_n_size > 0)                               \
        ? snprintf((buf), _inf_n_size, __VA_ARGS__)                      \
        : -1;                                                            \
    (_inf_n_written >= 0 && (size_t)_inf_n_written < _inf_n_size);       \
}))

#endif /* INF_LOG_H */
