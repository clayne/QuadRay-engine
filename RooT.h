/******************************************************************************/
/* Copyright (c) 2013-2015 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_ROOT_H
#define RT_ROOT_H

#include "engine.h"
#include "data/scenes/all_scn.h"

#undef Q /* short name for RT_SIMD_QUADS */
#undef S /* short name for RT_SIMD_WIDTH */
#undef W /* triplet pass-through wrapper */

#define RT_X_RES        800
#define RT_Y_RES        480

rt_astr     title       = "QuadRay engine demo, (C) 2013-2015 VectorChief";

rt_cell     x_res       = RT_X_RES;
rt_cell     y_res       = RT_Y_RES;
rt_cell     x_row       = RT_X_RES;
rt_word    *frame       = RT_NULL;

rt_cell     fsaa        = RT_FSAA_NO; /* no AA */
rt_cell     simd        = 0; /* default SIMD width will be chosen */
rt_cell     type        = 0; /* default SIMD sub-target will be chosen */
rt_cell     hide_num    = 0; /* hide all numbers on the screen if 1 */

rt_cell     d           = 1;    /* demo index */
rt_Scene   *sc[2]       = {0};  /* scenes array */

#endif /* RT_ROOT_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
