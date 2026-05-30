/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 realtime timer.
 */

#include "I8080Timer.h"
#include "I8080Logging.h"
#include "I8080Timing.h"
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>

//

typedef struct __attribute__((packed)) {
    union {
        struct {
            uint8_t             R[8];
        };
        struct {
            I8080TimerEnable_t  enable;
            uint8_t             opcode;
            uint8_t             msecFrac;
            uint8_t             msecWholeB0;
            uint8_t             msecWholeB1;
            uint8_t             msecWholeB2;
            uint8_t             msecWholeB3;
        };
    };
} I8080TimerRegister_t;

typedef struct __attribute__((packed)) {
    union {
        struct __attribute__((packed)) {
            uint8_t             R[16];
        };
        struct __attribute__((packed)) {
            uint8_t             S;
            uint8_t             M;
            uint8_t             H;
            uint8_t             d;
            uint8_t             m;
            uint8_t             Y;
            uint8_t             Ycent;
            uint8_t             fired;
        };
    };
} I8080DateTimeRegister_t;

typedef struct {
    I8080TimerContextPtr    context;
    unsigned                timer_id;
    struct timespec         timeout;
    pthread_t               thread;
    pthread_mutex_t         lock;
    pthread_cond_t          cond;
} I8080TimerThreadState_t;    

typedef struct I8080TimerContext {
    I8080DateTimeRegister_t     datetime;
    I8080TimerRegister_t        timers[8];
    I8080TimerThreadState_t     state[8];
    I8080SystemPtr              sys8080;
    time_t                      now_time;
    struct tm                   now_tm;
    I8080Microseconds           now_us;
} I8080TimerContext_t;

//

static inline
I8080Microseconds
I8080TimerRegisterIntervalInMicroseconds(
    I8080TimerRegister_t    timer
)
{
    I8080Microseconds   msec = ((unsigned)timer.msecWholeB0) |
                               ((unsigned)timer.msecWholeB1 << 8) |
                               ((unsigned)timer.msecWholeB2 << 16) |
                               ((unsigned)timer.msecWholeB3 << 24);
    return (msec + timer.msecFrac / 256.0f);
}

//

static inline
void
I8080TimerRegisterIntervalSetTimespec(
    I8080TimerRegister_t    timer,
    struct timespec         *ts
)
{
    I8080Microseconds       sec = ((unsigned)timer.msecWholeB0) |
                                  ((unsigned)timer.msecWholeB1 << 8) |
                                  ((unsigned)timer.msecWholeB2 << 16) |
                                  ((unsigned)timer.msecWholeB3 << 24);
    I8080Microseconds       whole;
    
    sec = modf(1e-3 * (sec + timer.msecFrac / 256.0f), &whole);
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_nsec += trunc(1e9 * sec);
    if ( ts->tv_nsec > 1e9 ) {
        // pthread_cond_timedwait() complains if the tv_nsec exceeds 1 s:
        ts->tv_nsec -= 1e9;
        ts->tv_sec++;
    }
    ts->tv_sec += trunc(whole);
}

//

static inline
void
I8080TimerRegisterSetIntervalWithMicroseconds(
    I8080TimerRegister_t    *timer,
    I8080Microseconds       usec
)
{
    I8080Microseconds       whole;
    uint64_t                msec;
    
    usec = modf(usec / 1000.0, &whole);
    timer->msecFrac = trunc(usec * 256);
    msec = (uint64_t)trunc(whole);
    if ( msec >= 0x100000000 ) msec = 0xFFFFFFFF;
    timer->msecWholeB0 = msec & 0xFF, msec >>= 8;
    timer->msecWholeB1 = msec & 0xFF, msec >>= 8;
    timer->msecWholeB2 = msec & 0xFF, msec >>= 8;
    timer->msecWholeB3 = msec & 0xFF;
}

//

static inline
I8080TimerContext_t*
I8080TimerContextDateTimeFill(
    I8080TimerContext_t    *context
)
{
    context->datetime.S = context->now_tm.tm_sec;
    context->datetime.M = context->now_tm.tm_min;
    context->datetime.H = context->now_tm.tm_hour;
    context->datetime.d = context->now_tm.tm_mday;
    context->datetime.m = context->now_tm.tm_mon + 1;
    context->datetime.Y = (context->now_tm.tm_year + 1900) % 100;
    context->datetime.Ycent = (context->now_tm.tm_year + 1900) / 100;
    return context;
}

//

static inline
I8080TimerContext_t*
I8080TimerContextDateTimeNow(
    I8080TimerContext_t    *context
)
{
    time(&context->now_time);
    localtime_r(&context->now_time, &context->now_tm);
    return context;
}

