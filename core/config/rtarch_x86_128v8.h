/******************************************************************************/
/* Copyright (c) 2013-2016 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_X86_128V8_H
#define RT_RTARCH_X86_128V8_H

#include "rtarch_x86.h"

#define RT_SIMD_REGS        8
#define RT_SIMD_ALIGN       16
#define RT_SIMD_WIDTH64     2
#define RT_SIMD_SET64(s, v) s[0]=s[1]=v
#define RT_SIMD_WIDTH32     4
#define RT_SIMD_SET32(s, v) s[0]=s[1]=s[2]=s[3]=v

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_x86_128v8.h: Implementation of x86:i386 fp32 AVX1/2 instructions.
 *
 * This file is a part of the unified SIMD assembler framework (rtarch.h)
 * designed to be compatible with different processor architectures,
 * while maintaining strictly defined common API.
 *
 * Recommended naming scheme for instructions:
 *
 * cmdp*_ri - applies [cmd] to [p]acked: [r]egister from [i]mmediate
 * cmdp*_rr - applies [cmd] to [p]acked: [r]egister from [r]egister
 *
 * cmdp*_rm - applies [cmd] to [p]acked: [r]egister from [m]emory
 * cmdp*_ld - applies [cmd] to [p]acked: as above
 *
 * cmdpx_** - applies [cmd] to [p]acked unsigned integer args, [x] - default
 * cmdpn_** - applies [cmd] to [p]acked   signed integer args, [n] - negatable
 * cmdps_** - applies [cmd] to [p]acked floating point   args, [s] - scalable
 *
 * cmdo*_** - applies [cmd] to 32-bit SIMD register/memory/immediate args
 * cmdp*_** - applies [cmd] to L-size SIMD register/memory/immediate args
 * cmdq*_** - applies [cmd] to 64-bit SIMD register/memory/immediate args
 *
 * The cmdp*_** (rtbase.h) instructions are intended for SPMD programming model
 * and can be configured to work with 32/64-bit data-elements (int, fp).
 * In this model data-paths are fixed-width, BASE and SIMD data-elements are
 * width-compatible, code-path divergence is handled via CHECK_MASK macro.
 * Matching element-sized BASE subset cmdy*_** is defined in rtbase.h.
 *
 * Interpretation of instruction parameters:
 *
 * upper-case params have triplet structure and require W to pass-forward
 * lower-case params are singular and can be used/passed as such directly
 *
 * XD - SIMD register serving as destination only, if present
 * XG - SIMD register serving as destination and fisrt source
 * XS - SIMD register serving as second source (first if any)
 * XT - SIMD register serving as third source (second if any)
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
 */

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#if defined (RT_SIMD_CODE)

#if defined (RT_128) && (RT_128 >= 8)

#define K 0

/* mandatory escape prefix for some opcodes */
#define ESC                                                                 \
        EMITB(0x66)

/* mandatory escape prefix for some opcodes */
#define xF3                                                                 \
        EMITB(0xF3)

/* fwait instruction for legacy processors (fix for fstcw) */
#define FWT                                                                 \
        EMITB(0x9B)

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/* registers    REG,  MOD,  SIB */

#define Xmm0    0x00, 0x03, EMPTY
#define Xmm1    0x01, 0x03, EMPTY
#define Xmm2    0x02, 0x03, EMPTY
#define Xmm3    0x03, 0x03, EMPTY
#define Xmm4    0x04, 0x03, EMPTY
#define Xmm5    0x05, 0x03, EMPTY
#define Xmm6    0x06, 0x03, EMPTY
#define Xmm7    0x07, 0x03, EMPTY

/******************************************************************************/
/**********************************   AVX   ***********************************/
/******************************************************************************/

/* adr (D = adr S) */

#define adrpx_ld(RD, MS, DS) /* RD is a BASE reg, MS/DS is SIMD-aligned */  \
        EMITB(0x8D)                                                         \
        MRM(REG(RD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/**************************   packed generic (AVX1)   *************************/

/* mov (D = S) */

#define movix_rr(XD, XS)                                                    \
        V2X(0x00,    K, 0) EMITB(0x28)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movix_ld(XD, MS, DS)                                                \
        V2X(0x00,    K, 0) EMITB(0x28)                                      \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define movix_st(XS, MD, DD)                                                \
        V2X(0x00,    K, 0) EMITB(0x29)                                      \
        MRM(REG(XS), MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

/* mmv (G = G mask-merge S, mask: 0 - keeps G, 1 - picks S with elem-size frag)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, XS unmasked frags */

#define mmvix_rr(XG, XS)                                                    \
        VEX(REN(XG), K, 1, 3) EMITB(0x4A)                                   \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))

#define mmvix_ld(XG, MS, DS)                                                \
        VEX(REN(XG), K, 1, 3) EMITB(0x4A)                                   \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x00))

