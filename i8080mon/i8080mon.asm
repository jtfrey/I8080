;
; i8080mon.s
; Intel 8080 System Monitor
;
; Modeled after wozmon for the Apple I.  Commands that are
; recognized are:
;
;     [address]                     print byte at address
;     [address1].[address2]         print bytes starting at address1 up to
;                                   (but not including) address2, 8 bytes per
;                                   line
;     [address]: [byte] {[byte] …}  set bytes starting at address using the
;                                   [byte] values
;     [address]R                    set the program counter to address
;     R                             set the program coutner to the last
;                                   successfully-parsed starting address from
;                                   a print or set bytes command
;
; E.g. with the i8e8e emulator running the monitor:
;
;     > 2000:21 13 20 7E B7 CA 0E 20
;     > 2008:D3 00 23 C3 03 20 3E 0A
;     > 2010:D3 00 C7 48 65 6C 6C 6F
;     > 2018:2C 20 77 6F 72 6C 64 21
;     > 2020:00
;     > 2000.2021
;     
;     2000:21 13 20 7E B7 CA 0E 20 
;     2008:D3 00 23 C3 03 20 3E 0A 
;     2010:D3 00 C7 48 65 6C 6C 6F 
;     2018:2C 20 77 6F 72 6C 64 21 
;     2020:00 
;     > R
;     Hello, world!
;     > X
;
; The machine code pasted-into the monitor can be found in the paste.asm
; source file.
;

;
; I/O channels for the TTY
;
TTY_IN          EQU     0
TTY_OUT         EQU     0

                CPU     8080
                ORG     0000h

;
; Reset vectors
;
; $0000 - $0040 are the 8-byte RST instruction jump
; vectors.  E.g. when the CPU executes a 'RST 0' it
; sets the program counter to $0000 and executes the
; instruction(s) found there; for 'RST 1' the target
; address is $0008, 'RST 2' is $0010, etc.
;
; Interrupts force a single byte into the CPU's
; instruction decode, and the RST instructions are
; the best candidate.  So one device raising an interrupt
; can issue a 'RST 2' versus a different device that
; issues a 'RST 4' when it raises an interrupt.  This
; makes it possible to have up to 8 devices to which the
; software can uniquely respond (in a simple,
; straightforward fashion).
;
RESET_VECTORS:
;
; Since the PC on an 8080 gets initialized to $0000 by
; the hardware, we will make reset vector 0 the boot
; vector for the system.  A software reset can then easily
; be accomplished with an 'RST 0' instruction, too.
;
RST0_VECTOR:
BOOT_VECTOR:        DI                              ; [1B][ 4] disable interrupts until we've
                                                    ;          finished startup
                    LXI     H, 0000h                ; [3B][10] HL <= $0000
                    SPHL                            ; [1B][ 5] SP <= HL = $0000
                    JMP     MONITOR                 ; [3B][10] Jump into the system monitor now
;
; The other 7 reset vectors are unused but this image will
; reserve space for each of them:
;
RST1_VECTOR:        DS      8
RST2_VECTOR:        DS      8
RST3_VECTOR:        DS      8
RST4_VECTOR:        DS      8
RST5_VECTOR:        DS      8
RST6_VECTOR:        DS      8
RST7_VECTOR:        DS      8
;
; There are another $C0 (192) bytes in the zero page.  We will use those
; for our input buffer and a few state variables.
;
MON_MODE            DB      00h                 ; Mode:  bit 7 : 0=single byte, 1=address range
MON_ADDR_0:         DW      0000h
MON_ADDR_1:         DW      0000h
MON_SAVED_CHAR:     DB      00h
INPUT_BUFFER:       DS      (0100h - $)
INPUT_BUFFER_END:
INPUT_BUFFER_LEN    EQU     (INPUT_BUFFER_END - INPUT_BUFFER)

;
; MONITOR
; The actual system monitor program.  There are no interrupts to which
; we will respond, so we won't bother enabling interrupts on the CPU
; (the boot vector disabled them).  Just go right into the main loop.
;
MONITOR:            CALL    READ_LINE               ; Read the next line from the user
                    ;
                    ; HL will be pointing to the base of the input buffer
                    ; and C will contain the character count
                    ;
                    MVI     A, 00h                  ; Default the mode to single-character print
                    STA     MON_MODE
$$FIRST_CHAR:       MOV     A, M                    ; Get leading character(s) from the buffer
                    CPI     ' '                     ; Whitespace?
                    JNZ     $$BARE_COMMANDS
                    INX     H
                    DCR     C
                    JZ      $$FIRST_CHAR            ; Discard all leading whitespace
