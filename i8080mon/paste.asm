TTY_OUT     EQU     0                           ; The TTY i/o is attached to channel 0

            CPU     8080
            ORG     2000h
            
            LXI     H, CSTR                     ; HL <= address of string to output
LOOP:       MOV     A, M                        ; A <= next character from string
            ORA     A                           ; A <= A | A, set flags
            JZ      DONE                        ; A is zero, the NUL terminator
            OUT     TTY_OUT                     ; Write the character to the TTY
            INX     H                           ; HL++
            JMP     LOOP                        ; Go process the next character
DONE:       MVI     A, '\n'                     ; Write a newline…
            OUT     TTY_OUT                     ; …to the TTY…
            RST     0                           ; …then call reset vector 0 (reenter monitor)
CSTR:       DB      'Hello, world!', 00h        ; NUL-terminated string to print to TTY
