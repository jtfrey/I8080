/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 registers API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080REGISTERS_H__
#define __I8080REGISTERS_H__

#include "I8080Config.h"
#include "I8080TextBuffer.h"

/**
 * Map register index to actual offset in register array
 * Depending on the endian-ness of the system on which this code is
 * compiled, the ordering of registers in the data structure may not
 * directly map to the index in the array form.  E.g. on little endian
 * register B|0 can be found at array index 1, and C|1 can be found at
 * index 0.  So on little endian we flip bit zero in the array indices.
 * @param X         the nominal register array index
 * @return          the actual register array index
 */
#ifdef BITS_BIG_ENDIAN
#    define I8080RgstrIdx(X) 
#else
#    define I8080RgstrIdx(X)      (X ^ 0b1)
#endif

/**
 * Cycle counter
 * A type used to count elapsed CPU cycles.
 */
typedef uint64_t I8080CycleCount;

/**
 * Calculate the elapsed µs for a cycle count
 * Since the 8080 ran at 2 MHz, this is simply half of the
 * cycle count:
 *
 *     N cycles  x  (1 s / 2e6 cycles) = (N / 2e6) s
 *   = (1/2) N
 *
 * @param cycles        the number of elapsed cycles
 * @return              the number of µs used by those cycles
 */
static inline
double
I8080CycleCountToMicroseconds(
    I8080CycleCount cycles
)
{
    return (double)cycles * 0.5;
}

/**
 * Data structure holding all of the 8080 registers
 * The 8080 consists of 6 8-bit general-purpose registers; an 8-bit
 * accumulator; an 8-bit condition codes bit vector; a 16-bit program
 * counter and stack pointer.  The 8-bit registers are named with letters
 * ABCDE and HL, but are internally referenced by index:
 *
 *      BCDEHL-A = 012345-7
 *
 * The 8-bit registers can be treated in pairs as 16-bit entities:
 *
 *      B + C => BC     (high-order byte + low-order byte)
 *      D + E => DE
 *      H + L => HL
 *      F + A => PSW    (program status word)
 *
 * This structure is arranged such that all registers can be referenced
 * by name.  The layout preserves the appropriate byte-ordering of the
 * register pairs.
 *
 * The individual bits of the condition codes register can also be
 * referenced directly (e.g. as CY, Z, P) or collectively as F.
 *
 * The 8-bit registers can also be referenced by index via the \p R
 * field; however, the \ref I8080RgstrIdx() macro must be used to map
 * the nominal register index to the actual in-memory index.
 *
 * One non-standard field is present:  \p CYCLS.  This field is used to
 * count the elapsed processor cycles as execution proceeds.
 */
typedef struct __attribute__((packed)) {
    union {
        uint8_t                     R[8];
        uint16_t                    RP[4];
        struct {
            union {
                struct {
#ifdef BITS_BIG_ENDIAN
                    uint8_t         B;
                    uint8_t         C;
                    uint8_t         D;
                    uint8_t         E;
                    uint8_t         H;
                    uint8_t         L;
#else
                    uint8_t         C;
                    uint8_t         B;
                    uint8_t         E;
                    uint8_t         D;
                    uint8_t         L;
                    uint8_t         H;
#endif
                };
                struct {
                    uint16_t        BC;
                    uint16_t        DE;
                    union {
                        uint16_t    M;
                        uint16_t    HL;
                    };
                };
            };
            union {
                struct {
#ifndef BITS_BIG_ENDIAN
                    uint8_t         A;
#endif
                    union {
                        uint8_t     F;
                        struct {
#ifdef BITS_BIG_ENDIAN
                            uint8_t S  : 1;
                            uint8_t Z  : 1;
                            uint8_t    : 1;
                            uint8_t AC : 1;
                            uint8_t    : 1;
                            uint8_t P  : 1;
                            uint8_t    : 1;
                            uint8_t CY : 1;
#else
                            uint8_t CY : 1;
                            uint8_t    : 1;
                            uint8_t P  : 1;
                            uint8_t    : 1;
                            uint8_t AC : 1;
                            uint8_t    : 1;
                            uint8_t Z  : 1;
                            uint8_t S  : 1;
#endif
                        };
                    };
#ifdef BITS_BIG_ENDIAN
                    uint8_t         A;
#endif
                };
                uint16_t            PSW;
            };
        };
    };
    uint16_t                        PC;
    uint16_t                        SP;
    uint8_t                         INTE;
    I8080CycleCount                 CYCLS;
} I8080Registers_t;

/**
 * Create and initialize a register set
 * Returns a \ref I8080Registers_t initialized to starting
 * values.
 */
static inline
I8080Registers_t
I8080Registers(void)
{
    I8080Registers_t    R = {
                            .BC = 0x0000,
                            .DE = 0x0000,
                            .HL = 0x0000,
                            .PSW = 0x0000,
                            .PC = 0x0000,
                            .SP = 0x0000,
                            .INTE = 0x01,
                            .CYCLS = 0ULL };
    return R;
}

/**
 * Exchange two 8-bit indexed registers' values
 * The value in the register at index \p I is swapped with the value at
 * index \p J.
 * @param RGSTRS        a \p I8080Registers_t
 * @param I             first register index
 * @param J             second register index
 */
#define I8080RegistersExchangeIndexed8b(RGSTRS, I, J) \
    { uint8_t TMP_ ## I = (RGSTRS).R[I8080RgstrIdx((I))]; \
     (RGSTRS).R[I8080RgstrIdx((I))] = (RGSTRS).R[I8080RgstrIdx((J))]; \
     (RGSTRS).R[I8080RgstrIdx((J))] = TMP_ ## I; \
    }

/**
 * Exchange two 8-bit named registers' values
 * The value in the register \b X is swapped with the value of
 * register \p Y.
 * @param RGSTRS        a \p I8080Registers_t
 * @param I             first register name
 * @param J             second register name
 */
#define I8080RegistersExchangeNamed8b(RGSTRS, X, Y) \
    { uint8_t TMP_ ## X = (RGSTRS).X; \
     (RGSTRS).X = (RGSTRS).Y; \
     (RGSTRS).Y = TMP_ ## X; \
    }

/**
 * Write a summary of the register values to a FILE stream
 * Writes a summary of the register values in \p rgstrs to the FILE stream
 * \p stream.
 * @param stream        the stream to which to write
 * @param rgstrs        pointer to the registers structure to summarize
 */
void I8080RegistersPrint(FILE *stream, I8080Registers_t *rgstrs);

/**
 * Write a summary of the register values to a text buffer.
 * Writes a summary of the register values in \p rgstrs to the text buffer
 * \p tbuff.
 * @param tbuff         the text buffer to which to write
 * @param rgstrs        pointer to the registers structure to summarize
 */
void I8080RegistersWriteToTextBuffer(I8080TextBufferRef tbuff, I8080Registers_t *rgstrs);

#endif /* __I8080REGISTERS_H__ */