$$BARE_COMMANDS:    CPI     'R'                     ; Is it a bare 'R' for [R]un?
                    JZ      $$RUN_CMD
                    CPI     'X'                     ; Is it a bare 'X' for e[X]it?
                    JNZ     $$INIT_PARSE
                    HLT                             ; Halt the system (exit the emulator)
                    
$$INIT_PARSE:       LXI     D, 0000h                ; We'll roll the parsed address into DE
$$PARSE_ADDR:       CALL    PARSE_HEXDIGIT          ; Try to parse a hex digit
                    JC      $$GOT_ADDR              ; Done parsing an address, skip ahead
                    XCHG                            ; HL <=> DE
                    DAD     H                       ; HL <= HL + HL = 2HL, effectively a HL <<= 1
                    DAD     H                       ; HL <= HL + HL = 2HL, effectively a HL <<= 1
                    DAD     H                       ; HL <= HL + HL = 2HL, effectively a HL <<= 1
                    DAD     H                       ; HL <= HL + HL = 2HL, effectively a HL <<= 1
                    ORA     L                       ; A <= A | L, parsed hex digit into low nibble
                    MOV     L, A                    ; HL <= (HL << 4) | A
                    XCHG                            ; DE <=> HL
                    MOV     A, C
                    ORA     A                       ; Check for C is zero
                    JZ      $$GOT_ADDR              ; Just an address, that's a single byte print
                    MOV     A, M                    ; A <= next character
                    JMP     $$PARSE_ADDR            ; Go back and do it again
                    
$$GOT_ADDR:         XCHG                            ; HL <=> DE
                    SHLD    MON_ADDR_0              ; MON_ADDR_0 <= HL = parsed 16-bit address
                    XCHG                            ; DE <=> HL
                    
                    ;
                    ; We get here under two conditions; either
                    ; CY is set, meaning a non-hex character was
                    ; encountered, or Z is set because C has
                    ; gone to zero.  If C has gone to zero, then
                    ; go do the print command.
                    ;
                    JZ      $$PRINT_CMD

                    ; If we get here then the next character is in A
                    ; already
$$CONTINUE_PARSE:   CPI     'R'                     ; An 'R' appended to an address
                    JZ      $$RUN_CMD
                    CPI     '.'                     ; A '.' means a ranged byte print…
                    JNZ     $$CHECK_SET
                    INX     H                       ; In preparation for parsing another address…
                    DCR     C                       ; …alter the input buffer pointer and limit…
                    JZ      $$PRINT_CMD             ; …and go do a standard print if no more chars.
                    MVI     A, 10000000b            ; Set mode to print range
                    STA     MON_MODE
                    XCHG                            ; HL <=> DE
                    LHLD    MON_ADDR_0              ; Stash the address we already parsed…
                    SHLD    MON_ADDR_1              ; …in the secondary address slot…
                    XCHG                            ; DE <=> HL
                    MOV     A, M
                    JNZ     $$PARSE_ADDR
                    JMP     MONITOR                 ; Botched command, ignore it
                    
$$CHECK_SET:        CPI     ':'                     ; Colon, for setting bytes?
                    JNZ     MONITOR                 ; If it's not a colon then this is a
                                                    ; botched command, ignore it
;
; Set bytes in memory
;
$$SET_CMD:          INX     H                       ; Increment input buffer pointer…
                    DCR     C                       ; …decrement the character limit counter…
                    JZ      MONITOR                 ; …back to a prompt if nothing's left…
                    XCHG                            ; …or shift the input buffer pointer to DE…
                    LHLD    MON_ADDR_0              ; …load the target store address into HL…
                    XCHG                            ; …then swap HL and DE again.
$$READ_BYTE:        MOV     A, M                    ; A <= next character
                    CALL    PARSE_HEXDIGIT
                    JC      $$NEXT_BYTE             ; If CY is set, it wasn't a hex character
                    JZ      $$WRITE_BYTE            ; If Z is set by PARSE_HEXDIGIT, we are at EOL
                    MOV     B, A                    ; B <= A (first nibble read)
                    MOV     A, M                    ; A <= next character
                    CALL    PARSE_HEXDIGIT
                    JNC     $$GOT_2ND_NBL           ; We got a second nibble, factor that in
                    MOV     A, B                    ; Restore read low nibble
                    STAX    D                       ; Just a low nibble, write that then move…
                    INX     D                       ; …to the next byte to read.
                    JMP     $$NEXT_BYTE
$$GOT_2ND_NBL:      PUSH    PSW                     ; Store the new nibble
                    MOV     A, B                    ; A <= B (first nibble read)
                    RAL                             ; We know CY is clear thanks to that JC above…
                    RAL                             ; …so no 1 bits will be shifted into A…
                    RAL                             ; …or out of A into CY…
                    RAL                             ; …when we move the first nibble high.
                    MOV     B, A                    ; B <= shifted first nibble
                    POP     PSW                     ; Restore second nibble
                    ORA     B                       ; A <= A | B = second nibble | shifted first
