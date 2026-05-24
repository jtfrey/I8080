/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implements the I8080 system API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080System.h"
#include "I8080Logging.h"

//

static I8080CycleCount I8080CycleCountByOpcode[256] = {
             4,    /* NOP */
            10,    /* LXI B, d16 */
             7,    /* STAX B */
             5,    /* INX B */
             5,    /* INR B */
             5,    /* DCR B */
             7,    /* MVI B, d8 */
             4,    /* RLC */
             4,    /* *NOP* */
            11,    /* DAD B */
             7,    /* LDAX B */
             5,    /* DCX B */
             5,    /* INR C */
             5,    /* DCR C */
             7,    /* MVI C, d8 */
             4,    /* RRC */
             4,    /* *NOP* */
            10,    /* LXI D, d16 */
             7,    /* STAX D */
             5,    /* INX D */
             5,    /* INR D */
             5,    /* DCR D */
             7,    /* MVI D, d8 */
             4,    /* RAL */
             4,    /* *NOP* */
            11,    /* DAD D */
             7,    /* LDAX D */
             5,    /* DCX D */
             5,    /* INR E */
             5,    /* DCR E */
             7,    /* MVI E, d8 */
             4,    /* RAR */
             4,    /* *NOP* */
            10,    /* LXI H, d16 */
            16,    /* SHLD adr */
             5,    /* INX H */
             5,    /* INR H */
             5,    /* DCR H */
             7,    /* MVI H, d8 */
             4,    /* DAA */
             4,    /* *NOP* */
            11,    /* DAD H */
            16,    /* LHLD adr */
             5,    /* DCX H */
             5,    /* INR L */
             5,    /* DCR L */
             7,    /* MVI L, d8 */
             4,    /* CMA */
             4,    /* *NOP* */
            10,    /* LXI SP, d16 */
            13,    /* STA adr */
             5,    /* INX SP */
            10,    /* INR M */
            10,    /* DCR M */
            10,    /* MVI M, d8 */
             4,    /* STC */
             4,    /* *NOP* */
            11,    /* DAD SP */
            13,    /* LDA adr */
             5,    /* DCX SP */
             5,    /* INR A */
             5,    /* DCR A */
             7,    /* MVI A, d8 */
             4,    /* CMC */
             5,    /* MOV B,B */
             5,    /* MOV B,C */
             5,    /* MOV B,D */
             5,    /* MOV B,E */
             5,    /* MOV B,H */
             5,    /* MOV B,L */
             7,    /* MOV B,M */
             5,    /* MOV B,A */
             5,    /* MOV C,B */
             5,    /* MOV C,C */
             5,    /* MOV C,D */
             5,    /* MOV C,E */
             5,    /* MOV C,H */
             5,    /* MOV C,L */
             7,    /* MOV C,M */
             5,    /* MOV C,A */
             5,    /* MOV D,B */
             5,    /* MOV D,C */
             5,    /* MOV D,D */
             5,    /* MOV D,E */
             5,    /* MOV D,H */
             5,    /* MOV D,L */
             7,    /* MOV D,M */
             5,    /* MOV D,A */
             5,    /* MOV E,B */
             5,    /* MOV E,C */
             5,    /* MOV E,D */
             5,    /* MOV E,E */
             5,    /* MOV E,H */
             5,    /* MOV E,L */
             7,    /* MOV E,M */
             5,    /* MOV E,A */
             5,    /* MOV H,B */
             5,    /* MOV H,C */
             5,    /* MOV H,D */
             5,    /* MOV H,E */
             5,    /* MOV H,H */
             5,    /* MOV H,L */
             7,    /* MOV H,M */
             5,    /* MOV H,A */
             5,    /* MOV L,B */
             5,    /* MOV L,C */
             5,    /* MOV L,D */
             5,    /* MOV L,E */
             5,    /* MOV L,H */
             5,    /* MOV L,L */
             7,    /* MOV L,M */
             5,    /* MOV L,A */
             7,    /* MOV M,B */
             7,    /* MOV M,C */
             7,    /* MOV M,D */
             7,    /* MOV M,E */
             7,    /* MOV M,H */
             7,    /* MOV M,L */
             7,    /* HLT */
             7,    /* MOV M,A */
             5,    /* MOV A,B */
             5,    /* MOV A,C */
             5,    /* MOV A,D */
             5,    /* MOV A,E */
             5,    /* MOV A,H */
             5,    /* MOV A,L */
             7,    /* MOV A,M */
             5,    /* MOV A,A */
             4,    /* ADD B */
             4,    /* ADD C */
             4,    /* ADD D */
             4,    /* ADD E */
             4,    /* ADD H */
             4,    /* ADD L */
             7,    /* ADD M */
             4,    /* ADD A */
             4,    /* ADC B */
             4,    /* ADC C */
             4,    /* ADC D */
             4,    /* ADC E */
             4,    /* ADC H */
             4,    /* ADC L */
             7,    /* ADC M */
             4,    /* ADC A */
             4,    /* SUB B */
             4,    /* SUB C */
             4,    /* SUB D */
             4,    /* SUB E */
             4,    /* SUB H */
             4,    /* SUB L */
             7,    /* SUB M */
             4,    /* SUB A */
             4,    /* SBB B */
             4,    /* SBB C */
             4,    /* SBB D */
             4,    /* SBB E */
             4,    /* SBB H */
             4,    /* SBB L */
             7,    /* SBB M */
             4,    /* SBB A */
             4,    /* ANA B */
             4,    /* ANA C */
             4,    /* ANA D */
             4,    /* ANA E */
             4,    /* ANA H */
             4,    /* ANA L */
             7,    /* ANA M */
             4,    /* ANA A */
             4,    /* XRA B */
             4,    /* XRA C */
             4,    /* XRA D */
             4,    /* XRA E */
             4,    /* XRA H */
             4,    /* XRA L */
             7,    /* XRA M */
             4,    /* XRA A */
             4,    /* ORA B */
             4,    /* ORA C */
             4,    /* ORA D */
             4,    /* ORA E */
             4,    /* ORA H */
             4,    /* ORA L */
             7,    /* ORA M */
             4,    /* ORA A */
             4,    /* CMP B */
             4,    /* CMP C */
             4,    /* CMP D */
             4,    /* CMP E */
             4,    /* CMP H */
             4,    /* CMP L */
             7,    /* CMP M */
             4,    /* CMP A */
             5,    /* RNZ */
            10,    /* POP B */
            10,    /* JNZ adr */
            10,    /* JMP adr */
            11,    /* CNZ adr */
            11,    /* PUSH B */
             7,    /* ADI d8 */
            11,    /* RST 0 */
             5,    /* RZ */
            10,    /* RET */
            10,    /* JZ adr */
             4,    /* *NOP* */
            11,    /* CZ adr */
            17,    /* CALL adr */
             7,    /* ACI d8 */
            11,    /* RST 1 */
             5,    /* RNC */
            10,    /* POP D */
            10,    /* JNC adr */
            10,    /* OUT d8 */
            11,    /* CNC adr */
            11,    /* PUSH D */
             7,    /* SUI d8 */
            11,    /* RST 2 */
             5,    /* RC */
             4,    /* *NOP* */
            10,    /* JC adr */
            10,    /* IN d8 */
            11,    /* CC adr */
             4,    /* *NOP* */
             7,    /* SBI d8 */
            11,    /* RST 3 */
             5,    /* RPO */
            10,    /* POP H */
            10,    /* JPO adr */
            18,    /* XTHL */
            11,    /* CPO adr */
            11,    /* PUSH H */
             7,    /* ANI d8 */
            11,    /* RST 4 */
             5,    /* RPE */
             5,    /* PCHL */
            10,    /* JPE adr */
             4,    /* XCHG */
            11,    /* CPE adr */
             4,    /* *NOP* */
             7,    /* XRI d8 */
            11,    /* RST 5 */
             5,    /* RP */
            10,    /* POP PSW */
            10,    /* JP adr */
             4,    /* DI */
            11,    /* CP adr */
            11,    /* PUSH PSW */
             7,    /* ORI d8 */
            11,    /* RST 6 */
             5,    /* RM */
             5,    /* SPHL */
            10,    /* JM adr */
             4,    /* EI */
            11,    /* CM adr */
             4,    /* *NOP* */
             7,    /* CPI d8 */
            11    /* RST 7 */
    };

