/******************************************************************************/
/* Copyright (c) 2013-2025 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_RTARCH_XHB_512X4V2_H
#define RT_RTARCH_XHB_512X4V2_H

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * rtarch_xHB_512x4v2.h: Implementation of x86_64 half+byte AVX512F/BW quads.
 *
 * This file is a part of the unified SIMD assembler framework (rtarch.h)
 * designed to be compatible with different processor architectures,
 * while maintaining strictly defined common API.
 *
 * Recommended naming scheme for instructions:
 *
 * cmdm*_rx - applies [cmd] to var-len packed SIMD: [r]egister (one operand)
 * cmdm*_rr - applies [cmd] to var-len packed SIMD: [r]egister from [r]egister
 *
 * cmdm*_rm - applies [cmd] to var-len packed SIMD: [r]egister from [m]emory
 * cmdm*_ld - applies [cmd] to var-len packed SIMD: as above (friendly alias)
 *
 * cmdg*_** - applies [cmd] to 16-bit elements SIMD args, packed-128-bit
 * cmdgb_** - applies [cmd] to u-char elements SIMD args, packed-128-bit
 * cmdgc_** - applies [cmd] to s-char elements SIMD args, packed-128-bit
 *
 * cmda*_** - applies [cmd] to 16-bit elements SIMD args, packed-256-bit
 * cmdab_** - applies [cmd] to u-char elements SIMD args, packed-256-bit
 * cmdac_** - applies [cmd] to s-char elements SIMD args, packed-256-bit
 *
 * cmdn*_** - applies [cmd] to 16-bit elements ELEM args, scalar-fp-only
 * cmdh*_** - applies [cmd] to 16-bit elements BASE args, BASE-regs-only
 * cmdb*_** - applies [cmd] to  8-bit elements BASE args, BASE-regs-only
 *
 * cmd*x_** - applies [cmd] to SIMD/BASE unsigned integer args, [x] - default
 * cmd*n_** - applies [cmd] to SIMD/BASE   signed integer args, [n] - negatable
 * cmd*s_** - applies [cmd] to SIMD/ELEM floating point   args, [s] - scalable
 *
 * The cmdm*_** (rtconf.h) instructions are intended for SPMD programming model
 * and simultaneously support 16/8-bit data elements (int, fp16 on ARM and x86).
 * In this model data paths are fixed-width, BASE and SIMD data elements are
 * width-compatible, code path divergence is handled via mkj**_** pseudo-ops.
 * Matching 16/8-bit BASE subsets cmdh* / cmdb* are defined in rtarch_*HB.h.
 *
 * Note, when using fixed-data-size 128/256-bit SIMD subsets simultaneously
 * upper 128-bit halves of full 256-bit SIMD registers may end up undefined.
 * On RISC targets they remain unchanged, while on x86-AVX they are zeroed.
 * This happens when registers written in 128-bit subset are then used/read
 * from within 256-bit subset. The same rule applies to mixing with 512-bit
 * and wider vectors. Use of scalars may leave respective vector registers
 * undefined, as seen from the perspective of any particular vector subset.
 *
 * 256-bit vectors used with wider subsets may not be compatible with regards
 * to memory loads/stores when mixed in the code. It means that data loaded
 * with wider vector and stored within 256-bit subset at the same address may
 * result in changing the initial representation in memory. The same can be
 * said about mixing vector and scalar subsets. Scalars can be completely
 * detached on some architectures. Use elm*x_st to store 1st vector element.
 * 128-bit vectors should be memory-compatible with any wider vector subset.
 *
 * Handling of NaNs in the floating point pipeline may not be consistent
 * across different architectures. Avoid NaNs entering the data flow by using
 * masking or control flow instructions. Apply special care when dealing with
 * floating point compare and min/max input/output. The result of floating point
 * compare instructions can be considered a -QNaN, though it is also interpreted
 * as integer -1 and is often treated as a mask. Most arithmetic instructions
 * should propagate QNaNs unchanged, however this behavior hasn't been tested.
 *
 * Note, that instruction subsets operating on vectors of different length
 * may support different number of SIMD registers, therefore mixing them
 * in the same code needs to be done with register awareness in mind.
 * For example, AVX-512 supports 32 SIMD registers, while AVX2 only has 16,
 * as does 256-bit paired subset on ARMv8, while 128-bit and SVE have 32.
 * These numbers should be consistent across architectures if properly
 * mapped to SIMD target mask presented in rtzero.h (compatibility layer).
 *
 * Interpretation of instruction parameters:
 *
 * upper-case params have triplet structure and require W to pass-forward
 * lower-case params are singular and can be used/passed as such directly
 *
 * XD - SIMD register serving as destination only, if present
 * XG - SIMD register serving as destination and first source
 * XS - SIMD register serving as second source (first if any)
 * XT - SIMD register serving as third source (second if any)
 *
 * RD - BASE register serving as destination only, if present
 * RG - BASE register serving as destination and first source
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

#if (defined RT_SIMD_CODE)

#if (RT_512X4 >= 1 && RT_512X4 <= 2)

#if (RT_512X4 == 1) /* instructions below require AVX512BW (not in AVX512F) */

#define ck1mx_rm(XS, MT, DT) /* not portable, do not use outside */         \
    ADR EVX(0,       RXB(MT), REN(XS), K, 1, 1) EMITB(0x75)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#define ck1mb_rm(XS, MT, DT) /* not portable, do not use outside */         \
    ADR EVX(0,       RXB(MT), REN(XS), K, 1, 1) EMITB(0x74)                 \
        MRM(0x01,    MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#define mz1mx_ld(XD, MS, DS) /* not portable, do not use outside */         \
    ADR EZW(RXB(XD), RXB(MS), REN(XD), K, 1, 2) EMITB(0x66)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define mz1mb_ld(XD, MS, DS) /* not portable, do not use outside */         \
    ADR EZX(RXB(XD), RXB(MS), REN(XD), K, 1, 2) EMITB(0x66)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#else  /* (RT_512X4 == 2) */

#define ck1mx_rm(XS, MT, DT) /* not portable, do not use outside */         \
        EVW(0,       RXB(XS),    0x00, K, 2, 2) EMITB(0x29)                 \
        MRM(0x01,    MOD(XS), REG(XS))

#define ck1mb_rm(XS, MT, DT) /* not portable, do not use outside */         \
        EVX(0,       RXB(XS),    0x00, K, 2, 2) EMITB(0x29)                 \
        MRM(0x01,    MOD(XS), REG(XS))

#define mz1mx_ld(XD, MS, DS) /* not portable, do not use outside */         \
        EVW(RXB(XD),       0,    0x00, K, 2, 2) EMITB(0x28)                 \
        MRM(REG(XD),    0x03,    0x01)

#define mz1mb_ld(XD, MS, DS) /* not portable, do not use outside */         \
        EVX(RXB(XD),       0,    0x00, K, 2, 2) EMITB(0x28)                 \
        MRM(REG(XD),    0x03,    0x01)

#endif /* (RT_512X4 == 2) */

/******************************************************************************/
/********************************   EXTERNAL   ********************************/
/******************************************************************************/

/******************************************************************************/
/**********************************   SIMD   **********************************/
/******************************************************************************/

/****************   packed half-precision generic move/logic   ****************/

/* mov (D = S) */

#define movmx_rr(XD, XS)                                                    \
        EVX(0,             0,    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVX(1,             1,    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVX(2,             2,    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))                                      \
        EVX(3,             3,    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movmx_ld(XD, MS, DS)                                                \
    ADR EVX(0,       RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
    ADR EVX(1,       RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
    ADR EVX(2,       RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
    ADR EVX(3,       RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#define movmx_st(XS, MD, DD)                                                \
    ADR EVX(0,       RXB(MD),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VAL(DD)), EMPTY)                                 \
    ADR EVX(1,       RXB(MD),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VZL(DD)), EMPTY)                                 \
    ADR EVX(2,       RXB(MD),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VSL(DD)), EMPTY)                                 \
    ADR EVX(3,       RXB(MD),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS),    0x02, REG(MD))                                      \
        AUX(SIB(MD), EMITW(VTL(DD)), EMPTY)

/* mmv (G = G mask-merge S) where (mask-elem: 0 keeps G, -1 picks S)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, 0-masked XS elems */

#if (RT_512X4 == 1)

#define mmvmx_rr(XG, XS)                                                    \
        andmx_rr(W(XS), Xmm0)                                               \
        annmx_rr(Xmm0, W(XG))                                               \
        orrmx_rr(Xmm0, W(XS))                                               \
        movmx_rr(W(XG), Xmm0)

#define mmvmx_ld(XG, MS, DS)                                                \
        notmx_rx(Xmm0)                                                      \
        andmx_rr(W(XG), Xmm0)                                               \
        annmx_ld(Xmm0, W(MS), W(DS))                                        \
        orrmx_rr(W(XG), Xmm0)

#define mmvmx_st(XS, MG, DG)                                                \
        andmx_rr(W(XS), Xmm0)                                               \
        annmx_ld(Xmm0, W(MG), W(DG))                                        \
        orrmx_rr(Xmm0, W(XS))                                               \
        movmx_st(Xmm0, W(MG), W(DG))

#else /* RT_512X4 == 2 */

#define mmvmx_rr(XG, XS)                                                    \
        ck1mx_rm(Xmm0, Mebp, inf_GPC07)                                     \
        EKW(0,             0,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1mx_rm(Xmm8, Mebp, inf_GPC07)                                     \
        EKW(1,             1,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1mx_rm(XmmG, Mebp, inf_GPC07)                                     \
        EKW(2,             2,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1mx_rm(XmmO, Mebp, inf_GPC07)                                     \
        EKW(3,             3,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mmvmx_ld(XG, MS, DS)                                                \
        ck1mx_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKW(0,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
        ck1mx_rm(Xmm8, Mebp, inf_GPC07)                                     \
    ADR EKW(1,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
        ck1mx_rm(XmmG, Mebp, inf_GPC07)                                     \
    ADR EKW(2,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
        ck1mx_rm(XmmO, Mebp, inf_GPC07)                                     \
    ADR EKW(3,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#define mmvmx_st(XS, MG, DG)                                                \
        ck1mx_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKW(0,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VAL(DG)), EMPTY)                                 \
        ck1mx_rm(Xmm8, Mebp, inf_GPC07)                                     \
    ADR EKW(1,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VZL(DG)), EMPTY)                                 \
        ck1mx_rm(XmmG, Mebp, inf_GPC07)                                     \
    ADR EKW(2,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VSL(DG)), EMPTY)                                 \
        ck1mx_rm(XmmO, Mebp, inf_GPC07)                                     \
    ADR EKW(3,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VTL(DG)), EMPTY)

#endif /* RT_512X4 == 2 */

/* and (G = G & S), (D = S & T) if (#D != #T) */

#define andmx_rr(XG, XS)                                                    \
        andmx3rr(W(XG), W(XG), W(XS))

#define andmx_ld(XG, MS, DS)                                                \
        andmx3ld(W(XG), W(XG), W(MS), W(DS))

#define andmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define andmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ann (G = ~G & S), (D = ~S & T) if (#D != #T) */

#define annmx_rr(XG, XS)                                                    \
        annmx3rr(W(XG), W(XG), W(XS))

#define annmx_ld(XG, MS, DS)                                                \
        annmx3ld(W(XG), W(XG), W(MS), W(DS))

#define annmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define annmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* orr (G = G | S), (D = S | T) if (#D != #T) */

#define orrmx_rr(XG, XS)                                                    \
        orrmx3rr(W(XG), W(XG), W(XS))

#define orrmx_ld(XG, MS, DS)                                                \
        orrmx3ld(W(XG), W(XG), W(MS), W(DS))

#define orrmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define orrmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xEB)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* orn (G = ~G | S), (D = ~S | T) if (#D != #T) */

#define ornmx_rr(XG, XS)                                                    \
        notmx_rx(W(XG))                                                     \
        orrmx_rr(W(XG), W(XS))

#define ornmx_ld(XG, MS, DS)                                                \
        notmx_rx(W(XG))                                                     \
        orrmx_ld(W(XG), W(MS), W(DS))

#define ornmx3rr(XD, XS, XT)                                                \
        notmx_rr(W(XD), W(XS))                                              \
        orrmx_rr(W(XD), W(XT))

#define ornmx3ld(XD, XS, MT, DT)                                            \
        notmx_rr(W(XD), W(XS))                                              \
        orrmx_ld(W(XD), W(MT), W(DT))

/* xor (G = G ^ S), (D = S ^ T) if (#D != #T) */

#define xormx_rr(XG, XS)                                                    \
        xormx3rr(W(XG), W(XG), W(XS))

#define xormx_ld(XG, MS, DS)                                                \
        xormx3ld(W(XG), W(XG), W(MS), W(DS))

#define xormx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define xormx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xEF)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* not (G = ~G), (D = ~S) */

#define notmx_rx(XG)                                                        \
        notmx_rr(W(XG), W(XG))

#define notmx_rr(XD, XS)                                                    \
        annmx3ld(W(XD), W(XS), Mebp, inf_GPC07)

/*************   packed half-precision integer arithmetic/shifts   ************/

#if (RT_512X4 < 2)

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addmx_rr(XG, XS)                                                    \
        addmx3rr(W(XG), W(XG), W(XS))

#define addmx_ld(XG, MS, DS)                                                \
        addmx3ld(W(XG), W(XG), W(MS), W(DS))

#define addmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        addmx_rx(W(XD))

#define addmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        addmx_rx(W(XD))

#define addmx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        addax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, unsigned */

#define adsmx_rr(XG, XS)                                                    \
        adsmx3rr(W(XG), W(XG), W(XS))

#define adsmx_ld(XG, MS, DS)                                                \
        adsmx3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        adsmx_rx(W(XD))

#define adsmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        adsmx_rx(W(XD))

#define adsmx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        adsax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, signed */

#define adsmn_rr(XG, XS)                                                    \
        adsmn3rr(W(XG), W(XG), W(XS))

#define adsmn_ld(XG, MS, DS)                                                \
        adsmn3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmn3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        adsmn_rx(W(XD))

#define adsmn3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        adsmn_rx(W(XD))

#define adsmn_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        adsan_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define submx_rr(XG, XS)                                                    \
        submx3rr(W(XG), W(XG), W(XS))

#define submx_ld(XG, MS, DS)                                                \
        submx3ld(W(XG), W(XG), W(MS), W(DS))

#define submx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        submx_rx(W(XD))

#define submx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        submx_rx(W(XD))

#define submx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        subax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, unsigned */

#define sbsmx_rr(XG, XS)                                                    \
        sbsmx3rr(W(XG), W(XG), W(XS))

#define sbsmx_ld(XG, MS, DS)                                                \
        sbsmx3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        sbsmx_rx(W(XD))

#define sbsmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        sbsmx_rx(W(XD))

#define sbsmx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        sbsax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, signed */

#define sbsmn_rr(XG, XS)                                                    \
        sbsmn3rr(W(XG), W(XG), W(XS))

#define sbsmn_ld(XG, MS, DS)                                                \
        sbsmn3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmn3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        sbsmn_rx(W(XD))

#define sbsmn3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        sbsmn_rx(W(XD))

#define sbsmn_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        sbsan_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mulmx_rr(XG, XS)                                                    \
        mulmx3rr(W(XG), W(XG), W(XS))

#define mulmx_ld(XG, MS, DS)                                                \
        mulmx3ld(W(XG), W(XG), W(MS), W(DS))

#define mulmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        mulmx_rx(W(XD))

#define mulmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        mulmx_rx(W(XD))

#define mulmx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        mulax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shlmx_ri(XG, IS)                                                    \
        shlmx3ri(W(XG), W(XG), W(IS))

#define shlmx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shlmx3ld(W(XG), W(XG), W(MS), W(DS))

#define shlmx3ri(XD, XS, IT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shlax3ri(W(XD), W(XS), W(IT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        shlax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        shlax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        shlax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        shlax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        shlax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        shlax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        shlax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shlmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shlax3ld(W(XD), W(XS), W(MT), W(DT))                                \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        shlax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        shlax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        shlax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        shlax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        shlax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        shlax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        shlax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrmx_ri(XG, IS)                                                    \
        shrmx3ri(W(XG), W(XG), W(IS))

#define shrmx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrmx3ld(W(XG), W(XG), W(MS), W(DS))

#define shrmx3ri(XD, XS, IT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shrax3ri(W(XD), W(XS), W(IT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        shrax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        shrax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        shrax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        shrax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        shrax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        shrax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        shrax_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shrax3ld(W(XD), W(XS), W(MT), W(DT))                                \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        shrax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        shrax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        shrax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        shrax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        shrax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        shrax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        shrax_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrmn_ri(XG, IS)                                                    \
        shrmn3ri(W(XG), W(XG), W(IS))

#define shrmn_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrmn3ld(W(XG), W(XG), W(MS), W(DS))

#define shrmn3ri(XD, XS, IT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shran3ri(W(XD), W(XS), W(IT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        shran_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        shran_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        shran_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        shran_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        shran_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        shran_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        shran_ri(W(XD), W(IT))                                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrmn3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        shran3ld(W(XD), W(XS), W(MT), W(DT))                                \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        shran_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        shran_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        shran_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        shran_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        shran_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        shran_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        shran_ld(W(XD), W(MT), W(DT))                                       \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svlmx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svlmx3rr(W(XG), W(XG), W(XS))

#define svlmx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svlmx3ld(W(XG), W(XG), W(MS), W(DS))

#define svlmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svlmx_rx(W(XD))

#define svlmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svlmx_rx(W(XD))

#define svlmx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Recx)                                                      \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x02))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x06))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x12))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x14))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x16))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x18))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x20))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x22))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x22))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x24))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x24))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x26))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x26))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x28))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x2A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x2C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x2E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x30))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x32))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x32))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x34))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x34))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x36))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x36))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x38))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x3A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x3C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x3E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x40))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x42))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x42))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x44))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x44))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x46))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x46))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x48))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x4A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x4C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x4E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x50))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x52))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x52))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x54))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x54))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x56))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x56))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x58))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x5A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x5C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x5E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x60))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x62))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x62))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x64))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x64))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x66))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x66))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x68))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x6A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x6C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x6E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x70))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x72))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x72))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x74))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x74))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x76))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x76))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x78))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x7A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x7C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x7E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x80))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x82))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x82))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x84))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x84))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x86))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x86))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x88))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x8A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x8C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x8E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x90))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x92))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x92))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x94))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x94))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x96))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x96))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x98))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9A))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x9A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9C))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x9C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9E))                              \
        shlhx_mx(Mebp,  inf_SCR01(0x9E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xA0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA2))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xA2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA4))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xA4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA6))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xA6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xA8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAA))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xAA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAC))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xAC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAE))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xAE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xB0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB2))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xB2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB4))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xB4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB6))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xB6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xB8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBA))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xBA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBC))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xBC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBE))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xBE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xC0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC2))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xC2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC4))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xC4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC6))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xC6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xC8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCA))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xCA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCC))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xCC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCE))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xCE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xD0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD2))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xD2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD4))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xD4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD6))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xD6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xD8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDA))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xDA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDC))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xDC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDE))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xDE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xE0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE2))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xE2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE4))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xE4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE6))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xE6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xE8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEA))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xEA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEC))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xEC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEE))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xEE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xF0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF2))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xF2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF4))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xF4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF6))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xF6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xF8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFA))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xFA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFC))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xFC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFE))                              \
        shlhx_mx(Mebp,  inf_SCR01(0xFE))                                    \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrmx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrmx3rr(W(XG), W(XG), W(XS))