$$WRITE_BYTE:       STAX    D                       ; Write low-nibble byte to target
                    INX     D
                    MOV     A, C                    ; Check if there are any characters…
                    ORA     C                       ; …remaining to be parsed…
                    JNZ     $$READ_BYTE
                    JMP     MONITOR                 ; …and back to the prompt if not.
$$NEXT_BYTE:        INX     H                       ; Increment the input buffer pointer…
                    DCR     C                       ; …decrement the character count…
                    JNZ     $$READ_BYTE             ; …and loop if there are more characters.
                    JMP     MONITOR                 ; All done, back to prompt
;
; Print bytes in memory
;
$$PRINT_CMD:        LHLD    MON_ADDR_0              ; Load address 0 into HL
                    XCHG                            ; DE <= address 0
                    MVI     B, 00h                  ; Byte out counter
                    LDA     MON_MODE                ; Load the print mode…
                    ORA     A                       ; …and get the flags set…
                    JM      $$PRINT_RANGE           ; M flag set, it's a range
                    LXI     H, 1                    ; HL <= 1 byte
                    XCHG                            ; HL holds base address, DE holds byte count
                    JMP     $$PRINT_LOOP
$$PRINT_RANGE:      LHLD    MON_ADDR_1              ; HL <= address 1 (start of range)
                    MOV     A, H                    ; Component-wise negate…
                    CMA                             ;
                    MOV     H, A                    ; …the bytes in H…
                    MOV     A, L                    ; 
                    CMA                             ; …and L…
                    MOV     L, A                    ; 
                    INX     H                       ; …then add 1 to get the 2's complement
                                                    ;  for the sake of a 16-bit addition.
                    DAD     D                       ; HL <= HL + DE = ~(start address) + end address
                    XCHG                            ; DE now holds the byte count
                    LHLD    MON_ADDR_1              ; HL <= address 1 (start of range)
                    SHLD    MON_ADDR_0              ; Overwrite address 0 for the sake of any
$$PRINT_LOOP:       MOV     A, B
                    ORA     A
                    JNZ     $$SKIP_ADDRESS
                    MOV     A, H
                    CALL    PRINT_BYTE
                    MOV     A, L
                    CALL    PRINT_BYTE
                    MVI     A, ':'
                    OUT     TTY_OUT
$$SKIP_ADDRESS:     MOV     A, M                    ; Load a byte
                    CALL    PRINT_BYTE
                    INR     B                       ; Increment byte out counter
                    MOV     A, B                    ; A <= B = byte out counter
                    ANI     07h                     ; Effectively (B % 8)
                    MOV     B, A                    ; B <= (B % 8)
                    JNZ     $$SAME_LINE
                    MVI     A, '\n'
                    OUT     TTY_OUT
                    JMP     $$PRINT_LOOP_NEXT
$$SAME_LINE:        MVI     A, ' '
                    OUT     TTY_OUT
                    
$$PRINT_LOOP_NEXT:  INX     H
                    DCX     D
                    MOV     A, D
                    ORA     E
                    JNZ     $$PRINT_LOOP
                    MOV     A, B
                    ORA     A
                    JZ      MONITOR
                    MVI     A, '\n'
                    OUT     TTY_OUT
                    JMP     MONITOR
;
; Set program counter, run code
;
$$RUN_CMD:          LHLD    MON_ADDR_0              ; HL <= last-parsed address
                    PCHL                            ; PC <= HL = last-parsed address
                                                    ; this implicitly jumps to that
                                                    ; address

;
; PARSE_HEXDIGIT
;
; The HL register points to the input buffer and is updated
; accordingly.  Likewise, the character count, C, is updated.
;
; Parses a hexdecimal digit from the ASCII character in A on
; entry.
;
; On return, carry will be clear if the character was parsable
; and the value will be in the low nibble of A.  If carry is
; clear but C has gone to zero, the Z flag will be set.
;
PARSE_HEXDIGIT:     STA     MON_SAVED_CHAR          ; Save the original character
                    SUI     030h                    ; A <= A - $30
                                                    ; This will leave the 0-9 digits as 0-9
                    CPI     10                      ; If this evaluates to A < 10, it's a number
                                                    ; digit
                    JC      $$GOT_NIBBLE            ; Value of digit is in A
                    ADI     0E9h                    ; A <= A + $E9, this will put A at $FA, etc.
                    CPI     0FAh                    ; If this evaluates to A >= $FA, it's a hex
                                                    ; digit
                    JNC     $$HAVE_HEXDIGIT         ; It's a hex digit, period
                    LDA     MON_SAVED_CHAR
                    RET                             ; Return with original character
