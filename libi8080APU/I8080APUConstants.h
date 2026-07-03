/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Header that defines constants for use with the Audio Processing Unit
 * (APU).
 */
 
#ifndef __I8080APUCONSTANTS_H__
#define __I8080APUCONSTANTS_H__

#include "I8080Config.h"

// 44100 Hz Sample Periods in UQ12.4 Fixed-Point Format
// --- OCTAVE 1 ---
#define I8080_APU_NOTE_C1       0x5448  /* Samples: 1348.49 -> 1348.50 */
#define I8080_APU_NOTE_CSHARP1  0x4F8D  /* Samples: 1272.81 -> 1272.81 */
#define I8080_APU_NOTE_D1       0x4B16  /* Samples: 1201.37 -> 1201.38 */
#define I8080_APU_NOTE_DSHARP1  0x46DF  /* Samples: 1133.94 -> 1133.94 */
#define I8080_APU_NOTE_E1       0x42E5  /* Samples: 1070.30 -> 1070.31 */
#define I8080_APU_NOTE_F1       0x3F24  /* Samples: 1010.23 -> 1010.25 */
#define I8080_APU_NOTE_FSHARP1  0x3B98  /* Samples:  953.53 ->  953.50 */
#define I8080_APU_NOTE_G1       0x3840  /* Samples:  900.01 ->  900.00 */
#define I8080_APU_NOTE_GSHARP1  0x3518  /* Samples:  849.50 ->  849.50 */
#define I8080_APU_NOTE_A1       0x321D  /* Samples:  801.82 ->  801.81 */
#define I8080_APU_NOTE_ASHARP1  0x2F4D  /* Samples:  756.82 ->  756.81 */
#define I8080_APU_NOTE_B1       0x2CA5  /* Samples:  714.34 ->  714.31 */
// --- OCTAVE 2 ---
#define I8080_APU_NOTE_C2       0x2A24  /* Samples:  674.25 ->  674.25 */
#define I8080_APU_NOTE_CSHARP2  0x27C6  /* Samples:  636.40 ->  636.38 */
#define I8080_APU_NOTE_D2       0x258B  /* Samples:  600.68 ->  600.69 */
#define I8080_APU_NOTE_DSHARP2  0x2370  /* Samples:  566.97 ->  567.00 */
#define I8080_APU_NOTE_E2       0x2172  /* Samples:  535.15 ->  535.12 */
#define I8080_APU_NOTE_F2       0x1F92  /* Samples:  505.11 ->  505.12 */
#define I8080_APU_NOTE_FSHARP2  0x1DCC  /* Samples:  476.76 ->  476.75 */
#define I8080_APU_NOTE_G2       0x1C20  /* Samples:  450.01 ->  450.00 */
#define I8080_APU_NOTE_GSHARP2  0x1A8C  /* Samples:  424.75 ->  424.75 */
#define I8080_APU_NOTE_A2       0x190F  /* Samples:  400.91 ->  400.94 */
#define I8080_APU_NOTE_ASHARP2  0x17A7  /* Samples:  378.41 ->  378.44 */
#define I8080_APU_NOTE_B2       0x1653  /* Samples:  357.17 ->  357.19 */
// --- OCTAVE 3 ---
#define I8080_APU_NOTE_C3       0x1512  /* Samples:  337.12 ->  337.12 */
#define I8080_APU_NOTE_CSHARP3  0x13E3  /* Samples:  318.20 ->  318.19 */
#define I8080_APU_NOTE_D3       0x12C5  /* Samples:  300.34 ->  300.31 */
#define I8080_APU_NOTE_DSHARP3  0x11B8  /* Samples:  283.49 ->  283.50 */
#define I8080_APU_NOTE_E3       0x10B9  /* Samples:  267.57 ->  267.56 */
#define I8080_APU_NOTE_F3       0x0FC9  /* Samples:  252.56 ->  252.56 */
#define I8080_APU_NOTE_FSHARP3  0x0EE6  /* Samples:  238.38 ->  238.38 */
#define I8080_APU_NOTE_G3       0x0E10  /* Samples:  225.00 ->  225.00 */
#define I8080_APU_NOTE_GSHARP3  0x0D46  /* Samples:  212.37 ->  212.38 */
#define I8080_APU_NOTE_A3       0x0C87  /* Samples:  200.45 ->  200.44 */
#define I8080_APU_NOTE_ASHARP3  0x0BD3  /* Samples:  189.20 ->  189.19 */
#define I8080_APU_NOTE_B3       0x0B29  /* Samples:  178.58 ->  178.56 */
// --- OCTAVE 4 ---
#define I8080_APU_NOTE_C4       0x0A89  /* Samples:  168.56 ->  168.56 */
#define I8080_APU_NOTE_CSHARP4  0x09F2  /* Samples:  159.10 ->  159.12 */
#define I8080_APU_NOTE_D4       0x0963  /* Samples:  150.17 ->  150.19 */
#define I8080_APU_NOTE_DSHARP4  0x08DC  /* Samples:  141.74 ->  141.75 */
#define I8080_APU_NOTE_E4       0x085D  /* Samples:  133.79 ->  133.81 */
#define I8080_APU_NOTE_F4       0x07E4  /* Samples:  126.28 ->  126.25 */
#define I8080_APU_NOTE_FSHARP4  0x0773  /* Samples:  119.19 ->  119.19 */
#define I8080_APU_NOTE_G4       0x0708  /* Samples:  112.50 ->  112.50 */
#define I8080_APU_NOTE_GSHARP4  0x06A3  /* Samples:  106.19 ->  106.19 */
#define I8080_APU_NOTE_A4       0x0644  /* Samples:  100.23 ->  100.25 */
#define I8080_APU_NOTE_ASHARP4  0x05EA  /* Samples:   94.60 ->   94.62 */
#define I8080_APU_NOTE_B4       0x0595  /* Samples:   89.29 ->   89.31 */
// --- OCTAVE 5 ---
#define I8080_APU_NOTE_C5       0x0544  /* Samples:   84.28 ->   84.25 */
#define I8080_APU_NOTE_CSHARP5  0x04F9  /* Samples:   79.55 ->   79.56 */
#define I8080_APU_NOTE_D5       0x04B1  /* Samples:   75.09 ->   75.06 */
#define I8080_APU_NOTE_DSHARP5  0x046E  /* Samples:   70.87 ->   70.88 */
#define I8080_APU_NOTE_E5       0x042E  /* Samples:   66.89 ->   66.88 */
#define I8080_APU_NOTE_F5       0x03F2  /* Samples:   63.14 ->   63.12 */
#define I8080_APU_NOTE_FSHARP5  0x03BA  /* Samples:   59.60 ->   59.62 */
#define I8080_APU_NOTE_G5       0x0384  /* Samples:   56.25 ->   56.25 */
#define I8080_APU_NOTE_GSHARP5  0x0351  /* Samples:   53.09 ->   53.06 */
#define I8080_APU_NOTE_A5       0x0322  /* Samples:   50.11 ->   50.12 */
#define I8080_APU_NOTE_ASHARP5  0x02F5  /* Samples:   47.30 ->   47.31 */
#define I8080_APU_NOTE_B5       0x02CA  /* Samples:   44.65 ->   44.62 */
// --- OCTAVE 6 ---
#define I8080_APU_NOTE_C6       0x02A2  /* Samples:   42.14 ->   42.12 */
#define I8080_APU_NOTE_CSHARP6  0x027C  /* Samples:   39.78 ->   39.75 */
#define I8080_APU_NOTE_D6       0x0259  /* Samples:   37.54 ->   37.56 */
#define I8080_APU_NOTE_DSHARP6  0x0237  /* Samples:   35.44 ->   35.44 */
#define I8080_APU_NOTE_E6       0x0217  /* Samples:   33.45 ->   33.44 */
#define I8080_APU_NOTE_F6       0x01F9  /* Samples:   31.57 ->   31.56 */
#define I8080_APU_NOTE_FSHARP6  0x01DD  /* Samples:   29.80 ->   29.81 */
#define I8080_APU_NOTE_G6       0x01C2  /* Samples:   28.13 ->   28.12 */
#define I8080_APU_NOTE_GSHARP6  0x01A9  /* Samples:   26.55 ->   26.56 */
#define I8080_APU_NOTE_A6       0x0191  /* Samples:   25.06 ->   25.06 */
#define I8080_APU_NOTE_ASHARP6  0x017A  /* Samples:   23.65 ->   23.62 */
#define I8080_APU_NOTE_B6       0x0165  /* Samples:   22.32 ->   22.31 */

