/******************************************************************************\
* Project:  MSP Emulation Layer for Vector Unit Computational Operations       *
* Authors:  Iconoclast                                                         *
* Release:  2015.01.30                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/

#include "vu.h"

#include "multiply.h"
#include "add.h"
#include "select.h"
#include "logical.h"
#include "divide.h"
#if 0
#include "pack.h"
#endif

ALIGNED i16 VR[32][N << VR_STATIC_WRAPAROUND];
ALIGNED i16 VACC[3][N];
#ifndef ARCH_MIN_SSE2
ALIGNED i16 V_result[N];
#endif

/*
 * These normally should have type `int` because they are Boolean T/F arrays.
 * However, since SSE2 uses 128-bit XMM's, and Win32 `int` storage is 32-bit,
 * we have the problem of 32*8 > 128 bits, so we use `short` to reduce packs.
 */
ALIGNED i16 cf_ne[N]; /* $vco:  high "NOTEQUAL" */
ALIGNED i16 cf_co[N]; /* $vco:  low "carry/borrow in/out" */
ALIGNED i16 cf_clip[N]; /* $vcc:  high (clip tests:  VCL, VCH, VCR) */
ALIGNED i16 cf_comp[N]; /* $vcc:  low (VEQ, VNE, VLT, VGE, VCL, VCH, VCR) */
ALIGNED i16 cf_vce[N]; /* $vce:  vector compare extension register */

VECTOR_OPERATION res_V(v16 vs, v16 vt)
{
    vt = vs; /* unused */
    message("C2\nRESERVED"); /* uncertain how to handle reserved, untested */
#ifdef ARCH_MIN_SSE2
    vs = _mm_setzero_si128();
    return (vs);
#else
    vector_wipe(V_result);
    return;
#endif
}
VECTOR_OPERATION res_M(v16 vs, v16 vt)
{ /* Ultra64 OS did have these, so one could implement this ext. */
    message("VMUL IQ");
#ifdef ARCH_MIN_SSE2
    vs = res_V(vs, vt);
    return (vs);
#else
    res_V(vs, vt);
    return;
#endif
}

/*
 * Op-code-accurate matrix of all the known RSP vector operations.
 * To do:  Either remove VMACQ, or add VRNDP, VRNDN, and VMULQ.
 *
 * Note that these are not our literal function names, just macro names.
 */
VECTOR_OPERATION (*COP2_C2[8 * 8])(v16, v16) = {
    VMULF  ,VMULU  ,res_M  ,res_M  ,VMUDL  ,VMUDM  ,VMUDN  ,VMUDH  , /* 000 */
    VMACF  ,VMACU  ,res_M  ,VMACQ  ,VMADL  ,VMADM  ,VMADN  ,VMADH  , /* 001 */
    VADD   ,VSUB   ,res_V  ,VABS   ,VADDC  ,VSUBC  ,res_V  ,res_V  , /* 010 */
    res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,VSAW   ,res_V  ,res_V  , /* 011 */
    VLT    ,VEQ    ,VNE    ,VGE    ,VCL    ,VCH    ,VCR    ,VMRG   , /* 100 */
    VAND   ,VNAND  ,VOR    ,VNOR   ,VXOR   ,VNXOR  ,res_V  ,res_V  , /* 101 */
    VRCP   ,VRCPL  ,VRCPH  ,VMOV   ,VRSQ   ,VRSQL  ,VRSQH  ,VNOP   , /* 110 */
    res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,res_V  ,res_V  , /* 111 */
}; /* 000     001     010     011     100     101     110     111 */

