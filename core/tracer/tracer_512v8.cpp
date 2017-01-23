/******************************************************************************/
/* Copyright (c) 2013-2017 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#undef  RT_SIMD
#define RT_SIMD 512 /* map vector-length-agnostic SIMD subsets to 512-bit */
#define RT_SIMD_CODE /* enable SIMD instruction definitions */

#if defined (RT_512) && (RT_512 & 8)
#undef  RT_512
#define RT_512 8
#define RT_RENDER_CODE /* enable contents of render0 routine */
#endif /* RT_512 */

#if defined (RT_X32) && (RT_SIMD_COMPAT_512 == 2)
#undef  RT_X32
#define RT_X32 2 /* enable BMI1+BMI2 for 512-bit AVX2 target on x32 */
#endif /* RT_X32 */

#if defined (RT_X64) && (RT_SIMD_COMPAT_512 == 2)
#undef  RT_X64
#define RT_X64 2 /* enable BMI1+BMI2 for 512-bit AVX2 target on x64 */
#endif /* RT_X64 */

#include "tracer.h"
#include "format.h"
#if RT_DEBUG >= 1
#include "system.h"
#endif /* RT_DEBUG */

/*
 * Global pointer tables
 * for quick entry point resolution.
 */
extern
rt_pntr t_ptr[3];
extern
rt_pntr t_mat[3];
extern
rt_pntr t_clp[3];
extern
rt_pntr t_pow[6];

namespace simd_512v8
{
#include "tracer.cpp"
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/