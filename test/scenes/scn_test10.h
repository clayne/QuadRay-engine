/******************************************************************************/
/* Copyright (c) 2013-2015 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#ifndef RT_SCN_TEST10_H
#define RT_SCN_TEST10_H

#include "format.h"

#include "all_mat.h"
#include "all_obj.h"

namespace scn_test10
{

/******************************************************************************/
/**********************************   BASE   **********************************/
/******************************************************************************/

rt_PLANE pl_floor01 =
{
    {      /*   RT_I,       RT_J,       RT_K    */
/* min */   {   -5.0,       -5.0,      -RT_INF  },
/* max */   {   +5.0,       +5.0,      +RT_INF  },
        {
/* OUTER        RT_U,       RT_V    */
/* scl */   {    1.0,        1.0    },
/* rot */              0.0           ,
/* pos */   {    0.0,        0.0    },

/* mat */   &mt_plain01_gray01,
        },
        {
/* INNER        RT_U,       RT_V    */
/* scl */   {    1.0,        1.0    },
/* rot */              0.0           ,
/* pos */   {    0.0,        0.0    },

/* mat */   &mt_plain01_gray02,
        },
    },
};

rt_PLANE pl_frame01 =
{
    {      /*   RT_I,       RT_J,       RT_K    */
/* min */   {   -1.0,       -1.0,      -RT_INF  },
/* max */   {   +1.0,       +1.0,      +RT_INF  },
        {
/* OUTER        RT_U,       RT_V    */
/* scl */   {    1.0,        1.0    },
/* rot */              0.0           ,
/* pos */   {    0.0,        0.0    },

/* mat */   &mt_plain01_gray01,
        },
        {
/* INNER        RT_U,       RT_V    */
/* scl */   {    1.0,        1.0    },
/* rot */              0.0           ,
/* pos */   {    0.0,        0.0    },

/* mat */   &mt_plain01_gray02,
        },
    },
};

/******************************************************************************/
/*********************************   CAMERA   *********************************/
/******************************************************************************/

rt_OBJECT ob_camera01[] =
{
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   { -105.0,        0.0,        0.0    },
/* pos */   {    0.0,      -15.0,        0.0    },
        },
        RT_OBJ_CAMERA(&cm_camera01)
    },
};

/******************************************************************************/
/*********************************   LIGHTS   *********************************/
/******************************************************************************/

rt_OBJECT ob_light01[] =
{
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {    0.0,        0.0,        0.0    },
        },
        RT_OBJ_LIGHT(&lt_light01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {    0.0,        0.0,        0.0    },
        },
        RT_OBJ_SPHERE(&sp_bulb01)
    },
};

/******************************************************************************/
/**********************************   TREE   **********************************/
/******************************************************************************/

rt_OBJECT ob_tree[] =
{
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {    0.0,        0.0,        0.0    },
        },
        RT_OBJ_PLANE(&pl_floor01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {   +4.0,       +4.0,        0.8    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {   -4.0,       +4.0,        0.8    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,       45.0    },
/* pos */   {   +4.0,       -4.0,        0.8    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,       45.0    },
/* pos */   {   -4.0,       -4.0,        0.8    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    5.0,        0.2,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {    0.0,       +1.2,        0.3    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    5.0,        0.2,        1.0    },
/* rot */   {  +15.0,        0.0,       90.0    },
/* pos */   {   -2.5,        0.0,        1.5    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    5.0,        0.2,        1.0    },
/* rot */   {  -15.0,        0.0,       90.0    },
/* pos */   {   +2.5,        0.0,        1.5    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    3.5,        0.2,        1.0    },
/* rot */   {    0.0,        0.0,      -45.0    },
/* pos */   {   -3.2,       -3.2,        0.3    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    3.5,        0.2,        1.0    },
/* rot */   {    0.0,        0.0,      +45.0    },
/* pos */   {   +3.2,       -3.2,        0.3    },
        },
        RT_OBJ_PLANE(&pl_frame01)
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {    0.0,        0.0,        4.0    },
        },
        RT_OBJ_ARRAY(&ob_light01),
    },
    {
        {  /*   RT_X,       RT_Y,       RT_Z    */
/* scl */   {    1.0,        1.0,        1.0    },
/* rot */   {    0.0,        0.0,        0.0    },
/* pos */   {    0.0,        0.0,        5.0    },
        },
        RT_OBJ_ARRAY(&ob_camera01)
    },
};

/******************************************************************************/
/**********************************   SCENE   *********************************/
/******************************************************************************/

rt_SCENE sc_root =
{
    RT_OBJ_ARRAY(&ob_tree),
};

} /* namespace scn_test10 */

#endif /* RT_SCN_TEST10_H */

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/