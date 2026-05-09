
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
/*1000*/        0x3E, 0x2C,         /*      MVI  #$2A   */
/*1002*/        0x27,               /*      DAA         */
/*1003*/        0xCE, 0x78,         /*      ACI  #$18   */
/*1005*/        0x27,               /*      DAA         */
/*1006*/        0xFE, 0x10,         /*      CPI  #$10   */
/*1008*/        0xCA, 0x0C, 0x10,   /*      JZ   +1     */
/*100B*/        0x76,               /*      HLT         */

/*100C*/        0xDB, 0x00,   
/*100E*/        0xFE, 0xFF,
/*1010*/        0xCA, 0x29, 0x10,
/*1013*/        0xFE, 0x04,
/*1015*/        0xCA, 0x29, 0x10,
/*1018*/        0xFE, 0x61,
/*101A*/        0xFA, 0x24, 0x10,
/*101D*/        0xFE, 0x7B,
/*101F*/        0xF2, 0x24, 0x10,
/*1022*/        0xE6, 0xDF,
/*1024*/        0xD3, 0x01,
/*1026*/        0xC3, 0x0C, 0x10,
/*1029*/        0x76 };

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
    I8080LoggingSetFile("test.log", false);
    
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