#define mmvix_st(XS, MG, DG)                                                \
        VEX(0x00,    K, 1, 2) EMITB(0x2E)                                   \
        MRM(REG(XS), MOD(MG), REG(MG))                                      \
        AUX(SIB(MG), CMD(DG), EMPTY)

/* and (G = G & S) */

#define andix_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x54)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define andix_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x54)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* ann (G = ~G & S) */

#define annix_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x55)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define annix_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x55)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* orr (G = G | S) */

#define orrix_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x56)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define orrix_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x56)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* orn (G = ~G | S) */

#define ornix_rr(XG, XS)                                                    \
        notix_rx(W(XG))                                                     \
        orrix_rr(W(XG), W(XS))

#define ornix_ld(XG, MS, DS)                                                \
        notix_rx(W(XG))                                                     \
        orrix_ld(W(XG), W(MS), W(DS))

/* xor (G = G ^ S) */

#define xorix_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x57)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define xorix_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x57)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* not (G = ~G) */

#define notix_rx(XG)                                                        \
        annix_ld(W(XG), Mebp, inf_GPC07)

/**************   packed single precision floating point (AVX1)   *************/

/* neg (G = -G) */

#define negis_rx(XG)                                                        \
        xorix_ld(W(XG), Mebp, inf_GPC06_32)

/* add (G = G + S) */

#define addis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x58)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define addis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x58)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* sub (G = G - S) */

#define subis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x5C)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define subis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x5C)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* mul (G = G * S) */

#define mulis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x59)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mulis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x59)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* div (G = G / S) */

#define divis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x5E)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define divis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x5E)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* sqr (D = sqrt S) */

#define sqris_rr(XD, XS)                                                    \
        V2X(0x00,    K, 0) EMITB(0x51)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define sqris_ld(XD, MS, DS)                                                \
        V2X(0x00,    K, 0) EMITB(0x51)                                      \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cbr (D = cbrt S) */

        /* cbe, cbs, cbr defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RCP != 1

#define rceis_rr(XD, XS)                                                    \
        V2X(0x00,    K, 0) EMITB(0x53)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rcsis_rr(XG, XS) /* destroys XS */                                  \
        mulis_rr(W(XS), W(XG))                                              \
        mulis_rr(W(XS), W(XG))                                              \
        addis_rr(W(XG), W(XG))                                              \
        subis_rr(W(XG), W(XS))

#endif /* RT_SIMD_COMPAT_RCP */

        /* rcp defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RSQ != 1

#define rseis_rr(XD, XS)                                                    \
        V2X(0x00,    K, 0) EMITB(0x52)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define rssis_rr(XG, XS) /* destroys XS */                                  \
        mulis_rr(W(XS), W(XG))                                              \
        mulis_rr(W(XS), W(XG))                                              \
        subis_ld(W(XS), Mebp, inf_GPC03_32)                                 \
        mulis_ld(W(XS), Mebp, inf_GPC02_32)                                 \
        mulis_rr(W(XG), W(XS))

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rsq defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

#if defined (RT_256) && (RT_256 < 2) || \
    defined (RT_128) && (RT_SIMD_COMPAT_128 == 1)

#define cvycs_rr(XD, XS)     /* not portable, do not use outside */         \
        V2X(0x00,    1, 0) EMITB(0x5A)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvycs_ld(XD, MS, DS) /* not portable, do not use outside */         \
        V2X(0x00,    1, 0) EMITB(0x5A)                                      \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define cvxds_rr(XD, XS)     /* not portable, do not use outside */         \
        V2X(0x00,    1, 1) EMITB(0x5A)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))


#define addds_rr(XG, XS)     /* not portable, do not use outside */         \
        V2X(REG(XG), 1, 1) EMITB(0x58)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define subds_rr(XG, XS)     /* not portable, do not use outside */         \
        V2X(REG(XG), 1, 1) EMITB(0x5C)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mulds_rr(XG, XS)     /* not portable, do not use outside */         \
        V2X(REG(XG), 1, 1) EMITB(0x59)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#if RT_SIMD_COMPAT_FMA == 0

