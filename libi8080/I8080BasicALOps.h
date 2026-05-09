/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 basic arithmetic and logic ops API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080BASICALOPS_H__
#define __I8080BASICALOPS_H__

#include "I8080Config.h"

/**
 * An arithmetic and logic unit
 * An ALU is a bit field comprising two 8-bit operands, an 8-bit
 * result, and a set of flags associated with a computation or
 * bit manipulation.
 *
 * The total size is 32 bits, so it nicely fits in a single
 * int.
 */
typedef struct __attribute__((packed)) {
    uint32_t        OP1 : 8;    /*!< operand #1 */
    uint32_t        OP2 : 8;    /*!< operand #2 */
    uint32_t        CY  : 1;    /*!< carry flag, can be set on input */
    uint32_t        P   : 1;    /*!< parity bit, set to indicate even vs. odd number of one bits
                                     in the result */
    uint32_t        AC  : 1;    /*!< half-carry bit, set to indicate carry out of the lower
                                     nibble */
    uint32_t        Z   : 1;    /*!< zero bit, set to indicate the result is zero */
    uint32_t        S   : 1;    /*!< sign bit, set to indicate the value of bit 7 of the result */
    uint32_t        WC  : 1;    /*!< when set, factor the carry flag value into the
                                     operation (e.g. carry into an ADD) */
    uint32_t        RPL : 1;    /*!< for unary operations, write-back to OP1 rather than RES */
    uint32_t        SET : 1;    /*!< after the operation, set the flags herein according to
                                     the result */
    uint32_t        RES : 8;    /*!< result of the operation */
} I8080ALU_t;

/**
 * Create a new ALU
 * Returns a \ref I8080ALU_t initialized with the provided operand
 * values.  The CY flag is set according to \p carry; the WC flag
 * is set according to \p use_carry; and the SET flag is set
 * according to \p set_flags.
 */
static inline
I8080ALU_t
I8080ALUCreate(
    uint8_t     op1,
    uint8_t     op2,
    bool        carry,
    bool        use_carry,
    bool        set_flags
)
{
    I8080ALU_t  new_ALU = { .OP1 = op1, .OP2 = op2, .RES = 0,
                            .CY = carry ? 0b1 : 0b0,
                            .WC = use_carry ? 0b1 : 0b0,
                            .P = 0b0, .AC = 0b0, .Z = 0b0, .S = 0b0,
                            .RPL = 0b0, .SET = set_flags ? 0b1 : 0b0 };
    return new_ALU;
}

/**
 * Swap operand values
 * The two operands of \p ALU are swapped.
 * @param ALU       a \ref I8080ALU_t
 */
#define I8080ALU2Swap(ALU) { uint8_t O1 = ALU->OP1; ALU->OP1 = ALU->OP2; ALU->OP2 = O1; }

