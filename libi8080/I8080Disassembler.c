/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines the I8080 disassembly API.
 *
 * C.f. https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf
 */

#include "I8080Disassembler.h"

#define PREFIX_INTE     "%23$s: %2$02hhX          [%22$2d]  "

#define PREFIX_1BYTE    "%23$s: %2$02hhX          [%22$2d]  "
#define PREFIX_2BYTE    "%23$s: %2$02hhX %3$02hhX       [%22$2d]  "
#define PREFIX_3BYTE    "%23$s: %2$02hhX %3$02hhX %4$02hhX    [%22$2d]  "

#define OPERAND_IMMED8  "#$%3$02hhX"
#define OPERAND_IMMED16 "#$%4$02hhX%3$02hhX"
#define OPERAND_ADDR    "$%4$02hhX%3$02hhX"

#define STATE_F         "[$%11$02hhX|%21$s]"
#define STATE_SP        "(SP=$%17$04hX)"

#define RESULT_B        " = $%5$02hhX"
#define RESULT_C        " = $%6$02hhX"
#define RESULT_D        " = $%7$02hhX"
#define RESULT_E        " = $%8$02hhX"
#define RESULT_H        " = $%9$02hhX"
#define RESULT_L        " = $%10$02hhX"
#define RESULT_F        " = " STATE_F
#define RESULT_A        " = $%12$02hhX"
#define RESULT_BC       " = $%13$04hX"
#define RESULT_DE       " = $%14$04hX"
#define RESULT_HL       " = $%15$04hX"
#define RESULT_PSW      " = $%16$04hX"
#define RESULT_SP       " = $%17$04hX"
#define RESULT_PC       " = $%18$04hX"
#define RESULT_INTE     " = $%19$02hhX"
#define RESULT_IND_M    " = $%20$02hhX"

/*
 * All printf() disassembly calls will use a common argument list:
 *
 *      1 16-bit address
 *      2 opcode
 *      3 operand 1
 *      4 operand 2
 *      5 B
 *      6 C
 *      7 D
 *      8 E
 *      9 H
 *     10 L
 *     11 F
 *     12 A
 *     13 BC
 *     14 DE
 *     15 HL
 *     16 PSW
 *     17 SP
 *     18 PC
 *     19 INTE
 *     20 (M)
 *     21 flags as string
 *     22 cycles
 *     23 formatted address
 */