/* fma (G = G + S * T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#define fmais_rr(XG, XS, XT)                                                \
        movix_st(W(XS), Mebp, inf_SCR01(0))                                 \
        mulis_rr(W(XS), W(XT))                                              \
        addis_rr(W(XG), W(XS))                                              \
        movix_ld(W(XS), Mebp, inf_SCR01(0))

#define fmais_ld(XG, XS, MT, DT)                                            \
        movix_st(W(XS), Mebp, inf_SCR01(0))                                 \
        mulis_ld(W(XS), W(MT), W(DT))                                       \
        addis_rr(W(XG), W(XS))                                              \
        movix_ld(W(XS), Mebp, inf_SCR01(0))

#elif RT_SIMD_COMPAT_FMA == 1

/* fma (G = G + S * T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#define fmais_rr(XG, XS, XT)                                                \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        cvycs_rr(W(XG), W(XS))                     /* 1st-pass -> */        \
        movix_st(W(XS), Mebp, inf_SCR02(0))                                 \
        cvycs_rr(W(XS), W(XT))                                              \
        mulds_rr(W(XS), W(XG))                                              \
        cvycs_ld(W(XG), Mebp, inf_SCR01(0))                                 \
        addds_rr(W(XG), W(XS))                                              \
        cvxds_rr(W(XG), W(XG))                     /* 1st-pass <- */        \
        movix_ld(W(XS), Mebp, inf_SCR02(0))

#define fmais_ld(XG, XS, MT, DT)                                            \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        cvycs_rr(W(XG), W(XS))                     /* 1st-pass -> */        \
        movix_st(W(XS), Mebp, inf_SCR02(0))                                 \
        cvycs_ld(W(XS), W(MT), W(DT))                                       \
        mulds_rr(W(XS), W(XG))                                              \
        cvycs_ld(W(XG), Mebp, inf_SCR01(0))                                 \
        addds_rr(W(XG), W(XS))                                              \
        cvxds_rr(W(XG), W(XG))                     /* 1st-pass <- */        \
        movix_ld(W(XS), Mebp, inf_SCR02(0))

#endif /* RT_SIMD_COMPAT_FMA */

#if RT_SIMD_COMPAT_FMS == 0

/* fms (G = G - S * T)
 * NOTE: due to final negation being outside of rounding on all Power systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#define fmsis_rr(XG, XS, XT)                                                \
        movix_st(W(XS), Mebp, inf_SCR01(0))                                 \
        mulis_rr(W(XS), W(XT))                                              \
        subis_rr(W(XG), W(XS))                                              \
        movix_ld(W(XS), Mebp, inf_SCR01(0))

#define fmsis_ld(XG, XS, MT, DT)                                            \
        movix_st(W(XS), Mebp, inf_SCR01(0))                                 \
        mulis_ld(W(XS), W(MT), W(DT))                                       \
        subis_rr(W(XG), W(XS))                                              \
        movix_ld(W(XS), Mebp, inf_SCR01(0))

#elif RT_SIMD_COMPAT_FMS == 1

/* fms (G = G - S * T)
 * NOTE: due to final negation being outside of rounding on all Power systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#define fmsis_rr(XG, XS, XT)                                                \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        cvycs_rr(W(XG), W(XS))                     /* 1st-pass -> */        \
        movix_st(W(XS), Mebp, inf_SCR02(0))                                 \
        cvycs_rr(W(XS), W(XT))                                              \
        mulds_rr(W(XS), W(XG))                                              \
        cvycs_ld(W(XG), Mebp, inf_SCR01(0))                                 \
        subds_rr(W(XG), W(XS))                                              \
        cvxds_rr(W(XG), W(XG))                     /* 1st-pass <- */        \
        movix_ld(W(XS), Mebp, inf_SCR02(0))

#define fmsis_ld(XG, XS, MT, DT)                                            \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        cvycs_rr(W(XG), W(XS))                     /* 1st-pass -> */        \
        movix_st(W(XS), Mebp, inf_SCR02(0))                                 \
        cvycs_ld(W(XS), W(MT), W(DT))                                       \
        mulds_rr(W(XS), W(XG))                                              \
        cvycs_ld(W(XG), Mebp, inf_SCR01(0))                                 \
        subds_rr(W(XG), W(XS))                                              \
        cvxds_rr(W(XG), W(XG))                     /* 1st-pass <- */        \
        movix_ld(W(XS), Mebp, inf_SCR02(0))

#endif /* RT_SIMD_COMPAT_FMS */

#else /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */ /* FMA comes with AVX2 */

