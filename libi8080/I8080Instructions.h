/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 instructions API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */
 
#ifndef __I8080INSTRUCTIONS_H__
#define __I8080INSTRUCTIONS_H__

#include "I8080Registers.h"
#include "I8080Memory.h"
#include "I8080DeviceIO.h"

/**
 * Forward declare the I8080SystemPtr type
 * This is here to make the compiler happy; since it's a pointer and
 * no further structuring is necessary in this header, we don't need
 * to pull-in the I8080System.h header -- we'll do that in
 * I8080Instructions.c.
 */
typedef struct I8080System * I8080SystemPtr;

/**
 * Type of an 8080 instruction
 * The I8080 base instruction is a single byte.
 */
typedef uint8_t I8080Instr_t;

/**
 * Produce a mask for a range of bit indices
 * Given the starting (low) bit index, \p BS, and the index of
 * the last bit to include, \p BE, generate a mask of ones in 
 * those bit indices and zeroes elsewhere.
 *
 *    I8080InstrMask(0, 7) => 0xFF
 *    I8080InstrMask(4, 7) => 0xF0
 *    I8080InstrMask(2, 5) => 0x3C
 * @param BS        index of the first (lower) bit
 * @param BE        index of the final (higher) bit
 * @return          the bit mask
 */
#define I8080InstrMask(BS, BE) \
            (((uint8_t)0xFF >> (8 - ((BE) - (BS) + 1))) << (BS))

/**
 * Extract the value of a range of bit indices
 * Applies a mask for bit indices \p BS through \p BE to \p I.
 * @param I         byte from which the value is extracted
 * @param BS        index of the first (lower) bit
 * @param BE        index of the final (higher) bit
 * @return          the extracted bit pattern
 */
#define I8080InstrExt(I, BS, BE) \
            ((I) & I8080InstrMask(BS, BE))

/**
 * Extract the value of a range of bit indices and shift
 * Applies a mask for bit indices \p BS through \p BE to \p I
 * and shift the result so that index \p BS is at index 0.
 * @param I         byte from which the value is extracted
 * @param BS        index of the first (lower) bit
 * @param BE        index of the final (higher) bit
 * @return          the extracted and shifted bit pattern
 */
#define I8080InstrExtShift(I, BS, BE) \
            (((I) >> (BS)) & I8080InstrMask(0, ((BE) - (BS))))

/**
 * Instruction dispatch function pointer
 * Type of a function that handles an I8080 instruction.  The dependency on
 * the system's register set and memory is met by passing both to the
 * function along with the instruction byte.
 *
 * The function should return the number of clock cycles required to
 * process the instruction.  If zero (0) is returned then the function
 * did NOT handle the instruction.
 */
typedef I8080CycleCount (*I8080InstrDispatchCallback)(I8080SystemPtr sys8080, I8080Instr_t instr);

/**
 * Sub-type of instruction handler node
 * Each instruction handler node can either be a fully-fledged instance
 * or a reference to a canonical node.  This allows nodes to be reused
 * without overriding their own linkage chains.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrHandlerSubTypeOriginal,      /*!< NOT a reference to another node  */
    kI8080InstrHandlerSubTypeReference      /*!< reference to another node */
} I8080InstrHandlerSubType_t;

/**
 * Action taken by an instruction handler
 * Each instruction handler node can either dispatch the matched instruction
 * to a function for processing or move it through a sub-list of handler
 * nodes for further matching.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrHandlerActionDispatch,       /*!< pass the instruction to the dispatch function  */
    kI8080InstrHandlerActionSubHandler      /*!< pass the instruction to a sub-handler chain */
} I8080InstrHandlerAction_t;

/**
 * Instruction opcode fine-grain validation function
 * In an instruction matches the masked bit pattern for a handler, this (optional)
 * function can be called to further validate that the handler indeed accepts the
 * opcode in question.
 * @param instr         the opcode to verify
 * @return              boolean true if the handler DOES accept the opcode
 */