const char* const I8080InstrDisassemblyFormat[256] = {
    PREFIX_1BYTE "NOP\n",
    PREFIX_3BYTE "LXI B, " OPERAND_IMMED16 RESULT_BC "\n",
    PREFIX_1BYTE "STAX B" RESULT_A "\n",
    PREFIX_1BYTE "INX B" RESULT_BC "\n",
    PREFIX_1BYTE "INR B" RESULT_B "\n",
    PREFIX_1BYTE "DCR B" RESULT_B "\n",
    PREFIX_2BYTE "MVI B, " OPERAND_IMMED8 RESULT_B "\n",
    PREFIX_1BYTE "RLC" RESULT_A "\n",
    PREFIX_1BYTE "NOP\n",
    PREFIX_1BYTE "DAD B" RESULT_HL "\n",
    PREFIX_1BYTE "LDAX B" RESULT_A "\n",
    PREFIX_1BYTE "DCX B" RESULT_BC "\n",
    PREFIX_1BYTE "INR C" RESULT_C "\n",
    PREFIX_1BYTE "DCR C" RESULT_C "\n",
    PREFIX_2BYTE "MVI C, " OPERAND_IMMED8 RESULT_C "\n",
    PREFIX_1BYTE "RRC" RESULT_A "\n",
    PREFIX_1BYTE "NOP\n",
    PREFIX_3BYTE "LXI D, " OPERAND_IMMED16 RESULT_DE "\n",
    PREFIX_1BYTE "STAX D" RESULT_A "\n",
    PREFIX_1BYTE "INX D" RESULT_DE "\n",
    PREFIX_1BYTE "INR D" RESULT_D "\n",
    PREFIX_1BYTE "DCR D" RESULT_D "\n",
    PREFIX_2BYTE "MVI D, " OPERAND_IMMED8 RESULT_D "\n",
    PREFIX_1BYTE "RAL" RESULT_A "\n",
    PREFIX_1BYTE "NOP\n",
    PREFIX_1BYTE "DAD D" RESULT_HL "\n",
    PREFIX_1BYTE "LDAX D" RESULT_A "\n",
    PREFIX_1BYTE "DCX D" RESULT_DE "\n",
    PREFIX_1BYTE "INR E" RESULT_E "\n",
    PREFIX_1BYTE "DCR E" RESULT_E "\n",
    PREFIX_2BYTE "MVI E, " OPERAND_IMMED8 RESULT_E "\n",
    PREFIX_1BYTE "RAR" RESULT_A "\n",
    PREFIX_1BYTE "NOP\n",
    PREFIX_3BYTE "LXI H, " OPERAND_IMMED16 RESULT_HL "\n",
    PREFIX_3BYTE "SHLD " OPERAND_ADDR RESULT_HL "\n",
    PREFIX_1BYTE "INX H" RESULT_HL "\n",
    PREFIX_1BYTE "INR H" RESULT_H "\n",
    PREFIX_1BYTE "DCR H" RESULT_H "\n",
    PREFIX_2BYTE "MVI H, " OPERAND_IMMED8 RESULT_H "\n",
    PREFIX_1BYTE "DAA" RESULT_A "\n",
    PREFIX_1BYTE "NOP\n",
    PREFIX_1BYTE "DAD H" RESULT_HL "\n",
    PREFIX_3BYTE "LHLD " OPERAND_ADDR RESULT_HL "\n",
    PREFIX_1BYTE "DCX H" RESULT_HL "\n",
    PREFIX_1BYTE "INR L" RESULT_L "\n",
    PREFIX_1BYTE "DCR L" RESULT_L "\n",
    PREFIX_2BYTE "MVI L, " OPERAND_IMMED8 RESULT_L "\n",
    PREFIX_1BYTE "CMA" RESULT_A "\n",
    PREFIX_1BYTE "NOP\n",
    PREFIX_3BYTE "LXI SP, " OPERAND_IMMED16 RESULT_SP "\n",
    PREFIX_1BYTE "STA " OPERAND_ADDR RESULT_A "\n",
    PREFIX_1BYTE "INX SP" RESULT_SP "\n",
    PREFIX_1BYTE "INR M ($%15$04hX)" RESULT_IND_M "\n",
    PREFIX_1BYTE "DCR M ($%15$04hX)" RESULT_IND_M "\n",
    PREFIX_1BYTE "MVI M ($%15$04hX), " OPERAND_IMMED8 RESULT_IND_M "\n",
    PREFIX_1BYTE "STC" RESULT_F "\n",
    PREFIX_1BYTE "NOP\n",
    PREFIX_1BYTE "DAD SP" RESULT_HL "\n",
    PREFIX_1BYTE "LDA " OPERAND_ADDR RESULT_A "\n",
    PREFIX_1BYTE "DCX SP" RESULT_SP "\n",
    PREFIX_1BYTE "INR A" RESULT_A "\n",
    PREFIX_1BYTE "DCR A" RESULT_A "\n",
    PREFIX_2BYTE "MVI A, " OPERAND_IMMED8 RESULT_A "\n",
    PREFIX_1BYTE "CMC" RESULT_F "\n",
    PREFIX_1BYTE "MOV B, B" RESULT_B "\n",
    PREFIX_1BYTE "MOV B, C" RESULT_B "\n",
    PREFIX_1BYTE "MOV B, D" RESULT_B "\n",
    PREFIX_1BYTE "MOV B, E" RESULT_B "\n",
    PREFIX_1BYTE "MOV B, H" RESULT_B "\n",
    PREFIX_1BYTE "MOV B, L" RESULT_B "\n",
    PREFIX_1BYTE "MOV B, M ($%15$04hX)" RESULT_B "\n",
    PREFIX_1BYTE "MOV B, A" RESULT_B "\n",
    PREFIX_1BYTE "MOV C, B" RESULT_C "\n",
    PREFIX_1BYTE "MOV C, C" RESULT_C "\n",
    PREFIX_1BYTE "MOV C, D" RESULT_C "\n",
    PREFIX_1BYTE "MOV C, E" RESULT_C "\n",
    PREFIX_1BYTE "MOV C, H" RESULT_C "\n",
    PREFIX_1BYTE "MOV C, L" RESULT_C "\n",
    PREFIX_1BYTE "MOV C, M ($%15$04hX)" RESULT_C "\n",
    PREFIX_1BYTE "MOV C, A" RESULT_C "\n",
    PREFIX_1BYTE "MOV D, B" RESULT_D "\n",
    PREFIX_1BYTE "MOV D, C" RESULT_D "\n",
    PREFIX_1BYTE "MOV D, D" RESULT_D "\n",
    PREFIX_1BYTE "MOV D, E" RESULT_D "\n",
    PREFIX_1BYTE "MOV D, H" RESULT_D "\n",
    PREFIX_1BYTE "MOV D, L" RESULT_D "\n",
    PREFIX_1BYTE "MOV D, M ($%15$04hX)" RESULT_D "\n",
    PREFIX_1BYTE "MOV D, A" RESULT_D "\n",
    PREFIX_1BYTE "MOV E, B" RESULT_E "\n",
    PREFIX_1BYTE "MOV E, C" RESULT_E "\n",
    PREFIX_1BYTE "MOV E, D" RESULT_E "\n",
    PREFIX_1BYTE "MOV E, E" RESULT_E "\n",
    PREFIX_1BYTE "MOV E, H" RESULT_E "\n",
    PREFIX_1BYTE "MOV E, L" RESULT_E "\n",
    PREFIX_1BYTE "MOV E, M ($%15$04hX)" RESULT_E "\n",
    PREFIX_1BYTE "MOV E, A" RESULT_E "\n",
    PREFIX_1BYTE "MOV H, B" RESULT_H "\n",
    PREFIX_1BYTE "MOV H, C" RESULT_H "\n",
    PREFIX_1BYTE "MOV H, D" RESULT_H "\n",
    PREFIX_1BYTE "MOV H, E" RESULT_H "\n",
    PREFIX_1BYTE "MOV H, H" RESULT_H "\n",
    PREFIX_1BYTE "MOV H, L" RESULT_H "\n",
    PREFIX_1BYTE "MOV H, M ($%15$04hX)" RESULT_H "\n",
    PREFIX_1BYTE "MOV H, A" RESULT_H "\n",
    PREFIX_1BYTE "MOV L, B" RESULT_L "\n",
    PREFIX_1BYTE "MOV L, C" RESULT_L "\n",
    PREFIX_1BYTE "MOV L, D" RESULT_L "\n",
    PREFIX_1BYTE "MOV L, E" RESULT_L "\n",
    PREFIX_1BYTE "MOV L, H" RESULT_L "\n",
    PREFIX_1BYTE "MOV L, L" RESULT_L "\n",
    PREFIX_1BYTE "MOV L, M ($%15$04hX)" RESULT_L "\n",
    PREFIX_1BYTE "MOV L, A" RESULT_L "\n",
    PREFIX_1BYTE "MOV M ($%15$04hX), B" RESULT_IND_M "\n",
    PREFIX_1BYTE "MOV M ($%15$04hX), C" RESULT_IND_M "\n",
    PREFIX_1BYTE "MOV M ($%15$04hX), D" RESULT_IND_M "\n",
    PREFIX_1BYTE "MOV M ($%15$04hX), E" RESULT_IND_M "\n",
    PREFIX_1BYTE "MOV M ($%15$04hX), H" RESULT_IND_M "\n",
    PREFIX_1BYTE "MOV M ($%15$04hX), L" RESULT_IND_M "\n",
    PREFIX_1BYTE "HLT\n",
    PREFIX_1BYTE "MOV M ($%15$04hX), A" RESULT_IND_M "\n",
    PREFIX_1BYTE "MOV A, B" RESULT_A "\n",
    PREFIX_1BYTE "MOV A, C" RESULT_A "\n",
    PREFIX_1BYTE "MOV A, D" RESULT_A "\n",
    PREFIX_1BYTE "MOV A, E" RESULT_A "\n",
    PREFIX_1BYTE "MOV A, H" RESULT_A "\n",
    PREFIX_1BYTE "MOV A, L" RESULT_A "\n",
    PREFIX_1BYTE "MOV A, M ($%15$04hX)" RESULT_A "\n",
    PREFIX_1BYTE "MOV A, A" RESULT_A "\n",
    PREFIX_1BYTE "ADD B" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADD C" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADD D" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADD E" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADD H" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADD L" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADD M ($%15$04hX)" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADD A" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC B" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC C" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC D" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC E" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC H" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC L" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC M ($%15$04hX)" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ADC A" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB B" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB C" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB D" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB E" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB H" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB L" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB M ($%15$04hX)" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SUB A" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB B" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB C" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB D" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB E" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB H" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB L" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB M ($%15$04hX)" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "SBB A" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA B" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA C" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA D" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA E" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA H" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA L" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA M ($%15$04hX)" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ANA A" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA B" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA C" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA D" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA E" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA H" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA L" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA M ($%15$04hX)" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "XRA A" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA B" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA C" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA D" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA E" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA H" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA L" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA M ($%15$04hX)" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "ORA A" RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "CMP B" RESULT_F "\n",
    PREFIX_1BYTE "CMP C" RESULT_F "\n",
    PREFIX_1BYTE "CMP D" RESULT_F "\n",
    PREFIX_1BYTE "CMP E" RESULT_F "\n",
    PREFIX_1BYTE "CMP H" RESULT_F "\n",
    PREFIX_1BYTE "CMP L" RESULT_F "\n",
    PREFIX_1BYTE "CMP M ($%15$04hX)" RESULT_F "\n",
    PREFIX_1BYTE "CMP A" RESULT_F "\n",
    PREFIX_1BYTE "RNZ\n",
    PREFIX_1BYTE "POP B" RESULT_BC STATE_SP "\n",
    PREFIX_1BYTE "JNZ " OPERAND_ADDR "\n",
    PREFIX_1BYTE "JMP " OPERAND_ADDR "\n",
    PREFIX_1BYTE "CNZ " OPERAND_ADDR "\n",
    PREFIX_1BYTE "PUSH B" RESULT_BC STATE_SP "\n",
    PREFIX_2BYTE "ADI " OPERAND_IMMED8 RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "RST 0\n",
    PREFIX_1BYTE "RZ\n",
    PREFIX_1BYTE "RET\n",
    PREFIX_1BYTE "JZ " OPERAND_ADDR "\n",
    PREFIX_1BYTE "JMP " OPERAND_ADDR "\n",
    PREFIX_1BYTE "CZ " OPERAND_ADDR "\n",
    PREFIX_1BYTE "CALL " OPERAND_ADDR "\n",
    PREFIX_2BYTE "ACI " OPERAND_IMMED8 RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "RST 1\n",
    PREFIX_1BYTE "RNC\n",
    PREFIX_1BYTE "POP D" RESULT_DE STATE_SP "\n",
    PREFIX_1BYTE "JNC " OPERAND_ADDR "\n",
    PREFIX_2BYTE "OUT " OPERAND_IMMED8 "\n",
    PREFIX_1BYTE "CNC " OPERAND_ADDR "\n",
    PREFIX_1BYTE "PUSH D" RESULT_DE STATE_SP "\n",
    PREFIX_2BYTE "SUI " OPERAND_IMMED8 RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "RST 2\n",
    PREFIX_1BYTE "RC\n",
    PREFIX_1BYTE "RET\n",
    PREFIX_1BYTE "JC " OPERAND_ADDR "\n",
    PREFIX_2BYTE "IN " OPERAND_IMMED8 "\n",
    PREFIX_1BYTE "CC " OPERAND_ADDR "\n",
    PREFIX_1BYTE "CALL " OPERAND_ADDR "\n",
    PREFIX_2BYTE "SBI " OPERAND_IMMED8 RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "RST 3\n",
    PREFIX_1BYTE "RPO\n",
    PREFIX_1BYTE "POP H" RESULT_HL STATE_SP "\n",
    PREFIX_1BYTE "JPO " OPERAND_ADDR "\n",
    PREFIX_1BYTE "XTHL" RESULT_HL "\n",
    PREFIX_1BYTE "CPO " OPERAND_ADDR "\n",
    PREFIX_1BYTE "PUSH H"  RESULT_HL STATE_SP "\n",
    PREFIX_2BYTE "ANI " OPERAND_IMMED8 RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "RST 4\n",
    PREFIX_1BYTE "RPE\n",
    PREFIX_1BYTE "PCHL" RESULT_PC "\n",
    PREFIX_1BYTE "JPE " OPERAND_ADDR "\n",
    PREFIX_1BYTE "XCHG (HL" RESULT_HL ", DE" RESULT_DE ")\n",
    PREFIX_1BYTE "CPE " OPERAND_ADDR "\n",
    PREFIX_1BYTE "CALL " OPERAND_ADDR "\n",
    PREFIX_2BYTE "XRI " OPERAND_IMMED8 RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "RST 5\n",
    PREFIX_1BYTE "RP\n",
    PREFIX_1BYTE "POP PSW" RESULT_PSW STATE_SP "\n",
    PREFIX_1BYTE "JP " OPERAND_ADDR "\n",
    PREFIX_1BYTE "DI" RESULT_INTE "\n",
    PREFIX_1BYTE "CP " OPERAND_ADDR "\n",
    PREFIX_1BYTE "PUSH PSW" RESULT_PSW STATE_SP "\n",
    PREFIX_2BYTE "ORI " OPERAND_IMMED8 RESULT_A " " STATE_F "\n",
    PREFIX_1BYTE "RST 6\n",
    PREFIX_1BYTE "RM\n",
    PREFIX_1BYTE "SPHL" RESULT_SP "\n",
    PREFIX_1BYTE "JM " OPERAND_ADDR "\n",
    PREFIX_1BYTE "EI" RESULT_INTE "\n",
    PREFIX_1BYTE "CM " OPERAND_ADDR "\n",
    PREFIX_1BYTE "CALL " OPERAND_ADDR "\n",
    PREFIX_2BYTE "CPI " OPERAND_IMMED8 RESULT_F "\n",
    PREFIX_1BYTE "RST 7\n"
};

