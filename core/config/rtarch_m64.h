/******************************************************************************/
/* Copyright (c) 2013-2016 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_M64_H
#define RT_RTARCH_M64_H

#define RT_BASE_REGS        16

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_m64.h: Implementation of MIPS64 r5/r6 BASE instructions.
 *
 * This file is a part of the unified SIMD assembler framework (rtarch.h)
 * designed to be compatible with different processor architectures,
 * while maintaining strictly defined common API.
 *
 * Recommended naming scheme for instructions:
 *
 * cmdxx_ri - applies [cmd] to [r]egister from [i]mmediate
 * cmdxx_mi - applies [cmd] to [m]emory   from [i]mmediate
 * cmdxx_rz - applies [cmd] to [r]egister from [z]ero-arg
 * cmdxx_mz - applies [cmd] to [m]emory   from [z]ero-arg
 *
 * cmdxx_rm - applies [cmd] to [r]egister from [m]emory
 * cmdxx_ld - applies [cmd] as above
 * cmdxx_mr - applies [cmd] to [m]emory   from [r]egister
 * cmdxx_st - applies [cmd] as above (arg list as cmdxx_ld)
 *
 * cmdxx_rr - applies [cmd] to [r]egister from [r]egister
 * cmdxx_mm - applies [cmd] to [m]emory   from [m]emory
 * cmdxx_rx - applies [cmd] to [r]egister (one-operand cmd)
 * cmdxx_mx - applies [cmd] to [m]emory   (one-operand cmd)
 *
 * cmdxx_rx - applies [cmd] to [r]egister from x-register
 * cmdxx_mx - applies [cmd] to [m]emory   from x-register
 * cmdxx_xr - applies [cmd] to x-register from [r]egister
 * cmdxx_xm - applies [cmd] to x-register from [m]emory
 *
 * cmdxx_rl - applies [cmd] to [r]egister from [l]abel
 * cmdxx_xl - applies [cmd] to x-register from [l]abel
 * cmdxx_lb - applies [cmd] as above
 * label_ld - applies [adr] as above
 *
 * stack_st - applies [mov] to stack from register (push)
 * stack_ld - applies [mov] to register from stack (pop)
 * stack_sa - applies [mov] to stack from all registers
 * stack_la - applies [mov] to all registers from stack
 *
 * cmdw*_** - applies [cmd] to 32-bit BASE register/memory/immediate args
 * cmdx*_** - applies [cmd] to A-size BASE register/memory/immediate args
 * cmdy*_** - applies [cmd] to L-size BASE register/memory/immediate args
 * cmdz*_** - applies [cmd] to 64-bit BASE register/memory/immediate args
 *
 * cmd*x_** - applies [cmd] to unsigned integer args, [x] - default
 * cmd*n_** - applies [cmd] to   signed integer args, [n] - negatable
 * cmd*p_** - applies [cmd] to   signed integer args, [p] - part-range
 *
 * cmd*z_** - applies [cmd] while setting condition flags, [z] - zero flag.
 * Regular cmd*x_**, cmd*n_** instructions may or may not set flags depending
 * on the target architecture, thus no assumptions can be made for jezxx/jnzxx.
 *
 * Interpretation of instruction parameters:
 *
 * upper-case params have triplet structure and require W to pass-forward
 * lower-case params are singular and can be used/passed as such directly
 *
 * RD - BASE register serving as destination only, if present
 * RG - BASE register serving as destination and fisrt source
 * RS - BASE register serving as second source (first if any)
 * RT - BASE register serving as third source (second if any)
 *
 * MD - BASE addressing mode (Oeax, M***, I***) (memory-dest)
 * MG - BASE addressing mode (Oeax, M***, I***) (memory-dsrc)
 * MS - BASE addressing mode (Oeax, M***, I***) (memory-src2)
 * MT - BASE addressing mode (Oeax, M***, I***) (memory-src3)
 *
 * DD - displacement value (DP, DF, DG, DH, DV) (memory-dest)
 * DG - displacement value (DP, DF, DG, DH, DV) (memory-dsrc)
 * DS - displacement value (DP, DF, DG, DH, DV) (memory-src2)
 * DT - displacement value (DP, DF, DG, DH, DV) (memory-src3)
 *
 * IS - immediate value (is used as a second or first source)
 * IT - immediate value (is used as a third or second source)
 *
 * Adjustable BASE/SIMD subsets (cmdx*, cmdy*, cmdp*) are defined in rtbase.h.
 * Mixing of 64/32-bit fields in backend structures may lead to misalignment
 * of 64-bit fields to 4-byte boundary, which is not supported on some targets.
 * Place fields carefully to ensure natural alignment for all data types.
 * Note that within cmdx*_** subset most of the instructions follow in-heap
 * address size (RT_ADDRESS or A) and only label_ld/st, jmpxx_xr/xm follow
 * pointer size (RT_POINTER or P) as code/data/stack segments are fixed.
 * Stack ops always work with full registers regardless of the mode chosen.
 *
 * 32-bit and 64-bit BASE subsets are not easily compatible on all targets,
 * thus any register modified with 32-bit op cannot be used in 64-bit subset.
 * Alternatively, data flow must not exceed 31-bit range for 32-bit operations
 * to produce consistent results usable in 64-bit subset across all targets.
 * Also registers written with 64-bit ops aren't always compatible with 32-bit,
 * as m64 requires the upper half to be all 0s or all 1s for m32 arithmetic.
 * Only a64 and x64 have a complete 32-bit support in 64-bit mode both zeroing
 * the upper half of the result, while m64 sign-extending all 32-bit operations
 * and p64 overflowing 32-bit arithmetic into the upper half. Similar reasons
 * of inconsistency prohibit use of IW immediate type within 64-bit subset,
 * where a64 and p64 zero-extend, while x64 and m64 sign-extend 32-bit value.
 *
 * Note that offset correction for endianness E is only applicable for addresses
 * within pointer fields, when (in-heap) address and pointer sizes don't match.
 * Working with 32-bit data in 64-bit fields in any other circumstances must be
 * done consistently within a subset of one size (32-bit, 64-bit or C/C++).
 * Alternatively, data written natively in C/C++ can be worked on from within
 * a given (one) subset if appropriate offset correction is used from rtarch.h.
 *
 * Setting-flags instructions' naming scheme may change again in the future for
 * better orthogonality with operands size, type and args-list. It is therefore
 * recommended to use combined-arithmetic-jump (arj) for better API stability
 * and maximum efficiency across all supported targets. For similar reasons
 * of higher performance on MIPS and Power use combined-compare-jump (cmj).
 * Not all canonical forms of BASE instructions have efficient implementation.
 * For example, some forms of shifts and division use stack ops on x86 targets,
 * while standalone remainder operations can only be done natively on MIPS.
 * Consider using special fixed-register forms for maximum performance.
 * Argument x-register (implied) is fixed by the implementation.
 * Some formal definitions are not given below to encourage
 * use of friendly aliases for better code readability.
 */

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

