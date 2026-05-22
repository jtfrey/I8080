;
; Two's-complement negation of the 16-bit value
; (A << 8) | C.  The B register is used as scratch
; for the complement of the high-byte.
;
; On exit, the high-byte is in A and B, low-byte
; in C.
;
; Clobbered:    B
;
CMA16AC_B_AC:   MACRO
                cma                     ; A <= ~A (high-byte)                           4
                mov     B, A            ; B <= ~A (high-byte)                           5
                mov     A, C            ; A <= C (low-byte)                             5
                cma                     ; A <= ~A (low-byte)                            4
                mov     C, A            ; C <= -C                                       5
                inx     BC              ; BC <= BC + 1                                  5
                mov     A, B            ; A <= B (high-byte)                            5
                ENDM                                                                   33

;
; Two's-complement negation of the 16-bit value
; (A << 8) | C.  The B register is used as scratch
; for the complement of the high-byte.
;
; On exit, the high-byte is in B, low-byte in C.
;
; Clobbered:    A, B
;
CMA16AC_B_BC:   MACRO
                cma                     ; A <= ~A (high-byte)                           4
                mov     B, A            ; B <= ~A (high-byte)                           5
                mov     A, C            ; A <= C (low-byte)                             5
                cma                     ; A <= ~A (low-byte)                            4
                mov     C, A            ; C <= -C                                       5
                inx     BC              ; BC <= BC + 1                                  5
                ENDM                                                                   28

;
; Two's-complement negation of the 16-bit value
; (B << 8) | C.  The A register is used for
; the arithmetic operations.
;
; On exit, the high-byte is in B, low-byte in C.
;
; Clobbered:    A
;
CMA16BC_A_BC:   MACRO
                mov     A, C            ; A <= C (low-byte)                             5
                cma                     ; A <= ~A (low-byte)                            4
                mov     C, A            ; C <= ~C                                       5
                mov     A, B            ; A <= B (high-byte)                            5
                cma                     ; A <= ~B (high-byte)                           4
                mov     B, A            ; B <= ~B                                       5
                inx     BC              ; BC <= BC + 1                                  5
                ENDM                                                                   33

;
; Two's-complement negation of the 16-bit value
; (A << 8) | E.  The D register is used as scratch
; for the complement of the high-byte.
;
; On exit, the high-byte is in A and D, low-byte
; in E.
;
; Clobbered:    D
;
CMA16AE_D_AE:   MACRO
                cma                     ; A <= ~A (high-byte)                           4
                mov     D, A            ; D <= ~A (high-byte)                           5
                mov     A, E            ; A <= E (low-byte)                             5
                cma                     ; A <= ~A (low-byte)                            4
                mov     E, A            ; E <= -E                                       5
                inx     DE              ; DE <= DE + 1                                  5
                mov     A, D            ; A <= D (high-byte)                            5
                ENDM                                                                   33

;
; Two's-complement negation of the 16-bit value
; (A << 8) | E.  The D register is used as scratch
; for the complement of the high-byte.
;
; On exit, the high-byte is in D, low-byte in E.
;
; Clobbered:    A, D
;
CMA16AE_D_DE:   MACRO
                cma                     ; A <= ~A (high-byte)                           4
                mov     D, A            ; D <= ~A (high-byte)                           5
                mov     A, E            ; A <= E (low-byte)                             5
                cma                     ; A <= ~A (low-byte)                            4
                mov     E, A            ; E <= -E                                       5
                inx     DE              ; DE <= DE + 1                                  5
                ENDM                                                                   28

;
; Two's-complement negation of the 16-bit value
; (D << 8) | E.  The A register is used for
; the arithmetic operations.
;
; On exit, the high-byte is in D, low-byte in E.
;
; Clobbered:    A
;
CMA16DE_A_DE:   MACRO
                mov     A, E            ; A <= E (low-byte)                             5
                cma                     ; A <= ~A (low-byte)                            4
                mov     E, A            ; E <= ~E                                       5
                mov     A, D            ; A <= D (high-byte)                            5
                cma                     ; A <= ~D (high-byte)                           4
                mov     D, A            ; D <= ~D                                       5
                inx     DE              ; DE <= DE + 1                                  5
                ENDM                                                                   33