//

static
void*
I8080TimerContextTimerThread(
    void        *arg
)
{
    I8080TimerThreadState_t *state = (I8080TimerThreadState_t*)arg;
    I8080TimerContext_t     *context = (I8080TimerContext_t*)state->context;
    I8080TimerEnable_t      enable = kI8080TimerEnableUnused;
    bool                    outer_loop = true;
    unsigned                timer_id;
    
    // Start with the lock held:        
    pthread_mutex_lock(&state->lock);
    
    timer_id = state->timer_id;
    
    while ( outer_loop ) {
        I8080TimerEnable_t  new_enable = context->timers[timer_id].enable;
        
        // Enable state change?
        if ( new_enable != enable ) {
            switch ( new_enable ) {
                case kI8080TimerEnableUnused:
                    outer_loop = false;
                    break;
                case kI8080TimerEnableOff:
                    break;
                case kI8080TimerEnableOn:
                case kI8080TimerEnableAutoReenable:
                    // Set the timeout interval:
                    I8080TimerRegisterIntervalSetTimespec(context->timers[timer_id], &state->timeout);
                    break;
            }
            context->timers[timer_id].enable = enable = new_enable;
        }
        switch ( enable ) {
            case kI8080TimerEnableUnused:
                // We're headed out of the loop...
                outer_loop = false;
                break;
            case kI8080TimerEnableOff:
                // Wait for a configurational change to wake us up:
                pthread_cond_wait(&state->cond, &state->lock);
                break;
            case kI8080TimerEnableOn:
            case kI8080TimerEnableAutoReenable: {
                bool        inner_loop = true;
                while ( inner_loop ) {
                    I8080Microseconds   now = I8080MicrosecondsMakeNow();
                    int                 rc;
                    
                    rc = pthread_cond_timedwait(&state->cond, &state->lock, &state->timeout);
                    if ( rc == ETIMEDOUT ) {
                        // Interrupt!!
                        DEBUG("Timer %u fired @ ∆t = %g µs", timer_id, I8080MicrosecondsMakeNow() - now);
                        context->datetime.fired = timer_id;
                        if ( context->sys8080 ) I8080SystemRaiseInterrupt(context->sys8080, context->timers[timer_id].opcode);
                        if ( enable == kI8080TimerEnableOn ) {
                            // Change the timer enable state and exit this loop:
                            context->timers[timer_id].enable = kI8080TimerEnableOff;
                            inner_loop = false;
                        } else {
                            // Schedule next timeout, go around this loop again:
                            I8080TimerRegisterIntervalSetTimespec(context->timers[timer_id], &state->timeout);
                            DEBUG("Rescheduled auto-reenabled timer %u", timer_id);
                        }
                    } else {
                        // Some sort of configurational change...
                        inner_loop = false;
                    }
                }
                break;
            }
        }
    }
    
    // We're holding the lock at this point, gotta release it before we exit:      
    pthread_mutex_unlock(&state->lock);
    
    return NULL;
}

//

I8080TimerContextPtr
I8080TimerContextCreate(
    I8080SystemPtr  sys8080
)
{
    I8080TimerContext_t     *context = (I8080TimerContext_t*)calloc(1, sizeof(I8080TimerContext_t));
    
    if ( context ) {
        int                 i;
        
        context->sys8080 = sys8080;
        for ( i = 0; i < 8; i++ ) {
            context->state[i].timer_id = i;
            context->state[i].context = context;
            context->timers[i].enable = kI8080TimerEnableUnused;
        }
    }
    return context;
}

//

void
I8080TimerContextDestroy(
    I8080TimerContextPtr    context
)
{
    int                     tid = 0;
    
    while ( tid < 8 ) {
        if ( context->timers[tid].enable != kI8080TimerEnableUnused ) {
            pthread_mutex_lock(&context->state[tid].lock);
            context->timers[tid].enable = kI8080TimerEnableUnused;
            pthread_cond_signal(&context->state[tid].cond);
            pthread_mutex_unlock(&context->state[tid].lock);
            pthread_join(context->state[tid].thread, NULL);
            pthread_detach(context->state[tid].thread);
            pthread_mutex_destroy(&context->state[tid].lock);
            pthread_cond_destroy(&context->state[tid].cond);
        }
        tid++;
    }
    free((void*)context);
}

//