/**
 * Increment the operand
 * RES = OP1 + 1, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1Inc(
    I8080ALU_t  *alu
)
{
    uint8_t     O = alu->OP1,
                ones = 0,
                C = 0b1,
                D = 0b0,
                R = 0b0;

    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->SET ) alu->AC = C;
    
    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->RPL ) alu->OP1 = R; else alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = 0b0;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Decrement the operand
 * RES = OP1 - 1, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1Dec(
    I8080ALU_t  *alu
)
{
    uint8_t     O = alu->OP1,
                ones = 0,
                C = 0b1,
                D = 0b0,
                R = 0b0;
                
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->SET ) alu->AC = C ? 0b0 : 0b1;
    
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ C) & 0b1, C = (~O & C) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->RPL ) alu->OP1 = R; else alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = 0b0;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * One's complement of the operand
 * RES = ~OP1, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1Cmpl1(
    I8080ALU_t  *alu
)
{
    uint8_t     O = alu->OP1,
                ones = 0,
                D = 0b0,
                R = 0b0;
    
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->SET ) alu->AC = D;
    
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O ^ 0b1) & 0b1, ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);
    
    if ( alu->RPL ) alu->OP1 = R; else alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = 0b0;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Two's complement of the operand
 * RES = ~OP1 + 1, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1Cmpl2(
    I8080ALU_t  *alu
)
{
    I8080ALU1Cmpl1(alu);
    I8080ALU1Inc(alu);
}

/**
 * Rotate bits right
 * RES = OP1 >> 1 | ((OP1 & 0b1) << 7), set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1RotR(
    I8080ALU_t  *alu
)
{
    uint8_t     O = alu->OP1,
                ones = 0,
                C = 0b0,
                D = 0b0,
                R = 0b0;
    
    C = (O & 0b1), ones += C, O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->SET ) alu->AC = D;
    
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);
    R = (R >> 1) | (C ? 0b10000000 : 0);
    
    if ( alu->RPL ) alu->OP1 = R; else alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = C;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Rotate bits left
 * RES = OP1 << 1 | ((OP1 & 0b10000000) >> 7), set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1RotL(
    I8080ALU_t  *alu
)
{
    uint8_t     O = alu->OP1,
                ones = 0,
                C = 0b0,
                D = 0b0,
                R = 0b0;
    
    C = (O & 0b10000000) ? 0b1 : 0b0, R = C, ones += C;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->SET ) alu->AC = D;
    
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);
    
    if ( alu->RPL ) alu->OP1 = R; else alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = C;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Shift bits right through carry
 * All bits shift to the right, with the carry bit moving into
 * position 7 and the value that moves out of position 0 replacing
 * the carry bit.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1RotCYR(
    I8080ALU_t  *alu
)
{
    uint8_t     O = alu->OP1,
                ones = 0,
                C = 0b0,
                D = 0b0,
                R = 0b0;
    
    C = (O & 0b1), ones += C, O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->SET ) alu->AC = D;
    
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);
    R = (R >> 1) | (alu->CY ? 0b10000000 : 0);
    
    if ( alu->RPL ) alu->OP1 = R; else alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = C;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Shift bits left through carry
 * All bits shift to the left, with the carry bit moving into
 * position 0 and the value that moves out of position 7 replacing
 * the carry bit.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU1RotCYL(
    I8080ALU_t  *alu
)
{
    uint8_t     O = alu->OP1,
                ones = 0,
                C = 0b0,
                D = 0b0,
                R = 0b0;
    
    C = (O & 0b10000000) ? 0b1 : 0b0, R = (alu->CY ? 0b10000000 : 0), ones += C;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    
    if ( alu->SET ) alu->AC = D;
    
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O >>= 1;
    D = (O & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);
    
    if ( alu->RPL ) alu->OP1 = R; else alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = C;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Binary sum of the operands
 * RES = OP1 + OP2, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU2Add(
    I8080ALU_t  *alu
)
{
    uint8_t     O1 = alu->OP1,
                O2 = alu->OP2,
                ones = 0,
                C = alu->WC ? alu->CY : 0b0,
                R = 0b0;
    
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0), C >>= 1, O1 >>=1, O2 >>= 1;
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0), C >>= 1, O1 >>=1, O2 >>= 1;
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0), C >>= 1, O1 >>=1, O2 >>= 1;
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0), C >>= 1, O1 >>=1, O2 >>= 1;
    
    if ( alu->SET ) alu->AC = C;
    
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0), C >>= 1, O1 >>=1, O2 >>= 1;
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0), C >>= 1, O1 >>=1, O2 >>= 1;
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0), C >>= 1, O1 >>=1, O2 >>= 1;
    C += (O1 & 0b1) + (O2 & 0b1), ones += (C & 0b1), R = (R >> 1) | ((C & 0b1) ? 0b10000000 : 0);
    
    alu->RES = R;
    if ( alu->SET ) {
        alu->S = (C & 0b1);
        alu->CY = (C >> 1);
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Difference of the operands
 * RES = OP1 - OP2, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU2Sub(
    I8080ALU_t  *alu
)
{
    uint8_t     RPL = alu->RPL,
                SET = alu->SET;
    
    // Swap:
    I8080ALU2Swap(alu);
    // Complement OP1:
    alu->RPL = 0b1, alu->SET = 0b0;
    I8080ALU1Cmpl2(alu);
    alu->RPL = RPL, alu->SET = SET;
    // Swap back:
    I8080ALU2Swap(alu);
    // Add:
    I8080ALU2Add(alu);
}

/**
 * Bitwise AND of the operands
 * RES = OP1 & OP2, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU2And(
    I8080ALU_t  *alu
)
{
    uint8_t     O1 = alu->OP1,
                O2 = alu->OP2,
                ones = 0,
                D = 0b0,
                R = 0b0;
    
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    
#ifdef I8080_AND_SETS_AC
    if ( alu->SET ) alu->AC = (O1 | O2) & 0b1;
#else
    if ( alu->SET ) alu->AC = 0b0;
#endif
    
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = (O1 & O2 & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);

    alu->RES = R;
    if ( alu->SET ) {
        alu->S = D;
        alu->CY = 0b0;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Bitwise XOR of the operands
 * RES = OP1 ^ OP2, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU2Xor(
    I8080ALU_t  *alu
)
{
    uint8_t     O1 = alu->OP1,
                O2 = alu->OP2,
                ones = 0,
                D = 0b0,
                R = 0b0;
    
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 ^ O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);

    alu->RES = R;
    if ( alu->SET ) {
        alu->AC = 0b0;
        alu->S = D;
        alu->CY = 0b0;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

/**
 * Bitwise OR of the operands
 * RES = OP1 | OP2, set flags if requested.
 * @param ALU       a \ref I8080ALU_t
 */
static inline
void
I8080ALU2Or(
    I8080ALU_t  *alu
)
{
    uint8_t     O1 = alu->OP1,
                O2 = alu->OP2,
                ones = 0,
                D = 0b0,
                R = 0b0;
    
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0), O1 >>= 1, O2 >>= 1;
    D = ((O1 | O2) & 0b1), ones += D, R = (R >> 1) | (D ? 0b10000000 : 0);

    alu->RES = R;
    if ( alu->SET ) {
        alu->AC = 0b0;
        alu->S = D;
        alu->CY = 0b0;
        alu->P = (ones % 2) ^ 0b1;
        alu->Z = (ones == 0);
    }
}

#endif /* __I8080BASICALOPS_H__ */