#include "rtarch_m32.h"

#if   defined (RT_M64)

/******************************************************************************/
/**********************************   M64   ***********************************/
/******************************************************************************/

/* mov (D = S)
 * set-flags: no */

#define movzx_ri(RD, IS)                                                    \
        AUW(EMPTY,    VAL(IS), REG(RD), EMPTY,   EMPTY,   EMPTY2, G3(IS))

#define movzx_mi(MD, DD, IS)                                                \
        AUW(SIB(MD),  VAL(IS), TDxx,    MOD(MD), VAL(DD), C1(DD), G3(IS))   \
        EMITW(0xFC000000 | MDM(TDxx,    MOD(MD), VAL(DD), B1(DD), P1(DD)))

#define movzx_rr(RD, RS)                                                    \
        EMITW(0x00000025 | MRM(REG(RD), REG(RS), TZxx))

#define movzx_ld(RD, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(REG(RD), MOD(MS), VAL(DS), B1(DS), P1(DS)))

#define movzx_st(RS, MD, DD)                                                \
        AUW(SIB(MD),  EMPTY,  EMPTY,    MOD(MD), VAL(DD), C1(DD), EMPTY2)   \
        EMITW(0xFC000000 | MDM(REG(RS), MOD(MD), VAL(DD), B1(DD), P1(DD)))


#define movzx_rj(RD, IT, IS)     /* IT - upper 32-bit, IS - lower 32-bit */ \
        AUW(EMPTY,    VAL(IT), REG(RD), EMPTY,   EMPTY,   EMPTY2, G3(IT))   \
        EMITW(0x24000000 | REG(RD)<<21 | REG(RD)<<16 | ((VAL(IS)>>31)&1))   \
        EMITW(0x0000003C | MRM(REG(RD), 0x00,    REG(RD)))                  \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RD), REG(RD), VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        /* if true ^ equals to -1 (not 1) */

#define movzx_mj(MD, DD, IT, IS) /* IT - upper 32-bit, IS - lower 32-bit */ \
        AUW(EMPTY,    VAL(IT), TMxx,    EMPTY,   EMPTY,   EMPTY2, G3(IT))   \
        EMITW(0x24000000 | TMxx<<21 | TMxx<<16 | ((VAL(IS)>>31)&1))         \
        EMITW(0x0000003C | MRM(TMxx,    0x00,    TMxx))                     \
        AUW(SIB(MD),  VAL(IS), TIxx,    MOD(MD), VAL(DD), C1(DD), G1(IS))   \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MD), VAL(DD), B1(DD), P1(DD)))

/* and (G = G & S)
 * set-flags: undefined (*x), yes (*z) */

#define andzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        /* if true ^ equals to -1 (not 1) */

#define andzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define andzx_rr(RG, RS)                                                    \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), REG(RS)))

#define andzx_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), TMxx))

#define andzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define andzx_mr(MG, DG, RS)                                                \
        andzx_st(W(RS), W(MG), W(DG))


#define andzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define andzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define andzz_rr(RG, RS)                                                    \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define andzz_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000024 | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define andzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define andzz_mr(MG, DG, RS)                                                \
        andzz_st(W(RS), W(MG), W(DG))

/* ann (G = ~G & S)
 * set-flags: undefined (*x), yes (*z) */

#define annzx_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzx_ri(W(RG), W(IS))

#define annzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define annzx_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzx_rr(W(RG), W(RS))

#define annzx_ld(RG, MS, DS)                                                \
        notzx_rx(W(RG))                                                     \
        andzx_ld(W(RG), W(MS), W(DS))

#define annzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define annzx_mr(MG, DG, RS)                                                \
        annzx_st(W(RS), W(MG), W(DG))


#define annzz_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzz_ri(W(RG), W(IS))

