/******************************************************************************/
/* Copyright (c) 2013-2017 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_P32_256V8_H
#define RT_RTARCH_P32_256V8_H

#include "rtarch_p64.h"

#define RT_SIMD_REGS_256        8
#define RT_SIMD_ALIGN_256       32
#define RT_SIMD_WIDTH64_256     4
#define RT_SIMD_SET64_256(s, v) s[0]=s[1]=s[2]=s[3]=v
#define RT_SIMD_WIDTH32_256     8
#define RT_SIMD_SET32_256(s, v) s[0]=s[1]=s[2]=s[3]=s[4]=s[5]=s[6]=s[7]=v

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_p32_256v8.h: Implementation of Power fp32 VMX instruction pairs.
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
 * cmdi*_** - applies [cmd] to 32-bit SIMD element args, packed-128-bit
 * cmdj*_** - applies [cmd] to 64-bit SIMD element args, packed-128-bit
 * cmdl*_** - applies [cmd] to L-size SIMD element args, packed-128-bit
 *
 * cmdc*_** - applies [cmd] to 32-bit SIMD element args, packed-256-bit
 * cmdd*_** - applies [cmd] to 64-bit SIMD element args, packed-256-bit
 * cmdf*_** - applies [cmd] to L-size SIMD element args, packed-256-bit
 *
 * cmdo*_** - applies [cmd] to 32-bit SIMD element args, packed-var-len
 * cmdp*_** - applies [cmd] to L-size SIMD element args, packed-var-len
 * cmdq*_** - applies [cmd] to 64-bit SIMD element args, packed-var-len
 *
 * cmd*x_** - applies [cmd] to [p]acked unsigned integer args, [x] - default
 * cmd*n_** - applies [cmd] to [p]acked   signed integer args, [n] - negatable
 * cmd*s_** - applies [cmd] to [p]acked floating point   args, [s] - scalable
 *
 * The cmdp*_** (rtbase.h) instructions are intended for SPMD programming model
 * and can be configured to work with 32/64-bit data-elements (int, fp).
 * In this model data-paths are fixed-width, BASE and SIMD data-elements are
 * width-compatible, code-path divergence is handled via mkj**_** pseudo-ops.
 * Matching element-sized BASE subset cmdy*_** is defined in rtbase.h as well.
 *
 * Note, when using fixed-data-size 128/256-bit SIMD subsets simultaneously
 * upper 128-bit halves of full 256-bit SIMD registers may end up undefined.
 * On RISC targets they remain unchanged, while on x86-AVX they are zeroed.
 * This happens when registers written in 128-bit subset are then used/read
 * from within 256-bit subset. The same rule applies to mixing of 256/512-bit.
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

#if defined (RT_256) && (RT_256 >= 8) && (RT_SIMD_COMPAT_XMM > 0)

#ifndef RT_RTARCH_P32_128V1_H
#undef  RT_128
#define RT_128  1
#include "rtarch_p32_128v1.h"
#endif /* RT_RTARCH_P32_128V1_H */

#undef  sregs_sa
#undef  sregs_la

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/**********************************   VMX   ***********************************/
/******************************************************************************/

/***************   packed single-precision generic move/logic   ***************/

/* mov (D = S) */

#define movcx_rr(XD, XS)                                                    \
        EMITW(0x10000484 | MXM(REG(XD), REG(XS), REG(XS)))                  \
        EMITW(0x10000484 | MXM(RYG(XD), RYG(XS), RYG(XS)))

#define movcx_ld(XD, MS, DS)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(REG(XD), Teax & (MOD(MS) == TPxx), TPxx))    \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(RYG(XD), Teax & (MOD(MS) == TPxx), TPxx))    \
                                                       /* ^ == -1 if true */

#define movcx_st(XS, MD, DD)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MD), VAL(DD), C2(DD), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MD), VAL(DD), B2(DD), P2(DD)))  \
        EMITW(0x7C0001CE | MXM(REG(XS), Teax & (MOD(MD) == TPxx), TPxx))    \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MD), VYL(DD), C2(DD), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MD), VYL(DD), B2(DD), P2(DD)))  \
        EMITW(0x7C0001CE | MXM(RYG(XS), Teax & (MOD(MD) == TPxx), TPxx))    \
                                                       /* ^ == -1 if true */

/* mmv (G = G mask-merge S, mask: 0 - keeps G, 1 - picks S with elem-size frag)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, XS unmasked frags */

#define mmvcx_rr(XG, XS)                                                    \
        EMITW(0x1000002A | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x1000042A | MXM(RYG(XG), RYG(XG), RYG(XS)))

#define mmvcx_ld(XG, MS, DS)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000002A | MXM(REG(XG), REG(XG), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000042A | MXM(RYG(XG), RYG(XG), TmmM))/* ^ == -1 if true */