#define svrmx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrmx3ld(W(XG), W(XG), W(MS), W(DS))

#define svrmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svrmx_rx(W(XD))

#define svrmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svrmx_rx(W(XD))

#define svrmx_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Recx)                                                      \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x02))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x06))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x12))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x14))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x16))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x18))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x20))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x22))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x22))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x24))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x24))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x26))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x26))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x28))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x2A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x2C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x2E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x30))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x32))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x32))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x34))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x34))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x36))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x36))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x38))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x3A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x3C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x3E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x40))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x42))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x42))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x44))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x44))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x46))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x46))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x48))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x4A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x4C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x4E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x50))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x52))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x52))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x54))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x54))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x56))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x56))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x58))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x5A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x5C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x5E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x60))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x62))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x62))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x64))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x64))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x66))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x66))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x68))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x6A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x6C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x6E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x70))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x72))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x72))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x74))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x74))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x76))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x76))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x78))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x7A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x7C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x7E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x80))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x82))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x82))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x84))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x84))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x86))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x86))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x88))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x8A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x8C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x8E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x90))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x92))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x92))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x94))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x94))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x96))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x96))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x98))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9A))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x9A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9C))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x9C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9E))                              \
        shrhx_mx(Mebp,  inf_SCR01(0x9E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xA0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA2))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xA2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA4))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xA4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA6))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xA6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xA8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAA))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xAA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAC))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xAC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAE))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xAE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xB0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB2))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xB2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB4))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xB4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB6))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xB6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xB8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBA))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xBA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBC))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xBC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBE))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xBE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xC0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC2))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xC2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC4))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xC4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC6))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xC6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xC8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCA))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xCA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCC))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xCC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCE))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xCE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xD0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD2))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xD2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD4))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xD4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD6))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xD6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xD8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDA))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xDA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDC))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xDC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDE))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xDE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xE0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE2))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xE2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE4))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xE4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE6))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xE6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xE8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEA))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xEA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEC))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xEC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEE))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xEE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xF0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF2))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xF2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF4))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xF4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF6))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xF6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xF8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFA))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xFA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFC))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xFC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFE))                              \
        shrhx_mx(Mebp,  inf_SCR01(0xFE))                                    \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrmn_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrmn3rr(W(XG), W(XG), W(XS))

#define svrmn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrmn3ld(W(XG), W(XG), W(MS), W(DS))

#define svrmn3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svrmn_rx(W(XD))

#define svrmn3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svrmn_rx(W(XD))

#define svrmn_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Recx)                                                      \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x00))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x02))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x04))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x06))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x08))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x10))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x12))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x14))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x16))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x18))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x1E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x20))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x22))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x22))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x24))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x24))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x26))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x26))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x28))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x2A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x2C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x2E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x2E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x30))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x32))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x32))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x34))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x34))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x36))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x36))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x38))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x3A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x3C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x3E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x3E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x40))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x42))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x42))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x44))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x44))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x46))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x46))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x48))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x4A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x4C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x4E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x4E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x50))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x52))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x52))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x54))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x54))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x56))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x56))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x58))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x5A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x5C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x5E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x5E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x60))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x62))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x62))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x64))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x64))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x66))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x66))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x68))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x6A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x6C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x6E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x6E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x70))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x72))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x72))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x74))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x74))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x76))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x76))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x78))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x7A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x7C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x7E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x7E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x80))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x82))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x82))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x84))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x84))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x86))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x86))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x88))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x8A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x8C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x8E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x8E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x90))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x92))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x92))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x94))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x94))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x96))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x96))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x98))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9A))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x9A))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9C))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x9C))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0x9E))                              \
        shrhn_mx(Mebp,  inf_SCR01(0x9E))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xA0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA2))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xA2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA4))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xA4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA6))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xA6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xA8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAA))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xAA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAC))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xAC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xAE))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xAE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xB0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB2))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xB2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB4))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xB4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB6))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xB6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xB8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBA))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xBA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBC))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xBC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xBE))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xBE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xC0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC2))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xC2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC4))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xC4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC6))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xC6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xC8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCA))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xCA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCC))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xCC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xCE))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xCE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xD0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD2))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xD2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD4))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xD4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD6))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xD6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xD8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDA))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xDA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDC))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xDC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xDE))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xDE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xE0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE2))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xE2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE4))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xE4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE6))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xE6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xE8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEA))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xEA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEC))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xEC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xEE))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xEE))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xF0))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF2))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xF2))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF4))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xF4))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF6))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xF6))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xF8))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFA))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xFA))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFC))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xFC))                                    \
        movhx_ld(Recx,  Mebp, inf_SCR02(0xFE))                              \
        shrhn_mx(Mebp,  inf_SCR01(0xFE))                                    \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#else /* RT_512X4 >= 2 */

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addmx_rr(XG, XS)                                                    \
        addmx3rr(W(XG), W(XG), W(XS))

#define addmx_ld(XG, MS, DS)                                                \
        addmx3ld(W(XG), W(XG), W(MS), W(DS))

#define addmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define addmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xFD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, unsigned */

#define adsmx_rr(XG, XS)                                                    \
        adsmx3rr(W(XG), W(XG), W(XS))

#define adsmx_ld(XG, MS, DS)                                                \
        adsmx3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define adsmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDD)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, signed */

#define adsmn_rr(XG, XS)                                                    \
        adsmn3rr(W(XG), W(XG), W(XS))

#define adsmn_ld(XG, MS, DS)                                                \
        adsmn3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmn3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define adsmn3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xED)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define submx_rr(XG, XS)                                                    \
        submx3rr(W(XG), W(XG), W(XS))

#define submx_ld(XG, MS, DS)                                                \
        submx3ld(W(XG), W(XG), W(MS), W(DS))

#define submx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define submx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xF9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, unsigned */

#define sbsmx_rr(XG, XS)                                                    \
        sbsmx3rr(W(XG), W(XG), W(XS))

#define sbsmx_ld(XG, MS, DS)                                                \
        sbsmx3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define sbsmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xD9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, signed */

#define sbsmn_rr(XG, XS)                                                    \
        sbsmn3rr(W(XG), W(XG), W(XS))

#define sbsmn_ld(XG, MS, DS)                                                \
        sbsmn3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmn3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define sbsmn3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xE9)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mulmx_rr(XG, XS)                                                    \
        mulmx3rr(W(XG), W(XG), W(XS))

