
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "I8080System.h"
#include "I8080Logging.h"


uint8_t     prgrm[] = {
                                    /* ORG     $1000                                        */
                                    
/* 1000       */0x3E, 0x2C,         /* MVI     A, #$2A                                      */
/* 1002       */0x27,               /* DAA                                                  */
/* 1003       */0xCE, 0x78,         /* ACI     #$18                                         */
/* 1005       */0x27,               /* DAA                                                  */
/* 1006       */0xFE, 0x10,         /* CPI     #$10                                         */
/* 1008       */0xCA, 0x0C, 0x10,   /* JZ      +1                                           */
/* 100B       */0x76,               /* HLT                                                  */

/* 100C       */0x26, 0x01,         /* MVI     H, #$01                                      */
/* 100E       */0x2E, 0xFF,         /* MVI     L, #$FF                                      */
/* 1010       */0xF9,               /* SPHL                                                 */
/* 1011       */0xC5,               /* PUSH    BC                                           */
/* 1012       */0xD5,               /* PUSH    DE                                           */
/* 1013       */0xE5,               /* PUSH    HL                                           */
/* 1014       */0xF5,               /* PUSH    PSW                                          */

/* LOOP:      */
/* 1015       */0xDB, 0x00,         /* IN      #0          ; Read from stdin                */
/* 1017       */0xFE, 0xFF,         /* CPI     #$FF        ; EOF?                           */
/* 1019       */0xCA, 0x33, 0x10,   /* JZ      DONE        ; Yep, exit the program          */
/* 101C       */0xFE, 0x04,         /* CPI     #$04        ; Ctrl-D?                        */
/* 101E       */0xCA, 0x33, 0x10,   /* JZ      DONE        ; Yep, exit the program          */
/* 1021       */0xFE, 0x61,         /* CPI     #$61        ; Character == 0x61 ('a')        */
/* 1023       */0xFA, 0x2E, 0x10,   /* JM      OUTPUT      ; Character < 0x61               */
/* 1026       */0xFE, 0x7B,         /* CPI     #$7B        ; Character == 0x7B ('z' + 1)    */
/* 1028       */0xF2, 0x2E, 0x10,   /* JP      OUTPUT      ; Character >= 0x7B              */
/* 102B       */0xCD, 0x38, 0x10,   /* CALL    UC          ; Call uppercase subroutine      */

/* OUTPUT:    */
/* 102E       */0xD3, 0x0A,         /* OUT     #10         ; Write the char to stderr       */
/* 1030       */0xC3, 0x15, 0x10,   /* JMP     LOOP        ; Go back for another            */
/* DONE:      */
/* 1033:      */0xF1,               /* POP      PSW                                         */
/* 1034:      */0xE1,               /* POP      HL                                          */
/* 1035:      */0xD1,               /* POP      DE                                          */
/* 1036:      */0xC1,               /* POP      BC                                          */
/* 1037       */0x76,               /* HLT                 ; Terminate the program          */

/* UC:        */
/* 1038       */0xEE, 0x20,         /* XRI     #$20        ; Convert to uppercase...        */
/* 103A       */0xC9                /* RET                                                  */

    };
    
volatile I8080SystemPtr sys8080 = NULL;

void handle_sigint(int sig) {
    I8080SystemBreak(sys8080);
}

int
main()
{
    I8080Device_t                   devStderr, devFile;
    I8080DeviceTTYContext_t         tty_context = {
                                        .stdin_context.options = kI8080DeviceStdInputOptsAll,
                                        .stdout_context.options = kI8080DeviceStdOutputOptsAll };
    I8080DeviceFileContext_t        file_context = {
                                        .variant = kI8080DeviceFileVariantPath,
                                        .path = {
                                            .filepath = "./output.txt",
                                            .mode = "a+" }
                                        };
    int                             instr_num = 0;
    
    signal(SIGINT, handle_sigint);
    I8080LoggingSetMaxLevel(kI8080LoggingLevelDebug);
    //I8080LoggingSetFormat(kI8080LoggingFormatDetailed);
    I8080LoggingSetFile("test.log", false);
    
    sys8080 = I8080SystemCreateWithTTYContext(kI8080SystemOpts2MHzClock, 0, &tty_context);
    
    // Register stderr as output device #10:
    devStderr = *I8080DeviceStdError;
    I8080DevBusRegisterDevice(sys8080->devbus, 10, &devStderr, NULL);
    
    // Register the file as output device #5:
    devFile = *I8080DeviceFileOut;
    I8080DevBusRegisterDevice(sys8080->devbus, 5, &devFile, &file_context);
    
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
    } while ( I8080SystemIsRunning(sys8080->state) );
    
    printf("\n");
    I8080RegistersPrint(stdout, &sys8080->rgstrs);
    I8080DevBusPrint(stdout, sys8080->devbus);
    
    I8080SystemSetPowerState(sys8080, false);
    I8080SystemDestroy(sys8080);
    
    return 0;
}