#ifndef ARCH_MIN_SSE2
u16 get_VCO(void)
{
    register u16 VCO;

    VCO = 0x0000
      | (cf_ne[0xF % 8] << 0xF)
      | (cf_ne[0xE % 8] << 0xE)
      | (cf_ne[0xD % 8] << 0xD)
      | (cf_ne[0xC % 8] << 0xC)
      | (cf_ne[0xB % 8] << 0xB)
      | (cf_ne[0xA % 8] << 0xA)
      | (cf_ne[0x9 % 8] << 0x9)
      | (cf_ne[0x8 % 8] << 0x8)
      | (cf_co[0x7 % 8] << 0x7)
      | (cf_co[0x6 % 8] << 0x6)
      | (cf_co[0x5 % 8] << 0x5)
      | (cf_co[0x4 % 8] << 0x4)
      | (cf_co[0x3 % 8] << 0x3)
      | (cf_co[0x2 % 8] << 0x2)
      | (cf_co[0x1 % 8] << 0x1)
      | (cf_co[0x0 % 8] << 0x0);
    return (VCO); /* Big endian becomes little. */
}
u16 get_VCC(void)
{
    register u16 VCC;

    VCC = 0x0000
      | (cf_clip[0xF % 8] << 0xF)
      | (cf_clip[0xE % 8] << 0xE)
      | (cf_clip[0xD % 8] << 0xD)
      | (cf_clip[0xC % 8] << 0xC)
      | (cf_clip[0xB % 8] << 0xB)
      | (cf_clip[0xA % 8] << 0xA)
      | (cf_clip[0x9 % 8] << 0x9)
      | (cf_clip[0x8 % 8] << 0x8)
      | (cf_comp[0x7 % 8] << 0x7)
      | (cf_comp[0x6 % 8] << 0x6)
      | (cf_comp[0x5 % 8] << 0x5)
      | (cf_comp[0x4 % 8] << 0x4)
      | (cf_comp[0x3 % 8] << 0x3)
      | (cf_comp[0x2 % 8] << 0x2)
      | (cf_comp[0x1 % 8] << 0x1)
      | (cf_comp[0x0 % 8] << 0x0);
    return (VCC); /* Big endian becomes little. */
}
u8 get_VCE(void)
{
    int result;
    register u8 VCE;

    result = 0x00
      | (cf_vce[07] << 0x7)
      | (cf_vce[06] << 0x6)
      | (cf_vce[05] << 0x5)
      | (cf_vce[04] << 0x4)
      | (cf_vce[03] << 0x3)
      | (cf_vce[02] << 0x2)
      | (cf_vce[01] << 0x1)
      | (cf_vce[00] << 0x0);
    VCE = result & 0xFF;
    return (VCE); /* Big endian becomes little. */
}
#else
u16 get_VCO(void)
{
    v16 xmm, hi, lo;
    register u16 VCO;

    hi = _mm_load_si128((v16 *)cf_ne);
    lo = _mm_load_si128((v16 *)cf_co);

/*
 * Rotate Boolean storage from LSB to MSB.
 */
    hi = _mm_slli_epi16(hi, 15);
    lo = _mm_slli_epi16(lo, 15);

    xmm = _mm_packs_epi16(lo, hi); /* Decompress INT16 Booleans to INT8 ones. */
    VCO = _mm_movemask_epi8(xmm) & 0x0000FFFF; /* PMOVMSKB combines each MSB. */
    return (VCO);
}
u16 get_VCC(void)
{
    v16 xmm, hi, lo;
    register u16 VCC;

    hi = _mm_load_si128((v16 *)cf_clip);
    lo = _mm_load_si128((v16 *)cf_comp);

/*
 * Rotate Boolean storage from LSB to MSB.
 */
    hi = _mm_slli_epi16(hi, 15);
    lo = _mm_slli_epi16(lo, 15);

    xmm = _mm_packs_epi16(lo, hi); /* Decompress INT16 Booleans to INT8 ones. */
    VCC = _mm_movemask_epi8(xmm) & 0x0000FFFF; /* PMOVMSKB combines each MSB. */
    return (VCC);
}
u8 get_VCE(void)
{
    v16 xmm, hi, lo;
    register u8 VCE;

    hi = _mm_setzero_si128();
    lo = _mm_load_si128((v16 *)cf_vce);

    lo = _mm_slli_epi16(lo, 15); /* Rotate Boolean storage from LSB to MSB. */

    xmm = _mm_packs_epi16(lo, hi); /* Decompress INT16 Booleans to INT8 ones. */
    VCE = _mm_movemask_epi8(xmm) & 0x000000FF; /* PMOVMSKB combines each MSB. */
    return (VCE);
}
#endif

