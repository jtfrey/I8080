/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 realtime timer.
 */
 
#ifndef __I8080TIMER_H__
#define __I8080TIMER_H__

#include "I8080Memory.h"
#include "I8080Timing.h"
#include "I8080System.h"
#include "I8080TextBuffer.h"

/**
 * Timer enablement state
 * Timers start in the unused state and progress through
 * the off/on/auto states.
 */
typedef enum __attribute__((packed)) {
    kI8080TimerEnableUnused         = 0x00, /*!< the timer is unused and has no resources
                                                 allocated to it */
    kI8080TimerEnableOff            = 0x01, /*!< the timer has had resources allocated to it
                                                 but is not running */
    kI8080TimerEnableOn             = 0x02, /*!< the timer is running */
    kI8080TimerEnableAutoReenable   = 0x82  /*!< the timer is running and will automatically
                                                 reenable itself when it expires */
} I8080TimerEnable_t;

/**
 * Type of a pointer to a realtime timer context
 * The realtime timer mapper uses a single page of memory.
 * It organizes that page into a series of registers that
 * communicate the date/time to user programs and also
 * provide a number of programmable timers that can be set
 * to generate interrupts on an interval, either one time
 * only or automatically repeating.
 *
 * A series of read-only registers lead-off the mapped
 * page:
 *
 *  $00       (read-only) current time, seconds
 *  $01       (read-only) current time, minutes
 *  $02       (read-only) current time, hours
 *  $03       (read-only) current date, day of month
 *  $04       (read-only) current date, month
 *  $05       (read-only) current date, two-digit year
 *  $06       (read-only) current date, century
 *  $07       (read-only) timer that raised interrupt
 *
 * Normally the timer index register at $07 is held at a
 * value of $FF, but while an interrupt handler is 
 * executing it contains the index of the timer that
 * produced the interrupt.
 *
 * The eight (8) timers with indices in [0, 7] are
 * presented starting at an offset of $10 from the base
 * page address.  Each timer occupies 8 bytes, making
 * their base offset $10 + (i * $08) — or $10, $18, $20,
 * etc.  The following summarizes i=0, but applies to
 * each of the eight timers:
 *
 *  $10         enable:  0=off, 1=running, 2=auto-reenable
 *  $11         opcode:  the 8080 opcode to execute when the
 *              timer expires
 *  $12         msecFrac: the fractional milliseconds for the
 *              timer interval, expressed as an 8-bit fixed-point
 *              fraction trunc([0, 1) * 256), for a resolution
 *              of ~4 µs
 *  $13         msecWholeB0:  the LSB of the whole number of
 *              milliseconds
 *  $14         msecWholeB1:  byte 1 of the whole number of
 *              milliseconds
 *  $15         msecWholeB2:  byte 2 of the whole number of
 *              milliseconds
 *  $16         msecWholeB3:  the MSB of the whole number of
 *              milliseconds
 *
 * The interval has a maximum value (1/256) ms under 2^32 ms:
 * 4294967295.996 ms, for a maximum interval just shy of 50 days.
 *
 * Between the read-only and timer registers 80 bytes are
 * accounted for.  The remainder of the page is technically
 * usable by user programs, but it is not advised.
 */
typedef struct I8080TimerContext * I8080TimerContextPtr;

/**
 * Create a new realtime timer context
 * Creates and returns a memory mapper context for a realtime
 * timer.  The \p sys8080 is needed for raising interrupts when
 * active timers elapse; if it is NULL, no interrupts will be
 * generated.
 *
 * All timers are unused and their intervals are set to
 * 0.0 ms.
 * @param sys8080           the emulator to which the timer
 *                          raises interrupts when timers
 *                          elapse
 * @return                  a newly-initialized context, NULL
 *                          on error
 */
I8080TimerContextPtr I8080TimerContextCreate(I8080SystemPtr sys8080);

/**
 * Destroy a realtime timer context
 * Invalidates and shuts down all active timers associated with
 * \p systime, then deallocates the object.
 * @param systime           the realtime timer context to
 *                          destroy
 */
void I8080TimerContextDestroy(I8080TimerContextPtr systime);

/**
 * Change the enablement state of a timer
 * Attempts to change the state of timer \p timer_id of
 * \p context.  There are eight timers with ids in the range
 * [0, 7].
 *
 * All timers start in the "unused" state.  The caller should
 * first set the desired timer interval, then either set
 * the enablement to "off" to startup the timer infrastructure
 * without running it, or the "on" or "auto-reenable" state
 * to initialize and start the timer runing.
 * @param systime           the realtime timer context to
 *                          alter
 * @param timer_id          the timer to alter
 * @param enable            the new enablement state
 */
void I8080TimerContextSetEnable(I8080TimerContextPtr context, unsigned timer_id, I8080TimerEnable_t enable);

/**
 * Change the opcode the timer will deliver on interrupt
 * When timer \p timer_id of \p context elapses, an interrupt will
 * be raised and the given \p opcode will be delivered to the CPU.
 * @param systime           the realtime timer context to
 *                          alter
 * @param timer_id          the timer to alter
 * @param opcode            the 8080 opcode to deliver with the
 *                          interrupt
 */
void I8080TimerContextSetOpcode(I8080TimerContextPtr context, unsigned timer_id, uint8_t opcode);

/**
 * Change the countdown interval for the timer
 * Set the number of microseconds timer \p timer_id of \p context
 * will count down before raising an interrupt.
 *
 * If the timer in question is already running, it must first be
 * transitioned to the "off" or "unused" state before altering
 * the interval.  In such cases, this function returns boolean
 * false as an indication that the interval is unchanged.
 * @param systime           the realtime timer context to
 *                          alter
 * @param timer_id          the timer to alter
 * @param interval          the number of microseconds to count
 *                          down
 * @return                  boolean true if the interval was
 *                          mutable
 */
bool I8080TimerContextSetInterval(I8080TimerContextPtr context, unsigned timer_id, I8080Microseconds interval);

/**
 * Write a summary of the realtime timer to a FILE stream
 * Writes a summary of the realtime timer in \p systime to the FILE stream
 * \p stream.
 * @param stream        the stream to which to write
 * @param systime       the realtime timer context to summarize
 */
void I8080TimerContextPrint(FILE *stream, I8080TimerContextPtr systime);

/**
 * Write a summary of the realtime timer to a text buffer
 * Writes a summary of the realtime timer in \p systime to the text buffer
 * \p tbuff.
 * @param tbuff         the text buffer to which to write
 * @param systime       the realtime timer context to summarize
 */
void I8080TimerContextWriteToTextBuffer(I8080TextBufferRef tbuff, I8080TimerContextPtr systime);

/**
 * Timer callbacks
 * Address of the callbacks that implement the realtime timers.
 */
extern const I8080MemMapperCallbacks *I8080TimerMapperCallbacks;

#endif /* __I8080TIMER_H__ */
