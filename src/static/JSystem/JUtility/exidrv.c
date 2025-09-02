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

void EXI_Null() {
    // what the
    register s32 a;
    
    asm {
        stw r3, a;
    }
}

void EXI_Unlock() {
    EXI_Null();
    CARD_UnlockFlag[CARD_ExiChannel] = 1;
}

void EXI_AlarmFunc(void) {
    OSSignalSemaphore(&CARD_Sem[CARD_ExiChannel]);
}

s32 EXI_CheckTimeOut(u32 arg0, u32 arg1) {
    OSTick var_r31;
    OSTick temp_r3;

    if ((temp_r3 = OSGetTick()) < arg0) {
        var_r31 = 0xFFFFFFFF - arg0;
        var_r31 += 1 + temp_r3;
    } else {
        var_r31 = temp_r3 - arg0;
    }

    if (OSTicksToMilliseconds(var_r31) > arg1) {
        return 1;
    }

    return 0;
}
