#include "os.h"
#include "types.h"

#include <dolphin.h>

extern OSSemaphore CARD_Sem[2];
extern void* func_CARD_Out[2];
extern void* func_CARD_In[2];
extern int CARD_Size[2];
extern int CARD_UnlockFlag[2];
extern int CARD_Status[2];
extern u16 CARD_ErrStatus[2];
extern int CARD_SectorSize[2];
extern int SD_ARG[2];
extern int CARD_ExiFreq;
extern u16 CARD_ExiChannel;
extern int CARD_WP_Flag[2];

void EXI_Null(s32 param1) {
    s32 sp8;

    sp8 = param1;
}

void EXI_Unlock(s32 arg0) {
    s32 sp8;

    EXI_Null(sp8);
    CARD_UnlockFlag[CARD_ExiChannel] = 1;
}

void EXI_AlarmFunc(void) {
    OSSignalSemaphore(&CARD_Sem[CARD_ExiChannel]);
}

s32 EXI_CheckTimeOut(u32 arg0, u32 arg1) {
    OSTick var_r31;
    OSTick temp_r3;

    var_r31 = OSGetTick();

    if (temp_r3 < arg0) {
        var_r31 = temp_r3 + (-1 - arg0) + 1;
    } else {
        var_r31 = temp_r3 - arg0;
    }

    if (OSTicksToMilliseconds(var_r31) > arg1) {
        return 1;
    }

    return 0;
}