#define mulmx_ld(XG, MS, DS)                                                \
        mulmx3ld(W(XG), W(XG), W(MS), W(DS))

#define mulmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define mulmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xD5)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shlmx_ri(XG, IS)                                                    \
        shlmx3ri(W(XG), W(XG), W(IS))

#define shlmx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shlmx3ld(W(XG), W(XG), W(MS), W(DS))

#define shlmx3ri(XD, XS, IT)                                                \
        EVX(0,             0, REG(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             1, REH(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             2, REI(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             3, REJ(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shlmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xF1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xF1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xF1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xF1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrmx_ri(XG, IS)                                                    \
        shrmx3ri(W(XG), W(XG), W(IS))

#define shrmx_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrmx3ld(W(XG), W(XG), W(MS), W(DS))

#define shrmx3ri(XD, XS, IT)                                                \
        EVX(0,             0, REG(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             1, REH(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             2, REI(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             3, REJ(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x02,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shrmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xD1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xD1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xD1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xD1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrmn_ri(XG, IS)                                                    \
        shrmn3ri(W(XG), W(XG), W(IS))

#define shrmn_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrmn3ld(W(XG), W(XG), W(MS), W(DS))

#define shrmn3ri(XD, XS, IT)                                                \
        EVX(0,             0, REG(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             1, REH(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             2, REI(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))                               \
        EVX(0,             3, REJ(XD), K, 1, 1) EMITB(0x71)                 \
        MRM(0x04,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

#define shrmn3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xE1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xE1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xE1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xE1)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svlmx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svlmx3rr(W(XG), W(XG), W(XS))

#define svlmx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svlmx3ld(W(XG), W(XG), W(MS), W(DS))

#define svlmx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svlmx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x12)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrmx_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrmx3rr(W(XG), W(XG), W(XS))

#define svrmx_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrmx3ld(W(XG), W(XG), W(MS), W(DS))

#define svrmx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svrmx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x10)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrmn_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrmn3rr(W(XG), W(XG), W(XS))

#define svrmn_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrmn3ld(W(XG), W(XG), W(MS), W(DS))

#define svrmn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(1,             1, REH(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(2,             2, REI(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVW(3,             3, REJ(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define svrmn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVW(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVW(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVW(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x11)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

#endif /* RT_512X4 >= 2 */

/*****************   packed half-precision integer compare   ******************/

#if (RT_512X4 < 2)

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define minmx_rr(XG, XS)                                                    \
        minmx3rr(W(XG), W(XG), W(XS))

#define minmx_ld(XG, MS, DS)                                                \
        minmx3ld(W(XG), W(XG), W(MS), W(DS))

#define minmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        minmx_rx(W(XD))

#define minmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        minmx_rx(W(XD))

#define minmx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        minax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define minmn_rr(XG, XS)                                                    \
        minmn3rr(W(XG), W(XG), W(XS))

#define minmn_ld(XG, MS, DS)                                                \
        minmn3ld(W(XG), W(XG), W(MS), W(DS))

#define minmn3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        minmn_rx(W(XD))

#define minmn3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        minmn_rx(W(XD))

#define minmn_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        minan_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxmx_rr(XG, XS)                                                    \
        maxmx3rr(W(XG), W(XG), W(XS))

#define maxmx_ld(XG, MS, DS)                                                \
        maxmx3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        maxmx_rx(W(XD))

#define maxmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        maxmx_rx(W(XD))

#define maxmx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        maxax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxmn_rr(XG, XS)                                                    \
        maxmn3rr(W(XG), W(XG), W(XS))

#define maxmn_ld(XG, MS, DS)                                                \
        maxmn3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmn3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        maxmn_rx(W(XD))

#define maxmn3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        maxmn_rx(W(XD))

#define maxmn_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        maxan_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqmx_rr(XG, XS)                                                    \
        ceqmx3rr(W(XG), W(XG), W(XS))

#define ceqmx_ld(XG, MS, DS)                                                \
        ceqmx3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqmx3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        ceqmx_rx(W(XD))

#define ceqmx3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        ceqmx_rx(W(XD))

#define ceqmx_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        ceqax_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtmn_rr(XG, XS)                                                    \
        cgtmn3rr(W(XG), W(XG), W(XS))

#define cgtmn_ld(XG, MS, DS)                                                \
        cgtmn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmn3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cgtmn_rx(W(XD))

#define cgtmn3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cgtmn_rx(W(XD))

#define cgtmn_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        cgtan_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnemx_rr(XG, XS)                                                    \
        cnemx3rr(W(XG), W(XG), W(XS))

#define cnemx_ld(XG, MS, DS)                                                \
        cnemx3ld(W(XG), W(XG), W(MS), W(DS))

#define cnemx3rr(XD, XS, XT)                                                \
        ceqmx3rr(W(XD), W(XS), W(XT))                                       \
        notmx_rx(W(XD))

#define cnemx3ld(XD, XS, MT, DT)                                            \
        ceqmx3ld(W(XD), W(XS), W(MT), W(DT))                                \
        notmx_rx(W(XD))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltmx_rr(XG, XS)                                                    \
        cltmx3rr(W(XG), W(XG), W(XS))

#define cltmx_ld(XG, MS, DS)                                                \
        cltmx3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmx3rr(XD, XS, XT)                                                \
        minmx3rr(W(XD), W(XS), W(XT))                                       \
        cnemx_rr(W(XD), W(XT))

#define cltmx3ld(XD, XS, MT, DT)                                            \
        minmx3ld(W(XD), W(XS), W(MT), W(DT))                                \
        cnemx_ld(W(XD), W(MT), W(DT))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltmn_rr(XG, XS)                                                    \
        cltmn3rr(W(XG), W(XG), W(XS))

#define cltmn_ld(XG, MS, DS)                                                \
        cltmn3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmn3rr(XD, XS, XT)                                                \
        cgtmn3rr(W(XD), W(XT), W(XS))

#define cltmn3ld(XD, XS, MT, DT)                                            \
        minmn3ld(W(XD), W(XS), W(MT), W(DT))                                \
        cnemx_ld(W(XD), W(MT), W(DT))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define clemx_rr(XG, XS)                                                    \
        clemx3rr(W(XG), W(XG), W(XS))

#define clemx_ld(XG, MS, DS)                                                \
        clemx3ld(W(XG), W(XG), W(MS), W(DS))

#define clemx3rr(XD, XS, XT)                                                \
        maxmx3rr(W(XD), W(XS), W(XT))                                       \
        ceqmx_rr(W(XD), W(XT))

#define clemx3ld(XD, XS, MT, DT)                                            \
        maxmx3ld(W(XD), W(XS), W(MT), W(DT))                                \
        ceqmx_ld(W(XD), W(MT), W(DT))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define clemn_rr(XG, XS)                                                    \
        clemn3rr(W(XG), W(XG), W(XS))

#define clemn_ld(XG, MS, DS)                                                \
        clemn3ld(W(XG), W(XG), W(MS), W(DS))

#define clemn3rr(XD, XS, XT)                                                \
        cgtmn3rr(W(XD), W(XS), W(XT))                                       \
        notmx_rx(W(XD))

#define clemn3ld(XD, XS, MT, DT)                                            \
        cgtmn3ld(W(XD), W(XS), W(MT), W(DT))                                \
        notmx_rx(W(XD))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtmx_rr(XG, XS)                                                    \
        cgtmx3rr(W(XG), W(XG), W(XS))

#define cgtmx_ld(XG, MS, DS)                                                \
        cgtmx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmx3rr(XD, XS, XT)                                                \
        maxmx3rr(W(XD), W(XS), W(XT))                                       \
        cnemx_rr(W(XD), W(XT))

#define cgtmx3ld(XD, XS, MT, DT)                                            \
        maxmx3ld(W(XD), W(XS), W(MT), W(DT))                                \
        cnemx_ld(W(XD), W(MT), W(DT))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgemx_rr(XG, XS)                                                    \
        cgemx3rr(W(XG), W(XG), W(XS))

#define cgemx_ld(XG, MS, DS)                                                \
        cgemx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemx3rr(XD, XS, XT)                                                \
        minmx3rr(W(XD), W(XS), W(XT))                                       \
        ceqmx_rr(W(XD), W(XT))

#define cgemx3ld(XD, XS, MT, DT)                                            \
        minmx3ld(W(XD), W(XS), W(MT), W(DT))                                \
        ceqmx_ld(W(XD), W(MT), W(DT))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgemn_rr(XG, XS)                                                    \
        cgemn3rr(W(XG), W(XG), W(XS))

#define cgemn_ld(XG, MS, DS)                                                \
        cgemn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemn3rr(XD, XS, XT)                                                \
        minmn3rr(W(XD), W(XS), W(XT))                                       \
        ceqmx_rr(W(XD), W(XT))

#define cgemn3ld(XD, XS, MT, DT)                                            \
        minmn3ld(W(XD), W(XS), W(MT), W(DT))                                \
        ceqmx_ld(W(XD), W(MT), W(DT))

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE16_2K8  0x00000000 /* none satisfy the condition */
#define RT_SIMD_MASK_FULL16_2K8  0xFC00FC00 /*  all satisfy the condition */

#define adpax3rr(XD, XS, XT)     /* not portable, do not use outside */     \
        VEX(RXB(XD), RXB(XT), REN(XS), 1, 1, 2) EMITB(0x01)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define adpax3ld(XD, XS, MT, DT) /* not portable, do not use outside */     \
        VEX(RXB(XD), RXB(MT), REN(XS), 1, 1, 2) EMITB(0x01)                 \
        MRM(REG(XD), MOD(MT), REG(MT))                                      \
        AUX(SIB(MT), CMD(DT), EMPTY)

#define mkjmx_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        adpax3ld(W(XS), W(XS), Mebp, inf_SCR01(0x20))                       \
        movax_st(W(XS), Mebp, inf_SCR02(0x00))                              \
        movax_ld(W(XS), Mebp, inf_SCR01(0x40))                              \
        adpax3ld(W(XS), W(XS), Mebp, inf_SCR01(0x60))                       \
        movax_st(W(XS), Mebp, inf_SCR02(0x20))                              \
        movax_ld(W(XS), Mebp, inf_SCR01(0x80))                              \
        adpax3ld(W(XS), W(XS), Mebp, inf_SCR01(0xA0))                       \
        movax_st(W(XS), Mebp, inf_SCR02(0x40))                              \
        movax_ld(W(XS), Mebp, inf_SCR01(0xC0))                              \
        adpax3ld(W(XS), W(XS), Mebp, inf_SCR01(0xE0))                       \
        adpax3ld(W(XS), W(XS), Mebp, inf_SCR02(0x40))                       \
        adpax3ld(W(XS), W(XS), Mebp, inf_SCR02(0x20))                       \
        adpax3ld(W(XS), W(XS), Mebp, inf_SCR02(0x00))                       \
        adpax3rr(W(XS), W(XS), W(XS))                                       \
        adpax3rr(W(XS), W(XS), W(XS))                                       \
        adpax3rr(W(XS), W(XS), W(XS))                                       \
        movrs_st(W(XS), Mebp, inf_SCR02(0))                                 \
        movmx_ld(W(XS), Mebp, inf_SCR01(0))                                 \
        cmpwx_mi(Mebp, inf_SCR02(0), IW(RT_SIMD_MASK_##mask##16_2K8))       \
        jeqxx_lb(lb)

#else /* RT_512X4 >= 2 */

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define minmx_rr(XG, XS)                                                    \
        minmx3rr(W(XG), W(XG), W(XS))

#define minmx_ld(XG, MS, DS)                                                \
        minmx3ld(W(XG), W(XG), W(MS), W(DS))

#define minmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x3A)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define minmn_rr(XG, XS)                                                    \
        minmn3rr(W(XG), W(XG), W(XS))

#define minmn_ld(XG, MS, DS)                                                \
        minmn3ld(W(XG), W(XG), W(MS), W(DS))

#define minmn3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minmn3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xEA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxmx_rr(XG, XS)                                                    \
        maxmx3rr(W(XG), W(XG), W(XS))

#define maxmx_ld(XG, MS, DS)                                                \
        maxmx3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmx3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxmx3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x3E)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxmn_rr(XG, XS)                                                    \
        maxmn3rr(W(XG), W(XG), W(XS))

#define maxmn_ld(XG, MS, DS)                                                \
        maxmn3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmn3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxmn3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xEE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqmx_rr(XG, XS)                                                    \
        ceqmx3rr(W(XG), W(XG), W(XS))

#define ceqmx_ld(XG, MS, DS)                                                \
        ceqmx3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqmx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define ceqmx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x00))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x00))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x00))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x00))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnemx_rr(XG, XS)                                                    \
        cnemx3rr(W(XG), W(XG), W(XS))

#define cnemx_ld(XG, MS, DS)                                                \
        cnemx3ld(W(XG), W(XG), W(MS), W(DS))

#define cnemx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define cnemx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x04))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x04))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x04))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x04))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltmx_rr(XG, XS)                                                    \
        cltmx3rr(W(XG), W(XG), W(XS))

#define cltmx_ld(XG, MS, DS)                                                \
        cltmx3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define cltmx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x01))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x01))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x01))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x01))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltmn_rr(XG, XS)                                                    \
        cltmn3rr(W(XG), W(XG), W(XS))

