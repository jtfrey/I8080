/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 timing API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080TIMING_H__
#define __I8080TIMING_H__

#include "I8080Config.h"
#include <time.h>

/**
 * Number of microseconds
 * A type alias (for a double-precision float) that is used to
 * represent a count of microseconds of time.  This will primarily
 * be used to turn a (sec,nsec) timespec into an
 * easily-manipulated form:  simple conditional tests and addition
 * can be used in lieu of operations on the native integer fields
 * of the timespec struct.
 */
typedef double I8080Microseconds;

/**
 * Generate a I8080Microseconds from the current timestamp
 * Calculate the number of microseconds associated with the
 * current timestamp and return a I8080Microseconds with that
 * value.
 * @return          the number of milliseconds represented
 *                  by \p t
 */
static inline
I8080Microseconds
I8080MicrosecondsMakeNow()
{
    struct timespec     t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (I8080Microseconds)(t.tv_sec * 1.0e6 + t.tv_nsec * 1e-3);
}

/**
 * Generate a I8080Microseconds from a timespec
 * Calculate the number of microseconds associated with the
 * timespec \p t and return a I8080Microseconds with that
 * value.
 * @param t         a timespec struct
 * @return          the number of milliseconds represented
 *                  by \p t
 */
static inline
I8080Microseconds
I8080MicrosecondsMakeWithTimespec(
    const struct timespec   t
)
{
    return (I8080Microseconds)(t.tv_sec * 1.0e6 + t.tv_nsec * 1e-3);
}

/**
 * Fill-in a timespec with a microsecond count
 * The timespec structure pointed at by \p out_timespec is filled-in
 * with the split-out seconds/nanoseconds in \p usec.
 * @param usec          the number of microseconds
 * @param out_timespec  pointer to the timespec structure to fill-in
 * @return              the value of out_timespec
 */
static inline
struct timespec*
I8080MicrosecondsToTimeSpec(
    I8080Microseconds   usec,
    struct timespec     *out_timespec
)
{
    double              s, ns = 1e9 * modf(usec * 1e-6, &s);
    out_timespec->tv_sec = (time_t)trunc(s);
    out_timespec->tv_nsec = (long)trunc(ns);
    return out_timespec;
}

/**
 * Sleep until a timing interval has elapsed
 * If the next cycle of \p ti has not yet elapsed, sleep until
 * the next cycle time is reached.  Before exiting the next
 * cycle time is set.
 * @param ti            the timing interval
 */
static inline
void
I8080TimingSleep(
    I8080Microseconds   usec
)
{
    if ( usec > 0.0 ) {
        struct timespec dt_req, dt_rem;
        
        I8080MicrosecondsToTimeSpec(usec, &dt_req);
        while ( nanosleep(&dt_req, &dt_rem) == -1 ) {
            dt_req = dt_rem;
        }
    }
}

#endif /* __I8080TIMING_H__ */