/* fma (G = G + S * T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmais_rr(XG, XS, XT)                                                \
        VEX(REG(XS), K, 1, 2) EMITB(0xB8)                                   \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmais_ld(XG, XS, MT, DT)                                            \
        VEX(REG(XS), K, 1, 2) EMITB(0xB8)                                   \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T)
 * NOTE: due to final negation being outside of rounding on all Power systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmsis_rr(XG, XS, XT)                                                \
        VEX(REG(XS), K, 1, 2) EMITB(0xBC)                                   \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmsis_ld(XG, XS, MT, DT)                                            \
        VEX(REG(XS), K, 1, 2) EMITB(0xBC)                                   \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMS */

#endif /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */

/* min (G = G < S ? G : S) */

#define minis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x5D)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define minis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x5D)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* max (G = G > S ? G : S) */

#define maxis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0x5F)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define maxis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0x5F)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cmp (G = G ? S) */

#define ceqis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))

#define ceqis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x00))

#define cneis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))

#define cneis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x04))

#define cltis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))

#define cltis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x01))

#define cleis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))

#define cleis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x02))

#define cgtis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))

#define cgtis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x06))

#define cgeis_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))

#define cgeis_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 0) EMITB(0xC2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x05))

/* cvz (D = fp-to-signed-int S)
 * rounding mode is encoded directly (can be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnzis_rr(XD, XS)     /* round towards zero */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x03))

#define rnzis_ld(XD, MS, DS) /* round towards zero */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x03))

#define cvzis_rr(XD, XS)     /* round towards zero */                       \
        V2X(0x00,    K, 2) EMITB(0x5B)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvzis_ld(XD, MS, DS) /* round towards zero */                       \
        V2X(0x00,    K, 2) EMITB(0x5B)                                      \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvp (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnpis_rr(XD, XS)     /* round towards +inf */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))

#define rnpis_ld(XD, MS, DS) /* round towards +inf */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x02))

#define cvpis_rr(XD, XS)     /* round towards +inf */                       \
        rnpis_rr(W(XD), W(XS))                                              \
        cvzis_rr(W(XD), W(XD))

#define cvpis_ld(XD, MS, DS) /* round towards +inf */                       \
        rnpis_ld(W(XD), W(MS), W(DS))                                       \
        cvzis_rr(W(XD), W(XD))

/* cvm (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnmis_rr(XD, XS)     /* round towards -inf */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))

#define rnmis_ld(XD, MS, DS) /* round towards -inf */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x01))

#define cvmis_rr(XD, XS)     /* round towards -inf */                       \
        rnmis_rr(W(XD), W(XS))                                              \
        cvzis_rr(W(XD), W(XD))

#define cvmis_ld(XD, MS, DS) /* round towards -inf */                       \
        rnmis_ld(W(XD), W(MS), W(DS))                                       \
        cvzis_rr(W(XD), W(XD))

/* cvn (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnnis_rr(XD, XS)     /* round towards near */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))

#define rnnis_ld(XD, MS, DS) /* round towards near */                       \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x00))

#define cvnis_rr(XD, XS)     /* round towards near */                       \
        cvtis_rr(W(XD), W(XS))

#define cvnis_ld(XD, MS, DS) /* round towards near */                       \
        cvtis_ld(W(XD), W(MS), W(DS))

/* cvn (D = signed-int-to-fp S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks) */

#define cvnin_rr(XD, XS)     /* round towards near */                       \
        cvtin_rr(W(XD), W(XS))

#define cvnin_ld(XD, MS, DS) /* round towards near */                       \
        cvtin_ld(W(XD), W(MS), W(DS))

/**************************   extended float (x87)   **************************/

