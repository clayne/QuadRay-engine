/******************************************************************************/
/* Copyright (c) 2013-2016 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_X32_512_H
#define RT_RTARCH_X32_512_H

#include "rtarch_x64.h"

#define RT_SIMD_REGS        16
#define RT_SIMD_ALIGN       64
#define RT_SIMD_WIDTH32     16
#define RT_SIMD_SET32(s, v) s[0]=s[1]=s[2]=s[3]=s[4]=s[5]=s[6]=s[7]=\
                            s[8]=s[9]=s[10]=s[11]=s[12]=s[13]=s[14]=s[15]=v
#define RT_SIMD_WIDTH64     8
#define RT_SIMD_SET64(s, v) s[0]=s[1]=s[2]=s[3]=s[4]=s[5]=s[6]=s[7]=v

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_x32_512.h: Implementation of x86_64 fp32 AVX-512 instructions.
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

#if defined (RT_512) && (RT_512 != 0)

#undef  sregs_sa
#undef  sregs_la
#undef  mxcsr_ld

#define K 2

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
#define Xmm8    0x08, 0x03, EMPTY
#define Xmm9    0x09, 0x03, EMPTY
#define XmmA    0x0A, 0x03, EMPTY
#define XmmB    0x0B, 0x03, EMPTY
#define XmmC    0x0C, 0x03, EMPTY
#define XmmD    0x0D, 0x03, EMPTY
#if     RT_SIMD_COMPAT_XMM < 2
#define XmmE    0x0E, 0x03, EMPTY            /* may be reserved in some cases */
#if     RT_SIMD_COMPAT_XMM < 1
#define XmmF    0x0F, 0x03, EMPTY            /* may be reserved in some cases */
#endif/*RT_SIMD_COMPAT_XMM < 1*/
#endif/*RT_SIMD_COMPAT_XMM < 2*/

/* The last two SIMD registers can be reserved by the assembler when building
 * RISC targets with SIMD wider than natively supported 128-bit, in which case
 * they will be occupied by temporary data. Two hidden registers may also come
 * in handy when implementing elaborate register-spill techniques in the future
 * for current targets with less native registers than architecturally exposed.
 *
 * It should be possible to reserve only 1 SIMD register (XmmF) to achieve the
 * goals above (totalling 15 regs) at the cost of extra loads in certain ops. */

/******************************************************************************/
/**********************************   AVX   ***********************************/
/******************************************************************************/

/* adr (D = adr S) */