#define cltmn_ld(XG, MS, DS)                                                \
        cltmn3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define cltmn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x01))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x01))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x01))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x01))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define clemx_rr(XG, XS)                                                    \
        clemx3rr(W(XG), W(XG), W(XS))

#define clemx_ld(XG, MS, DS)                                                \
        clemx3ld(W(XG), W(XG), W(MS), W(DS))

#define clemx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define clemx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x02))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x02))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x02))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x02))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define clemn_rr(XG, XS)                                                    \
        clemn3rr(W(XG), W(XG), W(XS))

#define clemn_ld(XG, MS, DS)                                                \
        clemn3ld(W(XG), W(XG), W(MS), W(DS))

#define clemn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define clemn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x02))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x02))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x02))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x02))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtmx_rr(XG, XS)                                                    \
        cgtmx3rr(W(XG), W(XG), W(XS))

#define cgtmx_ld(XG, MS, DS)                                                \
        cgtmx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define cgtmx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x06))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x06))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x06))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x06))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtmn_rr(XG, XS)                                                    \
        cgtmn3rr(W(XG), W(XG), W(XS))

#define cgtmn_ld(XG, MS, DS)                                                \
        cgtmn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define cgtmn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x06))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x06))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x06))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x06))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgemx_rr(XG, XS)                                                    \
        cgemx3rr(W(XG), W(XG), W(XS))

#define cgemx_ld(XG, MS, DS)                                                \
        cgemx3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemx3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define cgemx3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x05))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x05))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x05))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x05))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgemn_rr(XG, XS)                                                    \
        cgemn3rr(W(XG), W(XG), W(XS))

#define cgemn_ld(XG, MS, DS)                                                \
        cgemn3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemn3rr(XD, XS, XT)                                                \
        EVW(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVW(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

#define cgemn3ld(XD, XS, MT, DT)                                            \
    ADR EVW(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x05))                           \
        mz1mx_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x05))                           \
        mz1mx_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x05))                           \
        mz1mx_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVW(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x05))                           \
        mz1mx_ld(Z(XD), Mebp, inf_GPC07)

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE16_2K8  0x00000000 /* none satisfy the condition */
#define RT_SIMD_MASK_FULL16_2K8  0xFFFFFFFF /*  all satisfy the condition */

#define mk1hx_rx(RD)         /* not portable, do not use outside */         \
        VEX(RXB(RD),       0,    0x00, 0, 3, 1) EMITB(0x93)                 \
        MRM(REG(RD),    0x03,    0x01)

#define mkjmx_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        ck1mx_rm(W(XS), Mebp, inf_GPC07)                                    \
        mk1hx_rx(Reax)                                                      \
        REX(1,             0) EMITB(0x8B)                                   \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1mx_rm(V(XS), Mebp, inf_GPC07)                                    \
        mk1hx_rx(Reax)                                                      \
        REX(1,             0)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##16_2K8 & 0x1) << 1)))  \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1mx_rm(X(XS), Mebp, inf_GPC07)                                    \
        mk1hx_rx(Reax)                                                      \
        REX(1,             0)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##16_2K8 & 0x1) << 1)))  \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1mx_rm(Z(XS), Mebp, inf_GPC07)                                    \
        mk1hx_rx(Reax)                                                      \
        REX(0,             1)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##16_2K8 & 0x1) << 1)))  \
        MRM(0x00,       0x03, 0x07)                                         \
        cmpwx_ri(Reax, IW(RT_SIMD_MASK_##mask##16_2K8))                     \
        jeqxx_lb(lb)

#endif /* RT_512X4 >= 2 */

/****************   packed byte-precision generic move/logic   ****************/

/* mmv (G = G mask-merge S) where (mask-elem: 0 keeps G, -1 picks S)
 * uses Xmm0 implicitly as a mask register, destroys Xmm0, 0-masked XS elems */

#if (RT_512X4 == 1)

#define mmvmb_rr(XG, XS)                                                    \
        andmx_rr(W(XS), Xmm0)                                               \
        annmx_rr(Xmm0, W(XG))                                               \
        orrmx_rr(Xmm0, W(XS))                                               \
        movmx_rr(W(XG), Xmm0)

#define mmvmb_ld(XG, MS, DS)                                                \
        notmx_rx(Xmm0)                                                      \
        andmx_rr(W(XG), Xmm0)                                               \
        annmx_ld(Xmm0, W(MS), W(DS))                                        \
        orrmx_rr(W(XG), Xmm0)

#define mmvmb_st(XS, MG, DG)                                                \
        andmx_rr(W(XS), Xmm0)                                               \
        annmx_ld(Xmm0, W(MG), W(DG))                                        \
        orrmx_rr(Xmm0, W(XS))                                               \
        movmx_st(Xmm0, W(MG), W(DG))

#else /* RT_512X4 == 2 */

#define mmvmb_rr(XG, XS)                                                    \
        ck1mb_rm(Xmm0, Mebp, inf_GPC07)                                     \
        EKX(0,             0,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1mb_rm(Xmm8, Mebp, inf_GPC07)                                     \
        EKX(1,             1,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1mb_rm(XmmG, Mebp, inf_GPC07)                                     \
        EKX(2,             2,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))                                      \
        ck1mb_rm(XmmO, Mebp, inf_GPC07)                                     \
        EKX(3,             3,    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG), MOD(XS), REG(XS))

#define mmvmb_ld(XG, MS, DS)                                                \
        ck1mb_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKX(0,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VAL(DS)), EMPTY)                                 \
        ck1mb_rm(Xmm8, Mebp, inf_GPC07)                                     \
    ADR EKX(1,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VZL(DS)), EMPTY)                                 \
        ck1mb_rm(XmmG, Mebp, inf_GPC07)                                     \
    ADR EKX(2,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VSL(DS)), EMPTY)                                 \
        ck1mb_rm(XmmO, Mebp, inf_GPC07)                                     \
    ADR EKX(3,       RXB(MS),    0x00, K, 3, 1) EMITB(0x6F)                 \
        MRM(REG(XG),    0x02, REG(MS))                                      \
        AUX(SIB(MS), EMITW(VTL(DS)), EMPTY)

#define mmvmb_st(XS, MG, DG)                                                \
        ck1mb_rm(Xmm0, Mebp, inf_GPC07)                                     \
    ADR EKX(0,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VAL(DG)), EMPTY)                                 \
        ck1mb_rm(Xmm8, Mebp, inf_GPC07)                                     \
    ADR EKX(1,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VZL(DG)), EMPTY)                                 \
        ck1mb_rm(XmmG, Mebp, inf_GPC07)                                     \
    ADR EKX(2,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VSL(DG)), EMPTY)                                 \
        ck1mb_rm(XmmO, Mebp, inf_GPC07)                                     \
    ADR EKX(3,       RXB(MG),    0x00, K, 3, 1) EMITB(0x7F)                 \
        MRM(REG(XS),    0x02, REG(MG))                                      \
        AUX(SIB(MG), EMITW(VTL(DG)), EMPTY)

#endif /* RT_512X4 == 2 */

/* move/logic instructions are sizeless and provided in 16-bit subset above */

/*************   packed byte-precision integer arithmetic/shifts   ************/

#if (RT_512X4 < 2)

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addmb_rr(XG, XS)                                                    \
        addmb3rr(W(XG), W(XG), W(XS))

#define addmb_ld(XG, MS, DS)                                                \
        addmb3ld(W(XG), W(XG), W(MS), W(DS))

#define addmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        addmb_rx(W(XD))

#define addmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        addmb_rx(W(XD))

#define addmb_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        addab_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, unsigned */

#define adsmb_rr(XG, XS)                                                    \
        adsmb3rr(W(XG), W(XG), W(XS))

#define adsmb_ld(XG, MS, DS)                                                \
        adsmb3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        adsmb_rx(W(XD))

#define adsmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        adsmb_rx(W(XD))

#define adsmb_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        adsab_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, signed */

#define adsmc_rr(XG, XS)                                                    \
        adsmc3rr(W(XG), W(XG), W(XS))

#define adsmc_ld(XG, MS, DS)                                                \
        adsmc3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmc3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        adsmc_rx(W(XD))

#define adsmc3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        adsmc_rx(W(XD))

#define adsmc_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        adsac_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define submb_rr(XG, XS)                                                    \
        submb3rr(W(XG), W(XG), W(XS))

#define submb_ld(XG, MS, DS)                                                \
        submb3ld(W(XG), W(XG), W(MS), W(DS))

#define submb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        submb_rx(W(XD))

#define submb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        submb_rx(W(XD))

#define submb_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        subab_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, unsigned */

#define sbsmb_rr(XG, XS)                                                    \
        sbsmb3rr(W(XG), W(XG), W(XS))

#define sbsmb_ld(XG, MS, DS)                                                \
        sbsmb3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        sbsmb_rx(W(XD))

#define sbsmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        sbsmb_rx(W(XD))

#define sbsmb_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        sbsab_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, signed */

#define sbsmc_rr(XG, XS)                                                    \
        sbsmc3rr(W(XG), W(XG), W(XS))

#define sbsmc_ld(XG, MS, DS)                                                \
        sbsmc3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmc3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        sbsmc_rx(W(XD))

#define sbsmc3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        sbsmc_rx(W(XD))

#define sbsmc_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        sbsac_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#else /* RT_512X4 >= 2 */

/* add (G = G + S), (D = S + T) if (#D != #T) */

#define addmb_rr(XG, XS)                                                    \
        addmb3rr(W(XG), W(XG), W(XS))

#define addmb_ld(XG, MS, DS)                                                \
        addmb3ld(W(XG), W(XG), W(MS), W(DS))

#define addmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define addmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xFC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, unsigned */

#define adsmb_rr(XG, XS)                                                    \
        adsmb3rr(W(XG), W(XG), W(XS))