void
I8080TimerContextSetEnable(
    I8080TimerContextPtr    context,
    unsigned                timer_id,
    I8080TimerEnable_t      enable
)
{
    // Skip the no-ops:
    if ( context->timers[timer_id].enable == enable ) return;
    
    if ( context->timers[timer_id].enable == kI8080TimerEnableUnused ) {
        // Current state of the timer is unused, get the thread launched:
        context->timers[timer_id].enable = enable;
        pthread_mutex_init(&context->state[timer_id].lock, NULL);
        pthread_cond_init(&context->state[timer_id].cond, NULL);
        pthread_create(&context->state[timer_id].thread, NULL, I8080TimerContextTimerThread, &context->state[timer_id]);
    } else {
        // For all other changes we have to grab the lock first:
        pthread_mutex_lock(&context->state[timer_id].lock);
        
        if ( ((context->timers[timer_id].enable == kI8080TimerEnableOn) && (enable == kI8080TimerEnableAutoReenable)) ||
             ((context->timers[timer_id].enable == kI8080TimerEnableAutoReenable) && (enable == kI8080TimerEnableOn)) )
        {
            // If we're just swapping the auto-reset state of an existing
            // on state, we don't need to interrupt the thread's wait state:
            context->timers[timer_id].enable = enable;
            pthread_mutex_unlock(&context->state[timer_id].lock);
        } else {
            // Set the new state:
            context->timers[timer_id].enable = enable;
            
            // Signal the thread:
            pthread_cond_signal(&context->state[timer_id].cond);
            
            // If the new state is unused, destroy the thread et al.
            if ( enable == kI8080TimerEnableUnused ) {
                pthread_mutex_unlock(&context->state[timer_id].lock);
                pthread_join(context->state[timer_id].thread, NULL);
                pthread_detach(context->state[timer_id].thread);
                pthread_mutex_destroy(&context->state[timer_id].lock);
                pthread_cond_destroy(&context->state[timer_id].cond);
            } else {
                pthread_mutex_unlock(&context->state[timer_id].lock);
            }
        }
    }
}

//

void
I8080TimerContextSetOpcode(
    I8080TimerContextPtr    context,
    unsigned                timer_id,
    uint8_t                 opcode
)
{
    switch ( context->timers[timer_id].enable ) {
    
        case kI8080TimerEnableUnused:
            context->timers[timer_id].opcode = opcode;
            break;
        
        case kI8080TimerEnableOff:
        case kI8080TimerEnableOn:
        case kI8080TimerEnableAutoReenable:{
            // This is permissible, but we need to grab the lock to change the interval:
            pthread_mutex_lock(&context->state[timer_id].lock);
            context->timers[timer_id].opcode = opcode;
            pthread_cond_signal(&context->state[timer_id].cond);
            pthread_mutex_unlock(&context->state[timer_id].lock);
            break;
        }
    }
}

//

bool
I8080TimerContextSetInterval(
    I8080TimerContextPtr    context,
    unsigned                timer_id,
    I8080Microseconds       interval
)
{
    bool                    ok = true;
    
    switch ( context->timers[timer_id].enable ) {
    
        case kI8080TimerEnableUnused:
            I8080TimerRegisterSetIntervalWithMicroseconds(&context->timers[timer_id], interval);
            break;
        
        case kI8080TimerEnableOff: {
            // This is permissible, but we need to grab the lock to change the interval:
            pthread_mutex_lock(&context->state[timer_id].lock);
            I8080TimerRegisterSetIntervalWithMicroseconds(&context->timers[timer_id], interval);
            pthread_cond_signal(&context->state[timer_id].cond);
            pthread_mutex_unlock(&context->state[timer_id].lock);
            break;
        }
        
        case kI8080TimerEnableOn:
        case kI8080TimerEnableAutoReenable:
            ok = false;
            break;
            
    }
    return ok;
}

//