$$HAVE_HEXDIGIT:    ANI     00Fh                    ; Isolate the low nibble
$$GOT_NIBBLE:       ORA     A                       ; Ensure CY is clear
                    INX     H
                    DCR     C                       ; Decrement the
                    RET
;
; PRINT_BYTE
;
; Given a byte in A, print the high then the low nibble to
; the TTY.
;
PRINT_BYTE:         PUSH    PSW
                    ANI     0F0h
                    RAR
                    RAR
                    RAR
                    RAR
                    CALL    PRINT_HEXDIGIT
                    POP     PSW
                    ANI     00Fh
                    ; Falls through to PRINT_HEXDIGIT
;
; PRINT_HEXDIGIT
;
; Given a value in range [0,F] in A, transform that value into
; the appropriate ASCII character and output it to the TTY.
;
; The value in A is clobbered, but no other registers are
; affected.
;
PRINT_HEXDIGIT:     ADI     '0'                     ; Shift up to '0'
                    CPI     03Ah                    ; Is it $39 or less?
                    JC      $$OUT
                    ADI     007h                    ; Shift up to $41 for $3A et al.
$$OUT:              OUT     TTY_OUT
                    RET
;
; READ_LINE
;
; Read characters from the TTY input, discarding
; any invalid ASCII codes that are not part of any command.
;
READ_LINE:          MVI     A, '>'                  ; Display a simple prompt
                    OUT     TTY_OUT
                    MVI     A, ' '
                    OUT     TTY_OUT
                    LXI     H, INPUT_BUFFER
$$GETCH:            IN      TTY_IN                  ; A <= next input character
                    CPI     '\n'
                    JNZ     $$CHECK_BACKSPACE       ; A newline signals end of line
                    OUT     TTY_OUT
                    JMP     $$RETURN                ; Echo the newline then exit the subroutine
$$CHECK_BACKSPACE:  CPI     '_'
                    JNZ     $$IS_VALID              ; Not an underscore, go to validity check
                    CALL    IS_INPUT_EMPTY
                    JZ      $$GETCH                 ; Already at the start of the buffer
                    DCX     H                       ; Move back a character in the input buffer…
                    JMP     $$GETCH                 ; Go get another character

$$IS_VALID:         MOV     B, A                    ; B <= next input character
                    LXI     D, VALID_CHARS          ; DE <= base pointer to valid char array
                    
$$CHECK_LOOP:       LDAX    D                       ; A <= next valid char for comparison
                    ORA     A                       ; A <= A | A, set flags
                    JZ      $$GETCH                 ; NUL terminator, this is not a valid char
                    CMP     B                       ; A == B ?
                    JZ      $$ADD_CHAR              ; Character in A matches B, valid!
                    INX     D                       ; DE++
                    JMP     $$CHECK_LOOP            ; Check the next valid char
                    
$$ADD_CHAR:         MOV     M, A                    ; Store the character in the 
                    OUT     TTY_OUT                 ; Echo the character to the TTY
                    INX     H                       ; Increment the input buffer pointer
                    XRA     A
                    CMP     L                       ; L == 0 (implying end of the buffer)?
                    JNZ     $$GETCH                 ; …more characters to check.
                    ; If we have reached the end of the buffer, fall through
                    ; to the return code…
$$RETURN:           CALL    INPUT_LEN
                    JZ      READ_LINE               ; Zero length, just try again for a line
                    MOV     C, A                    ; C <= A = number of characters in buffer
                    LXI     H, INPUT_BUFFER         ; Reset the input buffer pointer for processing
                    RET                             ; Zero will NOT be set
VALID_CHARS:        DB      ' .0123456789:ABCDEFRX', 00h
;
; IS_INPUT_EMPTY
;
; Checks if HL points to the start of the input buffer.  The
; Z flag will be set on return if it is.
;
; The A register is clobbered by this subroutine.
;
IS_INPUT_EMPTY:     MVI     A, (INPUT_BUFFER & 0FFh)
                    CMP     L
                    RET
;
; INPUT_LEN
;
; Computes the number of characters present in the input buffer
; by subtracting the low-nibble of the base address of the
; input buffer from the current value of L.
;
; The length is returned in the A register.  Flags will be set
; by the ADD instruction so the caller can check for Z set/reset.
;
INPUT_LEN:          MVI     A, ~(INPUT_BUFFER & 0FFh) + 1
                    ADD     L
                    RET
                    
                    
;
; Align the image to a page boundary
;
    IF ($ # 0100h) != 0
                    DS      (0100h - ($ # 0100h) - 1)
                    DB      0FFh
    ENDIF