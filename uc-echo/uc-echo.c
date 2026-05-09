
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "I8080System.h"
#include "I8080Logging.h"
                
/*
                    ORG     $1000
LOOP:
1000    DB 00       IN      #0          ; Read from stdin
1002    FE FF       CPI     #$FF        ; EOF?
1004    CA 1D 10    JZ      DONE        ; Yep, exit the program
1007    FE 04       CPI     #$04        ; Ctrl-D?
1009    CA 1D 10    JZ      DONE        ; Yep, exit the program
100C    FE 61       CPI     #$61        ; Character == 0x61 ('a')
100E    FA 18 10    JM      OUTPUT      ; Character < 0x61
1011    FE 7B       CPI     #$7B        ; Character == 0x7B ('{', right after 'z')
1013    F2 18 10    JP      OUTPUT
1016    E6 DF       ANI     #$DF        ; Convert to uppercase...

OUTPUT:
1018    D3 01       OUT     #1          ; Write the char to stderr
101A    C3 00 10    JMP     LOOP        ; Go back for another

DONE:
101D    76          HLT                 ; Terminate the program
 */

uint8_t     prgrm[] = {
                0xAF,               /*   XRA    A    */
/* LOOP: */     0xCA, 0x01, 0x10,   /*   JZ     LOOP */

                0xDB, 0x00,
                0xFE, 0xFF,
                0xCA, 0x1D, 0x10,
                0xFE, 0x04,
                0xCA, 0x1D, 0x10,
                0xFE, 0x61,
                0xFA, 0x18, 0x10,
                0xFE, 0x7B,
                0xF2, 0x18, 0x10,
                0xE6, 0xDF,
                0xD3, 0x01,
                0xC3, 0x00, 0x10,
                0x76 };

volatile sig_atomic_t keepRunning = 1;

void handle_sigint(int sig) {
    keepRunning = 0;
}

int
main()
{
    I8080SystemPtr                  sys8080 = NULL;
    I8080Device_t                   devStderr;
    I8080DeviceTTYContext_t         tty_context = {
                                        .stdin_context.options = kI8080DeviceStdInputOptsAll,
                                        .stdout_context.options = kI8080DeviceStdOutputOptsAll };
    int                             instr_num = 0;
    
    signal(SIGINT, handle_sigint);
    I8080LoggingSetMaxLevel(kI8080LoggingLevelDebug);
    //I8080LoggingSetFormat(kI8080LoggingFormatDetailed);
    I8080LoggingSetFile("test.log", true);
    
    sys8080 = I8080SystemCreateWithTTYContext(kI8080SystemOpts2MHzClock, 0, &tty_context);
    
    // Register stderr as output device #1:
    devStderr = *I8080DeviceStdError;
    I8080DevBusRegisterDevice(sys8080->devbus, &devStderr, NULL);
    
    // Set power on:
    I8080SystemSetPowerState(sys8080, true);
    
    // Load the program into $1000:
    DEBUG("Loading demo program at $1000");
    I8080MemCopyIn(sys8080->sysmem, prgrm, sizeof(prgrm), 0x1000);
    
    // Set the program counter:
    I8080SystemSetPC(sys8080, 0x1000);
    
    // Run the program:
    do {
        bool    did_fail = I8080SystemStep(sys8080, NULL);
        
        if ( did_fail ) break;
        instr_num++;
    } while ( keepRunning && (sys8080->state & kI8080SystemStateRunning) );
    
    printf("\n");
    I8080RegistersPrint(stdout, &sys8080->rgstrs);
    I8080DevBusPrint(stdout, sys8080->devbus);
    
    I8080SystemSetPowerState(sys8080, false);
    I8080SystemDestroy(sys8080);
    
    return 0;
}