#define annzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x30000000) | (+(TP2(IS) != 0) & 0x00000024))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define annzz_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        andzz_rr(W(RG), W(RS))

#define annzz_ld(RG, MS, DS)                                                \
        notzx_rx(W(RG))                                                     \
        andzz_ld(W(RG), W(MS), W(DS))

#define annzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000024 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define annzz_mr(MG, DG, RS)                                                \
        annzz_st(W(RS), W(MG), W(DG))

/* orr (G = G | S)
 * set-flags: undefined (*x), yes (*z) */

#define orrzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        /* if true ^ equals to -1 (not 1) */

#define orrzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define orrzx_rr(RG, RS)                                                    \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), REG(RS)))

#define orrzx_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), TMxx))

#define orrzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define orrzx_mr(MG, DG, RS)                                                \
        orrzx_st(W(RS), W(MG), W(DG))


#define orrzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define orrzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define orrzz_rr(RG, RS)                                                    \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define orrzz_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000025 | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define orrzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define orrzz_mr(MG, DG, RS)                                                \
        orrzz_st(W(RS), W(MG), W(DG))

/* orn (G = ~G | S)
 * set-flags: undefined (*x), yes (*z) */

#define ornzx_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzx_ri(W(RG), W(IS))

#define ornzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define ornzx_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzx_rr(W(RG), W(RS))

#define ornzx_ld(RG, MS, DS)                                                \
        notzx_rx(W(RG))                                                     \
        orrzx_ld(W(RG), W(MS), W(DS))

#define ornzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define ornzx_mr(MG, DG, RS)                                                \
        ornzx_st(W(RS), W(MG), W(DG))


#define ornzz_ri(RG, IS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzz_ri(W(RG), W(IS))

#define ornzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x34000000) | (+(TP2(IS) != 0) & 0x00000025))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define ornzz_rr(RG, RS)                                                    \
        notzx_rx(W(RG))                                                     \
        orrzz_rr(W(RG), W(RS))

#define ornzz_ld(RG, MS, DS)                                                \
        notzx_rx(W(RG))                                                     \
        orrzz_ld(W(RG), W(MS), W(DS))

#define ornzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0x00000025 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define ornzz_mr(MG, DG, RS)                                                \
        ornzz_st(W(RS), W(MG), W(DG))

/* xor (G = G ^ S)
 * set-flags: undefined (*x), yes (*z) */

#define xorzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        /* if true ^ equals to -1 (not 1) */

#define xorzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define xorzx_rr(RG, RS)                                                    \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), REG(RS)))

#define xorzx_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), TMxx))

#define xorzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000026 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define xorzx_mr(MG, DG, RS)                                                \
        xorzx_st(W(RS), W(MG), W(DG))


#define xorzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G2(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define xorzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G2(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T2(IS), M2(IS)) | \
        (+(TP2(IS) == 0) & 0x38000000) | (+(TP2(IS) != 0) & 0x00000026))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define xorzz_rr(RG, RS)                                                    \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define xorzz_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000026 | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define xorzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000026 | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define xorzz_mr(MG, DG, RS)                                                \
        xorzz_st(W(RS), W(MG), W(DG))

/* not (G = ~G)
 * set-flags: no */

#define notzx_rx(RG)                                                        \
        EMITW(0x00000027 | MRM(REG(RG), TZxx,    REG(RG)))

