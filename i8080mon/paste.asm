TTY_OUT     EQU     0

            CPU     8080
            ORG     2000h
            
            LXI     H, CSTR
LOOP:       MOV     A, M
            ORA     A
            JZ      DONE
            OUT     TTY_OUT
            INX     H
            JMP     LOOP
DONE:       MVI     A, '\n'
            OUT     TTY_OUT
            RST     0
CSTR:       DB      'Hello, world!', 00h