void
I8080TimerContextPrint(
    FILE                    *stream,
    I8080TimerContextPtr    systime
)
{
    int                     i;
    
    I8080TimerContextDateTimeFill(I8080TimerContextDateTimeNow(systime));
    fprintf(stream, "I8080Timer:\n"
                    "  [$%1$02lX] |- 0x%2$02hhX (%2$u)\n"
                    "  [$%3$02lX] |- 0x%4$02hhX (%4$u)\n"
                    "  [$%5$02lX] |- 0x%6$02hhX (%6$u)\n"
                    "  [$%7$02lX] |- 0x%8$02hhX (%8$u)\n"
                    "  [$%9$02lX] |- 0x%10$02hhX (%10$u)\n"
                    "  [$%11$02lX] |- 0x%12$02hhX (%12$u)\n"
                    "  [$%13$02lX] |- 0x%14$02hhX (%14$u)\n"
                    "  [$%15$02lX] |- 0x%16$02hhX (%16$u)\n"
                    "        |- TIMERS:\n",
            offsetof(I8080TimerContext_t, datetime.S), systime->datetime.R[0], 
            offsetof(I8080TimerContext_t, datetime.M), systime->datetime.R[1], 
            offsetof(I8080TimerContext_t, datetime.H), systime->datetime.R[2], 
            offsetof(I8080TimerContext_t, datetime.d), systime->datetime.R[3],
            offsetof(I8080TimerContext_t, datetime.m), systime->datetime.R[4], 
            offsetof(I8080TimerContext_t, datetime.Y), systime->datetime.R[5], 
            offsetof(I8080TimerContext_t, datetime.Ycent), systime->datetime.R[6], 
            offsetof(I8080TimerContext_t, datetime.fired), systime->datetime.R[7]);
    for ( i = 0; i < 8; i++ ) {
        fprintf(stream, "  [$%02lX]     |- %X: ", offsetof(I8080TimerContext_t, timers[i]), i);
        if ( systime->timers[i].enable != kI8080TimerEnableUnused ) {
            I8080Microseconds   msec = I8080TimerRegisterIntervalInMicroseconds(systime->timers[i]);
            
            switch ( systime->timers[i].enable ) {
                case kI8080TimerEnableUnused:
                    break;
                case kI8080TimerEnableOff:
                    fprintf(stream, "%.3f ms, off (opcode: 0x%02hhX)\n",  msec, systime->timers[i].opcode);
                    break;
                case kI8080TimerEnableOn:
                    fprintf(stream, "%.3f ms, running (opcode: 0x%02hhX)\n",  msec, systime->timers[i].opcode);
                    break;
                case kI8080TimerEnableAutoReenable:
                    fprintf(stream, "%.3f ms, auto-reenable (opcode: 0x%02hhX)\n", msec, systime->timers[i].opcode);
                    break;
            }
        } else {
            fprintf(stream, "unused\n");
        }
    }
    fputc('\n', stream);
}

//

void
I8080TimerContextWriteToTextBuffer(
    I8080TextBufferRef      tbuff,
    I8080TimerContextPtr    systime
)
{
    int                     i;
    
    I8080TimerContextDateTimeFill(I8080TimerContextDateTimeNow(systime));
    I8080TextBufferPrintf(tbuff,
                    "I8080Timer:\n"
                    "  [$%1$02lX] |- 0x%2$02hhX (%2$u)\n"
                    "  [$%3$02lX] |- 0x%4$02hhX (%4$u)\n"
                    "  [$%5$02lX] |- 0x%6$02hhX (%6$u)\n"
                    "  [$%7$02lX] |- 0x%8$02hhX (%8$u)\n"
                    "  [$%9$02lX] |- 0x%10$02hhX (%10$u)\n"
                    "  [$%11$02lX] |- 0x%12$02hhX (%12$u)\n"
                    "  [$%13$02lX] |- 0x%14$02hhX (%14$u)\n"
                    "  [$%15$02lX] |- 0x%16$02hhX (%16$u)\n"
                    "        |- TIMERS:\n",
            offsetof(I8080TimerContext_t, datetime.S), systime->datetime.R[0], 
            offsetof(I8080TimerContext_t, datetime.M), systime->datetime.R[1], 
            offsetof(I8080TimerContext_t, datetime.H), systime->datetime.R[2], 
            offsetof(I8080TimerContext_t, datetime.d), systime->datetime.R[3],
            offsetof(I8080TimerContext_t, datetime.m), systime->datetime.R[4], 
            offsetof(I8080TimerContext_t, datetime.Y), systime->datetime.R[5], 
            offsetof(I8080TimerContext_t, datetime.Ycent), systime->datetime.R[6], 
            offsetof(I8080TimerContext_t, datetime.fired), systime->datetime.R[7]);
    for ( i = 0; i < 8; i++ ) {
        I8080TextBufferPrintf(tbuff, "  [$%02lX]     |- %X: ", offsetof(I8080TimerContext_t, timers[i]), i);
        if ( systime->timers[i].enable != kI8080TimerEnableUnused ) {
            I8080Microseconds   msec = I8080TimerRegisterIntervalInMicroseconds(systime->timers[i]);
            
            switch ( systime->timers[i].enable ) {
                case kI8080TimerEnableUnused:
                    break;
                case kI8080TimerEnableOff:
                    I8080TextBufferPrintf(tbuff, "%.3f ms, off (opcode: 0x%02hhX)\n",  msec, systime->timers[i].opcode);
                    break;
                case kI8080TimerEnableOn:
                    I8080TextBufferPrintf(tbuff, "%.3f ms, running (opcode: 0x%02hhX)\n",  msec, systime->timers[i].opcode);
                    break;
                case kI8080TimerEnableAutoReenable:
                    I8080TextBufferPrintf(tbuff, "%.3f ms, auto-reenable (opcode: 0x%02hhX)\n", msec, systime->timers[i].opcode);
                    break;
            }
        } else {
            I8080TextBufferPrintf(tbuff, "unused\n");
        }
    }
    I8080TextBufferPrintf(tbuff, "\n");
}