typedef bool (*I8080InstrFineGrainMatchCallback)(I8080Instr_t instr);

/**
 * Instruction handler registration
 * Any instruction with a bit pattern \p p satisfying the condition:
 *
 *     (p & mask) == pattern
 *
 * will either be passed to the dispatch function or processed by a
 * sub-handler chain, as determined by the \p action field.
 */
typedef struct I8080InstrHandler {
    I8080InstrHandlerSubType_t                  sub_type;
    union {
        struct {
            const char                          *name;          /*!< string constant, the name of this handler */
            I8080Instr_t                        mask;           /*!< the bitmask applied to the instruction to
                                                                     isolate the constant bits for some class of
                                                                     instruction */
            I8080Instr_t                        pattern;        /*!< the bit pattern after masking that instructions
                                                                     of this class will possess */
            I8080InstrFineGrainMatchCallback    fine_grain;     /*!< the fine-grain match function */
            I8080InstrHandlerAction_t           action;         /*!< how are matches handled? */
            union {
                I8080InstrDispatchCallback      dispatch;       /*!< the function that handles matched instructions */
                const struct I8080InstrHandler  *sub_handler;   /*!< finer-grain handler chain that builds on this handler's
                                                                     mask/pattern */
            };
        };
        const struct I8080InstrHandler          *reference;
    };
    const struct I8080InstrHandler              *next_handler;  /*!< link to the next handler in the chain */
} I8080InstrHandler_t;

/**
 * Fetch instruction from memory
 * The byte at the memory location pointed-at by the program counter is
 * read and returned.  The program counter is incremented by one.
 * @param rgstrs        the register set to use
 * @param sysmem        the system memory to use
 * @return              the byte read
 */
I8080Instr_t I8080InstrFetch(I8080SystemPtr sys8080);

/**
 * Dispatch an instruction
 * Given the instruction handler chain \p handlers, locate a handler that matches with
 * \p instr and dispatch it.
 * @param handlers      linked-list of instructional handler registrations
 * @param rgstrs        the register set to use
 * @param sysmem        the system memory to use
 * @param instr         the instruction to dispatch
 * @return              the number of CPU cycles required to complete the instruction, or zero
 *                      (0) if unhandled
 */
I8080CycleCount I8080InstrHandlerDispatch(const I8080InstrHandler_t *handlers, I8080SystemPtr sys8080, I8080Instr_t instr);

/**
 * Operand, register
 * Constants that determine which 8-bit register an instruction
 * references as an operand.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrRegisterB        = 0b000,
    kI8080InstrRegisterC        = 0b001,
    kI8080InstrRegisterD        = 0b010,
    kI8080InstrRegisterE        = 0b011,
    kI8080InstrRegisterH        = 0b100,
    kI8080InstrRegisterL        = 0b101,
    kI8080InstrRegisterMem      = 0b110,    /*!< Address in memory, not a register */
    kI8080InstrRegisterA        = 0b111,
} I8080InstrRegister_t;

extern const char* I8080InstrRegisterNames[];