#define fpuws_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD9)                                                         \
        MRM(0x00,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define fpuws_st(MD, DD) /* not portable, do not use outside */             \
        EMITB(0xD9)                                                         \
        MRM(0x03,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)


#define fpuwn_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xDB)                                                         \
        MRM(0x00,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define fpuwn_st(MD, DD) /* not portable, do not use outside */             \
        EMITB(0xDB)                                                         \
        MRM(0x03,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

#define fpuwt_st(MD, DD) /* not portable, do not use outside */             \
        EMITB(0xDB)                                                         \
        MRM(0x01,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)


#define addws_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD8)                                                         \
        MRM(0x00,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define subws_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD8)                                                         \
        MRM(0x04,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define sbrws_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD8)                                                         \
        MRM(0x05,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)


#define mulws_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD8)                                                         \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define divws_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD8)                                                         \
        MRM(0x06,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define dvrws_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD8)                                                         \
        MRM(0x07,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)


#define addes_xn(n)      /* ST(0) = ST(0) + ST(n), don't pop */             \
        EMITB(0xD8) EMITB(0xC0+(n))

#define addes_nx(n)      /* ST(n) = ST(n) + ST(0), don't pop */             \
        EMITB(0xDC) EMITB(0xC0+(n))

#define addes_np(n)      /* ST(n) = ST(n) + ST(0), pop stack */             \
        EMITB(0xDE) EMITB(0xC0+(n))


#define subes_xn(n)      /* ST(0) = ST(0) - ST(n), don't pop */             \
        EMITB(0xD8) EMITB(0xE0+(n))

#define subes_nx(n)      /* ST(n) = ST(n) - ST(0), don't pop */             \
        EMITB(0xDC) EMITB(0xE8+(n))

#define subes_np(n)      /* ST(n) = ST(n) - ST(0), pop stack */             \
        EMITB(0xDE) EMITB(0xE8+(n))


#define sbres_xn(n)      /* ST(0) = ST(n) - ST(0), don't pop */             \
        EMITB(0xD8) EMITB(0xE8+(n))

#define sbres_nx(n)      /* ST(n) = ST(0) - ST(n), don't pop */             \
        EMITB(0xDC) EMITB(0xE0+(n))

#define sbres_np(n)      /* ST(n) = ST(0) - ST(n), pop stack */             \
        EMITB(0xDE) EMITB(0xE0+(n))


#define mules_xn(n)      /* ST(0) = ST(0) * ST(n), don't pop */             \
        EMITB(0xD8) EMITB(0xC8+(n))

#define mules_nx(n)      /* ST(n) = ST(n) * ST(0), don't pop */             \
        EMITB(0xDC) EMITB(0xC8+(n))

#define mules_np(n)      /* ST(n) = ST(n) * ST(0), pop stack */             \
        EMITB(0xDE) EMITB(0xC8+(n))


#define dives_xn(n)      /* ST(0) = ST(0) / ST(n), don't pop */             \
        EMITB(0xD8) EMITB(0xF0+(n))

#define dives_nx(n)      /* ST(n) = ST(n) / ST(0), don't pop */             \
        EMITB(0xDC) EMITB(0xF8+(n))

#define dives_np(n)      /* ST(n) = ST(n) / ST(0), pop stack */             \
        EMITB(0xDE) EMITB(0xF8+(n))


#define dvres_xn(n)      /* ST(0) = ST(n) / ST(0), don't pop */             \
        EMITB(0xD8) EMITB(0xF8+(n))

#define dvres_nx(n)      /* ST(n) = ST(0) / ST(n), don't pop */             \
        EMITB(0xDC) EMITB(0xF0+(n))

#define dvres_np(n)      /* ST(n) = ST(0) / ST(n), pop stack */             \
        EMITB(0xDE) EMITB(0xF0+(n))


#define sqres_xx()       /* ST(0) = sqr ST(0), don't pop */                 \
        EMITB(0xD9) EMITB(0xFA)

#define neges_xx()       /* ST(0) = neg ST(0), don't pop */                 \
        EMITB(0xD9) EMITB(0xE0)

#define abses_xx()       /* ST(0) = abs ST(0), don't pop */                 \
        EMITB(0xD9) EMITB(0xE1)


#define xm2es_xx()       /* ST(0) = 2^ST(0)-1, don't pop, [-1.0 : +1.0] */  \
        EMITB(0xD9) EMITB(0xF0)

#define lg2es_xx()       /* ST(1) = ST(1)*lg2 ST(0), pop stack */           \
        EMITB(0xD9) EMITB(0xF1)

#define lp2es_xx()       /* ST(1) = ST(1)*lg2 ST(0)+1.0, pop stack */       \
        EMITB(0xD9) EMITB(0xF9)


#define sines_xx()       /* ST(0) = sin ST(0), don't pop, [-2^63:+2^63] */  \
        EMITB(0xD9) EMITB(0xFE)

#define coses_xx()       /* ST(0) = cos ST(0), don't pop, [-2^63:+2^63] */  \
        EMITB(0xD9) EMITB(0xFF)

#define scses_xx()       /* ST(0) = sin ST(0), push cos ST(0), original */  \
        EMITB(0xD9) EMITB(0xFB)

#define tanes_xx()       /* ST(0) = tan ST(0), push +1.0, [-2^63:+2^63] */  \
        EMITB(0xD9) EMITB(0xF2)

#define atnes_xx()       /* ST(1) = atn ST(1)/ST(0), pop stack */           \
        EMITB(0xD9) EMITB(0xF3)


#define remes_xx()       /* ST(0) = ST(0)-Q*ST(1), Q = rnd ST(0)/ST(1) */   \
        EMITB(0xD9) EMITB(0xF5)

#define rexes_xx()       /* ST(0) = ST(0)-Q*ST(1), Q = trn ST(0)/ST(1) */   \
        EMITB(0xD9) EMITB(0xF8)

#define rndes_xx()       /* ST(0) = rnd ST(0), round to integral value */   \
        EMITB(0xD9) EMITB(0xFC)

#define extes_xx()       /* ST(0) = exp ST(0), push mts ST(0) */            \
        EMITB(0xD9) EMITB(0xF4)

#define scles_xx()       /* exp ST(0) = exp ST(0) + rnd ST(1) */            \
        EMITB(0xD9) EMITB(0xFD)


#define cmpes_xn(n)      /* flags = ST(0) ? ST(n), don't pop */             \
        EMITB(0xDB) EMITB(0xF0+(n))

#define cmpes_pn(n)      /* flags = ST(0) ? ST(n), pop stack */             \
        EMITB(0xDF) EMITB(0xF0+(n))

#define moves_nx(n)      /* ST(n) = ST(0), don't pop */                     \
        EMITB(0xDD) EMITB(0xD0+(n))

#define moves_np(n)      /* ST(n) = ST(0), pop stack */                     \
        EMITB(0xDD) EMITB(0xD8+(n))

#define popes_xx()       /* ST(0) = ST(0), pop stack */                     \
        EMITB(0xDD) EMITB(0xD8)


#define fpucw_ld(MS, DS) /* not portable, do not use outside */             \
        EMITB(0xD9)                                                         \
        MRM(0x05,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define fpucw_st(MD, DD) /* not portable, do not use outside */             \
    FWT EMITB(0xD9)                                                         \
        MRM(0x07,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)


#define fpurz_xx()       /* not portable, do not use outside */             \
        fpucw_st(Mebp,  inf_SCR02(4))                                       \
        movwx_mi(Mebp,  inf_SCR02(0), IH(0x0C7F))                           \
        fpucw_ld(Mebp,  inf_SCR02(0))

#define fpurp_xx()       /* not portable, do not use outside */             \
        fpucw_st(Mebp,  inf_SCR02(4))                                       \
        movwx_mi(Mebp,  inf_SCR02(0), IH(0x087F))                           \
        fpucw_ld(Mebp,  inf_SCR02(0))

#define fpurm_xx()       /* not portable, do not use outside */             \
        fpucw_st(Mebp,  inf_SCR02(4))                                       \
        movwx_mi(Mebp,  inf_SCR02(0), IH(0x047F))                           \
        fpucw_ld(Mebp,  inf_SCR02(0))

#define fpurn_xx()       /* not portable, do not use outside */             \
        fpucw_ld(Mebp,  inf_SCR02(4))

/**************************   packed integer (AVX1)   *************************/

/* add (G = G + S) */

#define addix_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 1) EMITB(0xFE)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define addix_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 1) EMITB(0xFE)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* sub (G = G - S) */

#define subix_rr(XG, XS)                                                    \
        V2X(REG(XG), K, 1) EMITB(0xFA)                                      \
        MRM(REG(XG), MOD(XS), REG(XS))

#define subix_ld(XG, MS, DS)                                                \
        V2X(REG(XG), K, 1) EMITB(0xFA)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* shl (G = G << S)
 * for maximum compatibility, shift count mustn't exceed elem-size */

#define shlix_ri(XG, IS)                                                    \
        V2X(REG(XG), K, 1) EMITB(0x72)                                      \
        MRM(0x06,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS) & 0x1F))

#define shlix_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        V2X(REG(XG), K, 1) EMITB(0xF2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#if defined (RT_256) && (RT_256 < 2) || \
    defined (RT_128) && (RT_SIMD_COMPAT_128 == 1)

#define svlix_rr(XG, XS)     /* variable shift with per-elem count */       \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        movix_st(W(XS), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        stack_ld(Recx)                                                      \
        movix_ld(W(XG), Mebp, inf_SCR01(0))

#define svlix_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        movix_ld(W(XG), W(MS), W(DS))                                       \
        movix_st(W(XG), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shlwx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        stack_ld(Recx)                                                      \
        movix_ld(W(XG), Mebp, inf_SCR01(0))

#else /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */

#define svlix_rr(XG, XS)     /* variable shift with per-elem count */       \
        VEX(REG(XG), K, 1, 2) EMITB(0x47)                                   \
        MRM(REG(XG), MOD(XS), REG(XS))

#define svlix_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        VEX(REG(XG), K, 1, 2) EMITB(0x47)                                   \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#endif /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */

/* shr (G = G >> S)
 * for maximum compatibility, shift count mustn't exceed elem-size */

#define shrix_ri(XG, IS)                                                    \
        V2X(REG(XG), K, 1) EMITB(0x72)                                      \
        MRM(0x02,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS) & 0x1F))

#define shrix_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        V2X(REG(XG), K, 1) EMITB(0xD2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#if defined (RT_256) && (RT_256 < 2) || \
    defined (RT_128) && (RT_SIMD_COMPAT_128 == 1)

#define svrix_rr(XG, XS)     /* variable shift with per-elem count */       \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        movix_st(W(XS), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        stack_ld(Recx)                                                      \
        movix_ld(W(XG), Mebp, inf_SCR01(0))

#define svrix_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        movix_ld(W(XG), W(MS), W(DS))                                       \
        movix_st(W(XG), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrwx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        stack_ld(Recx)                                                      \
        movix_ld(W(XG), Mebp, inf_SCR01(0))

#else /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */

#define svrix_rr(XG, XS)     /* variable shift with per-elem count */       \
        VEX(REG(XG), K, 1, 2) EMITB(0x45)                                   \
        MRM(REG(XG), MOD(XS), REG(XS))

#define svrix_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        VEX(REG(XG), K, 1, 2) EMITB(0x45)                                   \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#endif /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */


#define shrin_ri(XG, IS)                                                    \
        V2X(REG(XG), K, 1) EMITB(0x72)                                      \
        MRM(0x04,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS) & 0x1F))