#define mmvcx_st(XS, MG, DG)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MG), VAL(DG), C2(DG), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MG), VAL(DG), B2(DG), P2(DG)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MG) == TPxx), TPxx))    \
        EMITW(0x1000002A | MXM(TmmM,    TmmM,    REG(XS)))                  \
        EMITW(0x7C0001CE | MXM(TmmM,    Teax & (MOD(MG) == TPxx), TPxx))    \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MG), VYL(DG), C2(DG), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MG), VYL(DG), B2(DG), P2(DG)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MG) == TPxx), TPxx))    \
        EMITW(0x1000042A | MXM(TmmM,    TmmM,    RYG(XS)))                  \
        EMITW(0x7C0001CE | MXM(TmmM,    Teax & (MOD(MG) == TPxx), TPxx))    \
                                                       /* ^ == -1 if true */

/* and (G = G & S), (D = S & T) if (D != S) */

#define andcx_rr(XG, XS)                                                    \
        andcx3rr(W(XG), W(XG), W(XS))

#define andcx_ld(XG, MS, DS)                                                \
        andcx3ld(W(XG), W(XG), W(MS), W(DS))

#define andcx3rr(XD, XS, XT)                                                \
        EMITW(0x10000404 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x10000404 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define andcx3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x10000404 | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x10000404 | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* ann (G = ~G & S), (D = ~S & T) if (D != S) */

#define anncx_rr(XG, XS)                                                    \
        anncx3rr(W(XG), W(XG), W(XS))

#define anncx_ld(XG, MS, DS)                                                \
        anncx3ld(W(XG), W(XG), W(MS), W(DS))

#define anncx3rr(XD, XS, XT)                                                \
        EMITW(0x10000444 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x10000444 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define anncx3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x10000444 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x10000444 | MXM(RYG(XD), TmmM,    RYG(XS)))

/* orr (G = G | S), (D = S | T) if (D != S) */

#define orrcx_rr(XG, XS)                                                    \
        orrcx3rr(W(XG), W(XG), W(XS))

#define orrcx_ld(XG, MS, DS)                                                \
        orrcx3ld(W(XG), W(XG), W(MS), W(DS))

#define orrcx3rr(XD, XS, XT)                                                \
        EMITW(0x10000484 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x10000484 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define orrcx3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x10000484 | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x10000484 | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* orn (G = ~G | S), (D = ~S | T) if (D != S) */

#define orncx_rr(XG, XS)                                                    \
        notcx_rx(W(XG))                                                     \
        orrcx_rr(W(XG), W(XS))

#define orncx_ld(XG, MS, DS)                                                \
        notcx_rx(W(XG))                                                     \
        orrcx_ld(W(XG), W(MS), W(DS))

#define orncx3rr(XD, XS, XT)                                                \
        movcx_rr(W(XD), W(XS))                                              \
        orncx_rr(W(XD), W(XT))

#define orncx3ld(XD, XS, MT, DT)                                            \
        movcx_rr(W(XD), W(XS))                                              \
        orncx_ld(W(XD), W(MT), W(DT))

/* xor (G = G ^ S), (D = S ^ T) if (D != S) */

#define xorcx_rr(XG, XS)                                                    \
        xorcx3rr(W(XG), W(XG), W(XS))

#define xorcx_ld(XG, MS, DS)                                                \
        xorcx3ld(W(XG), W(XG), W(MS), W(DS))

#define xorcx3rr(XD, XS, XT)                                                \
        EMITW(0x100004C4 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100004C4 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define xorcx3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100004C4 | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100004C4 | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* not (G = ~G) */

#define notcx_rx(XG)                                                        \
        EMITW(0x10000504 | MXM(REG(XG), REG(XG), REG(XG)))                  \
        EMITW(0x10000504 | MXM(RYG(XG), RYG(XG), RYG(XG)))

/************   packed single-precision floating-point arithmetic   ***********/

/* neg (G = -G) */

#define negcs_rx(XG)                                                        \
        EMITW(0x100004C4 | MXM(REG(XG), REG(XG), TmmS))                     \
        EMITW(0x100004C4 | MXM(RYG(XG), RYG(XG), TmmS))

/* add (G = G + S), (D = S + T) if (D != S) */

#define addcs_rr(XG, XS)                                                    \
        addcs3rr(W(XG), W(XG), W(XS))

#define addcs_ld(XG, MS, DS)                                                \
        addcs3ld(W(XG), W(XG), W(MS), W(DS))

#define addcs3rr(XD, XS, XT)                                                \
        EMITW(0x1000000A | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x1000000A | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define addcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000000A | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000000A | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* sub (G = G - S), (D = S - T) if (D != S) */

#define subcs_rr(XG, XS)                                                    \
        subcs3rr(W(XG), W(XG), W(XS))

#define subcs_ld(XG, MS, DS)                                                \
        subcs3ld(W(XG), W(XG), W(MS), W(DS))

#define subcs3rr(XD, XS, XT)                                                \
        EMITW(0x1000004A | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x1000004A | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define subcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000004A | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000004A | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* mul (G = G * S), (D = S * T) if (D != S) */

#define mulcs_rr(XG, XS)                                                    \
        mulcs3rr(W(XG), W(XG), W(XS))

#define mulcs_ld(XG, MS, DS)                                                \
        mulcs3ld(W(XG), W(XG), W(MS), W(DS))

#define mulcs3rr(XD, XS, XT)                                                \
        EMITW(0x1000002E | MXM(REG(XD), REG(XS), TmmS) | REG(XT) << 6)      \
        EMITW(0x1000002E | MXM(RYG(XD), RYG(XS), TmmS) | RYG(XT) << 6)

#define mulcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000002E | MXM(REG(XD), REG(XS), TmmS) | TmmM << 6)         \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000002E | MXM(RYG(XD), RYG(XS), TmmS) | TmmM << 6)

/* div (G = G / S), (D = S / T) if (D != S) */

#define divcs_rr(XG, XS)                                                    \
        divcs3rr(W(XG), W(XG), W(XS))

#define divcs_ld(XG, MS, DS)                                                \
        divcs3ld(W(XG), W(XG), W(MS), W(DS))

#if RT_SIMD_COMPAT_DIV == 1

#define divcs3rr(XD, XS, XT)                                                \
        movcx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movcx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x00))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x00))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x00))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x04))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x04))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x04))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x08))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x08))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x08))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x0C))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x10))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x10))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x10))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x14))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x14))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x14))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x18))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x18))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x18))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x1C))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        movcx_ld(W(XD), Mebp, inf_SCR01(0))