/**
 * 8-bit frequency mapping table
 * When using 8-bit registers, the frequency timer is an 8-bit
 * index into this table of 16-bit values.
 *
 * All notes in octave 1 and notes above G in octave 6 have
 * a half-pitch interpolation between notes; all other notes use
 * quarter-pitch interpolations.  This yields a total of 255
 * frequencies, with the final 256th frequency being a half-pitch
 * below B6.
 *
 * The 8-bit index for each full note is present in this header
 * as preprocessor macros of the form I8080_APU_NOTE_*_8BIT_IDX
 */
extern const uint16_t I8080APU8BitFreqTimerMap[256];

// 8-bit indices for full notes in the I8080APU8BitFreqTimerMap table
// --- OCTAVE 1 ---
#define I8080_APU_NOTE_C1_8BIT_IDX      0x00
#define I8080_APU_NOTE_CSHARP1_8BIT_IDX 0x02
#define I8080_APU_NOTE_D1_8BIT_IDX      0x04
#define I8080_APU_NOTE_DSHARP1_8BIT_IDX 0x06
#define I8080_APU_NOTE_E1_8BIT_IDX      0x08
#define I8080_APU_NOTE_F1_8BIT_IDX      0x0A
#define I8080_APU_NOTE_FSHARP1_8BIT_IDX 0x0C
#define I8080_APU_NOTE_G1_8BIT_IDX      0x0E
#define I8080_APU_NOTE_GSHARP1_8BIT_IDX 0x10
#define I8080_APU_NOTE_A1_8BIT_IDX      0x12
#define I8080_APU_NOTE_ASHARP1_8BIT_IDX 0x14
#define I8080_APU_NOTE_B1_8BIT_IDX      0x16
// --- OCTAVE 2 ---
#define I8080_APU_NOTE_C2_8BIT_IDX      0x1A
#define I8080_APU_NOTE_CSHARP2_8BIT_IDX 0x1E
#define I8080_APU_NOTE_D2_8BIT_IDX      0x22
#define I8080_APU_NOTE_DSHARP2_8BIT_IDX 0x26
#define I8080_APU_NOTE_E2_8BIT_IDX      0x2A
#define I8080_APU_NOTE_F2_8BIT_IDX      0x2E
#define I8080_APU_NOTE_FSHARP2_8BIT_IDX 0x32
#define I8080_APU_NOTE_G2_8BIT_IDX      0x36
#define I8080_APU_NOTE_GSHARP2_8BIT_IDX 0x3A
#define I8080_APU_NOTE_A2_8BIT_IDX      0x3E
#define I8080_APU_NOTE_ASHARP2_8BIT_IDX 0x42
#define I8080_APU_NOTE_B2_8BIT_IDX      0x46
// --- OCTAVE 3 ---
#define I8080_APU_NOTE_C3_8BIT_IDX      0x4A
#define I8080_APU_NOTE_CSHARP3_8BIT_IDX 0x4E
#define I8080_APU_NOTE_D3_8BIT_IDX      0x52
#define I8080_APU_NOTE_DSHARP3_8BIT_IDX 0x56
#define I8080_APU_NOTE_E3_8BIT_IDX      0x5A
#define I8080_APU_NOTE_F3_8BIT_IDX      0x5E
#define I8080_APU_NOTE_FSHARP3_8BIT_IDX 0x62
#define I8080_APU_NOTE_G3_8BIT_IDX      0x66
#define I8080_APU_NOTE_GSHARP3_8BIT_IDX 0x6A
#define I8080_APU_NOTE_A3_8BIT_IDX      0x6E
#define I8080_APU_NOTE_ASHARP3_8BIT_IDX 0x72
#define I8080_APU_NOTE_B3_8BIT_IDX      0x76
// --- OCTAVE 4 ---
#define I8080_APU_NOTE_C4_8BIT_IDX      0x7A
#define I8080_APU_NOTE_CSHARP4_8BIT_IDX 0x7E
#define I8080_APU_NOTE_D4_8BIT_IDX      0x82
#define I8080_APU_NOTE_DSHARP4_8BIT_IDX 0x86
#define I8080_APU_NOTE_E4_8BIT_IDX      0x8A
#define I8080_APU_NOTE_F4_8BIT_IDX      0x8E
#define I8080_APU_NOTE_FSHARP4_8BIT_IDX 0x92
#define I8080_APU_NOTE_G4_8BIT_IDX      0x96
#define I8080_APU_NOTE_GSHARP4_8BIT_IDX 0x9A
#define I8080_APU_NOTE_A4_8BIT_IDX      0x9E
#define I8080_APU_NOTE_ASHARP4_8BIT_IDX 0xA2
#define I8080_APU_NOTE_B4_8BIT_IDX      0xA6
// --- OCTAVE 5 ---
#define I8080_APU_NOTE_C5_8BIT_IDX      0xAA
#define I8080_APU_NOTE_CSHARP5_8BIT_IDX 0xAE
#define I8080_APU_NOTE_D5_8BIT_IDX      0xB2
#define I8080_APU_NOTE_DSHARP5_8BIT_IDX 0xB6
#define I8080_APU_NOTE_E5_8BIT_IDX      0xBA
#define I8080_APU_NOTE_F5_8BIT_IDX      0xBE
#define I8080_APU_NOTE_FSHARP5_8BIT_IDX 0xC2
#define I8080_APU_NOTE_G5_8BIT_IDX      0xC6
#define I8080_APU_NOTE_GSHARP5_8BIT_IDX 0xCA
#define I8080_APU_NOTE_A5_8BIT_IDX      0xCE
#define I8080_APU_NOTE_ASHARP5_8BIT_IDX 0xD2
#define I8080_APU_NOTE_B5_8BIT_IDX      0xD6
// --- OCTAVE 6 ---
#define I8080_APU_NOTE_C6_8BIT_IDX      0xDA
#define I8080_APU_NOTE_CSHARP6_8BIT_IDX 0xDE
#define I8080_APU_NOTE_D6_8BIT_IDX      0xE2
#define I8080_APU_NOTE_DSHARP6_8BIT_IDX 0xE6
#define I8080_APU_NOTE_E6_8BIT_IDX      0xEA
#define I8080_APU_NOTE_F6_8BIT_IDX      0xEE
#define I8080_APU_NOTE_FSHARP6_8BIT_IDX 0xF2
#define I8080_APU_NOTE_G6_8BIT_IDX      0xF6
#define I8080_APU_NOTE_GSHARP6_8BIT_IDX 0xF8
#define I8080_APU_NOTE_A6_8BIT_IDX      0xFA
#define I8080_APU_NOTE_ASHARP6_8BIT_IDX 0xFC
#define I8080_APU_NOTE_B6_8BIT_IDX      0xFE

#endif /* __I8080APUCONSTANTS_H__ */