#define notzx_mx(MG, DG)                                                    \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TDxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000027 | MRM(TDxx,    TZxx,    TDxx))                     \
        EMITW(0xFC000000 | MDM(TDxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

/* neg (G = -G)
 * set-flags: undefined (*x), yes (*z) */

#define negzx_rx(RG)                                                        \
        EMITW(0x0000002F | MRM(REG(RG), TZxx,    REG(RG)))

#define negzx_mx(MG, DG)                                                    \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x0000002F | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))


#define negzz_rx(RG)                                                        \
        EMITW(0x0000002F | MRM(REG(RG), TZxx,    REG(RG)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define negzz_mx(MG, DG)                                                    \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x0000002F | MRM(TMxx,    TZxx,    TMxx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

/* add (G = G + S)
 * set-flags: undefined (*x), yes (*z) */

#define addzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        /* if true ^ equals to -1 (not 1) */

#define addzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define addzx_rr(RG, RS)                                                    \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), REG(RS)))

#define addzx_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), TMxx))

#define addzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x0000002D | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define addzx_mr(MG, DG, RS)                                                \
        addzx_st(W(RS), W(MG), W(DG))


#define addzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define addzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    VAL(IS), T1(IS), M1(IS)) | \
        (+(TP1(IS) == 0) & 0x64000000) | (+(TP1(IS) != 0) & 0x0000002D))    \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define addzz_rr(RG, RS)                                                    \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define addzz_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000002D | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define addzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x0000002D | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define addzz_mr(MG, DG, RS)                                                \
        addzz_st(W(RS), W(MG), W(DG))

/* sub (G = G - S)
 * set-flags: undefined (*x), yes (*z) */

#define subzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), 0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        /* if true ^ equals to -1 (not 1) */

#define subzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define subzx_rr(RG, RS)                                                    \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), REG(RS)))

#define subzx_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), TMxx))

#define subzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x0000002F | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define subzx_mr(MG, DG, RS)                                                \
        subzx_st(W(RS), W(MG), W(DG))


#define subzz_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G1(IS))   \
        EMITW(0x00000000 | MIM(REG(RG), REG(RG), 0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define subzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  VAL(IS), TIxx,    MOD(MG), VAL(DG), C1(DG), G1(IS))   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MIM(TMxx,    TMxx,    0x00,    T1(IS), EMPTY1) | \
        (+(TP1(IS) == 0) & (0x64000000 | (0xFFFF & -VAL(IS)))) |            \
        (+(TP1(IS) != 0) & (0x0000002F | TIxx << 16)))                      \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define subzz_rr(RG, RS)                                                    \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define subzz_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000002F | MRM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define subzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x0000002F | MRM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define subzz_mr(MG, DG, RS)                                                \
        subzz_st(W(RS), W(MG), W(DG))

/* shl (G = G << S)
 * set-flags: undefined (*x), yes (*z) */

#define shlzx_rx(RG)                     /* reads Recx for shift count */   \
        EMITW(0x00000014 | MSM(REG(RG), REG(RG), Tecx))

#define shlzx_mx(MG, DG)                 /* reads Recx for shift count */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000014 | MSM(TMxx,    TMxx,    Tecx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shlzx_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MSM(REG(RG), REG(RG), 0x00) |                    \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */

#define shlzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MSM(TMxx,    TMxx,    0x00) |                    \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shlzx_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000014 | MSM(REG(RG), REG(RG), REG(RS)))

#define shlzx_ld(RG, MS, DS)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000014 | MSM(REG(RG), REG(RG), TMxx))

#define shlzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000014 | MSM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shlzx_mr(MG, DG, RS)                                                \
        shlzx_st(W(RS), W(MG), W(DG))


#define shlzz_rx(RG)                     /* reads Recx for shift count */   \
        EMITW(0x00000014 | MSM(REG(RG), REG(RG), Tecx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_mx(MG, DG)                 /* reads Recx for shift count */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000014 | MSM(TMxx,    TMxx,    Tecx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shlzz_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MSM(REG(RG), REG(RG), 0x00) |                    \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MSM(TMxx,    TMxx,    0x00) |                    \
        (+(VAL(IS) < 32) & (0x00000038 | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003C | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shlzz_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000014 | MSM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_ld(RG, MS, DS)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000014 | MSM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shlzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000014 | MSM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shlzz_mr(MG, DG, RS)                                                \
        shlzz_st(W(RS), W(MG), W(DG))

/* shr (G = G >> S)
 * set-flags: undefined (*x), yes (*z) */

#define shrzx_rx(RG)                     /* reads Recx for shift count */   \
        EMITW(0x00000016 | MSM(REG(RG), REG(RG), Tecx))

#define shrzx_mx(MG, DG)                 /* reads Recx for shift count */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000016 | MSM(TMxx,    TMxx,    Tecx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzx_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MSM(REG(RG), REG(RG), 0x00) |                    \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */

#define shrzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MSM(TMxx,    TMxx,    0x00) |                    \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzx_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000016 | MSM(REG(RG), REG(RG), REG(RS)))

#define shrzx_ld(RG, MS, DS)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000016 | MSM(REG(RG), REG(RG), TMxx))

#define shrzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000016 | MSM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzx_mr(MG, DG, RS)                                                \
        shrzx_st(W(RS), W(MG), W(DG))


#define shrzz_rx(RG)                     /* reads Recx for shift count */   \
        EMITW(0x00000016 | MSM(REG(RG), REG(RG), Tecx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_mx(MG, DG)                 /* reads Recx for shift count */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000016 | MSM(TMxx,    TMxx,    Tecx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzz_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MSM(REG(RG), REG(RG), 0x00) |                    \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MSM(TMxx,    TMxx,    0x00) |                    \
        (+(VAL(IS) < 32) & (0x0000003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003E | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzz_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000016 | MSM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_ld(RG, MS, DS)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000016 | MSM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define shrzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000016 | MSM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzz_mr(MG, DG, RS)                                                \
        shrzz_st(W(RS), W(MG), W(DG))


#define shrzn_rx(RG)                     /* reads Recx for shift count */   \
        EMITW(0x00000017 | MSM(REG(RG), REG(RG), Tecx))

#define shrzn_mx(MG, DG)                 /* reads Recx for shift count */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000017 | MSM(TMxx,    TMxx,    Tecx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzn_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MSM(REG(RG), REG(RG), 0x00) |                    \
        (+(VAL(IS) < 32) & (0x0000003B | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003F | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */

#define shrzn_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MSM(TMxx,    TMxx,    0x00) |                    \
        (+(VAL(IS) < 32) & (0x0000003B | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0000003F | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzn_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000017 | MSM(REG(RG), REG(RG), REG(RS)))

#define shrzn_ld(RG, MS, DS)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000017 | MSM(REG(RG), REG(RG), TMxx))

#define shrzn_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000017 | MSM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define shrzn_mr(MG, DG, RS)                                                \
        shrzn_st(W(RS), W(MG), W(DG))

/* ror (G = G >> S | G << 64 - S)
 * set-flags: undefined (*x), yes (*z) */

#define rorzx_rx(RG)                     /* reads Recx for shift count */   \
        EMITW(0x00000056 | MSM(REG(RG), REG(RG), Tecx))

#define rorzx_mx(MG, DG)                 /* reads Recx for shift count */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000056 | MSM(TMxx,    TMxx,    Tecx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define rorzx_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MSM(REG(RG), REG(RG), 0x00) |                    \
        (+(VAL(IS) < 32) & (0x0020003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0020003E | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */

#define rorzx_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MSM(TMxx,    TMxx,    0x00) |                    \
        (+(VAL(IS) < 32) & (0x0020003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0020003E | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define rorzx_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000056 | MSM(REG(RG), REG(RG), REG(RS)))

#define rorzx_ld(RG, MS, DS)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000056 | MSM(REG(RG), REG(RG), TMxx))

#define rorzx_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000056 | MSM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define rorzx_mr(MG, DG, RS)                                                \
        rorzx_st(W(RS), W(MG), W(DG))


#define rorzz_rx(RG)                     /* reads Recx for shift count */   \
        EMITW(0x00000056 | MSM(REG(RG), REG(RG), Tecx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define rorzz_mx(MG, DG)                 /* reads Recx for shift count */   \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000056 | MSM(TMxx,    TMxx,    Tecx))                     \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define rorzz_ri(RG, IS)                                                    \
        EMITW(0x00000000 | MSM(REG(RG), REG(RG), 0x00) |                    \
        (+(VAL(IS) < 32) & (0x0020003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0020003E | (0x1F & VAL(IS)) << 6)))           \
        /* if true ^ equals to -1 (not 1) */                                \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define rorzz_mi(MG, DG, IS)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000000 | MSM(TMxx,    TMxx,    0x00) |                    \
        (+(VAL(IS) < 32) & (0x0020003A | (0x1F & VAL(IS)) << 6)) |          \
        (+(VAL(IS) > 31) & (0x0020003E | (0x1F & VAL(IS)) << 6)))           \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define rorzz_rr(RG, RS)       /* Recx cannot be used as first operand */   \
        EMITW(0x00000056 | MSM(REG(RG), REG(RG), REG(RS)))                  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define rorzz_ld(RG, MS, DS)   /* Recx cannot be used as first operand */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000056 | MSM(REG(RG), REG(RG), TMxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RG), TZxx))/* <- set flags (Z) */

#define rorzz_st(RS, MG, DG)                                                \
        AUW(SIB(MG),  EMPTY,  EMPTY,    MOD(MG), VAL(DG), C1(DG), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))  \
        EMITW(0x00000056 | MSM(TMxx,    TMxx,    REG(RS)))                  \
        EMITW(0xFC000000 | MDM(TMxx,    MOD(MG), VAL(DG), B1(DG), P1(DG)))

#define rorzz_mr(MG, DG, RS)                                                \
        rorzz_st(W(RS), W(MG), W(DG))

/* pre-r6 */
#if defined (RT_M64) && RT_M64 < 6

/* mul (G = G * S)
 * set-flags: undefined */

#define mulzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001D | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define mulzx_rr(RG, RS)                                                    \
        EMITW(0x0000001D | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define mulzx_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001D | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))


#define mulzx_xr(RS)     /* Reax is in/out, Redx is out(high)-zero-ext */   \
        EMITW(0x0000001D | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))

#define mulzx_xm(MS, DS) /* Reax is in/out, Redx is out(high)-zero-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001D | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))


#define mulzn_xr(RS)     /* Reax is in/out, Redx is out(high)-sign-ext */   \
        EMITW(0x0000001C | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))

#define mulzn_xm(MS, DS) /* Reax is in/out, Redx is out(high)-sign-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001C | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))                     \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))


#define mulzp_xr(RS)     /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_rr(Reax, W(RS)) /* product must not exceed operands size */

#define mulzp_xm(MS, DS) /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_ld(Reax, W(MS), W(DS))  /* must not exceed operands size */

/* div (G = G / S)
 * set-flags: undefined */

#define divzx_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzx_rr(RG, RS)                /* RG no Reax, RS no Reax/Redx */   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzx_ld(RG, MS, DS)            /* RG no Reax, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))


#define divzn_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzn_rr(RG, RS)                /* RG no Reax, RS no Reax/Redx */   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))

#define divzn_ld(RG, MS, DS)            /* RG no Reax, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000012 | MRM(REG(RG), 0x00,    0x00))


#define prezx_xx()          /* to be placed immediately prior divzx_x* */   \
                                     /* to prepare Redx for int-divide */

#define prezn_xx()          /* to be placed immediately prior divzn_x* */   \
                                     /* to prepare Redx for int-divide */


#define divzx_xr(RS)     /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        EMITW(0x0000001F | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))

#define divzx_xm(MS, DS) /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001F | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))


#define divzn_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        EMITW(0x0000001E | MRM(0x00,    Teax,    REG(RS)))                  \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))

#define divzn_xm(MS, DS) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001E | MRM(0x00,    Teax,    TMxx))                     \
        EMITW(0x00000012 | MRM(Teax,    0x00,    0x00))


#define divzp_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xr(W(RS))              /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

#define divzp_xm(MS, DS) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xm(W(MS), W(DS))       /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

/* rem (G = G % S)
 * set-flags: undefined */

#define remzx_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzx_rr(RG, RS)                /* RG no Redx, RS no Reax/Redx */   \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzx_ld(RG, MS, DS)            /* RG no Redx, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001F | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))


#define remzn_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TIxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzn_rr(RG, RS)                /* RG no Redx, RS no Reax/Redx */   \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), REG(RS)))                  \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))

#define remzn_ld(RG, MS, DS)            /* RG no Redx, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000001E | MRM(0x00,    REG(RG), TMxx))                     \
        EMITW(0x00000010 | MRM(REG(RG), 0x00,    0x00))


#define remzx_xx()          /* to be placed immediately prior divzx_x* */   \

#define remzx_xr(RS)        /* to be placed immediately after divzx_xr */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */

#define remzx_xm(MS, DS)    /* to be placed immediately after divzx_xm */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */


#define remzn_xx()          /* to be placed immediately prior divzn_x* */   \
                                     /* to prepare for rem calculation */

#define remzn_xr(RS)        /* to be placed immediately after divzn_xr */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */

#define remzn_xm(MS, DS)    /* to be placed immediately after divzn_xm */   \
        EMITW(0x00000010 | MRM(Tedx,    0x00,    0x00))   /* Redx<-rem */

#else  /* r6 */

/* mul (G = G * S)
 * set-flags: undefined */

#define mulzx_ri(RG, IS)                                                    \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000009D | MRM(REG(RG), REG(RG), TIxx))

#define mulzx_rr(RG, RS)                                                    \
        EMITW(0x0000009D | MRM(REG(RG), REG(RG), REG(RS)))

#define mulzx_ld(RG, MS, DS)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000009D | MRM(REG(RG), REG(RG), TMxx))


#define mulzx_xr(RS)     /* Reax is in/out, Redx is out(high)-zero-ext */   \
        EMITW(0x000000DD | MRM(Tedx,    Teax,    REG(RS)))                  \
        EMITW(0x0000009D | MRM(Teax,    Teax,    REG(RS)))

#define mulzx_xm(MS, DS) /* Reax is in/out, Redx is out(high)-zero-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x000000DD | MRM(Tedx,    Teax,    TMxx))                     \
        EMITW(0x0000009D | MRM(Teax,    Teax,    TMxx))


#define mulzn_xr(RS)     /* Reax is in/out, Redx is out(high)-sign-ext */   \
        EMITW(0x000000DC | MRM(Tedx,    Teax,    REG(RS)))                  \
        EMITW(0x0000009C | MRM(Teax,    Teax,    REG(RS)))

#define mulzn_xm(MS, DS) /* Reax is in/out, Redx is out(high)-sign-ext */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x000000DC | MRM(Tedx,    Teax,    TMxx))                     \
        EMITW(0x0000009C | MRM(Teax,    Teax,    TMxx))


#define mulzp_xr(RS)     /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_rr(Reax, W(RS)) /* product must not exceed operands size */

#define mulzp_xm(MS, DS) /* Reax is in/out, prepares Redx for divzn_x* */   \
        mulzx_ld(Reax, W(MS), W(DS))  /* must not exceed operands size */

/* div (G = G / S)
 * set-flags: undefined */

#define divzx_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000009F | MRM(REG(RG), REG(RG), TIxx))

#define divzx_rr(RG, RS)                /* RG no Reax, RS no Reax/Redx */   \
        EMITW(0x0000009F | MRM(REG(RG), REG(RG), REG(RS)))

#define divzx_ld(RG, MS, DS)            /* RG no Reax, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000009F | MRM(REG(RG), REG(RG), TMxx))


#define divzn_ri(RG, IS)       /* Reax cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x0000009E | MRM(REG(RG), REG(RG), TIxx))

#define divzn_rr(RG, RS)                /* RG no Reax, RS no Reax/Redx */   \
        EMITW(0x0000009E | MRM(REG(RG), REG(RG), REG(RS)))

#define divzn_ld(RG, MS, DS)            /* RG no Reax, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000009E | MRM(REG(RG), REG(RG), TMxx))


#define prezx_xx()          /* to be placed immediately prior divzx_x* */   \
                                     /* to prepare Redx for int-divide */

#define prezn_xx()          /* to be placed immediately prior divzn_x* */   \
                                     /* to prepare Redx for int-divide */


#define divzx_xr(RS)     /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        EMITW(0x0000009F | MRM(Teax,    Teax,    REG(RS)))

#define divzx_xm(MS, DS) /* Reax is in/out, Redx is in(zero)/out(junk) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000009F | MRM(Teax,    Teax,    TMxx))


#define divzn_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        EMITW(0x0000009E | MRM(Teax,    Teax,    REG(RS)))

#define divzn_xm(MS, DS) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x0000009E | MRM(Teax,    Teax,    TMxx))


#define divzp_xr(RS)     /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xr(W(RS))              /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

#define divzp_xm(MS, DS) /* Reax is in/out, Redx is in-sign-ext-(Reax) */   \
        divzn_xm(W(MS), W(DS))       /* destroys Redx, Xmm0 (in ARMv7) */   \
                                     /* 24-bit int (fp32 div in ARMv7) */

/* rem (G = G % S)
 * set-flags: undefined */

#define remzx_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x000000DF | MRM(REG(RG), REG(RG), TIxx))

#define remzx_rr(RG, RS)                /* RG no Redx, RS no Reax/Redx */   \
        EMITW(0x000000DF | MRM(REG(RG), REG(RG), REG(RS)))

#define remzx_ld(RG, MS, DS)            /* RG no Redx, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x000000DF | MRM(REG(RG), REG(RG), TMxx))


#define remzn_ri(RG, IS)       /* Redx cannot be used as first operand */   \
        AUW(EMPTY,    VAL(IS), TIxx,    EMPTY,   EMPTY,   EMPTY2, G3(IS))   \
        EMITW(0x000000DE | MRM(REG(RG), REG(RG), TIxx))

#define remzn_rr(RG, RS)                /* RG no Redx, RS no Reax/Redx */   \
        EMITW(0x000000DE | MRM(REG(RG), REG(RG), REG(RS)))

#define remzn_ld(RG, MS, DS)            /* RG no Redx, MS no Oeax/Medx */   \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x000000DE | MRM(REG(RG), REG(RG), TMxx))


#define remzx_xx()          /* to be placed immediately prior divzx_x* */   \
        movzx_rr(Redx, Reax)         /* to prepare for rem calculation */

#define remzx_xr(RS)        /* to be placed immediately after divzx_xr */   \
        EMITW(0x000000DF | MRM(Tedx,    Tedx,    REG(RS)))/* Redx<-rem */

#define remzx_xm(MS, DS)    /* to be placed immediately after divzx_xm */   \
        EMITW(0x000000DF | MRM(Tedx,    Tedx,    TMxx))   /* Redx<-rem */


#define remzn_xx()          /* to be placed immediately prior divzn_x* */   \
        movzx_rr(Redx, Reax)         /* to prepare for rem calculation */

#define remzn_xr(RS)        /* to be placed immediately after divzn_xr */   \
        EMITW(0x000000DE | MRM(Tedx,    Tedx,    REG(RS)))/* Redx<-rem */

#define remzn_xm(MS, DS)    /* to be placed immediately after divzn_xm */   \
        EMITW(0x000000DE | MRM(Tedx,    Tedx,    TMxx))   /* Redx<-rem */

#endif /* r6 */

/* arj (G = G op S, if cc G then jump lb)
 * set-flags: undefined
 * refer to individual instruction descriptions
 * to stay within special register limitations */

     /* Definitions for arj's "op" and "cc" parameters
      * are provided in 32-bit rtarch_***.h files. */

#define arjzx_rx(RG, op, cc, lb)                                            \
        AR1(W(RG), op, zx_rx)                                               \
        CMZ(cc, MOD(RG), lb)

#define arjzx_mx(MG, DG, op, cc, lb)                                        \
        AR2(W(MG), W(DG), op, zz_mx)                                        \
        CMZ(cc, $t8,     lb)

#define arjzx_ri(RG, IS, op, cc, lb)                                        \
        AR2(W(RG), W(IS), op, zx_ri)                                        \
        CMZ(cc, MOD(RG), lb)

#define arjzx_mi(MG, DG, IS, op, cc, lb)                                    \
        AR3(W(MG), W(DG), W(IS), op, zz_mi)                                 \
        CMZ(cc, $t8,     lb)

#define arjzx_rr(RG, RS, op, cc, lb)                                        \
        AR2(W(RG), W(RS), op, zx_rr)                                        \
        CMZ(cc, MOD(RG), lb)

#define arjzx_ld(RG, MS, DS, op, cc, lb)                                    \
        AR3(W(RG), W(MS), W(DS), op, zx_ld)                                 \
        CMZ(cc, MOD(RG), lb)

#define arjzx_st(RS, MG, DG, op, cc, lb)                                    \
        AR3(W(RS), W(MG), W(DG), op, zz_st)                                 \
        CMZ(cc, $t8,     lb)

#define arjzx_mr(MG, DG, RS, op, cc, lb)                                    \
        arjzx_st(W(RS), W(MG), W(DG), op, cc, lb)

/* cmj (flags = S ? T, if cc flags then jump lb)
 * set-flags: undefined */

     /* Definitions for cmj's "cc" parameter
      * are provided in 32-bit rtarch_***.h files. */

#define cmjzx_rz(RS, cc, lb)                                                \
        CMZ(cc, MOD(RS), lb)

#define cmjzx_mz(MS, DS, cc, lb)                                            \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        CMZ(cc, $t8,     lb)

#define cmjzx_ri(RS, IT, cc, lb)                                            \
        CMI(cc, MOD(RS), REG(RS), W(IT), lb)

#define cmjzx_mi(MS, DS, IT, cc, lb)                                        \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        CMI(cc, $t8,     TMxx,    W(IT), lb)

#define cmjzx_rr(RS, RT, cc, lb)                                            \
        CMR(cc, MOD(RS), MOD(RT), lb)

#define cmjzx_rm(RS, MT, DT, cc, lb)                                        \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C1(DT), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MT), VAL(DT), B1(DT), P1(DT)))  \
        CMR(cc, MOD(RS), $t8,     lb)

#define cmjzx_mr(MS, DS, RT, cc, lb)                                        \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TMxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        CMR(cc, $t8,     MOD(RT), lb)

/* cmp (flags = S ? T)
 * set-flags: yes */

#define cmpzx_ri(RS, IT)                                                    \
        AUW(EMPTY,    VAL(IT), TRxx,    EMPTY,   EMPTY,   EMPTY2, G3(IT))   \
        EMITW(0x00000025 | MRM(TLxx,    REG(RS), TZxx))

#define cmpzx_mi(MS, DS, IT)                                                \
        AUW(SIB(MS),  VAL(IT), TRxx,    MOD(MS), VAL(DS), C1(DS), G3(IT))   \
        EMITW(0xDC000000 | MDM(TLxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))

#define cmpzx_rr(RS, RT)                                                    \
        EMITW(0x00000025 | MRM(TRxx,    REG(RT), TZxx))                     \
        EMITW(0x00000025 | MRM(TLxx,    REG(RS), TZxx))

#define cmpzx_rm(RS, MT, DT)                                                \
        AUW(SIB(MT),  EMPTY,  EMPTY,    MOD(MT), VAL(DT), C1(DT), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TRxx,    MOD(MT), VAL(DT), B1(DT), P1(DT)))  \
        EMITW(0x00000025 | MRM(TLxx,    REG(RS), TZxx))

#define cmpzx_mr(MS, DS, RT)                                                \
        AUW(SIB(MS),  EMPTY,  EMPTY,    MOD(MS), VAL(DS), C1(DS), EMPTY2)   \
        EMITW(0xDC000000 | MDM(TLxx,    MOD(MS), VAL(DS), B1(DS), P1(DS)))  \
        EMITW(0x00000025 | MRM(TRxx,    REG(RT), TZxx))

/* ver (Mebp/inf_VER = SIMD-version)
 * set-flags: no
 * 0th byte - 128-bit version, 1st byte - 256-bit version,
 * 2nd byte - 512-bit version, 3rd byte - upper, reserved */

     /* verxx_xx() in 32-bit rtarch_***.h files, destroys Reax, ... , Redi */

/************************* address-sized instructions *************************/

/* adr (D = adr S)
 * set-flags: no */

     /* adrxx_ld(RD, MS, DS) is defined in 32-bit rtarch_***.h files */

     /* adrpx_ld(RD, MS, DS) in 32-bit rtarch_***_***.h files, SIMD-aligned */

/************************* pointer-sized instructions *************************/

/* label (D = Reax = adr lb)
 * set-flags: no */

     /* label_ld(lb) is defined in rtarch.h file, loads label to Reax */

     /* label_st(lb, MD, DD) is defined in rtarch.h file, destroys Reax */

/* jmp (if unconditional jump S/lb, else if cc flags then jump lb)
 * set-flags: no
 * maximum byte-address-range for un/conditional jumps is signed 18/16-bit
 * based on minimum natively-encoded offset across supported targets (u/c)
 * MIPS:18-bit, Power:26-bit, AArch32:26-bit, AArch64:28-bit, x86:32-bit /
 * MIPS:18-bit, Power:16-bit, AArch32:26-bit, AArch64:21-bit, x86:32-bit */

     /* jccxx_** is defined in 32-bit rtarch_***.h files */

/************************* register-size instructions *************************/

/* stack (push stack = S, D = pop stack)
 * set-flags: no (sequence cmp/stack_la/jmp is not allowed on MIPS & Power)
 * adjust stack pointer with 8-byte (64-bit) steps on all current targets */

#define stack_st(RS)                                                        \
        EMITW(0x64000000 | MRM(0x00,    SPxx,    SPxx) | (-0x08 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    REG(RS)))

#define stack_ld(RD)                                                        \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    REG(RD)))                  \
        EMITW(0x64000000 | MRM(0x00,    SPxx,    SPxx) | (+0x08 & 0xFFFF))

#define stack_sa()   /* save all, [Reax - RegE] + 8 temps, 22 regs total */ \
        EMITW(0x64000000 | MRM(0x00,    SPxx,    SPxx) | (-0xB0 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Teax) | (+0x00 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Tecx) | (+0x08 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Tedx) | (+0x10 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Tebx) | (+0x18 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Tebp) | (+0x20 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Tesi) | (+0x28 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Tedi) | (+0x30 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Teg8) | (+0x38 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    Teg9) | (+0x40 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TegA) | (+0x48 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TegB) | (+0x50 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TegC) | (+0x58 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TegD) | (+0x60 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TegE) | (+0x68 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TMxx) | (+0x70 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TIxx) | (+0x78 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TDxx) | (+0x80 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TPxx) | (+0x88 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,    TNxx) | (+0x90 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,  1+TNxx) | (+0x98 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,  2+TNxx) | (+0xA0 & 0xFFFF))  \
        EMITW(0xFC000000 | MRM(0x00,    SPxx,  3+TNxx) | (+0xA8 & 0xFFFF))

#define stack_la()   /* load all, 8 temps + [RegE - Reax], 22 regs total */ \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,  3+TNxx) | (+0xA8 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,  2+TNxx) | (+0xA0 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,  1+TNxx) | (+0x98 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TNxx) | (+0x90 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TPxx) | (+0x88 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TDxx) | (+0x80 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TIxx) | (+0x78 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TMxx) | (+0x70 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TegE) | (+0x68 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TegD) | (+0x60 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TegC) | (+0x58 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TegB) | (+0x50 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    TegA) | (+0x48 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Teg9) | (+0x40 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Teg8) | (+0x38 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Tedi) | (+0x30 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Tesi) | (+0x28 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Tebp) | (+0x20 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Tebx) | (+0x18 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Tedx) | (+0x10 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Tecx) | (+0x08 & 0xFFFF))  \
        EMITW(0xDC000000 | MRM(0x00,    SPxx,    Teax) | (+0x00 & 0xFFFF))  \
        EMITW(0x64000000 | MRM(0x00,    SPxx,    SPxx) | (+0xB0 & 0xFFFF))

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#endif /* defined (RT_M64) */

#endif /* RT_RTARCH_M64_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