#define adsmb_ld(XG, MS, DS)                                                \
        adsmb3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define adsmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ads (G = G + S), (D = S + T) if (#D != #T) - saturate, signed */

#define adsmc_rr(XG, XS)                                                    \
        adsmc3rr(W(XG), W(XG), W(XS))

#define adsmc_ld(XG, MS, DS)                                                \
        adsmc3ld(W(XG), W(XG), W(MS), W(DS))

#define adsmc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define adsmc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xEC)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sub (G = G - S), (D = S - T) if (#D != #T) */

#define submb_rr(XG, XS)                                                    \
        submb3rr(W(XG), W(XG), W(XS))

#define submb_ld(XG, MS, DS)                                                \
        submb3ld(W(XG), W(XG), W(MS), W(DS))

#define submb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define submb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xF8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, unsigned */

#define sbsmb_rr(XG, XS)                                                    \
        sbsmb3rr(W(XG), W(XG), W(XS))

#define sbsmb_ld(XG, MS, DS)                                                \
        sbsmb3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define sbsmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xD8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* sbs (G = G - S), (D = S - T) if (#D != #T) - saturate, signed */

#define sbsmc_rr(XG, XS)                                                    \
        sbsmc3rr(W(XG), W(XG), W(XS))

#define sbsmc_ld(XG, MS, DS)                                                \
        sbsmc3ld(W(XG), W(XG), W(MS), W(DS))

#define sbsmc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define sbsmc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xE8)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

#endif /* RT_512X4 >= 2 */

/* mul (G = G * S), (D = S * T) if (#D != #T) */

#define mulmb_rr(XG, XS)                                                    \
        mulmb3rr(W(XG), W(XG), W(XS))

#define mulmb_ld(XG, MS, DS)                                                \
        mulmb3ld(W(XG), W(XG), W(MS), W(DS))

#define mulmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        mulmb_rx(W(XD))

#define mulmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        mulmb_rx(W(XD))

#define mulmb_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Recx)                                                      \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x00))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x00))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x01))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x01))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x01))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x02))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x02))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x03))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x03))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x03))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x04))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x04))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x05))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x05))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x05))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x06))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x06))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x07))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x07))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x07))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x08))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x08))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x09))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x09))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x09))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x0A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x0A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x0B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x0B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x0B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x0C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x0C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x0D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x0D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x0D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x0E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x0E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x0F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x0F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x0F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x10))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x10))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x11))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x11))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x11))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x12))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x12))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x13))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x13))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x13))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x14))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x14))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x15))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x15))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x15))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x16))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x16))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x17))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x17))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x17))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x18))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x18))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x19))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x19))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x19))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x1A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x1A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x1B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x1B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x1B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x1C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x1C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x1D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x1D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x1D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x1E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x1E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x1F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x1F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x1F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x20))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x20))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x21))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x21))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x21))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x22))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x22))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x22))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x23))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x23))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x23))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x24))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x24))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x24))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x25))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x25))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x25))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x26))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x26))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x26))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x27))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x27))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x27))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x28))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x28))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x29))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x29))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x29))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x2A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x2A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x2A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x2B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x2B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x2B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x2C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x2C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x2C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x2D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x2D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x2D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x2E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x2E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x2E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x2F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x2F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x2F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x30))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x30))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x31))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x31))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x31))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x32))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x32))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x32))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x33))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x33))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x33))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x34))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x34))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x34))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x35))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x35))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x35))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x36))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x36))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x36))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x37))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x37))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x37))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x38))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x38))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x39))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x39))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x39))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x3A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x3A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x3A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x3B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x3B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x3B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x3C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x3C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x3C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x3D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x3D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x3D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x3E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x3E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x3E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x3F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x3F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x3F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x40))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x40))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x41))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x41))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x41))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x42))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x42))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x42))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x43))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x43))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x43))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x44))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x44))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x44))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x45))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x45))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x45))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x46))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x46))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x46))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x47))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x47))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x47))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x48))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x48))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x49))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x49))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x49))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x4A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x4A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x4A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x4B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x4B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x4B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x4C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x4C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x4C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x4D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x4D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x4D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x4E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x4E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x4E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x4F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x4F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x4F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x50))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x50))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x51))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x51))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x51))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x52))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x52))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x52))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x53))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x53))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x53))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x54))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x54))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x54))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x55))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x55))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x55))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x56))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x56))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x56))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x57))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x57))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x57))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x58))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x58))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x59))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x59))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x59))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x5A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x5A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x5A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x5B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x5B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x5B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x5C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x5C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x5C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x5D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x5D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x5D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x5E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x5E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x5E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x5F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x5F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x5F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x60))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x60))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x61))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x61))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x61))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x62))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x62))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x62))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x63))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x63))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x63))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x64))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x64))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x64))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x65))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x65))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x65))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x66))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x66))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x66))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x67))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x67))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x67))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x68))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x68))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x69))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x69))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x69))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x6A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x6A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x6A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x6B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x6B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x6B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x6C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x6C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x6C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x6D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x6D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x6D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x6E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x6E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x6E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x6F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x6F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x6F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x70))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x70))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x71))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x71))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x71))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x72))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x72))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x72))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x73))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x73))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x73))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x74))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x74))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x74))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x75))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x75))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x75))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x76))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x76))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x76))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x77))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x77))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x77))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x78))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x78))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x79))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x79))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x79))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x7A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x7A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x7A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x7B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x7B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x7B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x7C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x7C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x7C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x7D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x7D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x7D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x7E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x7E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x7E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x7F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x7F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x7F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x80))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x80))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x81))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x81))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x81))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x82))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x82))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x82))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x83))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x83))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x83))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x84))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x84))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x84))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x85))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x85))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x85))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x86))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x86))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x86))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x87))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x87))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x87))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x88))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x88))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x89))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x89))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x89))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x8A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x8A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x8A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x8B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x8B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x8B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x8C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x8C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x8C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x8D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x8D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x8D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x8E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x8E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x8E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x8F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x8F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x8F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x90))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x90))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x91))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x91))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x91))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x92))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x92))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x92))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x93))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x93))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x93))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x94))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x94))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x94))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x95))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x95))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x95))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x96))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x96))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x96))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x97))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x97))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x97))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x98))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x98))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x99))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x99))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x99))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x9A))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x9A))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x9A))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x9B))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x9B))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x9B))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x9C))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x9C))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x9C))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x9D))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x9D))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x9D))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x9E))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x9E))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x9E))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0x9F))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0x9F))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0x9F))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA0))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA0))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA1))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA1))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA1))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA2))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA2))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA2))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA3))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA3))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA3))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA4))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA4))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA4))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA5))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA5))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA5))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA6))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA6))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA6))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA7))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA7))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA7))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA8))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA8))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xA9))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xA9))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xA9))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xAA))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xAA))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xAA))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xAB))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xAB))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xAB))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xAC))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xAC))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xAC))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xAD))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xAD))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xAD))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xAE))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xAE))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xAE))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xAF))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xAF))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xAF))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB0))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB0))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB1))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB1))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB1))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB2))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB2))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB2))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB3))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB3))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB3))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB4))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB4))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB4))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB5))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB5))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB5))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB6))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB6))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB6))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB7))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB7))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB7))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB8))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB8))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xB9))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xB9))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xB9))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xBA))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xBA))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xBA))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xBB))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xBB))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xBB))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xBC))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xBC))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xBC))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xBD))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xBD))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xBD))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xBE))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xBE))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xBE))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xBF))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xBF))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xBF))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC0))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC0))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC1))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC1))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC1))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC2))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC2))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC2))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC3))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC3))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC3))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC4))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC4))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC4))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC5))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC5))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC5))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC6))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC6))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC6))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC7))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC7))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC7))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC8))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC8))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xC9))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xC9))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xC9))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xCA))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xCA))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xCA))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xCB))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xCB))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xCB))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xCC))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xCC))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xCC))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xCD))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xCD))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xCD))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xCE))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xCE))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xCE))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xCF))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xCF))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xCF))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD0))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD0))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD1))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD1))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD1))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD2))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD2))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD2))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD3))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD3))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD3))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD4))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD4))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD4))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD5))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD5))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD5))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD6))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD6))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD6))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD7))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD7))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD7))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD8))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD8))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xD9))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xD9))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xD9))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xDA))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xDA))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xDA))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xDB))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xDB))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xDB))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xDC))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xDC))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xDC))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xDD))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xDD))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xDD))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xDE))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xDE))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xDE))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xDF))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xDF))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xDF))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE0))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE0))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE1))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE1))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE1))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE2))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE2))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE2))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE3))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE3))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE3))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE4))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE4))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE4))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE5))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE5))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE5))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE6))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE6))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE6))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE7))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE7))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE7))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE8))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE8))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xE9))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xE9))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xE9))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xEA))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xEA))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xEA))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xEB))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xEB))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xEB))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xEC))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xEC))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xEC))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xED))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xED))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xED))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xEE))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xEE))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xEE))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xEF))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xEF))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xEF))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF0))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF0))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF1))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF1))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF1))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF2))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF2))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF2))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF3))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF3))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF3))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF4))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF4))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF4))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF5))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF5))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF5))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF6))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF6))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF6))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF7))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF7))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF7))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF8))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF8))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xF9))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xF9))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xF9))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xFA))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xFA))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xFA))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xFB))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xFB))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xFB))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xFC))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xFC))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xFC))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xFD))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xFD))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xFD))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xFE))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xFE))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xFE))                              \
        movbx_ld(Recx,  Mebp, inf_SCR01(0xFF))                              \
        mulbx_ld(Recx,  Mebp, inf_SCR02(0xFF))                              \
        movbx_st(Recx,  Mebp, inf_SCR01(0xFF))                              \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* shl (G = G << S), (D = S << T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shlmb_ri(XG, IS)                                                    \
        shlmb3ri(W(XG), W(XG), W(IS))

#define shlmb_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shlmb3ld(W(XG), W(XG), W(MS), W(DS))

#define shlmb3ri(XD, XS, IT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movbx_ri(Recx, W(IT))                                               \
        shlmb_xx()                                                          \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shlmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movbx_ld(Recx, W(MT), W(DT))                                        \
        shlmb_xx()                                                          \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shlmb_xx() /* not portable, do not use outside */                   \
        shlbx_mx(Mebp,  inf_SCR01(0x00))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x01))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x02))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x03))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x04))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x05))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x06))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x07))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x08))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x09))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x0B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x0D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x0F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x10))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x11))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x12))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x13))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x14))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x15))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x16))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x17))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x18))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x19))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x1B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x1D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x1F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x20))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x21))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x22))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x23))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x24))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x25))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x26))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x27))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x28))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x29))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x2A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x2B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x2C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x2D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x2E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x2F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x30))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x31))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x32))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x33))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x34))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x35))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x36))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x37))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x38))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x39))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x3A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x3B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x3C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x3D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x3E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x3F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x40))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x41))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x42))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x43))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x44))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x45))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x46))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x47))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x48))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x49))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x4A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x4B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x4C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x4D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x4E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x4F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x50))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x51))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x52))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x53))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x54))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x55))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x56))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x57))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x58))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x59))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x5A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x5B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x5C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x5D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x5E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x5F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x60))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x61))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x62))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x63))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x64))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x65))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x66))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x67))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x68))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x69))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x6A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x6B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x6C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x6D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x6E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x6F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x70))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x71))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x72))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x73))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x74))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x75))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x76))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x77))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x78))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x79))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x7A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x7B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x7C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x7D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x7E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x7F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x80))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x81))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x82))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x83))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x84))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x85))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x86))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x87))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x88))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x89))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x8A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x8B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x8C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x8D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x8E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x8F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x90))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x91))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x92))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x93))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x94))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x95))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x96))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x97))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x98))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x99))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x9A))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x9B))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x9C))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x9D))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x9E))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0x9F))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA0))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA1))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA2))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA3))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA4))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA5))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA6))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA7))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA8))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xA9))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xAA))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xAB))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xAC))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xAD))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xAE))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xAF))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB0))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB1))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB2))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB3))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB4))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB5))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB6))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB7))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB8))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xB9))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xBA))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xBB))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xBC))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xBD))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xBE))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xBF))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC0))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC1))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC2))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC3))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC4))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC5))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC6))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC7))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC8))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xC9))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xCA))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xCB))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xCC))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xCD))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xCE))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xCF))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD0))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD1))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD2))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD3))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD4))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD5))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD6))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD7))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD8))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xD9))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xDA))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xDB))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xDC))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xDD))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xDE))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xDF))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE0))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE1))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE2))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE3))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE4))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE5))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE6))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE7))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE8))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xE9))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xEA))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xEB))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xEC))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xED))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xEE))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xEF))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF0))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF1))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF2))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF3))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF4))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF5))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF6))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF7))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF8))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xF9))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xFA))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xFB))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xFC))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xFD))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xFE))                                    \
        shlbx_mx(Mebp,  inf_SCR01(0xFF))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrmb_ri(XG, IS)                                                    \
        shrmb3ri(W(XG), W(XG), W(IS))

#define shrmb_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrmb3ld(W(XG), W(XG), W(MS), W(DS))

#define shrmb3ri(XD, XS, IT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movbx_ri(Recx, W(IT))                                               \
        shrmb_xx()                                                          \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movbx_ld(Recx, W(MT), W(DT))                                        \
        shrmb_xx()                                                          \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrmb_xx() /* not portable, do not use outside */                   \
        shrbx_mx(Mebp,  inf_SCR01(0x00))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x01))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x02))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x03))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x04))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x05))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x06))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x07))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x08))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x09))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x0B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x0D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x0F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x10))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x11))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x12))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x13))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x14))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x15))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x16))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x17))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x18))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x19))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x1B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x1D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x1F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x20))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x21))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x22))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x23))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x24))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x25))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x26))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x27))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x28))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x29))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x2A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x2B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x2C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x2D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x2E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x2F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x30))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x31))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x32))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x33))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x34))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x35))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x36))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x37))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x38))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x39))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x3A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x3B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x3C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x3D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x3E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x3F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x40))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x41))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x42))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x43))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x44))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x45))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x46))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x47))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x48))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x49))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x4A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x4B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x4C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x4D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x4E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x4F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x50))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x51))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x52))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x53))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x54))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x55))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x56))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x57))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x58))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x59))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x5A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x5B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x5C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x5D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x5E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x5F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x60))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x61))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x62))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x63))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x64))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x65))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x66))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x67))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x68))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x69))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x6A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x6B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x6C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x6D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x6E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x6F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x70))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x71))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x72))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x73))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x74))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x75))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x76))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x77))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x78))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x79))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x7A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x7B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x7C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x7D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x7E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x7F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x80))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x81))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x82))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x83))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x84))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x85))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x86))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x87))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x88))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x89))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x8A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x8B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x8C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x8D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x8E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x8F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x90))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x91))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x92))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x93))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x94))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x95))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x96))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x97))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x98))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x99))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x9A))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x9B))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x9C))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x9D))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x9E))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0x9F))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA0))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA1))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA2))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA3))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA4))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA5))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA6))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA7))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA8))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xA9))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xAA))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xAB))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xAC))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xAD))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xAE))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xAF))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB0))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB1))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB2))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB3))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB4))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB5))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB6))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB7))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB8))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xB9))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xBA))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xBB))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xBC))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xBD))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xBE))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xBF))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC0))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC1))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC2))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC3))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC4))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC5))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC6))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC7))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC8))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xC9))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xCA))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xCB))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xCC))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xCD))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xCE))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xCF))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD0))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD1))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD2))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD3))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD4))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD5))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD6))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD7))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD8))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xD9))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xDA))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xDB))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xDC))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xDD))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xDE))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xDF))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE0))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE1))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE2))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE3))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE4))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE5))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE6))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE7))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE8))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xE9))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xEA))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xEB))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xEC))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xED))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xEE))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xEF))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF0))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF1))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF2))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF3))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF4))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF5))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF6))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF7))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF8))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xF9))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xFA))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xFB))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xFC))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xFD))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xFE))                                    \
        shrbx_mx(Mebp,  inf_SCR01(0xFF))

