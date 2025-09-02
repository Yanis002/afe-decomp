#include "types.h"

#include <dolphin.h>

OSAlarm CARD_Alarm[CARD_NUM_CHANS];
OSSemaphore CARD_Sem[CARD_NUM_CHANS];

const int CARD_TBL_CLOCK_DIV[] = {
    0xFFFFFFFF, 0xFF000305, 0xFF000305, 0xFF000305, 0xFF000305, 0xFF010405, 0xFF010405, 0xFF010405,
    0xFF010505, 0xFF020505, 0xFF020505, 0xFF020505, 0xFF020505, 0xFF020505, 0xFF020505, 0xFF030505,
};

int CARD_WP_Flag[CARD_NUM_CHANS];
u16 CARD_ExiChannel;
u16 CARD_ExiFreq[CARD_NUM_CHANS];
int SD_ARG[CARD_NUM_CHANS];
int CARD_SectorSize[CARD_NUM_CHANS];
u16 CARD_ErrStatus[CARD_NUM_CHANS];
int CARD_Status[CARD_NUM_CHANS];
int CARD_UnlockFlag[CARD_NUM_CHANS];
int CARD_Size[CARD_NUM_CHANS];
int func_CARD_In[CARD_NUM_CHANS];
int func_CARD_Out[CARD_NUM_CHANS];

void CARD_Reset();

int CARD_IF_Reset() {
    int i;

    EXIInit();
    CARD_ExiChannel = 0;

    for (i = 0; i < CARD_NUM_CHANS; i++) {
        OSInitSemaphore(&CARD_Sem[i], 0);
        OSCreateAlarm(&CARD_Alarm[i]);
        CARD_ErrStatus[i] = 0;
    }

    return 0;
}

u16 CARD_InitD(int param_1, int param_2) {
    int iVar1;
    u16 uVar2;

    func_CARD_In[CARD_ExiChannel] = param_1;
    func_CARD_Out[CARD_ExiChannel] = param_2;
    CARD_Size[CARD_ExiChannel] = 0;
    CARD_ErrStatus[CARD_ExiChannel] = 0;

    while ((iVar1 = EXIProbeEx(CARD_ExiChannel)) == 0);

    if (iVar1 == 1) {
        CARD_Reset();
        return 0x90;
    } else {
        return CARD_ErrStatus[CARD_ExiChannel];
    }

    // return uVar2;
}

u16 CARD_SelectedNo() {
    return CARD_ExiChannel;
}

u16 CARD_Select(u16 param_1) {
    if (CARD_ExiChannel != param_1) {
        CARD_ExiChannel = param_1;
    }

    CARD_ErrStatus[CARD_ExiChannel] = 0;
    return CARD_ErrStatus[CARD_ExiChannel];
}

void CARD_Reset() {
}

typedef struct Test {
    u16 unk_00;
} Test;

int CARD_Getstatus(Test* param1) {
    int iVar1;

    param1->unk_00 = 0x831F;

    while ((iVar1 = EXIProbeEx(CARD_ExiChannel)) == 0);

    if (iVar1 != 1) {
        param1->unk_00 |= 0x420;
    }

    if (CARD_WP_Flag[CARD_ExiChannel] != 0) {
        param1->unk_00 |= 0x80;
    }

    return 0;
}