#define shrin_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        V2X(REG(XG), K, 1) EMITB(0xE2)                                      \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#if defined (RT_256) && (RT_256 < 2) || \
    defined (RT_128) && (RT_SIMD_COMPAT_128 == 1)

#define svrin_rr(XG, XS)     /* variable shift with per-elem count */       \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        movix_st(W(XS), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x00))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x04))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x08))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x0C))                                    \
        stack_ld(Recx)                                                      \
        movix_ld(W(XG), Mebp, inf_SCR01(0))

#define svrin_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        movix_st(W(XG), Mebp, inf_SCR01(0))                                 \
        movix_ld(W(XG), W(MS), W(DS))                                       \
        movix_st(W(XG), Mebp, inf_SCR02(0))                                 \
        stack_st(Recx)                                                      \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x00))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x04))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x08))                                    \
        movwx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrwn_mx(Mebp,  inf_SCR01(0x0C))                                    \
        stack_ld(Recx)                                                      \
        movix_ld(W(XG), Mebp, inf_SCR01(0))

#else /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */

#define svrin_rr(XG, XS)     /* variable shift with per-elem count */       \
        VEX(REG(XG), K, 1, 2) EMITB(0x46)                                   \
        MRM(REG(XG), MOD(XS), REG(XS))

#define svrin_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        VEX(REG(XG), K, 1, 2) EMITB(0x46)                                   \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#endif /* (RT_SIMD_COMPAT_128 == 2) || RT_256 >= 2 */