/* shr (G = G >> S), (D = S >> T) if (#D != #T) - plain, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define shrmc_ri(XG, IS)                                                    \
        shrmc3ri(W(XG), W(XG), W(IS))

#define shrmc_ld(XG, MS, DS) /* loads SIMD, uses first elem, rest zeroed */ \
        shrmc3ld(W(XG), W(XG), W(MS), W(DS))

#define shrmc3ri(XD, XS, IT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movbx_ri(Recx, W(IT))                                               \
        shrmc_xx()                                                          \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrmc3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        stack_st(Recx)                                                      \
        movbx_ld(Recx, W(MT), W(DT))                                        \
        shrmc_xx()                                                          \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

#define shrmc_xx() /* not portable, do not use outside */                   \
        shrbn_mx(Mebp,  inf_SCR01(0x00))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x01))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x02))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x03))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x04))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x05))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x06))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x07))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x08))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x09))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x0A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x0B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x0C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x0D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x0E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x0F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x10))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x11))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x12))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x13))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x14))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x15))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x16))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x17))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x18))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x19))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x1A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x1B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x1C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x1D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x1E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x1F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x20))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x21))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x22))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x23))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x24))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x25))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x26))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x27))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x28))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x29))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x2A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x2B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x2C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x2D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x2E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x2F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x30))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x31))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x32))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x33))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x34))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x35))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x36))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x37))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x38))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x39))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x3A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x3B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x3C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x3D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x3E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x3F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x40))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x41))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x42))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x43))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x44))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x45))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x46))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x47))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x48))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x49))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x4A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x4B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x4C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x4D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x4E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x4F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x50))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x51))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x52))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x53))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x54))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x55))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x56))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x57))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x58))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x59))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x5A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x5B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x5C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x5D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x5E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x5F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x60))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x61))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x62))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x63))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x64))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x65))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x66))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x67))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x68))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x69))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x6A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x6B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x6C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x6D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x6E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x6F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x70))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x71))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x72))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x73))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x74))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x75))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x76))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x77))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x78))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x79))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x7A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x7B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x7C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x7D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x7E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x7F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x80))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x81))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x82))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x83))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x84))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x85))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x86))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x87))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x88))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x89))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x8A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x8B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x8C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x8D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x8E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x8F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x90))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x91))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x92))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x93))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x94))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x95))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x96))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x97))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x98))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x99))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x9A))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x9B))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x9C))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x9D))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x9E))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0x9F))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA0))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA1))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA2))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA3))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA4))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA5))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA6))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA7))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA8))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xA9))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xAA))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xAB))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xAC))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xAD))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xAE))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xAF))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB0))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB1))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB2))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB3))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB4))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB5))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB6))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB7))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB8))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xB9))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xBA))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xBB))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xBC))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xBD))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xBE))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xBF))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC0))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC1))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC2))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC3))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC4))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC5))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC6))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC7))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC8))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xC9))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xCA))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xCB))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xCC))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xCD))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xCE))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xCF))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD0))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD1))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD2))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD3))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD4))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD5))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD6))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD7))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD8))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xD9))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xDA))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xDB))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xDC))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xDD))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xDE))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xDF))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE0))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE1))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE2))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE3))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE4))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE5))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE6))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE7))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE8))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xE9))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xEA))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xEB))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xEC))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xED))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xEE))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xEF))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF0))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF1))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF2))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF3))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF4))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF5))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF6))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF7))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF8))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xF9))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xFA))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xFB))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xFC))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xFD))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xFE))                                    \
        shrbn_mx(Mebp,  inf_SCR01(0xFF))

/* svl (G = G << S), (D = S << T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svlmb_rr(XG, XS)     /* variable shift with per-elem count */       \
        svlmb3rr(W(XG), W(XG), W(XS))

#define svlmb_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svlmb3ld(W(XG), W(XG), W(MS), W(DS))

#define svlmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svlmb_rx(W(XD))

#define svlmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svlmb_rx(W(XD))

#define svlmb_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Recx)                                                      \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x01))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x01))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x02))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x03))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x03))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x05))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x05))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x06))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x07))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x07))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x09))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x09))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x0B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x0D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x0F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x11))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x11))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x12))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x13))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x13))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x14))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x15))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x15))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x16))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x17))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x17))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x18))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x19))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x19))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x1B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x1D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x1F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x20))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x21))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x21))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x22))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x22))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x23))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x23))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x24))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x24))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x25))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x25))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x26))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x26))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x27))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x27))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x28))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x29))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x29))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x2A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x2B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x2C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x2D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x2E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x2F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x30))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x31))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x31))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x32))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x32))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x33))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x33))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x34))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x34))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x35))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x35))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x36))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x36))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x37))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x37))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x38))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x39))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x39))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x3A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x3B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x3C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x3D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x3E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x3F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x40))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x41))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x41))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x42))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x42))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x43))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x43))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x44))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x44))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x45))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x45))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x46))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x46))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x47))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x47))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x48))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x49))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x49))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x4A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x4B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x4C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x4D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x4E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x4F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x50))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x51))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x51))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x52))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x52))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x53))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x53))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x54))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x54))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x55))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x55))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x56))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x56))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x57))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x57))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x58))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x59))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x59))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x5A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x5B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x5C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x5D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x5E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x5F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x60))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x61))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x61))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x62))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x62))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x63))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x63))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x64))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x64))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x65))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x65))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x66))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x66))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x67))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x67))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x68))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x69))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x69))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x6A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x6B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x6C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x6D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x6E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x6F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x70))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x71))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x71))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x72))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x72))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x73))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x73))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x74))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x74))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x75))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x75))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x76))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x76))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x77))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x77))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x78))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x79))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x79))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x7A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x7B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x7C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x7D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x7E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x7F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x80))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x81))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x81))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x82))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x82))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x83))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x83))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x84))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x84))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x85))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x85))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x86))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x86))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x87))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x87))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x88))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x89))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x89))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x8A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x8B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x8C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x8D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x8E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x8F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x90))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x91))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x91))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x92))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x92))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x93))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x93))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x94))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x94))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x95))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x95))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x96))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x96))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x97))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x97))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x98))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x99))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x99))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9A))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x9A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9B))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x9B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9C))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x9C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9D))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x9D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9E))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x9E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9F))                              \
        shlbx_mx(Mebp,  inf_SCR01(0x9F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA1))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA2))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA3))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA4))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA5))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA6))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA7))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA9))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xA9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAA))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xAA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAB))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xAB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAC))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xAC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAD))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xAD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAE))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xAE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAF))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xAF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB1))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB2))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB3))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB4))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB5))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB6))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB7))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB9))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xB9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBA))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xBA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBB))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xBB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBC))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xBC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBD))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xBD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBE))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xBE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBF))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xBF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC1))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC2))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC3))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC4))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC5))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC6))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC7))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC9))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xC9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCA))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xCA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCB))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xCB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCC))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xCC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCD))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xCD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCE))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xCE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCF))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xCF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD1))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD2))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD3))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD4))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD5))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD6))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD7))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD9))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xD9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDA))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xDA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDB))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xDB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDC))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xDC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDD))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xDD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDE))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xDE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDF))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xDF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE1))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE2))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE3))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE4))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE5))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE6))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE7))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE9))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xE9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEA))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xEA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEB))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xEB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEC))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xEC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xED))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xED))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEE))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xEE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEF))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xEF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF1))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF2))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF3))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF4))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF5))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF6))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF7))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF9))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xF9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFA))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xFA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFB))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xFB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFC))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xFC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFD))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xFD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFE))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xFE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFF))                              \
        shlbx_mx(Mebp,  inf_SCR01(0xFF))                                    \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, unsigned
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrmb_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrmb3rr(W(XG), W(XG), W(XS))

#define svrmb_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrmb3ld(W(XG), W(XG), W(MS), W(DS))

#define svrmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svrmb_rx(W(XD))

#define svrmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svrmb_rx(W(XD))

#define svrmb_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Recx)                                                      \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x00))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x01))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x01))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x02))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x03))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x03))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x04))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x05))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x05))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x06))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x07))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x07))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x08))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x09))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x09))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x0B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x0D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x0F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x10))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x11))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x11))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x12))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x13))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x13))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x14))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x15))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x15))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x16))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x17))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x17))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x18))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x19))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x19))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x1B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x1D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x1E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x1F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x20))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x21))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x21))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x22))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x22))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x23))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x23))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x24))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x24))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x25))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x25))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x26))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x26))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x27))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x27))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x28))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x29))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x29))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x2A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x2B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x2C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x2D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x2E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x2F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x30))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x31))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x31))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x32))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x32))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x33))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x33))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x34))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x34))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x35))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x35))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x36))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x36))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x37))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x37))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x38))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x39))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x39))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x3A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x3B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x3C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x3D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x3E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x3F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x40))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x41))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x41))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x42))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x42))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x43))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x43))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x44))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x44))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x45))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x45))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x46))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x46))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x47))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x47))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x48))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x49))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x49))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x4A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x4B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x4C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x4D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x4E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x4F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x50))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x51))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x51))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x52))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x52))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x53))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x53))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x54))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x54))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x55))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x55))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x56))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x56))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x57))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x57))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x58))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x59))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x59))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x5A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x5B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x5C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x5D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x5E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x5F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x60))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x61))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x61))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x62))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x62))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x63))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x63))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x64))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x64))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x65))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x65))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x66))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x66))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x67))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x67))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x68))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x69))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x69))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x6A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x6B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x6C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x6D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x6E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x6F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x70))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x71))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x71))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x72))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x72))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x73))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x73))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x74))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x74))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x75))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x75))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x76))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x76))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x77))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x77))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x78))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x79))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x79))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x7A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x7B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x7C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x7D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x7E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x7F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x80))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x81))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x81))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x82))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x82))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x83))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x83))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x84))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x84))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x85))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x85))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x86))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x86))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x87))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x87))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x88))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x89))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x89))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x8A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x8B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x8C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x8D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x8E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x8F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x90))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x91))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x91))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x92))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x92))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x93))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x93))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x94))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x94))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x95))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x95))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x96))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x96))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x97))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x97))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x98))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x99))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x99))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9A))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x9A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9B))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x9B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9C))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x9C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9D))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x9D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9E))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x9E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9F))                              \
        shrbx_mx(Mebp,  inf_SCR01(0x9F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA1))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA2))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA3))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA4))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA5))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA6))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA7))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA9))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xA9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAA))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xAA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAB))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xAB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAC))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xAC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAD))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xAD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAE))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xAE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAF))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xAF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB1))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB2))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB3))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB4))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB5))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB6))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB7))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB9))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xB9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBA))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xBA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBB))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xBB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBC))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xBC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBD))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xBD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBE))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xBE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBF))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xBF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC1))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC2))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC3))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC4))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC5))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC6))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC7))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC9))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xC9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCA))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xCA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCB))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xCB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCC))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xCC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCD))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xCD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCE))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xCE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCF))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xCF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD1))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD2))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD3))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD4))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD5))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD6))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD7))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD9))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xD9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDA))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xDA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDB))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xDB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDC))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xDC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDD))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xDD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDE))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xDE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDF))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xDF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE1))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE2))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE3))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE4))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE5))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE6))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE7))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE9))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xE9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEA))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xEA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEB))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xEB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEC))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xEC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xED))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xED))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEE))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xEE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEF))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xEF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF1))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF2))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF3))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF4))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF5))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF6))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF7))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF9))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xF9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFA))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xFA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFB))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xFB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFC))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xFC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFD))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xFD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFE))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xFE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFF))                              \
        shrbx_mx(Mebp,  inf_SCR01(0xFF))                                    \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* svr (G = G >> S), (D = S >> T) if (#D != #T) - variable, signed
 * for maximum compatibility: shift count must be modulo elem-size */

#define svrmc_rr(XG, XS)     /* variable shift with per-elem count */       \
        svrmc3rr(W(XG), W(XG), W(XS))

#define svrmc_ld(XG, MS, DS) /* variable shift with per-elem count */       \
        svrmc3ld(W(XG), W(XG), W(MS), W(DS))

#define svrmc3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        svrmc_rx(W(XD))

#define svrmc3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        svrmc_rx(W(XD))

#define svrmc_rx(XD) /* not portable, do not use outside */                 \
        stack_st(Recx)                                                      \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x00))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x00))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x01))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x01))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x02))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x02))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x03))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x03))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x04))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x04))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x05))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x05))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x06))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x06))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x07))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x07))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x08))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x08))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x09))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x09))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x0A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x0B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x0C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x0D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x0E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x0F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x0F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x10))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x10))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x11))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x11))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x12))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x12))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x13))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x13))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x14))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x14))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x15))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x15))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x16))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x16))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x17))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x17))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x18))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x18))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x19))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x19))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x1A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x1B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x1C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x1D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x1E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x1F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x1F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x20))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x20))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x21))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x21))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x22))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x22))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x23))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x23))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x24))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x24))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x25))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x25))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x26))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x26))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x27))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x27))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x28))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x28))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x29))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x29))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x2A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x2B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x2C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x2D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x2E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x2F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x2F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x30))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x30))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x31))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x31))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x32))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x32))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x33))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x33))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x34))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x34))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x35))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x35))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x36))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x36))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x37))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x37))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x38))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x38))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x39))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x39))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x3A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x3B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x3C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x3D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x3E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x3F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x3F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x40))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x40))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x41))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x41))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x42))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x42))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x43))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x43))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x44))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x44))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x45))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x45))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x46))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x46))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x47))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x47))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x48))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x48))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x49))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x49))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x4A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x4B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x4C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x4D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x4E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x4F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x4F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x50))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x50))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x51))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x51))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x52))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x52))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x53))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x53))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x54))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x54))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x55))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x55))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x56))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x56))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x57))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x57))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x58))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x58))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x59))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x59))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x5A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x5B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x5C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x5D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x5E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x5F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x5F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x60))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x60))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x61))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x61))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x62))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x62))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x63))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x63))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x64))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x64))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x65))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x65))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x66))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x66))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x67))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x67))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x68))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x68))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x69))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x69))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x6A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x6B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x6C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x6D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x6E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x6F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x6F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x70))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x70))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x71))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x71))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x72))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x72))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x73))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x73))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x74))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x74))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x75))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x75))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x76))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x76))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x77))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x77))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x78))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x78))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x79))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x79))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x7A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x7B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x7C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x7D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x7E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x7F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x7F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x80))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x80))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x81))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x81))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x82))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x82))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x83))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x83))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x84))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x84))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x85))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x85))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x86))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x86))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x87))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x87))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x88))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x88))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x89))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x89))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x8A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x8B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x8C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x8D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x8E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x8F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x8F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x90))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x90))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x91))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x91))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x92))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x92))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x93))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x93))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x94))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x94))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x95))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x95))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x96))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x96))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x97))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x97))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x98))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x98))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x99))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x99))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9A))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x9A))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9B))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x9B))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9C))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x9C))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9D))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x9D))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9E))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x9E))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0x9F))                              \
        shrbn_mx(Mebp,  inf_SCR01(0x9F))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA0))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA1))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA2))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA3))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA4))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA5))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA6))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA7))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA8))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xA9))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xA9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAA))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xAA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAB))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xAB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAC))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xAC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAD))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xAD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAE))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xAE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xAF))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xAF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB0))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB1))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB2))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB3))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB4))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB5))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB6))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB7))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB8))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xB9))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xB9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBA))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xBA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBB))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xBB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBC))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xBC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBD))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xBD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBE))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xBE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xBF))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xBF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC0))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC1))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC2))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC3))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC4))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC5))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC6))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC7))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC8))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xC9))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xC9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCA))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xCA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCB))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xCB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCC))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xCC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCD))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xCD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCE))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xCE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xCF))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xCF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD0))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD1))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD2))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD3))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD4))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD5))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD6))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD7))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD8))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xD9))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xD9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDA))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xDA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDB))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xDB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDC))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xDC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDD))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xDD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDE))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xDE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xDF))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xDF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE0))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE1))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE2))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE3))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE4))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE5))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE6))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE7))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE8))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xE9))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xE9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEA))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xEA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEB))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xEB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEC))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xEC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xED))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xED))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEE))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xEE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xEF))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xEF))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF0))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF0))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF1))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF1))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF2))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF2))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF3))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF3))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF4))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF4))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF5))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF5))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF6))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF6))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF7))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF7))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF8))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF8))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xF9))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xF9))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFA))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xFA))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFB))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xFB))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFC))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xFC))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFD))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xFD))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFE))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xFE))                                    \
        movbx_ld(Recx,  Mebp, inf_SCR02(0xFF))                              \
        shrbn_mx(Mebp,  inf_SCR01(0xFF))                                    \
        stack_ld(Recx)                                                      \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/*****************   packed byte-precision integer compare   ******************/

