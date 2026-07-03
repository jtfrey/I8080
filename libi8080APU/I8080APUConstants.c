/*
 * I8080
 * An implementation of the I8080 processor.
 *
 * Dr. Jeffrey Frey
 * University of Delaware
 *
 * Implmeentation of constants for use with the Audio Processing Unit
 * (APU).
 */

#include "I8080APUConstants.h"

const uint16_t I8080APU8BitFreqTimerMap[256] = {
        0x5448,     /* 0x00: C1 — Samples: 1348.49 -> 1348.50 */
        0x51EA,
        0x4F8D,		/* 0x02: CSHARP1 — Samples: 1272.81 -> 1272.81 */
        0x4D51,
        0x4B16,		/* 0x04: D1 — Samples: 1201.37 -> 1201.38 */
        0x48FA,
        0x46DF,		/* 0x06: DSHARP1 — Samples: 1133.94 -> 1133.94 */
        0x44E2,
        0x42E5,		/* 0x08: E1 — Samples: 1070.30 -> 1070.31 */
        0x4104,
        0x3F24,		/* 0x0A: F1 — Samples: 1010.23 -> 1010.25 */
        0x3D5E,
        0x3B98,		/* 0x0C: FSHARP1 — Samples:  953.53 ->  953.50 */
        0x39EC,
        0x3840,		/* 0x0E: G1 — Samples:  900.01 ->  900.00 */
        0x36AC,
        0x3518,		/* 0x10: GSHARP1 — Samples:  849.50 ->  849.50 */
        0x339A,
        0x321D,		/* 0x12: A1 — Samples:  801.82 ->  801.81 */
        0x30B5,
        0x2F4D,		/* 0x14: ASHARP1 — Samples:  756.82 ->  756.81 */
        0x2DF9,
        0x2CA5,		/* 0x16: B1 — Samples:  714.34 ->  714.31 */
        0x2C04,
        0x2B64,
        0x2AC4,
        0x2A24,		/* 0x1A: C2 — Samples:  674.25 ->  674.25 */
        0x298B,
        0x28F4,
        0x285D,
        0x27C6,		/* 0x1E: CSHARP2 — Samples:  636.40 ->  636.38 */
        0x2735,
        0x26A7,
        0x2619,
        0x258B,		/* 0x22: D2 — Samples:  600.68 ->  600.69 */
        0x2502,
        0x247C,
        0x23F6,
        0x2370,		/* 0x26: DSHARP2 — Samples:  566.97 ->  567.00 */
        0x22EF,
        0x2270,
        0x21F1,
        0x2172,		/* 0x2A: E2 — Samples:  535.15 ->  535.12 */
        0x20FA,
        0x2082,
        0x200A,
        0x1F92,		/* 0x2E: F2 — Samples:  505.11 ->  505.12 */
        0x1F1F,
        0x1EAE,
        0x1E3D,
        0x1DCC,		/* 0x32: FSHARP2 — Samples:  476.76 ->  476.75 */
        0x1D61,
        0x1CF6,
        0x1C8B,
        0x1C20,		/* 0x36: G2 — Samples:  450.01 ->  450.00 */
        0x1BBB,
        0x1B56,
        0x1AF1,
        0x1A8C,		/* 0x3A: GSHARP2 — Samples:  424.75 ->  424.75 */
        0x1A2C,
        0x19CD,
        0x196E,
        0x190F,		/* 0x3E: A2 — Samples:  400.91 ->  400.94 */
        0x18B5,
        0x185B,
        0x1801,
        0x17A7,		/* 0x42: ASHARP2 — Samples:  378.41 ->  378.44 */
        0x1752,
        0x16FD,
        0x16A8,
        0x1653,		/* 0x46: B2 — Samples:  357.17 ->  357.19 */
        0x1602,
        0x15B2,
        0x1562,
        0x1512,		/* 0x4A: C3 — Samples:  337.12 ->  337.12 */
        0x14C4,
        0x1479,
        0x142E,
        0x13E3,		/* 0x4E: CSHARP3 — Samples:  318.20 ->  318.19 */
        0x139A,
        0x1353,
        0x130C,
        0x12C5,		/* 0x52: D3 — Samples:  300.34 ->  300.31 */
        0x1281,
        0x123E,
        0x11FB,
        0x11B8,		/* 0x56: DSHARP3 — Samples:  283.49 ->  283.50 */
        0x1176,
        0x1137,
        0x10F8,
        0x10B9,		/* 0x5A: E3 — Samples:  267.57 ->  267.56 */
        0x107D,
        0x1041,
        0x1005,
        0x0FC9,		/* 0x5E: F3 — Samples:  252.56 ->  252.56 */
        0x0F8E,
        0x0F56,
        0x0F1E,
        0x0EE6,		/* 0x62: FSHARP3 — Samples:  238.38 ->  238.38 */
        0x0EAF,
        0x0E7A,
        0x0E45,
        0x0E10,		/* 0x66: G3 — Samples:  225.00 ->  225.00 */
        0x0DDC,
        0x0DAA,
        0x0D78,
        0x0D46,		/* 0x6A: GSHARP3 — Samples:  212.37 ->  212.38 */
        0x0D14,
        0x0CE5,
        0x0CB6,
        0x0C87,		/* 0x6E: A3 — Samples:  200.45 ->  200.44 */
        0x0C5A,
        0x0C2D,
        0x0C00,
        0x0BD3,		/* 0x72: ASHARP3 — Samples:  189.20 ->  189.19 */
        0x0BA7,
        0x0B7D,
        0x0B53,
        0x0B29,		/* 0x76: B3 — Samples:  178.58 ->  178.56 */
        0x0B01,
        0x0AD9,
        0x0AB1,
        0x0A89,		/* 0x7A: C4 — Samples:  168.56 ->  168.56 */
        0x0A61,
        0x0A3C,
        0x0A17,
        0x09F2,		/* 0x7E: CSHARP4 — Samples:  159.10 ->  159.12 */
        0x09CC,
        0x09A9,
        0x0986,
        0x0963,		/* 0x82: D4 — Samples:  150.17 ->  150.19 */
        0x093F,
        0x091E,
        0x08FD,
        0x08DC,		/* 0x86: DSHARP4 — Samples:  141.74 ->  141.75 */
        0x08BA,
        0x089B,
        0x087C,
        0x085D,		/* 0x8A: E4 — Samples:  133.79 ->  133.81 */
        0x083E,
        0x0820,
        0x0802,
        0x07E4,		/* 0x8E: F4 — Samples:  126.28 ->  126.25 */
        0x07C7,
        0x07AB,
        0x078F,
        0x0773,		/* 0x92: FSHARP4 — Samples:  119.19 ->  119.19 */
        0x0756,
        0x073C,
        0x0722,
        0x0708,		/* 0x96: G4 — Samples:  112.50 ->  112.50 */
        0x06EE,
        0x06D5,
        0x06BC,
        0x06A3,		/* 0x9A: GSHARP4 — Samples:  106.19 ->  106.19 */
        0x0689,
        0x0672,
        0x065B,
        0x0644,		/* 0x9E: A4 — Samples:  100.23 ->  100.25 */
        0x062C,
        0x0616,
        0x0600,
        0x05EA,		/* 0xA2: ASHARP4 — Samples:   94.60 ->   94.62 */
        0x05D4,
        0x05BF,
        0x05AA,
        0x0595,		/* 0xA6: B4 — Samples:   89.29 ->   89.31 */
        0x0580,
        0x056C,
        0x0558,
        0x0544,		/* 0xAA: C5 — Samples:   84.28 ->   84.25 */
        0x052F,
        0x051D,
        0x050B,
        0x04F9,		/* 0xAE: CSHARP5 — Samples:   79.55 ->   79.56 */
        0x04E7,
        0x04D5,
        0x04C3,
        0x04B1,		/* 0xB2: D5 — Samples:   75.09 ->   75.06 */
        0x049E,
        0x048E,
        0x047E,
        0x046E,		/* 0xB6: DSHARP5 — Samples:   70.87 ->   70.88 */
        0x045E,
        0x044E,
        0x043E,
        0x042E,		/* 0xBA: E5 — Samples:   66.89 ->   66.88 */
        0x041F,
        0x0410,
        0x0401,
        0x03F2,		/* 0xBE: F5 — Samples:   63.14 ->   63.12 */
        0x03E4,
        0x03D6,
        0x03C8,
        0x03BA,		/* 0xC2: FSHARP5 — Samples:   59.60 ->   59.62 */
        0x03AB,
        0x039E,
        0x0391,
        0x0384,		/* 0xC6: G5 — Samples:   56.25 ->   56.25 */
        0x0375,
        0x0369,
        0x035D,
        0x0351,		/* 0xCA: GSHARP5 — Samples:   53.09 ->   53.06 */
        0x0343,
        0x0338,
        0x032D,
        0x0322,		/* 0xCE: A5 — Samples:   50.11 ->   50.12 */
        0x0316,
        0x030B,
        0x0300,
        0x02F5,		/* 0xD2: ASHARP5 — Samples:   47.30 ->   47.31 */
        0x02E8,
        0x02DE,
        0x02D4,
        0x02CA,		/* 0xD6: B5 — Samples:   44.65 ->   44.62 */
        0x02C0,
        0x02B6,
        0x02AC,
        0x02A2,		/* 0xDA: C6 — Samples:   42.14 ->   42.12 */
        0x0297,
        0x028E,
        0x0285,
        0x027C,		/* 0xDE: CSHARP6 — Samples:   39.78 ->   39.75 */
        0x0271,
        0x0269,
        0x0261,
        0x0259,		/* 0xE2: D6 — Samples:   37.54 ->   37.56 */
        0x024F,
        0x0247,
        0x023F,
        0x0237,		/* 0xE6: DSHARP6 — Samples:   35.44 ->   35.44 */
        0x022F,
        0x0227,
        0x021F,
        0x0217,		/* 0xEA: E6 — Samples:   33.45 ->   33.44 */
        0x020E,
        0x0207,
        0x0200,
        0x01F9,		/* 0xEE: F6 — Samples:   31.57 ->   31.56 */
        0x01F2,
        0x01EB,
        0x01E4,
        0x01DD,		/* 0xF2: FSHARP6 — Samples:   29.80 ->   29.81 */
        0x01D4,
        0x01CE,
        0x01C8,
        0x01C2,		/* 0xF6: G6 — Samples:   28.13 ->   28.12 */
        0x01B5,
        0x01A9,		/* 0xF8: GSHARP6 — Samples:   26.55 ->   26.56 */
        0x019D,
        0x0191,		/* 0xFA: A6 — Samples:   25.06 ->   25.06 */
        0x0185,
        0x017A,		/* 0xFC: ASHARP6 — Samples:   23.65 ->   23.62 */
        0x016F,
        0x0165,		/* 0xFE: B6 — Samples:   22.32 ->   22.31 */
        0x015A
};