//
#pragma mark -
//

static
bool
I8080TimerContextRead(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             *byte,
    const void          *context
)
{
    I8080TimerContextPtr    systime = (I8080TimerContextPtr)context;
    I8080Addr_t             timer_addr = addr - range.base;
    
    /* Date-time? */
    if ( timer_addr < sizeof(I8080DateTimeRegister_t) ) {
        I8080TimerContextDateTimeFill(I8080TimerContextDateTimeNow(systime));
        *byte = systime->datetime.R[timer_addr];
        return true;
    } else {
        unsigned            timer_id;
        
        /* Map the address offset to the timer: */
        timer_addr -= sizeof(I8080DateTimeRegister_t);
        timer_id = timer_addr / sizeof(I8080TimerRegister_t);
        timer_addr %= sizeof(I8080TimerRegister_t);
        if ( timer_id >= 0 && timer_id < 8 ) {
            *byte = systime->timers[timer_id].R[timer_addr];
            return true;
        }
    }
    return false;
}

//

static
bool
I8080TimerContextWrite(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    I8080Addr_t         addr,
    uint8_t             byte,
    const void          *context
)
{
    I8080TimerContextPtr    systime = (I8080TimerContextPtr)context;
    I8080Addr_t             timer_addr = addr - range.base;
    
    /* Date-time? */
    if ( timer_addr < sizeof(I8080DateTimeRegister_t) ) {
        /* No-op */
        return true;
    } else {
        unsigned            timer_id;
        
        /* Map the address offset to the timer: */
        timer_addr -= sizeof(I8080DateTimeRegister_t);
        timer_id = timer_addr / sizeof(I8080TimerRegister_t);
        timer_addr %= sizeof(I8080TimerRegister_t);
        if ( timer_id >= 0 && timer_id < 8 ) {
            switch ( timer_addr ) {
                case 0:
                    I8080TimerContextSetEnable(systime, timer_id, byte);
                    break;
                case 1:
                    I8080TimerContextSetOpcode(systime, timer_id, byte);
                    break;
                case 2:
                case 3:
                case 4:
                case 5:
                case 6: {
                    // This is only allowed if the state is unused or off:
                    switch ( systime->timers[timer_id].enable ) {
                        case kI8080TimerEnableUnused:
                            systime->timers[timer_id].R[timer_addr] = byte;
                            break;
                        case kI8080TimerEnableOff:
                            pthread_mutex_lock(&systime->state[timer_id].lock);
                            systime->timers[timer_id].R[timer_addr] = byte;
                            pthread_mutex_unlock(&systime->state[timer_id].lock);
                            break;
                        case kI8080TimerEnableOn:
                        case kI8080TimerEnableAutoReenable:
                            break;
                    }
                }
            }
            return true;
        }
    }
    return false;
}

//

static
bool
I8080TimerContextReset(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080TimerContextPtr    systime = (I8080TimerContextPtr)context;
    unsigned                i = 0;
    
    while ( i < 8 ) {
        I8080TimerContextSetEnable(systime, i, kI8080TimerEnableUnused);
        memset(&systime->timers[i], 0, sizeof(systime->timers[i]));
        i++;
    }
    return true;
}

//

static
void
I8080TimerContextShutdown(
    I8080MemRef         mem,
    I8080AddrRange_t    range,
    const void          *context
)
{
    I8080TimerContextPtr    systime = (I8080TimerContextPtr)context;
    I8080TimerContextDestroy(systime);
}

//

const I8080MemMapperCallbacks __I8080TimerMapperCallbacks = {
            .mapper_name = "realtime-timers",
            .rewrite_addr = NULL,
            .read = I8080TimerContextRead,
            .write = I8080TimerContextWrite,
            .reset = I8080TimerContextReset,
            .shutdown = I8080TimerContextShutdown
        };
const I8080MemMapperCallbacks *I8080TimerMapperCallbacks = &__I8080TimerMapperCallbacks;