#if (RT_512X4 < 2)

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define minmb_rr(XG, XS)                                                    \
        minmb3rr(W(XG), W(XG), W(XS))

#define minmb_ld(XG, MS, DS)                                                \
        minmb3ld(W(XG), W(XG), W(MS), W(DS))

#define minmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        minmb_rx(W(XD))

#define minmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        minmb_rx(W(XD))

#define minmb_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        minab_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define minmc_rr(XG, XS)                                                    \
        minmc3rr(W(XG), W(XG), W(XS))

#define minmc_ld(XG, MS, DS)                                                \
        minmc3ld(W(XG), W(XG), W(MS), W(DS))

#define minmc3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        minmc_rx(W(XD))

#define minmc3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        minmc_rx(W(XD))

#define minmc_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        minac_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxmb_rr(XG, XS)                                                    \
        maxmb3rr(W(XG), W(XG), W(XS))

#define maxmb_ld(XG, MS, DS)                                                \
        maxmb3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        maxmb_rx(W(XD))

#define maxmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        maxmb_rx(W(XD))

#define maxmb_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        maxab_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxmc_rr(XG, XS)                                                    \
        maxmc3rr(W(XG), W(XG), W(XS))

#define maxmc_ld(XG, MS, DS)                                                \
        maxmc3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmc3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        maxmc_rx(W(XD))

#define maxmc3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        maxmc_rx(W(XD))

#define maxmc_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        maxac_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqmb_rr(XG, XS)                                                    \
        ceqmb3rr(W(XG), W(XG), W(XS))

#define ceqmb_ld(XG, MS, DS)                                                \
        ceqmb3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqmb3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        ceqmb_rx(W(XD))

#define ceqmb3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        ceqmb_rx(W(XD))

#define ceqmb_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        ceqab_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtmc_rr(XG, XS)                                                    \
        cgtmc3rr(W(XG), W(XG), W(XS))

#define cgtmc_ld(XG, MS, DS)                                                \
        cgtmc3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmc3rr(XD, XS, XT)                                                \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_st(W(XT), Mebp, inf_SCR02(0))                                 \
        cgtmc_rx(W(XD))

#define cgtmc3ld(XD, XS, MT, DT)                                            \
        movmx_st(W(XS), Mebp, inf_SCR01(0))                                 \
        movmx_ld(W(XD), W(MT), W(DT))                                       \
        movmx_st(W(XD), Mebp, inf_SCR02(0))                                 \
        cgtmc_rx(W(XD))

#define cgtmc_rx(XD) /* not portable, do not use outside */                 \
        movax_ld(W(XD), Mebp, inf_SCR01(0x00))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0x00))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x00))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x20))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0x20))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x20))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x40))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0x40))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x40))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x60))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0x60))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x60))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0x80))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0x80))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0x80))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xA0))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0xA0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xA0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xC0))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0xC0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xC0))                              \
        movax_ld(W(XD), Mebp, inf_SCR01(0xE0))                              \
        cgtac_ld(W(XD), Mebp, inf_SCR02(0xE0))                              \
        movax_st(W(XD), Mebp, inf_SCR01(0xE0))                              \
        movmx_ld(W(XD), Mebp, inf_SCR01(0))

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnemb_rr(XG, XS)                                                    \
        cnemb3rr(W(XG), W(XG), W(XS))

#define cnemb_ld(XG, MS, DS)                                                \
        cnemb3ld(W(XG), W(XG), W(MS), W(DS))

#define cnemb3rr(XD, XS, XT)                                                \
        ceqmb3rr(W(XD), W(XS), W(XT))                                       \
        notmx_rx(W(XD))

#define cnemb3ld(XD, XS, MT, DT)                                            \
        ceqmb3ld(W(XD), W(XS), W(MT), W(DT))                                \
        notmx_rx(W(XD))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltmb_rr(XG, XS)                                                    \
        cltmb3rr(W(XG), W(XG), W(XS))

#define cltmb_ld(XG, MS, DS)                                                \
        cltmb3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmb3rr(XD, XS, XT)                                                \
        minmb3rr(W(XD), W(XS), W(XT))                                       \
        cnemb_rr(W(XD), W(XT))

#define cltmb3ld(XD, XS, MT, DT)                                            \
        minmb3ld(W(XD), W(XS), W(MT), W(DT))                                \
        cnemb_ld(W(XD), W(MT), W(DT))

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltmc_rr(XG, XS)                                                    \
        cltmc3rr(W(XG), W(XG), W(XS))

#define cltmc_ld(XG, MS, DS)                                                \
        cltmc3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmc3rr(XD, XS, XT)                                                \
        cgtmc3rr(W(XD), W(XT), W(XS))

#define cltmc3ld(XD, XS, MT, DT)                                            \
        minmc3ld(W(XD), W(XS), W(MT), W(DT))                                \
        cnemb_ld(W(XD), W(MT), W(DT))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define clemb_rr(XG, XS)                                                    \
        clemb3rr(W(XG), W(XG), W(XS))

#define clemb_ld(XG, MS, DS)                                                \
        clemb3ld(W(XG), W(XG), W(MS), W(DS))

#define clemb3rr(XD, XS, XT)                                                \
        maxmb3rr(W(XD), W(XS), W(XT))                                       \
        ceqmb_rr(W(XD), W(XT))

#define clemb3ld(XD, XS, MT, DT)                                            \
        maxmb3ld(W(XD), W(XS), W(MT), W(DT))                                \
        ceqmb_ld(W(XD), W(MT), W(DT))

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define clemc_rr(XG, XS)                                                    \
        clemc3rr(W(XG), W(XG), W(XS))

#define clemc_ld(XG, MS, DS)                                                \
        clemc3ld(W(XG), W(XG), W(MS), W(DS))

#define clemc3rr(XD, XS, XT)                                                \
        cgtmc3rr(W(XD), W(XS), W(XT))                                       \
        notmx_rx(W(XD))

#define clemc3ld(XD, XS, MT, DT)                                            \
        cgtmc3ld(W(XD), W(XS), W(MT), W(DT))                                \
        notmx_rx(W(XD))

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtmb_rr(XG, XS)                                                    \
        cgtmb3rr(W(XG), W(XG), W(XS))

#define cgtmb_ld(XG, MS, DS)                                                \
        cgtmb3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmb3rr(XD, XS, XT)                                                \
        maxmb3rr(W(XD), W(XS), W(XT))                                       \
        cnemb_rr(W(XD), W(XT))

#define cgtmb3ld(XD, XS, MT, DT)                                            \
        maxmb3ld(W(XD), W(XS), W(MT), W(DT))                                \
        cnemb_ld(W(XD), W(MT), W(DT))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgemb_rr(XG, XS)                                                    \
        cgemb3rr(W(XG), W(XG), W(XS))

#define cgemb_ld(XG, MS, DS)                                                \
        cgemb3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemb3rr(XD, XS, XT)                                                \
        minmb3rr(W(XD), W(XS), W(XT))                                       \
        ceqmb_rr(W(XD), W(XT))

#define cgemb3ld(XD, XS, MT, DT)                                            \
        minmb3ld(W(XD), W(XS), W(MT), W(DT))                                \
        ceqmb_ld(W(XD), W(MT), W(DT))

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgemc_rr(XG, XS)                                                    \
        cgemc3rr(W(XG), W(XG), W(XS))

#define cgemc_ld(XG, MS, DS)                                                \
        cgemc3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemc3rr(XD, XS, XT)                                                \
        minmc3rr(W(XD), W(XS), W(XT))                                       \
        ceqmb_rr(W(XD), W(XT))

#define cgemc3ld(XD, XS, MT, DT)                                            \
        minmc3ld(W(XD), W(XS), W(MT), W(DT))                                \
        ceqmb_ld(W(XD), W(MT), W(DT))

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE08_2K8    0x00     /* none satisfy the condition */
#define RT_SIMD_MASK_FULL08_2K8    0xFF     /*  all satisfy the condition */

#define movov_rr(XD, XS)     /* not portable, do not use outside */         \
        EVX(RXB(XD), RXB(XS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(XS), REG(XS))

#define movov_ld(XD, MS, DS) /* not portable, do not use outside */         \
    ADR EVX(RXB(XD), RXB(MS),    0x00, K, 0, 1) EMITB(0x28)                 \
        MRM(REG(XD), MOD(MS), REG(MS))                                      \
        AUX(SIB(MS), CMD(DS), EMPTY)

#define movov_st(XS, MD, DD) /* not portable, do not use outside */         \
    ADR EVX(RXB(XS), RXB(MD),    0x00, K, 0, 1) EMITB(0x29)                 \
        MRM(REG(XS), MOD(MD), REG(MD))                                      \
        AUX(SIB(MD), CMD(DD), EMPTY)

#define prmov_rx(XG)         /* not portable, do not use outside */         \
        EVX(RXB(XG), RXB(XG), REN(XG), K, 1, 3) EMITB(0x43)                 \
        MRM(REG(XG), MOD(XG), REG(XG))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x4E))  /* permute two 256-bit halves */

