#include "vu.h"

static void VMADH(int vd, int vs, int vt, int e)
{
    register signed long long product;
    register int i;

    for (i = 0; i < 8; i++)
    {
        product = VR[vs][i]*VR[vt][ei[e][i]];
        VACC[i].DW += product << 16;
    }
    for (i = 0; i < 8; i++) /* Sign-clamp bits 31..16 of ACC to dest. VR. */
        if (VACC[i].DW & 0x800000000000) /* acc < 0 */
            if ((VACC[i].DW & 0xFFFF80000000) != 0xFFFF80000000)
                VR[vd][i] = 0x8000; /* slice underflow */
            else
                VR[vd][i] = VACC[i].s[MD];
        else
            if ((VACC[i].DW & 0xFFFF80000000) != 0x000000000000)
                VR[vd][i] = 0x7FFF; /* slice overflow */
            else
                VR[vd][i] = VACC[i].s[MD];
    return;
}