#define divcs3ld(XD, XS, MT, DT)                                            \
        movcx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movcx_ld(W(XD), W(MT), W(DT))                                       \
        movcx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x00))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x00))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x00))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x04))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x04))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x04))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x08))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x08))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x08))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x0C))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x10))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x10))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x10))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x14))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x14))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x14))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x18))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x18))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x18))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        movws_ld(Tff2,  Mebp, inf_SCR02(0x1C))                              \
        divws_rr(Tff1, Tff2)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        movcx_ld(W(XD), Mebp, inf_SCR01(0))

#else /* RT_SIMD_COMPAT_DIV */

#define divcs3rr(XD, XS, XT)                                                \
        EMITW(0x1000010A | MXM(TmmW,    0x00,    REG(XT)))                  \
        EMITW(0x1000002F | MXM(TmmZ,    TmmW,    TmmU) | REG(XT) << 6)      \
        EMITW(0x1000002E | MXM(TmmW,    TmmW,    TmmW) | TmmZ << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    REG(XS), TmmS) | TmmW << 6)         \
        EMITW(0x1000002F | MXM(REG(XD), TmmZ, REG(XS)) | REG(XT) << 6)      \
        EMITW(0x1000002E | MXM(REG(XD), REG(XD), TmmZ) | TmmW << 6)         \
        EMITW(0x1000010A | MXM(TmmW,    0x00,    RYG(XT)))                  \
        EMITW(0x1000002F | MXM(TmmZ,    TmmW,    TmmU) | RYG(XT) << 6)      \
        EMITW(0x1000002E | MXM(TmmW,    TmmW,    TmmW) | TmmZ << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    RYG(XS), TmmS) | TmmW << 6)         \
        EMITW(0x1000002F | MXM(RYG(XD), TmmZ, RYG(XS)) | RYG(XT) << 6)      \
        EMITW(0x1000002E | MXM(RYG(XD), RYG(XD), TmmZ) | TmmW << 6)

#define divcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000010A | MXM(TmmW,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x1000002F | MXM(TmmZ,    TmmW,    TmmU) | TmmM << 6)         \
        EMITW(0x1000002E | MXM(TmmW,    TmmW,    TmmW) | TmmZ << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    REG(XS), TmmS) | TmmW << 6)         \
        EMITW(0x1000002F | MXM(REG(XD), TmmZ, REG(XS)) | TmmM << 6)         \
        EMITW(0x1000002E | MXM(REG(XD), REG(XD), TmmZ) | TmmW << 6)         \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000010A | MXM(TmmW,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x1000002F | MXM(TmmZ,    TmmW,    TmmU) | TmmM << 6)         \
        EMITW(0x1000002E | MXM(TmmW,    TmmW,    TmmW) | TmmZ << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    RYG(XS), TmmS) | TmmW << 6)         \
        EMITW(0x1000002F | MXM(RYG(XD), TmmZ, RYG(XS)) | TmmM << 6)         \
        EMITW(0x1000002E | MXM(RYG(XD), RYG(XD), TmmZ) | TmmW << 6)

#endif /* RT_SIMD_COMPAT_DIV */

/* sqr (D = sqrt S) */

#if RT_SIMD_COMPAT_SQR == 1

#define sqrcs_rr(XD, XS)                                                    \
        movcx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x00))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x00))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x04))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x04))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x08))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x08))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x10))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x10))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x14))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x14))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x18))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x18))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        movcx_ld(W(XD), Mebp, inf_SCR01(0))