#define adrpx_ld(RD, MS, DS) /* RD is a BASE reg, MS/DS is SIMD-aligned */  \
    ADR REW(RXB(RD), RXB(MS)) EMITB(0x8D)                                   \
        MRM(REG(RD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/**************************   packed generic (AVX3)   *************************/

/* mov (D = S) */

#define movox_rr(XD, XS)                                                    \
        EVX(RXB(XD), RXB(XS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movox_ld(XD, MS, DS)                                                \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define movox_st(XS, MD, DD)                                                \
    ADR EVX(RXB(XS), RXB(MD),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS), MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

/* mmv (G = G mask-merge S, mask: 0 - keeps G, 1 - picks S with elem-size frag)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, XS unmasked frags */

#define mmvox_rr(XG, XS)                                                    \
        ck1ox_rm(Xmm0, Mebp, inf_GPC07)                                     \
        mk1ox_rr(W(XG), W(XS))

#define mmvox_ld(XG, MS, DS)                                                \
        ck1ox_rm(Xmm0, Mebp, inf_GPC07)                                     \
        mk1ox_ld(W(XG), W(MS), W(DS))

#define mmvox_st(XS, MG, DG)                                                \
        ck1ox_rm(Xmm0, Mebp, inf_GPC07)                                     \
        mk1ox_st(W(XS), W(MG), W(DG))

#define mk1ox_rr(XG, XS)     /* not portable, do not use outside */         \
        EKX(RXB(XG), RXB(XS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mk1ox_ld(XG, MS, DS) /* not portable, do not use outside */         \
    ADR EKX(RXB(XG), RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define mk1ox_st(XS, MG, DG) /* not portable, do not use outside */         \
    ADR EKX(RXB(XS), RXB(MG),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS), MOD(MG), REG(MG))                                      \
        AUX(SIB(MG), CMD(DG), EMPTY)

#if (RT_512 < 2)

/* and (G = G & S) */

#define andox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define andox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* ann (G = ~G & S) */

#define annox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define annox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* orr (G = G | S) */

#define orrox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define orrox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* orn (G = ~G | S) */

#define ornox_rr(XG, XS)                                                    \
        notox_rx(W(XG))                                                     \
        orrox_rr(W(XG), W(XS))

#define ornox_ld(XG, MS, DS)                                                \
        notox_rx(W(XG))                                                     \
        orrox_ld(W(XG), W(MS), W(DS))

/* xor (G = G ^ S) */

#define xorox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define xorox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#else /* RT_512 >= 2 */

/* and (G = G & S) */

#define andox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x54)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define andox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x54)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* ann (G = ~G & S) */

#define annox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x55)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define annox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x55)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* orr (G = G | S) */

#define orrox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x56)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define orrox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x56)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* orn (G = ~G | S) */

#define ornox_rr(XG, XS)                                                    \
        notox_rx(W(XG))                                                     \
        orrox_rr(W(XG), W(XS))

#define ornox_ld(XG, MS, DS)                                                \
        notox_rx(W(XG))                                                     \
        orrox_ld(W(XG), W(MS), W(DS))

/* xor (G = G ^ S) */

#define xorox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x57)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define xorox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x57)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#endif /* RT_512 >= 2 */

/* not (G = ~G) */

#define notox_rx(XG)                                                        \
        annox_ld(W(XG), Mebp, inf_GPC07)

/**************   packed single precision floating point (AVX3)   *************/

/* neg (G = -G) */

#define negos_rx(XG)                                                        \
        xorox_ld(W(XG), Mebp, inf_GPC06_32)

/* add (G = G + S) */

#define addos_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x58)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define addos_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x58)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* sub (G = G - S) */

#define subos_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x5C)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define subos_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x5C)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* mul (G = G * S) */

#define mulos_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x59)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mulos_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x59)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* div (G = G / S) */

#define divos_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x5E)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define divos_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x5E)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* sqr (D = sqrt S) */

#define sqros_rr(XD, XS)                                                    \
        EVX(RXB(XD), RXB(XS),    0x00, K, 0, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define sqros_ld(XD, MS, DS)                                                \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 0, 1) EMITB(0x51)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cbr (D = cbrt S) */

        /* cbe, cbs, cbr defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rcp (D = 1.0 / S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RCP != 1

#define rceos_rr(XD, XS)                                                    \
        movox_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movox_ld(W(XD), Mebp, inf_GPC01_32)                                 \
        divos_ld(W(XD), Mebp, inf_SCR01(0))

#define rcsos_rr(XG, XS) /* destroys XS */

#endif /* RT_SIMD_COMPAT_RCP */

        /* rcp defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* rsq (D = 1.0 / sqrt S)
 * accuracy/behavior may vary across supported targets, use accordingly */

#if RT_SIMD_COMPAT_RSQ != 1

#define rseos_rr(XD, XS)                                                    \
        sqros_rr(W(XD), W(XS))                                              \
        movox_st(W(XD), Mebp, inf_SCR01(0))                                 \
        movox_ld(W(XD), Mebp, inf_GPC01_32)                                 \
        divos_ld(W(XD), Mebp, inf_SCR01(0))

#define rssos_rr(XG, XS) /* destroys XS */

#endif /* RT_SIMD_COMPAT_RSQ */

        /* rsq defined in rtbase.h
         * under "COMMON SIMD INSTRUCTIONS" section */

/* fma (G = G + S * T)
 * NOTE: x87 fpu-fallbacks for fma/fms use round-to-nearest mode by default,
 * enable RT_SIMD_COMPAT_FMR for current SIMD rounding mode to be honoured */

#if RT_SIMD_COMPAT_FMA <= 1

#define fmaos_rr(XG, XS, XT)                                                \
    ADR EVX(RXB(XG), RXB(XT), REN(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmaos_ld(XG, XS, MT, DT)                                            \
    ADR EVX(RXB(XG), RXB(MT), REN(XS), K, 1, 2) EMITB(0xB8)                 \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMA */

/* fms (G = G - S * T)
 * NOTE: due to final negation being outside of rounding on all Power systems
 * only symmetric rounding modes (RN, RZ) are compatible across all targets */

#if RT_SIMD_COMPAT_FMS <= 1

#define fmsos_rr(XG, XS, XT)                                                \
    ADR EVX(RXB(XG), RXB(XT), REN(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(XT), REG(XT))

#define fmsos_ld(XG, XS, MT, DT)                                            \
    ADR EVX(RXB(XG), RXB(MT), REN(XS), K, 1, 2) EMITB(0xBC)                 \
        MRM(REG(XG), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#endif /* RT_SIMD_COMPAT_FMS */

/* min (G = G < S ? G : S) */

#define minos_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x5D)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define minos_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x5D)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* max (G = G > S ? G : S) */

#define maxos_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 0, 1) EMITB(0x5F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define maxos_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 0, 1) EMITB(0x5F)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cmp (G = G ? S) */

#define ceqos_rr(XG, XS)                                                    \
        EVX(0,       RXB(XS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define ceqos_ld(XG, MS, DS)                                                \
    ADR EVX(0,       RXB(MS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x00))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cneos_rr(XG, XS)                                                    \
        EVX(0,       RXB(XS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cneos_ld(XG, MS, DS)                                                \
    ADR EVX(0,       RXB(MS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x04))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cltos_rr(XG, XS)                                                    \
        EVX(0,       RXB(XS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cltos_ld(XG, MS, DS)                                                \
    ADR EVX(0,       RXB(MS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x01))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cleos_rr(XG, XS)                                                    \
        EVX(0,       RXB(XS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cleos_ld(XG, MS, DS)                                                \
    ADR EVX(0,       RXB(MS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x02))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cgtos_rr(XG, XS)                                                    \
        EVX(0,       RXB(XS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cgtos_ld(XG, MS, DS)                                                \
    ADR EVX(0,       RXB(MS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x06))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cgeos_rr(XG, XS)                                                    \
        EVX(0,       RXB(XS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define cgeos_ld(XG, MS, DS)                                                \
    ADR EVX(0,       RXB(MS), REN(XG), K, 0, 1) EMITB(0xC2)                 \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x05))                                  \
        mz1ox_ld(W(XG), Mebp, inf_GPC07)

#define mz1ox_rr(XG, XS)     /* not portable, do not use outside */         \
        EZX(RXB(XG), RXB(XS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mz1ox_ld(XG, MS, DS) /* not portable, do not use outside */         \
    ADR EZX(RXB(XG), RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define mz1ox_st(XS, MG, DG) /* not portable, do not use outside */         \
    ADR EZX(RXB(XS), RXB(MG),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS), MOD(MG), REG(MG))                                      \
        AUX(SIB(MG), CMD(DG), EMPTY)

/* cvz (D = fp-to-signed-int S)
 * rounding mode is encoded directly (can be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnzos_rr(XD, XS)     /* round towards zero */                       \
        EVX(RXB(XD), RXB(XS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x03))

#define rnzos_ld(XD, MS, DS) /* round towards zero */                       \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x03))

#define cvzos_rr(XD, XS)     /* round towards zero */                       \
        EVX(RXB(XD), RXB(XS),    0x00, K, 2, 1) EMITB(0x5B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvzos_ld(XD, MS, DS) /* round towards zero */                       \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 2, 1) EMITB(0x5B)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvp (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnpos_rr(XD, XS)     /* round towards +inf */                       \
        EVX(RXB(XD), RXB(XS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))

#define rnpos_ld(XD, MS, DS) /* round towards +inf */                       \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x02))

#define cvpos_rr(XD, XS)     /* round towards +inf */                       \
        rnpos_rr(W(XD), W(XS))                                              \
        cvzos_rr(W(XD), W(XD))

#define cvpos_ld(XD, MS, DS) /* round towards +inf */                       \
        rnpos_ld(W(XD), W(MS), W(DS))                                       \
        cvzos_rr(W(XD), W(XD))

/* cvm (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnmos_rr(XD, XS)     /* round towards -inf */                       \
        EVX(RXB(XD), RXB(XS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))

#define rnmos_ld(XD, MS, DS) /* round towards -inf */                       \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x01))

#define cvmos_rr(XD, XS)     /* round towards -inf */                       \
        rnmos_rr(W(XD), W(XS))                                              \
        cvzos_rr(W(XD), W(XD))

#define cvmos_ld(XD, MS, DS) /* round towards -inf */                       \
        rnmos_ld(W(XD), W(MS), W(DS))                                       \
        cvzos_rr(W(XD), W(XD))

/* cvn (D = fp-to-signed-int S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks)
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnnos_rr(XD, XS)     /* round towards near */                       \
        EVX(RXB(XD), RXB(XS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))

#define rnnos_ld(XD, MS, DS) /* round towards near */                       \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x00))

#define cvnos_rr(XD, XS)     /* round towards near */                       \
        cvtos_rr(W(XD), W(XS))

#define cvnos_ld(XD, MS, DS) /* round towards near */                       \
        cvtos_ld(W(XD), W(MS), W(DS))

/* cvn (D = signed-int-to-fp S)
 * rounding mode encoded directly (cannot be used in FCTRL blocks) */

#define cvnon_rr(XD, XS)     /* round towards near */                       \
        cvton_rr(W(XD), W(XS))

#define cvnon_ld(XD, MS, DS) /* round towards near */                       \
        cvton_ld(W(XD), W(MS), W(DS))

/**************************   extended float (x87)   **************************/

#define fpuws_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xD9)                                   \
        MRM(0x00,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define fpuws_st(MD, DD) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MD)) EMITB(0xD9)                                   \
        MRM(0x03,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)


#define fpuwn_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xDB)                                   \
        MRM(0x00,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define fpuwn_st(MD, DD) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MD)) EMITB(0xDB)                                   \
        MRM(0x03,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

#define fpuwt_st(MD, DD) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MD)) EMITB(0xDB)                                   \
        MRM(0x01,    MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)


#define addws_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xD8)                                   \
        MRM(0x00,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define subws_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xD8)                                   \
        MRM(0x04,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define sbrws_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xD8)                                   \
        MRM(0x05,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)


#define mulws_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xD8)                                   \
        MRM(0x01,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define divws_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xD8)                                   \
        MRM(0x06,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define dvrws_ld(MS, DS) /* not portable, do not use outside */             \
    ADR REX(0,       RXB(MS)) EMITB(0xD8)                                   \
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
    ADR REX(0,       RXB(MS)) EMITB(0xD9)                                   \
        MRM(0x05,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define fpucw_st(MD, DD) /* not portable, do not use outside */             \
FWT ADR REX(0,       RXB(MD)) EMITB(0xD9)                                   \
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

/**************************   packed integer (AVX3)   *************************/

/* add (G = G + S) */

#define addox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 1) EMITB(0xFE)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define addox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xFE)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* sub (G = G - S) */

#define subox_rr(XG, XS)                                                    \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 1) EMITB(0xFA)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define subox_ld(XG, MS, DS)                                                \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xFA)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* shl (G = G << S)
 * for maximum compatibility, shift count mustn't exceed elem-size */

#define shlox_ri(XG, IS)                                                    \
        EVX(0,       RXB(XG), REN(XG), K, 1, 1) EMITB(0x72)                 \
        MRM(0x06,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS) & 0x1F))

#define shlox_ld(XG, MS, DS) /* loads SIMD, uses 1 elem at given address */ \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xF2)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define svlox_rr(XG, XS)     /* variable shift with per-elem count */       \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define svlox_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 2) EMITB(0x47)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* shr (G = G >> S)
 * for maximum compatibility, shift count mustn't exceed elem-size */

#define shrox_ri(XG, IS)                                                    \
        EVX(0,       RXB(XG), REN(XG), K, 1, 1) EMITB(0x72)                 \
        MRM(0x02,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS) & 0x1F))

#define shrox_ld(XG, MS, DS) /* loads SIMD, uses 1 elem at given address */ \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xD2)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define svrox_rr(XG, XS)     /* variable shift with per-elem count */       \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define svrox_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 2) EMITB(0x45)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)


#define shron_ri(XG, IS)                                                    \
        EVX(0,       RXB(XG), REN(XG), K, 1, 1) EMITB(0x72)                 \
        MRM(0x04,    MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IS) & 0x1F))

#define shron_ld(XG, MS, DS) /* loads SIMD, uses 1 elem at given address */ \
    ADR EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 1) EMITB(0xE2)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define svron_rr(XG, XS)     /* variable shift with per-elem count */       \
        EVX(RXB(XG), RXB(XS), REN(XG), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define svron_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        EVX(RXB(XG), RXB(MS), REN(XG), K, 1, 2) EMITB(0x46)                 \
        MRM(REG(XG), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/**************************   helper macros (AVX3)   **************************/

/* simd mask
 * compatibility with AVX-512 and ARM-SVE can be achieved by always keeping
 * one hidden SIMD register holding all 1s and using one hidden mask register
 * first in cmp (c**ps) to produce compatible result in target SIMD register
 * then in CHECK_MASK to facilitate branching on a given condition value */

#define RT_SIMD_MASK_NONE       0x0000      /* none satisfy the condition */
#define RT_SIMD_MASK_FULL       0xFFFF      /*  all satisfy the condition */

#define movsn_rx(RD)         /* not portable, do not use outside */         \
        VEX(RXB(RD),       0,    0x00, 0, 0, 1) EMITB(0x93)                 \
        MRM(REG(RD),    0x03,    0x01)

#define ck1ox_rm(XS, MT, DT) /* not portable, do not use outside */         \
    ADR EVX(0,       RXB(MT), REN(XS), K, 1, 1) EMITB(0x76)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#define CHECK_MASK(lb, mask, XS) /* destroys Reax, jump lb if mask == S */  \
        ck1ox_rm(W(XS), Mebp, inf_GPC07)                                    \
        movsn_rx(Reax)                                                      \
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
    ADR VEX(0,       RXB(MS),    0x00, 0, 0, 1) EMITB(0xAE)                 \
        MRM(0x02,    MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define mxcsr_st(MD, DD) /* not portable, do not use outside */             \
    ADR VEX(0,       RXB(MD),    0x00, 0, 0, 1) EMITB(0xAE)                 \
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

#define rndos_rr(XD, XS)                                                    \
        EVX(RXB(XD), RXB(XS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))

#define rndos_ld(XD, MS, DS)                                                \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMITB(0x04))

#define cvtos_rr(XD, XS)                                                    \
        EVX(RXB(XD), RXB(XS),    0x00, K, 1, 1) EMITB(0x5B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvtos_ld(XD, MS, DS)                                                \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 1, 1) EMITB(0x5B)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvt (D = signed-int-to-fp S)
 * rounding mode comes from fp control register (set in FCTRL blocks)
 * NOTE: only default ROUNDN is supported on pre-VSX Power systems */

#define cvton_rr(XD, XS)                                                    \
        EVX(RXB(XD), RXB(XS),    0x00, K, 0, 1) EMITB(0x5B)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define cvton_ld(XD, MS, DS)                                                \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 0, 1) EMITB(0x5B)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

/* cvr (D = fp-to-signed-int S)
 * rounding mode is encoded directly (cannot be used in FCTRL blocks)
 * NOTE: on targets with full-IEEE SIMD fp-arithmetic the ROUND*_F mode
 * isn't always taken into account when used within full-IEEE ASM block
 * NOTE: due to compatibility with legacy targets, SIMD fp-to-int
 * round instructions are only accurate within 32-bit signed int range */

#define rnros_rr(XD, XS, mode)                                              \
        EVX(RXB(XD), RXB(XS),    0x00, K, 1, 3) EMITB(0x08)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(RT_SIMD_MODE_##mode&3))

#define cvros_rr(XD, XS, mode)                                              \
        rnros_rr(W(XD), W(XS), mode)                                        \
        cvzos_rr(W(XD), W(XD))

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

/* sregs */

#define sregs_sa() /* save all SIMD regs, destroys Reax */                  \
        movxx_ld(Reax, Mebp, inf_REGS)                                      \
        movox_st(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm7, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm8, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(Xmm9, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(XmmA, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(XmmB, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(XmmC, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_st(XmmD, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
    ADR EVX(1,             0,    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(0x06,       0x00,    0x00)                                      \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
    ADR EVX(1,             0,    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(0x07,       0x00,    0x00)

#define sregs_la() /* load all SIMD regs, destroys Reax */                  \
        movxx_ld(Reax, Mebp, inf_REGS)                                      \
        movox_ld(Xmm0, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm1, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm2, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm3, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm4, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm5, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm6, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm7, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm8, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(Xmm9, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(XmmA, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(XmmB, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(XmmC, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
        movox_ld(XmmD, Oeax, PLAIN)                                         \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
    ADR EVX(1,             0,    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(0x06,       0x00,    0x00)                                      \
        addxx_ri(Reax, IB(RT_SIMD_WIDTH32*4))                               \
    ADR EVX(1,             0,    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(0x07,       0x00,    0x00)

#endif /* RT_512 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_X32_512_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/