#define shlov_ri(XG, IS)     /* not portable, do not use outside */         \
        shlov3ri(W(XG), W(XG), W(IS))

#define shlov3ri(XD, XS, IT) /* not portable, do not use outside */         \
        EVX(0,       RXB(XS), REN(XD), K, 1, 1) EMITB(0x72)                 \
        MRM(0x06,    MOD(XS), REG(XS))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(VAL(IT)))

/* #define bsncx_rx(XS, mask)              (defined in HB_256-bit header) */

#define mkjmb_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        movov_st(Xmm0, Mebp, inf_SCR01(0x00))                               \
        movov_st(Xmm1, Mebp, inf_SCR01(0x40))                               \
        movov_st(Xmm2, Mebp, inf_SCR01(0x80))                               \
        movov_st(Xmm3, Mebp, inf_SCR01(0xC0))                               \
        movov_rr(Xmm0, W(XS))                                               \
        movov_rr(Xmm1, V(XS))                                               \
        movov_rr(Xmm2, X(XS))                                               \
        movov_rr(Xmm3, Z(XS))                                               \
        VEX(0,             0,    0x00, 1, 0, 1) EMITB(0x50)                 \
        MRM(0x00,       0x03,    0x00)                                      \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        prmov_rx(Xmm0)                                                      \
        prmov_rx(Xmm1)                                                      \
        prmov_rx(Xmm2)                                                      \
        prmov_rx(Xmm3)                                                      \
        bsncx_rx(Xmm0, mask)                                                \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        shlov_ri(Xmm0, IB(8))                                               \
        shlov_ri(Xmm1, IB(8))                                               \
        shlov_ri(Xmm2, IB(8))                                               \
        shlov_ri(Xmm3, IB(8))                                               \
        bsncx_rx(Xmm0, mask)                                                \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        prmov_rx(Xmm0)                                                      \
        prmov_rx(Xmm1)                                                      \
        prmov_rx(Xmm2)                                                      \
        prmov_rx(Xmm3)                                                      \
        bsncx_rx(Xmm0, mask)                                                \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        shlov_ri(Xmm0, IB(8))                                               \
        shlov_ri(Xmm1, IB(8))                                               \
        shlov_ri(Xmm2, IB(8))                                               \
        shlov_ri(Xmm3, IB(8))                                               \
        bsncx_rx(Xmm0, mask)                                                \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        prmov_rx(Xmm0)                                                      \
        prmov_rx(Xmm1)                                                      \
        prmov_rx(Xmm2)                                                      \
        prmov_rx(Xmm3)                                                      \
        bsncx_rx(Xmm0, mask)                                                \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        shlov_ri(Xmm0, IB(8))                                               \
        shlov_ri(Xmm1, IB(8))                                               \
        shlov_ri(Xmm2, IB(8))                                               \
        shlov_ri(Xmm3, IB(8))                                               \
        bsncx_rx(Xmm0, mask)                                                \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        prmov_rx(Xmm0)                                                      \
        prmov_rx(Xmm1)                                                      \
        prmov_rx(Xmm2)                                                      \
        prmov_rx(Xmm3)                                                      \
        bsncx_rx(Xmm0, mask)                                                \
        bsncx_rx(Xmm1, mask)                                                \
        bsncx_rx(Xmm2, mask)                                                \
        bsncx_rx(Xmm3, mask)                                                \
        movov_ld(Xmm0, Mebp, inf_SCR01(0x00))                               \
        movov_ld(Xmm1, Mebp, inf_SCR01(0x40))                               \
        movov_ld(Xmm2, Mebp, inf_SCR01(0x80))                               \
        movov_ld(Xmm3, Mebp, inf_SCR01(0xC0))                               \
        cmpwx_ri(Reax, IB(RT_SIMD_MASK_##mask##08_2K8))                     \
        jeqxx_lb(lb)

#else /* RT_512X4 >= 2 */

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), unsigned */

#define minmb_rr(XG, XS)                                                    \
        minmb3rr(W(XG), W(XG), W(XS))

#define minmb_ld(XG, MS, DS)                                                \
        minmb3ld(W(XG), W(XG), W(MS), W(DS))

#define minmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDA)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* min (G = G < S ? G : S), (D = S < T ? S : T) if (#D != #T), signed */

#define minmc_rr(XG, XS)                                                    \
        minmc3rr(W(XG), W(XG), W(XS))

#define minmc_ld(XG, MS, DS)                                                \
        minmc3ld(W(XG), W(XG), W(MS), W(DS))

#define minmc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define minmc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x38)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), unsigned */

#define maxmb_rr(XG, XS)                                                    \
        maxmb3rr(W(XG), W(XG), W(XS))

#define maxmb_ld(XG, MS, DS)                                                \
        maxmb3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 1) EMITB(0xDE)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* max (G = G > S ? G : S), (D = S > T ? S : T) if (#D != #T), signed */

#define maxmc_rr(XG, XS)                                                    \
        maxmc3rr(W(XG), W(XG), W(XS))

#define maxmc_ld(XG, MS, DS)                                                \
        maxmc3ld(W(XG), W(XG), W(MS), W(DS))

#define maxmc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(1,             1, REH(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(2,             2, REI(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))                                      \
        EVX(3,             3, REJ(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD), MOD(XT), REG(XT))

#define maxmc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMPTY)                                 \
    ADR EVX(1,       RXB(MT), REH(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMPTY)                                 \
    ADR EVX(2,       RXB(MT), REI(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMPTY)                                 \
    ADR EVX(3,       RXB(MT), REJ(XS), K, 1, 2) EMITB(0x3C)                 \
        MRM(REG(XD),    0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMPTY)

/* ceq (G = G == S ? -1 : 0), (D = S == T ? -1 : 0) if (#D != #T) */

#define ceqmb_rr(XG, XS)                                                    \
        ceqmb3rr(W(XG), W(XG), W(XS))

#define ceqmb_ld(XG, MS, DS)                                                \
        ceqmb3ld(W(XG), W(XG), W(MS), W(DS))

#define ceqmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x00))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define ceqmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x00))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x00))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x00))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x00))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* cne (G = G != S ? -1 : 0), (D = S != T ? -1 : 0) if (#D != #T) */

#define cnemb_rr(XG, XS)                                                    \
        cnemb3rr(W(XG), W(XG), W(XS))

#define cnemb_ld(XG, MS, DS)                                                \
        cnemb3ld(W(XG), W(XG), W(MS), W(DS))

#define cnemb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x04))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define cnemb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x04))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x04))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x04))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x04))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), unsigned */

#define cltmb_rr(XG, XS)                                                    \
        cltmb3rr(W(XG), W(XG), W(XS))

#define cltmb_ld(XG, MS, DS)                                                \
        cltmb3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define cltmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x01))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x01))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x01))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x01))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* clt (G = G < S ? -1 : 0), (D = S < T ? -1 : 0) if (#D != #T), signed */

#define cltmc_rr(XG, XS)                                                    \
        cltmc3rr(W(XG), W(XG), W(XS))

#define cltmc_ld(XG, MS, DS)                                                \
        cltmc3ld(W(XG), W(XG), W(MS), W(DS))

#define cltmc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x01))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define cltmc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x01))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x01))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x01))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x01))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), unsigned */

#define clemb_rr(XG, XS)                                                    \
        clemb3rr(W(XG), W(XG), W(XS))

#define clemb_ld(XG, MS, DS)                                                \
        clemb3ld(W(XG), W(XG), W(MS), W(DS))

#define clemb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define clemb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x02))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x02))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x02))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x02))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* cle (G = G <= S ? -1 : 0), (D = S <= T ? -1 : 0) if (#D != #T), signed */

#define clemc_rr(XG, XS)                                                    \
        clemc3rr(W(XG), W(XG), W(XS))

#define clemc_ld(XG, MS, DS)                                                \
        clemc3ld(W(XG), W(XG), W(MS), W(DS))

#define clemc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x02))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define clemc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x02))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x02))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x02))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x02))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), unsigned */

#define cgtmb_rr(XG, XS)                                                    \
        cgtmb3rr(W(XG), W(XG), W(XS))

#define cgtmb_ld(XG, MS, DS)                                                \
        cgtmb3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define cgtmb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x06))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x06))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x06))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x06))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* cgt (G = G > S ? -1 : 0), (D = S > T ? -1 : 0) if (#D != #T), signed */

#define cgtmc_rr(XG, XS)                                                    \
        cgtmc3rr(W(XG), W(XG), W(XS))

#define cgtmc_ld(XG, MS, DS)                                                \
        cgtmc3ld(W(XG), W(XG), W(MS), W(DS))

#define cgtmc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x06))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define cgtmc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x06))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x06))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x06))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x06))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), unsigned */

#define cgemb_rr(XG, XS)                                                    \
        cgemb3rr(W(XG), W(XG), W(XS))

#define cgemb_ld(XG, MS, DS)                                                \
        cgemb3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemb3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define cgemb3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x05))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x05))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x05))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3E)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x05))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* cge (G = G >= S ? -1 : 0), (D = S >= T ? -1 : 0) if (#D != #T), signed */

#define cgemc_rr(XG, XS)                                                    \
        cgemc3rr(W(XG), W(XG), W(XS))

#define cgemc_ld(XG, MS, DS)                                                \
        cgemc3ld(W(XG), W(XG), W(MS), W(DS))

#define cgemc3rr(XD, XS, XT)                                                \
        EVX(0,             0, REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             1, REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             2, REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
        EVX(0,             3, REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,    MOD(XT), REG(XT))                                      \
        AUX(EMPTY,   EMPTY,   EMITB(0x05))                                  \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

#define cgemc3ld(XD, XS, MT, DT)                                            \
    ADR EVX(0,       RXB(MT), REG(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VAL(DT)), EMITB(0x05))                           \
        mz1mb_ld(W(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REH(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VZL(DT)), EMITB(0x05))                           \
        mz1mb_ld(V(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REI(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VSL(DT)), EMITB(0x05))                           \
        mz1mb_ld(X(XD), Mebp, inf_GPC07)                                    \
    ADR EVX(0,       RXB(MT), REJ(XS), K, 1, 3) EMITB(0x3F)                 \
        MRM(0x01,       0x02, REG(MT))                                      \
        AUX(SIB(MT), EMITW(VTL(DT)), EMITB(0x05))                           \
        mz1mb_ld(Z(XD), Mebp, inf_GPC07)

/* mkj (jump to lb) if (S satisfies mask condition) */

#define RT_SIMD_MASK_NONE08_2K8  0x00000000 /* none satisfy the condition */
#define RT_SIMD_MASK_FULL08_2K8  0xFFFFFFFF /*  all satisfy the condition */

#define mk1bx_rx(RD)         /* not portable, do not use outside */         \
        VEW(RXB(RD),       0,    0x00, 0, 3, 1) EMITB(0x93)                 \
        MRM(REG(RD),    0x03,    0x01)

#define mkjmb_rx(XS, mask, lb)   /* destroys Reax, if S == mask jump lb */  \
        ck1mb_rm(W(XS), Mebp, inf_GPC07)                                    \
        mk1bx_rx(Reax)                                                      \
        REW(1,             0) EMITB(0x8B)                                   \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1mb_rm(V(XS), Mebp, inf_GPC07)                                    \
        mk1bx_rx(Reax)                                                      \
        REW(1,             0)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##08_2K8 & 0x1) << 1)))  \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1mb_rm(X(XS), Mebp, inf_GPC07)                                    \
        mk1bx_rx(Reax)                                                      \
        REW(1,             0)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##08_2K8 & 0x1) << 1)))  \
        MRM(0x07,       0x03, 0x00)                                         \
        ck1mb_rm(Z(XS), Mebp, inf_GPC07)                                    \
        mk1bx_rx(Reax)                                                      \
        REW(0,             1)                                               \
        EMITB(0x03 | (0x08 << ((RT_SIMD_MASK_##mask##08_2K8 & 0x1) << 1)))  \
        MRM(0x00,       0x03, 0x07)                                         \
        movzx_mj(Mebp, inf_SCR02(0), IW(RT_SIMD_MASK_##mask##08_2K8),       \
                                     IW(RT_SIMD_MASK_##mask##08_2K8))       \
        cmpzx_rm(Reax, Mebp, inf_SCR02(0))                                  \
        jeqxx_lb(lb)

#endif /* RT_512X4 >= 2 */

/******************************************************************************/
/********************************   INTERNAL   ********************************/
/******************************************************************************/

#endif /* RT_512X4 */

#endif /* RT_SIMD_CODE */

#endif /* RT_RTARCH_XHB_512X4V2_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