void
I8080FullInstrContextDisassembleToFile(
    FILE                    *fptr,
    I8080FullInstrContext_t *instr,
    I8080MemRef             sysmem,
    I8080Registers_t        *rgstrs
)
{
    char                    addr[6];
    char                    flag_str[9];
    uint8_t                 ind_M = I8080MemRead(sysmem, rgstrs->HL);
    
    if ( instr->is_inte ) {
        strcpy(addr, ">INTE");
    } else {
        snprintf(addr, sizeof(addr), "$%04hX", instr->addr);
    }
    snprintf(flag_str, sizeof(flag_str),
        "%c%c-%c-%c-%c",
        rgstrs->S ? 'S' : 's',
        rgstrs->Z ? 'Z' : 'z',
        rgstrs->AC ? 'A' : 'a',
        rgstrs->P ? 'P' : 'p',
        rgstrs->CY ? 'C' : 'c');
    fprintf(fptr,
            I8080InstrDisassemblyFormat[instr->opcode],
            instr->addr,
            instr->opcode,
            instr->operand1,
            instr->operand2,
            rgstrs->B,
            rgstrs->C,
            rgstrs->D,
            rgstrs->E,
            rgstrs->H,
            rgstrs->L,
            rgstrs->F,
            rgstrs->A,
            rgstrs->BC,
            rgstrs->DE,
            rgstrs->HL,
            rgstrs->PSW,
            rgstrs->SP,
            rgstrs->PC,
            rgstrs->INTE,
            ind_M,
            flag_str,
            instr->cycles,
            addr);
}