//

I8080CycleCount
I8080InstrHandlerMonolithic(
    I8080SystemPtr  sys8080,
    I8080Instr_t    opcode
)
{
    I8080CycleCount cycles = I8080CycleCountByOpcode[opcode];
    uint8_t         o8;
    uint16_t        o16;
    uint32_t        o32;
    
    #define O8()        (I8080InstrFetch(sys8080))
    #define O16()       (I8080InstrFetch(sys8080) | ((uint16_t)I8080InstrFetch(sys8080) << 8))
    #define RADDR(A)    (I8080MemRead(sys8080->sysmem, (A)))
    #define WADDR(A, B) (I8080MemWrite(sys8080->sysmem, (A), (B)))
    #define RM()        (I8080MemRead(sys8080->sysmem, sys8080->rgstrs.HL))
    #define WM(B)       (I8080MemWrite(sys8080->sysmem, sys8080->rgstrs.HL, (B)))
    #define PUSH(W)     (I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, (W) >> 8), \
                         I8080MemWrite(sys8080->sysmem, --sys8080->rgstrs.SP, (W) & 0xFF))
    #define POP()       (I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++) | \
                         ((uint16_t)I8080MemRead(sys8080->sysmem, sys8080->rgstrs.SP++) << 8))
    
    DEBUG("$%02hhX => %s", opcode, I8080InstrSummaryTable[opcode].symbolic);
    
    switch ( opcode ) {
    
#pragma mark - Jump
        case 0xC3:  /* JMP */
            sys8080->rgstrs.PC = O16();
            break;
        case 0xC2:  /* JNZ */
            o16 = O16(); if ( ! sys8080->rgstrs.Z ) sys8080->rgstrs.PC = o16;
            break;
        case 0xCA:  /* JZ */
            o16 = O16(); if ( sys8080->rgstrs.Z ) sys8080->rgstrs.PC = o16;
            break;
        case 0xD2:  /* JNC */
            o16 = O16(); if ( ! sys8080->rgstrs.CY ) sys8080->rgstrs.PC = o16;
            break;
        case 0xDA:  /* JC */
            o16 = O16(); if ( sys8080->rgstrs.CY ) sys8080->rgstrs.PC = o16;
            break;
        case 0xE2:  /* JPO */
            o16 = O16(); if ( ! sys8080->rgstrs.P ) sys8080->rgstrs.PC = o16;
            break;
        case 0xEA:  /* JPE */
            o16 = O16(); if ( sys8080->rgstrs.P ) sys8080->rgstrs.PC = o16;
            break;
        case 0xF2:  /* JP */
            o16 = O16(); if ( ! sys8080->rgstrs.S ) sys8080->rgstrs.PC = o16;
            break;
        case 0xFA:  /* JM */
            o16 = O16(); if ( sys8080->rgstrs.S ) sys8080->rgstrs.PC = o16;
            break;
        case 0xE9:  /* PCHL */
            sys8080->rgstrs.PC = sys8080->rgstrs.HL;
            break;
    
#pragma mark - Call
        case 0xCD:  /* CALL */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = O16();
            break;
        case 0xC4:  /* CNZ */
            o16 = O16(); if ( ! sys8080->rgstrs.Z ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
        case 0xCC:  /* CZ */
            o16 = O16(); if ( sys8080->rgstrs.Z ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
        case 0xD4:  /* CNC */
            o16 = O16(); if ( ! sys8080->rgstrs.CY ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
        case 0xDC:  /* CC */
            o16 = O16(); if ( sys8080->rgstrs.CY ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
        case 0xE4:  /* CPO */
            o16 = O16(); if ( ! sys8080->rgstrs.P ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
        case 0xEC:  /* CPE */
            o16 = O16(); if ( sys8080->rgstrs.P ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
        case 0xF4:  /* CP */
            o16 = O16(); if ( ! sys8080->rgstrs.S ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
        case 0xFC:  /* CM */
            o16 = O16(); if ( sys8080->rgstrs.S ) cycles += 6, PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = o16;
            break;
    
#pragma mark - Return
        case 0xC9:  /* RET */
            sys8080->rgstrs.PC = POP();
            break;
        case 0xC0:  /* RNZ */
            if ( ! sys8080->rgstrs.Z ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;
        case 0xC8:  /* RZ */
            if ( sys8080->rgstrs.Z ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;
        case 0xD0:  /* RNC */
            if ( ! sys8080->rgstrs.CY ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;
        case 0xD8:  /* RC */
            if ( sys8080->rgstrs.CY ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;
        case 0xE0:  /* RPO */
            if ( ! sys8080->rgstrs.P ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;
        case 0xE8:  /* RPE */
            if ( sys8080->rgstrs.P ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;
        case 0xF0:  /* RP */
            if ( ! sys8080->rgstrs.S ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;
        case 0xF8:  /* RM */
            if ( sys8080->rgstrs.S ) cycles += 6, sys8080->rgstrs.PC = POP();
            break;

#pragma mark - Move, immediate
        case 0x06:  /* MVI B, D8 */
            sys8080->rgstrs.B = O8();
            break;
        case 0x0E:  /* MVI C, D8 */
            sys8080->rgstrs.C = O8();
            break;
        case 0x16:  /* MVI D, D8 */
            sys8080->rgstrs.D = O8();
            break;
        case 0x1E:  /* MVI E, D8 */
            sys8080->rgstrs.E = O8();
            break;
        case 0x26:  /* MVI H, D8 */
            sys8080->rgstrs.H = O8();
            break;
        case 0x2E:  /* MVI L, D8 */
            sys8080->rgstrs.L = O8();
            break;
        case 0x36:  /* MVI M, D8 */
            WM(O8());
            break;
        case 0x3E:  /* MVI A, D8 */
            sys8080->rgstrs.A = O8();
            break;

#pragma mark - Accumulator, immediate
        case 0xC6:  /* ADI D8 */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)O8();
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0xCE:  /* ACI D8 */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)O8() + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0xD6:  /* SUI D8 */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~O8() + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xDE:  /* SBI D8 */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(O8() + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xE6:  /* ANI D8 */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & O8();
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o8 & 0b100000000) ? 1 : 0;
            break;
        case 0xEE:  /* XRI D8 */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ O8();
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o8 & 0b100000000) ? 1 : 0;
            break;
        case 0xF6:  /* ORI D8 */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | O8();
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o8 & 0b100000000) ? 1 : 0;
            break;
        case 0xFE:  /* CPI D8 */
            o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~O8() + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;

#pragma mark - Load, immediate
        case 0x01:  /* LXI BC, D16 */
            sys8080->rgstrs.BC = O16();
            break;
        case 0x11:  /* LXI DE, D16 */
            sys8080->rgstrs.DE = O16();
            break;
        case 0x21:  /* LXI HL, D16 */
            sys8080->rgstrs.HL = O16();
            break;
        case 0x31:  /* LXI SP, D16 */
            sys8080->rgstrs.SP = O16();
            break;

#pragma mark - Double add
        case 0x09:  /* DAD BC */
            o32 = (uint32_t)sys8080->rgstrs.HL + (uint32_t)sys8080->rgstrs.BC;
            sys8080->rgstrs.HL = o32 & 0xFFFF;
            sys8080->rgstrs.CY = (o32 & 0xFFFF0000) ? 1 : 0;
            break;
        case 0x19:  /* DAD DE */
            o32 = (uint32_t)sys8080->rgstrs.HL + (uint32_t)sys8080->rgstrs.DE;
            sys8080->rgstrs.HL = o32 & 0xFFFF;
            sys8080->rgstrs.CY = (o32 & 0xFFFF0000) ? 1 : 0;
            break;
        case 0x29:  /* DAD HL */
            o32 = (uint32_t)sys8080->rgstrs.HL + (uint32_t)sys8080->rgstrs.HL;
            sys8080->rgstrs.HL = o32 & 0xFFFF;
            sys8080->rgstrs.CY = (o32 & 0xFFFF0000) ? 1 : 0;
            break;
        case 0x39:  /* DAD SP */
            o32 = (uint32_t)sys8080->rgstrs.HL + (uint32_t)sys8080->rgstrs.SP;
            sys8080->rgstrs.HL = o32 & 0xFFFF;
            sys8080->rgstrs.CY = (o32 & 0xFFFF0000) ? 1 : 0;
            break;

#pragma mark - Increment
        case 0x04:  /* INR  B */
            o8 = ++sys8080->rgstrs.B;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x0C:  /* INR  C */
            o8 = ++sys8080->rgstrs.C;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x14:  /* INR  D */
            o8 = ++sys8080->rgstrs.D;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x1C:  /* INR  E */
            o8 = ++sys8080->rgstrs.E;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x24:  /* INR  H */
            o8 = ++sys8080->rgstrs.H;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x2C:  /* INR  L */
            o8 = ++sys8080->rgstrs.L;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x34:  /* INR  M */
            o8 = RM() + 1, WM(o8);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x3C:  /* INR  A */
            o8 = ++sys8080->rgstrs.A;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        
        case 0x03:  /* INX BC */
            sys8080->rgstrs.BC++;
            break;
        case 0x13:  /* INX DE */
            sys8080->rgstrs.DE++;
            break;
        case 0x23:  /* INX HL */
            sys8080->rgstrs.HL++;
            break;
        case 0x33:  /* INX SP */
            sys8080->rgstrs.SP++;
            break;

#pragma mark - Decrement
        case 0x05:  /* DCR  B */
            o8 = --sys8080->rgstrs.B;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x0D:  /* DCR  C */
            o8 = --sys8080->rgstrs.C;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x15:  /* DCR  D */
            o8 = --sys8080->rgstrs.D;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x1D:  /* DCR  E */
            o8 = --sys8080->rgstrs.E;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x25:  /* DCR  H */
            o8 = --sys8080->rgstrs.H;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x2D:  /* DCR  L */
            o8 = --sys8080->rgstrs.L;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x35:  /* DCR  M */
            o8 = RM() - 1, WM(o8);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        case 0x3D:  /* DCR  A */
            o8 = --sys8080->rgstrs.A;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0;
            break;
        
        case 0x0B:  /* DCX BC */
            sys8080->rgstrs.BC--;
            break;
        case 0x1B:  /* DCX DE */
            sys8080->rgstrs.DE--;
            break;
        case 0x2B:  /* DCX HL */
            sys8080->rgstrs.HL--;
            break;
        case 0x3B:  /* DCX SP */
            sys8080->rgstrs.SP--;
            break;

#pragma mark - Load/store
        case 0x0A:  /* LDAX BC */
            sys8080->rgstrs.A = RADDR(sys8080->rgstrs.BC);
            break;
        case 0x1A:  /* LDAX DE */
            sys8080->rgstrs.A = RADDR(sys8080->rgstrs.DE);
            break;
        case 0x2A:  /* LHLD <addr> */
            sys8080->rgstrs.HL = RADDR(O16());
            break;
        case 0x3A:  /* LDA <addr> */
            sys8080->rgstrs.A = RADDR(O16());
            break;

        case 0x02:  /* STAX BC */
            WADDR(sys8080->rgstrs.BC, sys8080->rgstrs.A);
            break;
        case 0x12:  /* STAX DE */
            WADDR(sys8080->rgstrs.DE, sys8080->rgstrs.A);
            break;
        case 0x22:  /* SHLD <addr> */
            WADDR(O16(), sys8080->rgstrs.HL);
            break;
        case 0x32:  /* STA <addr> */
            WADDR(O16(), sys8080->rgstrs.A);
            break;

#pragma mark - Restart
        case 0xC7:  /* RST 0 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0000;
            break;
        case 0xCF:  /* RST 1 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0008;
            break;
        case 0xD7:  /* RST 2 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0010;
            break;
        case 0xDF:  /* RST 3 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0018;
            break;
        case 0xE7:  /* RST 4 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0020;
            break;
        case 0xEF:  /* RST 5 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0028;
            break;
        case 0xF7:  /* RST 6 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0030;
            break;
        case 0xFF:  /* RST 7 */
            PUSH(sys8080->rgstrs.PC), sys8080->rgstrs.PC = 0x0038;
            break;

#pragma mark - Rotate
        case 0x07:  /* RLC */
            sys8080->rgstrs.CY = (sys8080->rgstrs.A & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.A = (sys8080->rgstrs.A << 1) | sys8080->rgstrs.CY;
            break;
        case 0x0F:  /* RRC */
            sys8080->rgstrs.CY = (sys8080->rgstrs.A & 0b00000001),
            sys8080->rgstrs.A = (sys8080->rgstrs.A >> 1) | (sys8080->rgstrs.CY << 7);
            break;
        case 0x17:  /* RAL */
            sys8080->rgstrs.A = o16 = ((uint16_t)sys8080->rgstrs.A << 1) | sys8080->rgstrs.CY,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x1F:  /* RAR */
            sys8080->rgstrs.A = ((o8 = sys8080->rgstrs.A) >> 1) | (sys8080->rgstrs.CY << 7),
            sys8080->rgstrs.CY = (o8 & 0b00000001);
            break;
            
#pragma mark - Control
        case 0x00:  /* NOP */
            break;
        case 0x76:  /* HLT */
            sys8080->state &= ~kI8080SystemStateRunning;
            break;
        case 0xF3:  /* DI */
            sys8080->rgstrs.INTE = 0b0;
            break;
        case 0xFB:  /* EI */
            sys8080->rgstrs.INTE = 0b1;
            break;
            
#pragma mark - Stack ops
        case 0xC5:  /* PUSH BC */
            PUSH(sys8080->rgstrs.BC);
            break;
        case 0xD5:  /* PUSH DE */
            PUSH(sys8080->rgstrs.DE);
            break;
        case 0xE5:  /* PUSH HL */
            PUSH(sys8080->rgstrs.HL);
            break;
        case 0xF5:  /* PUSH PSW */
            PUSH(sys8080->rgstrs.PSW);
            break;
        
        case 0xC1:  /* POP BC */
            sys8080->rgstrs.BC = POP();
            break;
        case 0xD1:  /* POP DE */
            sys8080->rgstrs.DE = POP();
            break;
        case 0xE1:  /* POP HL */
            sys8080->rgstrs.HL = POP();
            break;
        case 0xF1:  /* POP PSW */
            sys8080->rgstrs.PSW = POP();
            break;
        
        case 0xE3:  /* XTHL */
            o16 = POP(), PUSH(sys8080->rgstrs.HL), sys8080->rgstrs.HL = o16;
            break;
        case 0xF9:  /* SPHL */
            sys8080->rgstrs.SP = sys8080->rgstrs.HL;
            break;

#pragma mark - Specials
        case 0xEB:  /* XCHG */
            o16 = sys8080->rgstrs.HL,
            sys8080->rgstrs.HL = sys8080->rgstrs.DE,
            sys8080->rgstrs.DE = o16;
            break;
        case 0x27:  /* DAA */
            o16 = sys8080->rgstrs.A;
            if ( (o16 & 0xF) > 9 ) o16 += 6;
            if ( (o16 & 0xF0) > (9 << 4) ) o16 += (6 << 4);
            sys8080->rgstrs.CY = (o16 & 0xFF00) ? 0b1 : 0b0;
            sys8080->rgstrs.A = o16 & 0xFF;
            break;
        case 0x2F:  /* CMA */
            sys8080->rgstrs.A ^= 0b11111111;
            break;
        case 0x37:  /* STC */
            sys8080->rgstrs.CY = 0b1;
            break;
        case 0x3F:  /* CMC */
            sys8080->rgstrs.CY ^= 0b1;
            break;

#pragma mark - Move
        case 0x40:  /* MOV B, B */
            break;
        case 0x41:  /* MOV B, C */
            sys8080->rgstrs.B = sys8080->rgstrs.C;
            break;
        case 0x42:  /* MOV B, D */
            sys8080->rgstrs.B = sys8080->rgstrs.D;
            break;
        case 0x43:  /* MOV B, E */
            sys8080->rgstrs.B = sys8080->rgstrs.E;
            break;
        case 0x44:  /* MOV B, H */
            sys8080->rgstrs.B = sys8080->rgstrs.H;
            break;
        case 0x45:  /* MOV B, L */
            sys8080->rgstrs.B = sys8080->rgstrs.L;
            break;
        case 0x46:  /* MOV B, M */
            sys8080->rgstrs.B = RM();
            break;
        case 0x47:  /* MOV B, A */
            sys8080->rgstrs.B = sys8080->rgstrs.A;
            break;
        
        case 0x48:  /* MOV C, B */
            sys8080->rgstrs.C = sys8080->rgstrs.B;
            break;
        case 0x49:  /* MOV C, C */
            break;
        case 0x4A:  /* MOV C, D */
            sys8080->rgstrs.C = sys8080->rgstrs.D;
            break;
        case 0x4B:  /* MOV C, E */
            sys8080->rgstrs.C = sys8080->rgstrs.E;
            break;
        case 0x4C:  /* MOV C, H */
            sys8080->rgstrs.C = sys8080->rgstrs.H;
            break;
        case 0x4D:  /* MOV C, L */
            sys8080->rgstrs.C = sys8080->rgstrs.L;
            break;
        case 0x4E:  /* MOV C, M */
            sys8080->rgstrs.C = RM();
            break;
        case 0x4F:  /* MOV C, A */
            sys8080->rgstrs.C = sys8080->rgstrs.A;
            break;
            
        case 0x50:  /* MOV D, B */
            sys8080->rgstrs.D = sys8080->rgstrs.B;
            break;
        case 0x51:  /* MOV D, C */
            sys8080->rgstrs.D = sys8080->rgstrs.C;
            break;
        case 0x52:  /* MOV D, D */
            break;
        case 0x53:  /* MOV D, E */
            sys8080->rgstrs.D = sys8080->rgstrs.E;
            break;
        case 0x54:  /* MOV D, H */
            sys8080->rgstrs.D = sys8080->rgstrs.H;
            break;
        case 0x55:  /* MOV D, L */
            sys8080->rgstrs.D = sys8080->rgstrs.L;
            break;
        case 0x56:  /* MOV D, M */
            sys8080->rgstrs.D = RM();
            break;
        case 0x57:  /* MOV D, A */
            sys8080->rgstrs.D = sys8080->rgstrs.A;
            break;
        
        case 0x58:  /* MOV E, B */
            sys8080->rgstrs.E = sys8080->rgstrs.B;
            break;
        case 0x59:  /* MOV E, C */
            sys8080->rgstrs.E = sys8080->rgstrs.C;
            break;
        case 0x5A:  /* MOV E, D */
            sys8080->rgstrs.E = sys8080->rgstrs.D;
            break;
        case 0x5B:  /* MOV E, E */
            break;
        case 0x5C:  /* MOV E, H */
            sys8080->rgstrs.E = sys8080->rgstrs.H;
            break;
        case 0x5D:  /* MOV E, L */
            sys8080->rgstrs.E = sys8080->rgstrs.L;
            break;
        case 0x5E:  /* MOV E, M */
            sys8080->rgstrs.E = RM();
            break;
        case 0x5F:  /* MOV E, A */
            sys8080->rgstrs.E = sys8080->rgstrs.A;
            break;

        case 0x60:  /* MOV H, B */
            sys8080->rgstrs.H = sys8080->rgstrs.B;
            break;
        case 0x61:  /* MOV H, C */
            sys8080->rgstrs.H = sys8080->rgstrs.C;
            break;
        case 0x62:  /* MOV H, D */
            sys8080->rgstrs.H = sys8080->rgstrs.D;
            break;
        case 0x63:  /* MOV H, E */
            sys8080->rgstrs.H = sys8080->rgstrs.E;
            break;
        case 0x64:  /* MOV H, H */
            break;
        case 0x65:  /* MOV H, L */
            sys8080->rgstrs.H = sys8080->rgstrs.L;
            break;
        case 0x66:  /* MOV H, M */
            sys8080->rgstrs.H = RM();
            break;
        case 0x67:  /* MOV H, A */
            sys8080->rgstrs.H = sys8080->rgstrs.A;
            break;
        
        case 0x68:  /* MOV L, B */
            sys8080->rgstrs.L = sys8080->rgstrs.B;
            break;
        case 0x69:  /* MOV L, C */
            sys8080->rgstrs.L = sys8080->rgstrs.C;
            break;
        case 0x6A:  /* MOV L, D */
            sys8080->rgstrs.L = sys8080->rgstrs.D;
            break;
        case 0x6B:  /* MOV L, E */
            sys8080->rgstrs.L = sys8080->rgstrs.E;
            break;
        case 0x6C:  /* MOV L, H */
            sys8080->rgstrs.L = sys8080->rgstrs.H;
            break;
        case 0x6D:  /* MOV L, L */
            break;
        case 0x6E:  /* MOV L, M */
            sys8080->rgstrs.L = RM();
            break;
        case 0x6F:  /* MOV L, A */
            sys8080->rgstrs.L = sys8080->rgstrs.A;
            break;

        case 0x70:  /* MOV M, B */
            WM(sys8080->rgstrs.B);
            break;
        case 0x71:  /* MOV M, C */
            WM(sys8080->rgstrs.C);
            break;
        case 0x72:  /* MOV M, D */
            WM(sys8080->rgstrs.D);
            break;
        case 0x73:  /* MOV M, E */
            WM(sys8080->rgstrs.E);
            break;
        case 0x74:  /* MOV M, H */
            WM(sys8080->rgstrs.H);
            break;
        case 0x75:  /* MOV M, L */
            WM(sys8080->rgstrs.L);
            break;
        //  case 0x76:  /* MOV M, M */
        //    break;
        case 0x77:  /* MOV M, A */
            WM(sys8080->rgstrs.A);
            break;
        
        case 0x78:  /* MOV A, B */
            sys8080->rgstrs.A = sys8080->rgstrs.B;
            break;
        case 0x79:  /* MOV A, C */
            sys8080->rgstrs.A = sys8080->rgstrs.C;
            break;
        case 0x7A:  /* MOV A, D */
            sys8080->rgstrs.A = sys8080->rgstrs.D;
            break;
        case 0x7B:  /* MOV A, E */
            sys8080->rgstrs.A = sys8080->rgstrs.E;
            break;
        case 0x7C:  /* MOV A, H */
            sys8080->rgstrs.A = sys8080->rgstrs.H;
            break;
        case 0x7D:  /* MOV A, L */
            sys8080->rgstrs.A = sys8080->rgstrs.L;
            break;
        case 0x7E:  /* MOV A, M */
            sys8080->rgstrs.A = RM();
            break;
        case 0x7F:  /* MOV A, A */
            break;

#pragma mark - Accumulator
        case 0x80:  /* ADD B */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.B;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x81:  /* ADD C */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.C;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x82:  /* ADD D */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.D;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x83:  /* ADD E */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.E;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x84:  /* ADD H */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.H;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x85:  /* ADD L */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.L;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x86:  /* ADD M */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)RM();
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x87:  /* ADD A */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.A;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;

        case 0x88:  /* ADC B */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.B + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x89:  /* ADC C */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.C + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x8A:  /* ADC D */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.D + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x8B:  /* ADC E */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.E + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x8C:  /* ADC H */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.H + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x8D:  /* ADC L */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.L + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x8E:  /* ADC M */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)RM() + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
        case 0x8F:  /* ADC A */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)sys8080->rgstrs.A + sys8080->rgstrs.CY;
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 1 : 0;
            break;
            
        case 0x90:  /* SUB B */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.B + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x91:  /* SUB C */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.C + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x92:  /* SUB D */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.D + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x93:  /* SUB E */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.E + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x94:  /* SUB H */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.H + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x95:  /* SUB L */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.L + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x96:  /* SUB M */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~RM() + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x97:  /* SUB A */
            sys8080->rgstrs.A = 0;
            sys8080->rgstrs.Z = 1, sys8080->rgstrs.S = 0,
            sys8080->rgstrs.CY = 0;
            break;

        case 0x98:  /* SBB B */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(sys8080->rgstrs.B + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x99:  /* SBB C */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(sys8080->rgstrs.C + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x9A:  /* SBB D */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(sys8080->rgstrs.D + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x9B:  /* SBB E */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(sys8080->rgstrs.E + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x9C:  /* SBB H */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(sys8080->rgstrs.H + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x9D:  /* SBB L */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(sys8080->rgstrs.L + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x9E:  /* SBB M */
            sys8080->rgstrs.A = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~(RM() + sys8080->rgstrs.CY) + 1);
            sys8080->rgstrs.Z = sys8080->rgstrs.A ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0x9F:  /* SBB A */
            /*
             *     0b00001111 + (~(0b00001111 + 0) + 1)
             *                + (0b11110001)
             *     0b11110001
             *     0b00001111
             *   1 0b00000000
             *     
             *          => CY not set, A = 0, CY = ~1 = 0
             *     
             *     
             *     0b00001111 + (~(0b00001111 + 1) + 1)
             *                + (~(0b00010000) + 1)
             *                + (0b11101111 + 1)
             *                = (0b11110000)
             *     0b00001111
             *     0b11110000
             *   0 0b11111111
             *     
             *          => CY set, A = -1, CY = ~0 = 1
             *
             *  State of CY does not change!
             */
            sys8080->rgstrs.A = sys8080->rgstrs.CY ? 0b11111111 : 0b00000000;
            sys8080->rgstrs.Z = sys8080->rgstrs.CY ? 0 : 1, sys8080->rgstrs.S = sys8080->rgstrs.CY;
            break;
            
        case 0xA0:  /* ANA B */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & sys8080->rgstrs.B;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA1:  /* ANA C */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & sys8080->rgstrs.C;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA2:  /* ANA D */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & sys8080->rgstrs.D;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA3:  /* ANA E */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & sys8080->rgstrs.E;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA4:  /* ANA H */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & sys8080->rgstrs.H;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA5:  /* ANA L */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & sys8080->rgstrs.L;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA6:  /* ANA M */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & RM();
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA7:  /* ANA A */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A & sys8080->rgstrs.A;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
            
        case 0xA8:  /* XRA B */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ sys8080->rgstrs.B;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xA9:  /* XRA C */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ sys8080->rgstrs.C;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xAA:  /* XRA D */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ sys8080->rgstrs.D;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xAB:  /* XRA E */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ sys8080->rgstrs.E;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xAC:  /* XRA H */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ sys8080->rgstrs.H;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xAD:  /* XRA L */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ sys8080->rgstrs.L;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xAE:  /* XRA M */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ RM();
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xAF:  /* XRA A */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A ^ sys8080->rgstrs.A;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
            
        case 0xB0:  /* ORA B */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | sys8080->rgstrs.B;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xB1:  /* ORA C */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | sys8080->rgstrs.C;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xB2:  /* ORA D */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | sys8080->rgstrs.D;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xB3:  /* ORA E */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | sys8080->rgstrs.E;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xB4:  /* ORA H */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | sys8080->rgstrs.H;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xB5:  /* ORA L */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | sys8080->rgstrs.L;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xB6:  /* ORA M */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | RM();
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
        case 0xB7:  /* ORA A */
            sys8080->rgstrs.A = o8 = sys8080->rgstrs.A | sys8080->rgstrs.A;
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o8 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = 0b0;
            break;
            
        case 0xB8:  /* CMP B */
            o8 = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.B + 1);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xB9:  /* CMP C */
            o8 = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.C + 1);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xBA:  /* CMP D */
            o8 = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.D + 1);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xBB:  /* CMP E */
            o8 = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.E + 1);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xBC:  /* CMP H */
            o8 = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.H + 1);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xBD:  /* CMP L */
            o8 = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~sys8080->rgstrs.L + 1);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xBE:  /* CMP M */
            o8 = o16 = (uint16_t)sys8080->rgstrs.A + (uint16_t)(~RM() + 1);
            sys8080->rgstrs.Z = o8 ? 0 : 1, sys8080->rgstrs.S = (o16 & 0b10000000) ? 1 : 0,
            sys8080->rgstrs.CY = (o16 & 0b100000000) ? 0 : 1;
            break;
        case 0xBF:  /* CMP A */
            sys8080->rgstrs.Z = 1, sys8080->rgstrs.S = 0,
            sys8080->rgstrs.CY = 0;
            break;
    }
    return cycles;
    
    #undef O8
    #undef O16
    #undef RADDR
    #undef WADDR
    #undef RM
    #undef WM
    #undef PUSH
    #undef POP
}

//
#pragma mark -
//

static
I8080SystemPtr
I8080SystemAlloc(
    size_t          additional_bytes
)
{
    I8080System_t   *sys8080 = (I8080System_t*)calloc(1, additional_bytes + sizeof(I8080System_t));
    
    if ( sys8080 ) {
        if ( additional_bytes ) {
            sys8080->aux_data = (void*)sys8080 + sizeof(I8080System_t);
        } else {
            sys8080->aux_data = NULL;
        }
    }
    return sys8080;
}

//

I8080SystemPtr
I8080SystemCreate(
    I8080SystemOpts_t       options,
    size_t                  additional_bytes
)
{
    I8080System_t   *sys8080 = I8080SystemAlloc(additional_bytes);
    
    if ( sys8080 ) {
        sys8080->options = options & kI8080SystemOptsAll;
        sys8080->rgstrs = I8080Registers();
        sys8080->sysmem = I8080MemCreate();
        sys8080->devbus = I8080DevBusCreate();
        
        if ( options & kI8080SystemOptsAccelInstr ) {
            int         i;
            for ( i = 0; i < 256; i++ ) {
                sys8080->itbl.handler_names[i] = "monolithic-function";
                sys8080->itbl.dispatchers[i] = I8080InstrHandlerMonolithic;
            }
            sys8080->itbl.n_instructions = 256;
        } else {
            I8080InstrTableInit(&sys8080->itbl, I8080DefaultISA);
        }
        
        sys8080->tty = *I8080DeviceTTY;
        I8080DevBusRegisterDevice(sys8080->devbus, 0, &sys8080->tty, NULL);
        
        sys8080->state = kI8080SystemStateOff;
        
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
        pthread_mutex_init(&sys8080->interrupt.lock, NULL);
#endif
        
        DEBUG("Created system %p", sys8080);
    }
    return sys8080;
}

//

I8080SystemPtr
I8080SystemCreateWithTTYContext(
    I8080SystemOpts_t       options,
    size_t                  additional_bytes,
    I8080DeviceTTYContext_t *tty_context
)
{
    I8080System_t   *sys8080 = I8080SystemAlloc(additional_bytes + sizeof(I8080DeviceTTYContext_t));
    
    if ( sys8080 ) {
        I8080DeviceTTYContext_t     *built_in_context;
        
        sys8080->options = options & kI8080SystemOptsAll;
        sys8080->rgstrs = I8080Registers();
        sys8080->sysmem = I8080MemCreate();
        sys8080->devbus = I8080DevBusCreate();
        
        if ( options & kI8080SystemOptsAccelInstr ) {
            int         i;
            for ( i = 0; i < 256; i++ ) {
                sys8080->itbl.handler_names[i] = "monolithic-function";
                sys8080->itbl.dispatchers[i] = I8080InstrHandlerMonolithic;
            }
            sys8080->itbl.n_instructions = 256;
        } else {
            I8080InstrTableInit(&sys8080->itbl, I8080DefaultISA);
        }
        
        sys8080->tty = *I8080DeviceTTY;
        built_in_context = (I8080DeviceTTYContext_t*)sys8080->aux_data;
        sys8080->aux_data += sizeof(I8080DeviceTTYContext_t);
        *built_in_context = *tty_context;
        I8080DevBusRegisterDevice(sys8080->devbus, 0, &sys8080->tty, built_in_context);
        
        sys8080->state = kI8080SystemStateOff;
        
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
        pthread_mutex_init(&sys8080->interrupt.lock, NULL);
#endif
        
        DEBUG("Created system %p with TTY context", sys8080);
    }
    return sys8080;
}

//

void
I8080SystemDestroy(
    I8080SystemPtr  sys8080
)
{
    I8080DevBusDestroy(sys8080->devbus);
    I8080MemDestroy(sys8080->sysmem);
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    pthread_mutex_destroy(&sys8080->interrupt.lock);
#endif
    free((void*)sys8080);
    DEBUG("Destroyed system %p", sys8080);
}

//

void
__I8080SystemReset(
    I8080SystemPtr  sys8080
)
{
    sys8080->rgstrs = I8080Registers();
    I8080MemReset(sys8080->sysmem);
    I8080DevBusReset(sys8080->devbus);
    sys8080->state = kI8080SystemStateOn;
    INFO("System %p reset and ready", sys8080);
}

void
I8080SystemReset(
    I8080SystemPtr  sys8080
)
{
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    pthread_mutex_lock(&sys8080->interrupt.lock);
#endif
    __I8080SystemReset(sys8080);
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    sys8080->interrupt.is_raised = false;
    pthread_mutex_unlock(&sys8080->interrupt.lock);
#endif
}

//

bool
I8080SystemSetPowerState(
    I8080SystemPtr  sys8080,
    bool            is_on
)
{
    bool            ok = false;
    
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    pthread_mutex_lock(&sys8080->interrupt.lock);
#endif
    if ( is_on ) {
        if ( ! (sys8080->state & kI8080SystemStateOn) ) {
            __I8080SystemReset(sys8080);
            ok = true;
        }
    } else {
        if ( sys8080->state != kI8080SystemStateOff ) {
            I8080DevBusShutdown(sys8080->devbus);
            sys8080->state = kI8080SystemStateOff;
            INFO("System %p shutdown and off", sys8080);
            ok = true;
        }
    }
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    sys8080->interrupt.is_raised = false;
    pthread_mutex_unlock(&sys8080->interrupt.lock);
#endif
    return ok;
}

//

void
I8080SystemBreak(
    I8080SystemPtr  sys8080
)
{
    if ( I8080SystemIsReady(sys8080->state) ) {
        sys8080->state = (sys8080->state & ~kI8080SystemStateRunning) | kI8080SystemStateBreak;
    }
}

//

void
I8080SystemRaiseInterrupt(
    I8080SystemPtr  sys8080,
    I8080Instr_t    interrupt_opcode
)
{
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    if ( I8080SystemIsRunning(sys8080->state) ) {
        pthread_mutex_lock(&sys8080->interrupt.lock);
        if ( sys8080->rgstrs.INTE ) {
            sys8080->interrupt.is_raised = true;
            sys8080->interrupt.opcode = interrupt_opcode;
            sys8080->rgstrs.INTE = 0;
        }
        pthread_mutex_unlock(&sys8080->interrupt.lock);
    }
#endif
}

//

void
I8080SystemSetStall(
    I8080SystemPtr  sys8080,
    bool            should_stall
)
{
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    pthread_mutex_lock(&sys8080->interrupt.lock);
#endif
    if ( I8080SystemIsRunning(sys8080->state) ) {
        if ( should_stall ) sys8080->state |= kI8080SystemStateStall;
        else                sys8080->state &= ~kI8080SystemStateStall;
    }
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
    pthread_mutex_unlock(&sys8080->interrupt.lock);
#endif
}

//

bool
I8080SystemSetPC(
    I8080SystemPtr  sys8080,
    I8080Addr_t     origin
)
{
    if ( I8080SystemIsReady(sys8080->state) ) {
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
        pthread_mutex_lock(&sys8080->interrupt.lock);
#endif
        sys8080->rgstrs.PC = origin;
        DEBUG("Forcibly set the PC to $%04hX", origin);
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
        pthread_mutex_unlock(&sys8080->interrupt.lock);
#endif
        return true;
    }
    return false;
}

//

bool
I8080SystemStep(
    I8080SystemPtr  sys8080,
    I8080CycleCount *cycles
)
{
    bool            ok = false;
    
    if ( I8080SystemIsReady(sys8080->state) ) {
        I8080Instr_t    instr;
        I8080CycleCount elapsed = 0;
        
        if ( ! I8080SystemIsRunning(sys8080->state) ) {
            sys8080->state |= kI8080SystemStateRunning;
            sys8080->last_cycle = I8080MicrosecondsMakeNow();
            INFO("System transitioned to running state");
        }
        if ( (sys8080->state & kI8080SystemStateStall) == 0 ) {
#ifdef I8080_SYSTEM_ENABLE_INTERRUPT_API
            pthread_mutex_lock(&sys8080->interrupt.lock);
            
            if ( sys8080->interrupt.is_raised ) {
                instr = sys8080->interrupt.opcode;
                DEBUG("Interrupt instruction: 0x%02hhX", instr);
                sys8080->interrupt.is_raised = false;
            } else {
                instr = I8080InstrFetch(sys8080);
                DEBUG("Fetched instruction: 0x%02hhX", instr);
            }
            
            pthread_mutex_unlock(&sys8080->interrupt.lock);
#else
            instr = I8080InstrFetch(sys8080);
            DEBUG("Fetched instruction: 0x%02hhX", instr);
#endif

            if ( sys8080->itbl.dispatchers[instr] ) {
                elapsed = sys8080->itbl.dispatchers[instr](sys8080, instr);
                sys8080->rgstrs.CYCLS += elapsed;
                
                if ( sys8080->options & kI8080SystemOpts2MHzClock ) {
                    I8080Microseconds   now = I8080MicrosecondsMakeNow(),
                                        dt = (sys8080->last_cycle + (double)elapsed * 0.5) - now;
                    
                    if ( dt > 0.0 ) {
                        DEBUG("Sleeping for %.3f µs to fix clockspeed", dt);
                        I8080TimingSleep(dt);
                        now += dt;
                    }
                    sys8080->last_cycle = now;
                }
                ok = true;
            } else {
                // Stop running the program and indicate an invalid opcode was encountered:
                sys8080->state = kI8080SystemStateOn | kI8080SystemStateBadInstr;
                ERROR("An illegal instruction (0x%02hhX) was encountered", instr);
            }
        } else {
            if ( sys8080->options & kI8080SystemOpts2MHzClock ) {
                I8080Microseconds   now = I8080MicrosecondsMakeNow(),
                                    dt = (sys8080->last_cycle + (double)elapsed * 0.5) - now;
                
                if ( dt > 0.0 ) {
                    DEBUG("Sleeping for %.3f µs to fix clockspeed", dt);
                    I8080TimingSleep(dt);
                    now += dt;
                }
                sys8080->last_cycle = now;
            }
        }
        if ( cycles ) *cycles = elapsed;
    }
    return ok;
}

//

void
I8080SystemRun(
    I8080SystemPtr  sys8080,
    I8080Addr_t     origin
)
{
    if ( I8080SystemSetPC(sys8080, origin) ) {
        do {
            if ( ! I8080SystemStep(sys8080, NULL) ) break;
        } while ( I8080SystemIsRunning(sys8080->state) );
    }
}