/**************************   helper macros (AVX1)   **************************/

/* simd mask
 * compatibility with AVX-512 and ARM-SVE can be achieved by always keeping
 * one hidden SIMD register holding all 1s and using one hidden mask register
 * first in cmp (c**ps) to produce compatible result in target SIMD register
 * then in CHECK_MASK to facilitate branching on a given condition value */

#define RT_SIMD_MASK_NONE       0x00        /* none satisfy the condition */
#define RT_SIMD_MASK_FULL       0x0F+K*0xF0 /*  all satisfy the condition */

#define movsn_rr(RD, XS) /* not portable, do not use outside */             \
        V2X(0x00,    K, 0) EMITB(0x50)                                      \
        MRM(REG(RD), MOD(XS), REG(XS))

#define CHECK_MASK(lb, mask, XS) /* destroys Reax, jump lb if mask == S */  \
        movsn_rr(Reax, W(XS))                                               \
        cmpwx_ri(Reax, IH(RT_SIMD_MASK_##mask))                             \
        jeqxx_lb(lb)

/* simd mode
 * set via FCTRL macros, *_F for faster non-IEEE mode (optional on MIPS/Power),
 * original FCTRL blocks (FCTRL_ENTER/FCTRL_LEAVE) are defined in rtbase.h
 * NOTE: ARMv7 always uses ROUNDN non-IEEE mode for SIMD fp-arithmetic,
 * while fp<->int conversion takes ROUND* into account via VFP fallback */

#if RT_SIMD_FLUSH_ZERO == 0

#define RT_SIMD_MODE_ROUNDN     0x00    /* round towards near */
#define RT_SIMD_MODE_ROUNDM     0x01    /* round towards -inf */
#define RT_SIMD_MODE_ROUNDP     0x02    /* round towards +inf */
#define RT_SIMD_MODE_ROUNDZ     0x03    /* round towards zero */

#else /* RT_SIMD_FLUSH_ZERO */

#define RT_SIMD_MODE_ROUNDN     0x04    /* round towards near */
#define RT_SIMD_MODE_ROUNDM     0x05    /* round towards -inf */
#define RT_SIMD_MODE_ROUNDP     0x06    /* round towards +inf */
#define RT_SIMD_MODE_ROUNDZ     0x07    /* round towards zero */

#endif /* RT_SIMD_FLUSH_ZERO */

#define RT_SIMD_MODE_ROUNDN_F   0x04    /* round towards near */
#define RT_SIMD_MODE_ROUNDM_F   0x05    /* round towards -inf */
#define RT_SIMD_MODE_ROUNDP_F   0x06    /* round towards +inf */
#define RT_SIMD_MODE_ROUNDZ_F   0x07    /* round towards zero */

#define mxcsr_ld(MS, DS) /* not portable, do not use outside */             \
        V2X(0x00,    0, 0) EMITB(0xAE)                                      \
        MRM(0x02,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define mxcsr_st(MD, DD) /* not portable, do not use outside */             \
        V2X(0x00,    0, 0) EMITB(0xAE)                                      \
        MRM(0x03,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

#if RT_SIMD_FAST_FCTRL == 0

#define FCTRL_SET(mode)   /* sets given mode into fp control register */    \
        movwx_mi(Mebp, inf_SCR02(4), IH(RT_SIMD_MODE_##mode << 13 | 0x1F80))\
        mxcsr_ld(Mebp, inf_SCR02(4))

#define FCTRL_RESET()     /* resumes default mode (ROUNDN) upon leave */    \
        mxcsr_ld(Mebp, inf_FCTRL((RT_SIMD_MODE_ROUNDN&3)*4))

#else /* RT_SIMD_FAST_FCTRL */

#define FCTRL_SET(mode)   /* sets given mode into fp control register */    \
        mxcsr_ld(Mebp, inf_FCTRL((RT_SIMD_MODE_##mode&3)*4))

#define FCTRL_RESET()     /* resumes default mode (ROUNDN) upon leave */    \
        mxcsr_ld(Mebp, inf_FCTRL((RT_SIMD_MODE_ROUNDN&3)*4))

#endif /* RT_SIMD_FAST_FCTRL */

/* cvt (D = fp-to-signed-int S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: ROUNDZ is not supported on pre-VSX Power systems, use cvz
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rndis_rr(XD, XS)                                                    \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))

#define rndis_ld(XD, MS, DS)                                                \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x04))

#define cvtis_rr(XD, XS)                                                    \
        V2X(0x00,    K, 1) EMITB(0x5B)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvtis_ld(XD, MS, DS)                                                \
        V2X(0x00,    K, 1) EMITB(0x5B)                                      \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvt (D = signed-int-to-fp S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: only default ROUNDN is supported on pre-VSX Power systems */

#define cvtin_rr(XD, XS)                                                    \
        V2X(0x00,    K, 0) EMITB(0x5B)                                      \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvtin_ld(XD, MS, DS)                                                \
        V2X(0x00,    K, 0) EMITB(0x5B)                                      \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvr (D = fp-to-signed-int S)
 * rounding mode is encoded directly (cannot be used in FCTRL blocks)
 * NOTE: on targets with full-IEEE SIMD fp-arithmetic the ROUND*_F mode
 * isn't always taken into account when used within full-IEEE ASM block
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnris_rr(XD, XS, mode)                                              \
        VEX(0x00,    K, 1, 3) EMITB(0x08)                                   \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(RT_SIMD_MODE_##mode&3))

#define cvris_rr(XD, XS, mode)                                              \
        rnris_rr(W(XD), W(XS), mode)                                        \
        cvzis_rr(W(XD), W(XD))

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

/* sregs */

#define sregs_sa() /* save all SIMD regs, destroys Reax */                  \
        movxx_ld(Reax, Mebp, inf_REGS)                                      \
        movix_st(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_st(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_st(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_st(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_st(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_st(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_st(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_st(Xmm7, Oeax, PLAIN)

#define sregs_la() /* load all SIMD regs, destroys Reax */                  \
        movxx_ld(Reax, Mebp, inf_REGS)                                      \
        movix_ld(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_ld(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_ld(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_ld(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_ld(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_ld(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_ld(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movix_ld(Xmm7, Oeax, PLAIN)

#endif /* RT_128 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_X86_128V8_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/