/*
 * CTC2 resources
 * not sure how to vectorize going the other direction into SSE2
 */
void set_VCO(u16 VCO)
{
    register int i;

    for (i = 0; i < N; i++)
        cf_co[i] = (VCO >> (i + 0x0)) & 1;
    for (i = 0; i < N; i++)
        cf_ne[i] = (VCO >> (i + 0x8)) & 1;
    return; /* Little endian becomes big. */
}
void set_VCC(u16 VCC)
{
    register int i;

    for (i = 0; i < N; i++)
        cf_comp[i] = (VCC >> (i + 0x0)) & 1;
    for (i = 0; i < N; i++)
        cf_clip[i] = (VCC >> (i + 0x8)) & 1;
    return; /* Little endian becomes big. */
}
void set_VCE(u8 VCE)
{
    register int i;

    for (i = 0; i < N; i++)
        cf_vce[i] = (VCE >> i) & 1;
    return; /* Little endian becomes big. */
}

#ifndef ARCH_MIN_SSE2
/*
 * vector-scalar element decoding
 * Obsolete.  Consider using at least the SSE2 algorithms instead.
 */
static const int ei[1 << 4][N] = {
    { 00, 01, 02, 03, 04, 05, 06, 07 }, /* none (vector-only operand) */
    { 00, 01, 02, 03, 04, 05, 06, 07 },
    { 00, 00, 02, 02, 04, 04, 06, 06 }, /* 0Q */
    { 01, 01, 03, 03, 05, 05, 07, 07 }, /* 1Q */
    { 00, 00, 00, 00, 04, 04, 04, 04 }, /* 0H */
    { 01, 01, 01, 01, 05, 05, 05, 05 }, /* 1H */
    { 02, 02, 02, 02, 06, 06, 06, 06 }, /* 2H */
    { 03, 03, 03, 03, 07, 07, 07, 07 }, /* 3H */
    { 00, 00, 00, 00, 00, 00, 00, 00 }, /* 0 */
    { 01, 01, 01, 01, 01, 01, 01, 01 }, /* 1 */
    { 02, 02, 02, 02, 02, 02, 02, 02 }, /* 2 */
    { 03, 03, 03, 03, 03, 03, 03, 03 }, /* 3 */
    { 04, 04, 04, 04, 04, 04, 04, 04 }, /* 4 */
    { 05, 05, 05, 05, 05, 05, 05, 05 }, /* 5 */
    { 06, 06, 06, 06, 06, 06, 06, 06 }, /* 6 */
    { 07, 07, 07, 07, 07, 07, 07, 07 }  /* 7 */
};

static const int log_mask[1 << 4] = { /* inverse logarithms, truncated to int */
    1 - 1,
    1 - 1,
    2 - 1, 2 - 1,
    4 - 1, 4 - 1, 4 - 1, 4 - 1,
    8 - 1, 8 - 1, 8 - 1, 8 - 1, 8 - 1, 8 - 1, 8 - 1, 8 - 1,
};

INLINE VECTOR_OPERATION SHUFFLE_VECTOR(v16 vd, const unsigned int e)
{
    i16 SV[8];
    register unsigned int i, j;

#if (0 == 0)
    j = log_mask[e];
    for (i = 0; i < N; i++)
        SV[i] = vd[(i & ~j) | (e & j)];
#else
    if (e & 0x8)
        for (i = 0; i < N; i++)
            SV[i] = vd[(i & ~0x7) | (e & 0x7)];
    else if (e & 0x4)
        for (i = 0; i < N; i++)
            SV[i] = vd[(i & ~0x3) | (e & 0x3)];
    else if (e & 0x2)
        for (i = 0; i < N; i++)
            SV[i] = vd[(i & ~0x1) | (e & 0x1)];
    else /* if ((e == 0b0000) || (e == 0b0001)) */
        for (i = 0; i < N; i++)
            SV[i] = vd[(i & ~0x0) | (e & 0x0)];
#endif
    vector_copy(vd, SV);
    return;
}
#endif