#define sqrcs_ld(XD, MS, DS)                                                \
        movcx_ld(W(XD), W(MS), W(DS))                                       \
        movcx_st(W(XD), Mebp, inf_SCR01(0))                                 \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x00))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x00))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x04))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x04))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x08))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x08))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x0C))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x10))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x10))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x14))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x14))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x18))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x18))                              \
        movws_ld(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        sqrws_rr(Tff1, Tff1)                                                \
        movws_st(Tff1,  Mebp, inf_SCR01(0x1C))                              \
        movcx_ld(W(XD), Mebp, inf_SCR01(0))

#else /* RT_SIMD_COMPAT_SQR */

#define sqrcs_rr(XD, XS)                                                    \
        EMITW(0x1000014A | MXM(TmmW,    0x00,    REG(XS)))                  \
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | REG(XS) << 6)      \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | REG(XS) << 6)      \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(REG(XD), TmmW,    TmmS) | REG(XS) << 6)      \
        EMITW(0x1000014A | MXM(TmmW,    0x00,    RYG(XS)))                  \
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | RYG(XS) << 6)      \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | RYG(XS) << 6)      \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(RYG(XD), TmmW,    TmmS) | RYG(XS) << 6)

#define sqrcs_ld(XD, MS, DS)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000014A | MXM(TmmW,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | TmmM << 6)         \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | TmmM << 6)         \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(REG(XD), TmmW,    TmmS) | REG(XS) << 6)      \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000014A | MXM(TmmW,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | TmmM << 6)         \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(TmmZ,    TmmW,    TmmS) | TmmW << 6)         \
        EMITW(0x1000002E | MXM(TmmT,    TmmW,    TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | TmmM << 6)         \
        EMITW(0x1000002F | MXM(TmmW,    TmmZ,    TmmW) | TmmT << 6)         \
        EMITW(0x1000002E | MXM(RYG(XD), TmmW,    TmmS) | RYG(XS) << 6)

#endif /* RT_SIMD_COMPAT_SQR */

/* cbr (D = cbrt S) */

        /* cbe, cbs, cbr defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RCP != 1

#define rcecs_rr(XD, XS)                                                    \
        EMITW(0x1000010A | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x1000010A | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rcscs_rr(XG, XS) /* destroys XS */                                  \
        EMITW(0x1000002F | MXM(REG(XS), REG(XG), TmmU) | REG(XS) << 6)      \
        EMITW(0x1000002E | MXM(REG(XG), REG(XG), REG(XG)) | REG(XS) << 6)   \
        EMITW(0x1000002F | MXM(RYG(XS), RYG(XG), TmmU) | RYG(XS) << 6)      \
        EMITW(0x1000002E | MXM(RYG(XG), RYG(XG), RYG(XG)) | RYG(XS) << 6)

#endif /* RT_SIMD_COMPAT_RCP */

        /* rcp defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RSQ != 1

#define rsecs_rr(XD, XS)                                                    \
        EMITW(0x1000014A | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x1000014A | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rsscs_rr(XG, XS) /* destroys XS */                                  \
        EMITW(0x1000002E | MXM(TmmZ,    REG(XG), TmmS) | REG(XG) << 6)      \
        EMITW(0x1000002E | MXM(TmmW,    REG(XG), TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | REG(XS) << 6)      \
        EMITW(0x1000002F | MXM(REG(XG), TmmZ,    REG(XG)) | TmmW << 6)      \
        EMITW(0x1000002E | MXM(TmmZ,    RYG(XG), TmmS) | RYG(XG) << 6)      \
        EMITW(0x1000002E | MXM(TmmW,    RYG(XG), TmmS) | TmmV << 6)         \
        EMITW(0x1000002F | MXM(TmmZ,    TmmZ,    TmmU) | RYG(XS) << 6)      \
        EMITW(0x1000002F | MXM(RYG(XG), TmmZ,    RYG(XG)) | TmmW << 6)

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rsq defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* fma (G = G + S * T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmacs_rr(XG, XS, XT)                                                \
        EMITW(0x1000002E | MXM(REG(XG), REG(XS), REG(XG)) | REG(XT) << 6)   \
        EMITW(0x1000002E | MXM(RYG(XG), RYG(XS), RYG(XG)) | RYG(XT) << 6)

#define fmacs_ld(XG, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000002E | MXM(REG(XG), REG(XS), REG(XG)) | TmmM << 6)      \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000002E | MXM(RYG(XG), RYG(XS), RYG(XG)) | TmmM << 6)

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T)
 * NOTE: due to final negation being outside of rounding on all Power systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmscs_rr(XG, XS, XT)                                                \
        EMITW(0x1000002F | MXM(REG(XG), REG(XS), REG(XG)) | REG(XT) << 6)   \
        EMITW(0x1000002F | MXM(RYG(XG), RYG(XS), RYG(XG)) | RYG(XT) << 6)

#define fmscs_ld(XG, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000002F | MXM(REG(XG), REG(XS), REG(XG)) | TmmM << 6)      \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000002F | MXM(RYG(XG), RYG(XS), RYG(XG)) | TmmM << 6)

#endif /* RT_SIMD_COMPAT_FMS */

/*************   packed single-precision floating-point compare   *************/

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (D != S) */

#define mincs_rr(XG, XS)                                                    \
        mincs3rr(W(XG), W(XG), W(XS))

#define mincs_ld(XG, MS, DS)                                                \
        mincs3ld(W(XG), W(XG), W(MS), W(DS))

#define mincs3rr(XD, XS, XT)                                                \
        EMITW(0x1000044A | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x1000044A | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define mincs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000044A | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000044A | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (D != S) */

#define maxcs_rr(XG, XS)                                                    \
        maxcs3rr(W(XG), W(XG), W(XS))

#define maxcs_ld(XG, MS, DS)                                                \
        maxcs3ld(W(XG), W(XG), W(MS), W(DS))

#define maxcs3rr(XD, XS, XT)                                                \
        EMITW(0x1000040A | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x1000040A | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define maxcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000040A | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x1000040A | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* cmp (G = G ? S), (D = S ? T) if (D != S) */

#define ceqcs_rr(XG, XS)                                                    \
        ceqcs3rr(W(XG), W(XG), W(XS))

#define ceqcs_ld(XG, MS, DS)                                                \
        ceqcs3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqcs3rr(XD, XS, XT)                                                \
        EMITW(0x100000C6 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100000C6 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define ceqcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100000C6 | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100000C6 | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */


#define cnecs_rr(XG, XS)                                                    \
        cnecs3rr(W(XG), W(XG), W(XS))

#define cnecs_ld(XG, MS, DS)                                                \
        cnecs3ld(W(XG), W(XG), W(MS), W(DS))

#define cnecs3rr(XD, XS, XT)                                                \
        EMITW(0x100000C6 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        EMITW(0x100000C6 | MXM(RYG(XD), RYG(XS), RYG(XT)))                  \
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))

#define cnecs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100000C6 | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        EMITW(0x10000504 | MXM(REG(XD), REG(XD), REG(XD)))                  \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100000C6 | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */\
        EMITW(0x10000504 | MXM(RYG(XD), RYG(XD), RYG(XD)))


#define cltcs_rr(XG, XS)                                                    \
        cltcs3rr(W(XG), W(XG), W(XS))

#define cltcs_ld(XG, MS, DS)                                                \
        cltcs3ld(W(XG), W(XG), W(MS), W(DS))

#define cltcs3rr(XD, XS, XT)                                                \
        EMITW(0x100002C6 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x100002C6 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define cltcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100002C6 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100002C6 | MXM(RYG(XD), TmmM,    RYG(XS)))


#define clecs_rr(XG, XS)                                                    \
        clecs3rr(W(XG), W(XG), W(XS))

#define clecs_ld(XG, MS, DS)                                                \
        clecs3ld(W(XG), W(XG), W(MS), W(DS))

#define clecs3rr(XD, XS, XT)                                                \
        EMITW(0x100001C6 | MXM(REG(XD), REG(XT), REG(XS)))                  \
        EMITW(0x100001C6 | MXM(RYG(XD), RYG(XT), RYG(XS)))

#define clecs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100001C6 | MXM(REG(XD), TmmM,    REG(XS)))                  \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100001C6 | MXM(RYG(XD), TmmM,    RYG(XS)))


#define cgtcs_rr(XG, XS)                                                    \
        cgtcs3rr(W(XG), W(XG), W(XS))

#define cgtcs_ld(XG, MS, DS)                                                \
        cgtcs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtcs3rr(XD, XS, XT)                                                \
        EMITW(0x100002C6 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100002C6 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define cgtcs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100002C6 | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100002C6 | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */


#define cgecs_rr(XG, XS)                                                    \
        cgecs3rr(W(XG), W(XG), W(XS))

#define cgecs_ld(XG, MS, DS)                                                \
        cgecs3ld(W(XG), W(XG), W(MS), W(DS))

#define cgecs3rr(XD, XS, XT)                                                \
        EMITW(0x100001C6 | MXM(REG(XD), REG(XS), REG(XT)))                  \
        EMITW(0x100001C6 | MXM(RYG(XD), RYG(XS), RYG(XT)))

#define cgecs3ld(XD, XS, MT, DT)                                            \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VAL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VAL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100001C6 | MXM(REG(XD), REG(XS), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MT), VYL(DT), C2(DT), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MT), VYL(DT), B2(DT), P2(DT)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MT) == TPxx), TPxx))    \
        EMITW(0x100001C6 | MXM(RYG(XD), RYG(XS), TmmM))/* ^ == -1 if true */

/* simd mask
 * compatibility with AVX-512 and ARM-SVE can be achieved by always keeping
 * one hidden SIMD register holding all 1s and using one hidden mask register
 * first in cmp (c**ps) to produce compatible result in target SIMD register
 * then in mkj**_** to facilitate branching on a given condition value */

#define RT_SIMD_MASK_NONE32_256  MN32_256   /* none satisfy the condition */
#define RT_SIMD_MASK_FULL32_256  MF32_256   /*  all satisfy the condition */

/* #define S0(mask)    S1(mask)            (defined in 32_128-bit header) */
/* #define S1(mask)    S##mask             (defined in 32_128-bit header) */

#define SMN32_256(xs, lb) /* not portable, do not use outside */            \
        EMITW(0x10000484 | MXM(TmmM, xs,  xs+16))                           \
        EMITW(0x10000486 | MXM(TmmM, TmmM, TmmQ))                           \
        ASM_BEG ASM_OP2(beq, cr6, lb) ASM_END

#define SMF32_256(xs, lb) /* not portable, do not use outside */            \
        EMITW(0x10000404 | MXM(TmmM, xs,  xs+16))                           \
        EMITW(0x10000486 | MXM(TmmM, TmmM, TmmQ))                           \
        ASM_BEG ASM_OP2(blt, cr6, lb) ASM_END

#define mkjcx_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        AUW(EMPTY, EMPTY, EMPTY, REG(XS), lb,                               \
        S0(RT_SIMD_MASK_##mask##32_256), EMPTY2)

/*************   packed single-precision floating-point convert   *************/

/* cvz (D = fp-to-signed-int S)
 * rounding mode is encoded directly (can be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnzcs_rr(XD, XS)     /* round towards zero */                       \
        EMITW(0x1000024A | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x1000024A | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnzcs_ld(XD, MS, DS) /* round towards zero */                       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000024A | MXM(REG(XD), 0x00,    TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000024A | MXM(RYG(XD), 0x00,    TmmM))/* ^ == -1 if true */

#define cvzcs_rr(XD, XS)     /* round towards zero */                       \
        EMITW(0x100003CA | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x100003CA | MXM(RYG(XD), 0x00,    RYG(XS)))

#define cvzcs_ld(XD, MS, DS) /* round towards zero */                       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x100003CA | MXM(REG(XD), 0x00,    TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x100003CA | MXM(RYG(XD), 0x00,    TmmM))/* ^ == -1 if true */

/* cvp (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnpcs_rr(XD, XS)     /* round towards +inf */                       \
        EMITW(0x1000028A | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x1000028A | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnpcs_ld(XD, MS, DS) /* round towards +inf */                       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000028A | MXM(REG(XD), 0x00,    TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000028A | MXM(RYG(XD), 0x00,    TmmM))/* ^ == -1 if true */

#define cvpcs_rr(XD, XS)     /* round towards +inf */                       \
        rnpcs_rr(W(XD), W(XS))                                              \
        cvzcs_rr(W(XD), W(XD))

#define cvpcs_ld(XD, MS, DS) /* round towards +inf */                       \
        rnpcs_ld(W(XD), W(MS), W(DS))                                       \
        cvzcs_rr(W(XD), W(XD))

/* cvm (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnmcs_rr(XD, XS)     /* round towards -inf */                       \
        EMITW(0x100002CA | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x100002CA | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnmcs_ld(XD, MS, DS) /* round towards -inf */                       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x100002CA | MXM(REG(XD), 0x00,    TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x100002CA | MXM(RYG(XD), 0x00,    TmmM))/* ^ == -1 if true */

#define cvmcs_rr(XD, XS)     /* round towards -inf */                       \
        rnmcs_rr(W(XD), W(XS))                                              \
        cvzcs_rr(W(XD), W(XD))

#define cvmcs_ld(XD, MS, DS) /* round towards -inf */                       \
        rnmcs_ld(W(XD), W(MS), W(DS))                                       \
        cvzcs_rr(W(XD), W(XD))

/* cvn (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnncs_rr(XD, XS)     /* round towards near */                       \
        EMITW(0x1000020A | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x1000020A | MXM(RYG(XD), 0x00,    RYG(XS)))

#define rnncs_ld(XD, MS, DS) /* round towards near */                       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000020A | MXM(REG(XD), 0x00,    TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000020A | MXM(RYG(XD), 0x00,    TmmM))/* ^ == -1 if true */

#define cvncs_rr(XD, XS)     /* round towards near */                       \
        rnncs_rr(W(XD), W(XS))                                              \
        cvzcs_rr(W(XD), W(XD))

#define cvncs_ld(XD, MS, DS) /* round towards near */                       \
        rnncs_ld(W(XD), W(MS), W(DS))                                       \
        cvzcs_rr(W(XD), W(XD))

/* cvn (D = signed-int-to-fp S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks) */

#define cvncn_rr(XD, XS)     /* round towards near */                       \
        EMITW(0x1000034A | MXM(REG(XD), 0x00,    REG(XS)))                  \
        EMITW(0x1000034A | MXM(RYG(XD), 0x00,    RYG(XS)))

#define cvncn_ld(XD, MS, DS) /* round towards near */                       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000034A | MXM(REG(XD), 0x00,    TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000034A | MXM(RYG(XD), 0x00,    TmmM))/* ^ == -1 if true */

/************   packed single-precision integer arithmetic/shifts   ***********/

/* add (G = G + S) */

#define addcx_rr(XG, XS)                                                    \
        EMITW(0x10000080 | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x10000080 | MXM(RYG(XG), RYG(XG), RYG(XS)))

#define addcx_ld(XG, MS, DS)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000080 | MXM(REG(XG), REG(XG), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000080 | MXM(RYG(XG), RYG(XG), TmmM))/* ^ == -1 if true */

/* sub (G = G - S) */

#define subcx_rr(XG, XS)                                                    \
        EMITW(0x10000480 | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x10000480 | MXM(RYG(XG), RYG(XG), RYG(XS)))

#define subcx_ld(XG, MS, DS)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000480 | MXM(REG(XG), REG(XG), TmmM))/* ^ == -1 if true */\
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000480 | MXM(RYG(XG), RYG(XG), TmmM))/* ^ == -1 if true */

/* shl (G = G << S)
 * for maximum compatibility, shift count mustn't exceed elem-size */

#define shlcx_ri(XG, IS)                                                    \
        EMITW(0x1000038C | MXM(TmmM,    (0x1F & VAL(IS)), 0x00))            \
        EMITW(0x10000184 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000184 | MXM(RYG(XG), RYG(XG), TmmM))

#if RT_ENDIAN == 0

#define shlcx_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C00008E | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1003028C | MXM(TmmM,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x10000184 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000184 | MXM(RYG(XG), RYG(XG), TmmM))

#else /* RT_ENDIAN == 1 */

#define shlcx_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C00008E | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000028C | MXM(TmmM,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x10000184 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000184 | MXM(RYG(XG), RYG(XG), TmmM))

#endif /* RT_ENDIAN == 1 */

#define svlcx_rr(XG, XS)     /* variable shift with per-elem count */       \
        EMITW(0x10000184 | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x10000184 | MXM(RYG(XG), RYG(XG), RYG(XS)))

#define svlcx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000184 | MXM(REG(XG), REG(XG), TmmM))                     \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000184 | MXM(RYG(XG), RYG(XG), TmmM))

/* shr (G = G >> S)
 * for maximum compatibility, shift count mustn't exceed elem-size */

#define shrcx_ri(XG, IS)                                                    \
        EMITW(0x1000038C | MXM(TmmM,    (0x1F & VAL(IS)), 0x00))            \
        EMITW(0x10000284 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000284 | MXM(RYG(XG), RYG(XG), TmmM))

#if RT_ENDIAN == 0

#define shrcx_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C00008E | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1003028C | MXM(TmmM,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x10000284 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000284 | MXM(RYG(XG), RYG(XG), TmmM))

#else /* RT_ENDIAN == 1 */

#define shrcx_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C00008E | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000028C | MXM(TmmM,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x10000284 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000284 | MXM(RYG(XG), RYG(XG), TmmM))

#endif /* RT_ENDIAN == 1 */

#define svrcx_rr(XG, XS)     /* variable shift with per-elem count */       \
        EMITW(0x10000284 | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x10000284 | MXM(RYG(XG), RYG(XG), RYG(XS)))

#define svrcx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000284 | MXM(REG(XG), REG(XG), TmmM))                     \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000284 | MXM(RYG(XG), RYG(XG), TmmM))


#define shrcn_ri(XG, IS)                                                    \
        EMITW(0x1000038C | MXM(TmmM,    (0x1F & VAL(IS)), 0x00))            \
        EMITW(0x10000384 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000384 | MXM(RYG(XG), RYG(XG), TmmM))

#if RT_ENDIAN == 0

#define shrcn_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C00008E | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1003028C | MXM(TmmM,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x10000384 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000384 | MXM(RYG(XG), RYG(XG), TmmM))

#else /* RT_ENDIAN == 1 */

#define shrcn_ld(XG, MS, DS) /* loads SIMD, uses 64-bit at given address */ \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C00008E | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000028C | MXM(TmmM,    0x00,    TmmM))/* ^ == -1 if true */\
        EMITW(0x10000384 | MXM(REG(XG), REG(XG), TmmM))                     \
        EMITW(0x10000384 | MXM(RYG(XG), RYG(XG), TmmM))

#endif /* RT_ENDIAN == 1 */

#define svrcn_rr(XG, XS)     /* variable shift with per-elem count */       \
        EMITW(0x10000384 | MXM(REG(XG), REG(XG), REG(XS)))                  \
        EMITW(0x10000384 | MXM(RYG(XG), RYG(XG), RYG(XS)))

#define svrcn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000384 | MXM(REG(XG), REG(XG), TmmM))                     \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x10000384 | MXM(RYG(XG), RYG(XG), TmmM))

/**************************   helper macros (SIMD)   **************************/

/* cvt (D = fp-to-signed-int S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: ROUNDZ is not supported on pre-VSX Power systems, use cvz
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rndcs_rr(XD, XS)                                                    \
        EMITW(0x1000000A | MXM(REG(XD), TmmR,    REG(XS)))                  \
        EMITW(0x1000020A | MXM(REG(XD), 0x00,    REG(XD)))                  \
        EMITW(0x1000000A | MXM(RYG(XD), TmmR,    RYG(XS)))                  \
        EMITW(0x1000020A | MXM(RYG(XD), 0x00,    RYG(XD)))

#define rndcs_ld(XD, MS, DS)                                                \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VAL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VAL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000000A | MXM(REG(XD), TmmR,    TmmM))/* ^ == -1 if true */\
        EMITW(0x1000020A | MXM(REG(XD), 0x00,    REG(XD)))                  \
        AUW(EMPTY,    EMPTY,  EMPTY,    MOD(MS), VYL(DS), C2(DS), EMPTY2)   \
        EMITW(0x38000000 | MPM(TPxx,    REG(MS), VYL(DS), B2(DS), P2(DS)))  \
        EMITW(0x7C0000CE | MXM(TmmM,    Teax & (MOD(MS) == TPxx), TPxx))    \
        EMITW(0x1000000A | MXM(RYG(XD), TmmR,    TmmM))/* ^ == -1 if true */\
        EMITW(0x1000020A | MXM(RYG(XD), 0x00,    RYG(XD)))

#define cvtcs_rr(XD, XS)                                                    \
        rndcs_rr(W(XD), W(XS))                                              \
        cvzcs_rr(W(XD), W(XD))

#define cvtcs_ld(XD, MS, DS)                                                \
        rndcs_ld(W(XD), W(MS), W(DS))                                       \
        cvzcs_rr(W(XD), W(XD))

/* cvt (D = signed-int-to-fp S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: only default ROUNDN is supported on pre-VSX Power systems */

#define cvtin_rr(XD, XS)                                                    \
        cvnin_rr(W(XD), W(XS))                                            /*!*/

#define cvtin_ld(XD, MS, DS)                                                \
        cvnin_ld(W(XD), W(MS), W(DS))                                     /*!*/

/* cvr (D = fp-to-signed-int S)
 * rounding mode is encoded directly (cannot be used in FCTRL blocks)
 * NOTE: on targets with full-IEEE SIMD fp-arithmetic the ROUND*_F mode
 * isn't always taken into account when used within full-IEEE ASM block
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnrcs_rr(XD, XS, mode)                                              \
        EMITW(0x1000020A | MXM(REG(XD), 0x00,    REG(XS)) |                 \
        (RT_SIMD_MODE_##mode&3) << 6)                                       \
        EMITW(0x1000020A | MXM(RYG(XD), 0x00,    RYG(XS)) |                 \
        (RT_SIMD_MODE_##mode&3) << 6)

#define cvrcs_rr(XD, XS, mode)                                              \
        rnrcs_rr(W(XD), W(XS), mode)                                        \
        cvzcs_rr(W(XD), W(XD))

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

/* sregs */

#define sregs_sa() /* save all SIMD regs, destroys Reax */                  \
        movxx_ld(Reax, Mebp, inf_REGS)                                      \
        movcx_st(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm7, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm8, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(Xmm9, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(XmmA, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(XmmB, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(XmmC, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_st(XmmD, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0001CE | MXM(TmmE,    0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0001CE | MXM(TmmE+16, 0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0001CE | MXM(TmmQ,    0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0001CE | MXM(TmmM,    0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm7, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm8, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(Xmm9, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(XmmA, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(XmmB, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(XmmC, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_st(XmmD, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        EMITW(0xD8000000 | MXM(TmmE,    Teax,    0x00))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        EMITW(0xD8000000 | MXM(TmmF,    Teax,    0x00))

#define sregs_la() /* load all SIMD regs, destroys Reax */                  \
        movxx_ld(Reax, Mebp, inf_REGS)                                      \
        movcx_ld(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm7, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm8, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(Xmm9, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(XmmA, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(XmmB, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(XmmC, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        movcx_ld(XmmD, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0000CE | MXM(TmmE,    0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0000CE | MXM(TmmE+16, 0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0000CE | MXM(TmmQ,    0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_256*4))                           \
        EMITW(0x7C0000CE | MXM(TmmM,    0x00,    Teax))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm7, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm8, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(Xmm9, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(XmmA, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(XmmB, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(XmmC, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        movtx_ld(XmmD, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        EMITW(0xC8000000 | MXM(TmmE,    Teax,    0x00))                     \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32_128*4))                           \
        EMITW(0xC8000000 | MXM(TmmF,    Teax,    0x00))

#endif /* RT_256 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_P32_256V8_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/