/**
 * Operand, register pair
 * Constants that determine which 16-bit register an instruction
 * references as an operand.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrRegisterPairBC   = 0b00,
    kI8080InstrRegisterPairDE   = 0b01,
    kI8080InstrRegisterPairHL   = 0b10,
    kI8080InstrRegisterPairPSW  = 0b11,
    kI8080InstrRegisterPairSP   = 0b11,
} I8080InstrRegisterPair_t;

extern const char* I8080InstrRegisterPairNames[];

/**
 * Instruction sub-type, arithmetic and logic operations
 * Constants that determine what ALU operation is indicated by an
 * instruction.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrALUOpAdd         = 0b000,
    kI8080InstrALUOpAddC        = 0b001,
    kI8080InstrALUOpSub         = 0b010,
    kI8080InstrALUOpSubC        = 0b011,
    kI8080InstrALUOpAnd         = 0b100,
    kI8080InstrALUOpXor         = 0b101,
    kI8080InstrALUOpOr          = 0b110,
    kI8080InstrALUOpCmp         = 0b111
} I8080InstrALUOp_t;

/**
 * Instruction sub-type, bitwise rotate operations
 * Constants that determine what bitwise rotation operation is indicated by an
 * instruction.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrRotateOpLeft             = 0b00,
    kI8080InstrRotateOpRight            = 0b01,
    kI8080InstrRotateOpLeftThruCarry    = 0b10,
    kI8080InstrRotateOpRightThruCarry   = 0b11
} I8080InstrRotateOp_t;

/**
 * Instruction sub-type, direct-address operations
 * Constants that determine what direct-address operation is indicated by an
 * instruction.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrDirectAddrOpSTA  = 0b10,
    kI8080InstrDirectAddrOpLDA  = 0b11,
    kI8080InstrDirectAddrOpSHLD = 0b00,
    kI8080InstrDirectAddrOpLHLD = 0b01
} I8080InstrDirectAddrOp_t;

/**
 * Instruction sub-type, conditional operations
 * Constants that determine what conditional criterion is indicated by
 * an instruction.
 */
typedef enum __attribute__((packed)) {
    kI8080InstrCondOnNotZero    = 0b000,
    kI8080InstrCondOnZero       = 0b001,
    kI8080InstrCondOnNoCarry    = 0b010,
    kI8080InstrCondOnCarry      = 0b011,
    kI8080InstrCondOnParityOdd  = 0b100,
    kI8080InstrCondOnParityEven = 0b101,
    kI8080InstrCondOnPlus       = 0b110,
    kI8080InstrCondOnMinus      = 0b111
} I8080InstrCond_t;

/**
 * A compiled instruction dispatch table
 * The linked-list tree of instruction handlers can be flattened into
 * a 256-element array of dispatch functions.  This requires 4097 bytes
 * of memory for this data structure, but scheduling is O(1) complexity.
 * When the table is compiled from the linked list, any collisions
 * of one opcode to more than one instruction will be noted on stderr.
 */
typedef struct {
    int                         n_instructions;         /*!< the number of instructions filled-in by the compile */
    const char                  *handler_names[256];    /*!< the handler name for each opcode is noted */
    I8080InstrDispatchCallback  dispatchers[256];       /*!< the dispatch function for the opcode */
} I8080InstrTable_t;

/**
 * Compile an instruction dispatch table
 * The linked-list tree of instruction handlers represented by \p handlers
 * has all 256 opcodes presented to it.  The handler and dispatch function
 * that matches is entered into the \p itbl at that opcode index.
 * @param itbl      pointer to the compiled instruction table
 * @param handlers  the linked-list tree of instruction handlers
 */
void I8080InstrTableInit(I8080InstrTable_t *itbl, const I8080InstrHandler_t *handlers);

/**
 * The default I8080 ISA
 * A linked-list tree of the full 8080 instruction set.  Can be passed
 * to \ref I8080InstrTableInit() to produce a compiled table for quick
 * and easy instruction dispatch.
 */
extern const I8080InstrHandler_t *I8080DefaultISA;

/**
 * Information about an 8080 instruction
 */
typedef struct {
    I8080Instr_t    opcode;         /*!< the opcode */
    const char      *symbolic;      /*!< symbolic representation of the instruction */
    int             n_bytes;        /*!< the number of operand bytes + the opcode byte */
    const char      *description;   /*!< a description of what the instruciton does */
} I8080InstrSummary_t;

/**
 * Mapping of instruction information indexed by opcode
 * Given an 8080 opcode, the array element at that index contains information
 * about the instruction.
 */
extern const I8080InstrSummary_t I8080InstrSummaryTable[];

#endif /* __I8080INSTRUCTIONS_H__ */
