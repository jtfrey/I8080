;
; # K E R N E L
;
; The kernel is located starting at memory address $0000.  The
; RESTART entry points are used to dispatch kernel calls:
;
;   RST0    display a message and halt
;   RST1    userland -> kernel function entry point
;   RST2    A <- (HL+DX) -- indexed indirect load of the
;           accumulator
;
; ## RST0 — Shutdown
;
; Since this vector exists at $0000 and a system booted without
; an explicit program counter (PC) specified will begin
; executing instructions here, it displays a brief message then
; halts the system.
;
; ## RST1 — KrnlFn
; 
; Before calling a kernel function with RST 1 the caller must:
;
;     - set register DE to the kernel function number
;     - set registers B/C/BC to the first one/two arguments
;       to the kernel function
;     - push any additional arguments to the userland stack
;
; Upon entry to the kernel a context switch from userland to
; kernel is made:
;
;     - HL is pushed to the user stack
;     - The userland stack pointer before that push is
;         - HL <- 2, HL += SP
;     - Store the computed pointer in the USERSTACK word
;       situated directly above the kernel stack ($03FE)
;     - H <- kernel stack pointer, high byte
;     - L <- kernel stack pointer, low byte
;     - Swap HL <-> SP, kernel stack is now selected
;     - Push the other three register pairs to the kernel
;       stack
;
; At this point the context switch is complete.  Next, the
; kernel function number must be turned into an address table
; slot by multiplying by 2.  The function number is in DE,
; so E is shifted left (with the exiting bit moving to the
; CY register) and D is shifted left (with CY from E moving
; in at the right) — and DE = DE * 2, the offset to the
; appropriate jump table slot.  The base address of the
; jump table is loaded into HL, DE is added, and 
;

KRNLSTCKL   equ     0feh
KRNLSTCKH   equ     03h
USERSTACK   equ     03feh

            org     0000h


;
; Shutdown
;
; Display a message and shutdown the system.
;
Shutdown:
RST0:       lxi     b, NOPRGRMSTR
            jmp     Halt        ; jump to the halt routine
            ds      8 - ($ - RST0)

;
; KrnlFn
;
; Prepare to execute a context switch from userland to
; kernel and dispatch to a numbered kernel function.
;
KrnlFn:
RST1:       push    hl          ; store user stack pointer
            lxi     h, 2        ; which was two bytes above where
            dad     sp          ; we just pushed HL
            jmp     KRNLENTRY   ; continue at the rest of the fn
            ds      8 - ($ - RST1)

;
; LDAIdxIndrct
;
; Load (HL + DE) into the accumulator w/o destroying
; either register.  This subroutine works fine from
; both userland and the kernel.
;
LDAIdxIndrct:
RST2:       push    hl          ; save HL
            dad     d           ; HL <- HL + DE
            mov     a, M        ; A <- (HL)
            pop     hl
            ret
            ds      8 - ($ - RST2)

;
; STAIdxIndrct
;
; Store the accumulator into (HL + DE) into w/o destroying
; either register.  This subroutine works fine from
; both userland and the kernel.
;
STAIdxIndrct:
RST3:       push    hl          ; save HL
            dad     d           ; HL <- HL + DE
            mov     M, a        ; (HL) <- A
            pop     hl
            ret
            ds      8 - ($ - RST3)

RST4:       ds      8 - ($ - RST4)
RST5:       ds      8 - ($ - RST5)
RST6:       ds      8 - ($ - RST6)
RST7:       ds      8 - ($ - RST7)

;
; Execution comes here when the system needs to print a
; message and terminate execution.
;
Halt:       ldax    b           ; load next character
            cpi     0h          ; a NUL terminates the string
            jz      Halt_X      ; exit the subroutine
            out     0h          ; write the character to stdout
            inx     b           ; move to next character address
            jmp     Halt        ; next iteration
Halt_X:     hlt
NOPRGRMSTR: db      0ah, 0ah, 'ERROR:  No program to execute (PC=$0000)', 0ah, 0ah, 0h



KRNLENTRY:  shld    USERSTACK   ; stash the user stack address
            
            mvi     l, KRNLSTCKL; with the user stack pointer
            mvi     h, KRNLSTCKH; stashed, load the kernel stack
            sphl                ; pointer, HL <-> SP
            push    psw         ; BC and DE are used for return values
                                ; but we will save and restore F/A
            
            ; DE has the kernel function number
            ora     a           ; ensure carry is clear
            mov     a, e        ; multiply the function number by
            rlc                 ; two to get the proper offset
            mov     e, a        ; done with the lower byte
            mov     a, d        ; do the higher byte, shifting-in
            rlc                 ; the carry-out of the lower byte
            mov     d, a        ; DE now has DE <<= 1
            lxi     h, KRNLJMPT ; load the jump table address into HL
            dad     d           ; HL <- HL + DE
            mov     e, M        ; E <- (HL) = LSB of jump address
            inx     h           ; HL++
            mov     d, M        ; D <- (HL) = MSB of jump address
            xchg                ; HL <-> DE
            pchl                ; PC <- HL = jump to kernel function
            
KRNLEXIT:   pop     psw         ; restore the F/A register states
            
            lhld    USERSTACK   ; restore the user stack pointer
            dcx     hl          ; which was 2 bytes above where
            dcx     hl          ; we pushed HL on entry
            sphl                ; HL <-> SP
            pop     hl          ; restore the HL register…hlt
            ret                 ; and return to the caller



;
; The KeRNeL JuMp Table contains the dispatch address for each
; of the kernel functions.  The address of function zero (0) is
; at KRNLJMPT, function one (1) is at KRNLJMPT+2, etc.
;
KRNLJMPT:   dw      CSTROUT     ;
            dw      BYTETOHEX   ;
            
            
            
;
; FN 0
; CSTROUT
;
; A NUL-terminated C string at the address in BC is written
; to output device 0
;
CSTROUT:    ldax    b           ; load next character
            cpi     0h          ; a NUL terminates the string
            jz      KRNLEXIT    ; exit the subroutine
            out     0h          ; write the character to stdout
            inx     b           ; move to next character address
            jmp     CSTROUT     ; loop

;
; FN 1
; BYTETOHEX
;
; A byte in C is expanded into two ASCII characters in B and
; C.
;
BYTETOHEX:  mov     b, c        ; B <- C
            mov     a, c        ; A <- C
            ani     0fh         ; isolate the low nibble
            cpi     10          ; if 0-9 then skip the
            jm      BYTETOHEX0  ; initial addition of $11
            adi     07h         ; necessary for A-F
BYTETOHEX0: adi     030h        ; shift up another $30
            mov     c, a        ; low nibble is done
            mov     a, b        ; get the copy
            rar                 ; A >>= 4
            rar
            rar
            rar
            ani     0fh         ; A &= $F
            cpi     10          ; if 0-9 then skip the
            jm      BYTETOHEX1  ; initial addition of $11
            adi     07h         ; necessary for A-F
BYTETOHEX1: adi     030h        ; shift up another $30
            mov     b, a        ; low nibble is done
            jmp     KRNLEXIT    ; exit the subroutine

