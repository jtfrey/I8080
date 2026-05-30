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

int
main()
{
    I8080TimerContextPtr    systime = I8080TimerContextCreate(NULL);
    int                     sleepy_time = 13;
    
    I8080LoggingSetMaxLevel(kI8080LoggingLevelDebug);
    if ( systime ) {
        I8080TimerContextSetInterval(systime, 0, 4.5 * 1e6);
        I8080TimerContextPrint(stdout, systime);
        sleep(3);
        I8080TimerContextSetEnable(systime, 0, kI8080TimerEnableAutoReenable);
        I8080TimerContextPrint(stdout, systime);
        printf("sleeping for %d s...\n", sleepy_time); fflush(stdout);
        sleep(sleepy_time);
        I8080TimerContextDestroy(systime);
    }
    return 0;
}