void
I8080FullInstrContextDisassembleToTextBuffer(
    I8080TextBufferRef      tbuff,
    I8080FullInstrContext_t *instr,
    I8080MemRef             sysmem,
    I8080Registers_t        *rgstrs
)
{
    char                    addr[6];
    char                    flag_str[9];
    uint8_t                 ind_M = I8080MemRead(sysmem, rgstrs->HL);
    
    if ( instr->is_inte ) {
        strcpy(addr, ">INTE");
    } else {
        snprintf(addr, sizeof(addr), "$%04hX", instr->addr);
    }
    snprintf(flag_str, sizeof(flag_str),
        "%c%c-%c-%c-%c",
        rgstrs->S ? 'S' : 's',
        rgstrs->Z ? 'Z' : 'z',
        rgstrs->AC ? 'A' : 'a',
        rgstrs->P ? 'P' : 'p',
        rgstrs->CY ? 'C' : 'c');
    I8080TextBufferPrintf(tbuff,
            I8080InstrDisassemblyFormat[instr->opcode],
            instr->addr,
            instr->opcode,
            instr->operand1,
            instr->operand2,
            rgstrs->B,
            rgstrs->C,
            rgstrs->D,
            rgstrs->E,
            rgstrs->H,
            rgstrs->L,
            rgstrs->F,
            rgstrs->A,
            rgstrs->BC,
            rgstrs->DE,
            rgstrs->HL,
            rgstrs->PSW,
            rgstrs->SP,
            rgstrs->PC,
            rgstrs->INTE,
            ind_M,
            flag_str,
            instr->cycles,
            addr);
}
