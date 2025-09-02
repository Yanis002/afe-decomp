#include "types.h"

#include <dolphin.h>

typedef struct CID {
    u8 data[0x10];
} CID;

typedef struct CSD {
    u8 data[0x12];
} CSD;

typedef struct SDSTATUS {
    u8 data[0x40];
} SDSTATUS;

typedef struct CMD {
    u8 data[0x05];
} CMD;

typedef struct RES {
    u8 data[0x80];
} RES;

RES SD_RES[CARD_NUM_CHANS];
CMD SD_CMD[CARD_NUM_CHANS];
CID SD_CID[CARD_NUM_CHANS];
SDSTATUS SD_SDSTATUS[CARD_NUM_CHANS];
CSD SD_CSD[CARD_NUM_CHANS];
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

u32 TEMP_BSS_ORDER_FIX_REMOVE_ME() {
    CARD_Sem[0].count + CARD_Alarm[0].fire;
    return SD_SDSTATUS[0].data[0] + SD_CID[0].data[0] + SD_CSD[0].data[0];
}

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

    while ((iVar1 = EXIProbeEx(CARD_ExiChannel)) == 0)
        ;

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

    while ((iVar1 = EXIProbeEx(CARD_ExiChannel)) == 0) {}

    if (iVar1 != 1) {
        param1->unk_00 |= 0x420;
    }

    if (CARD_WP_Flag[CARD_ExiChannel] != 0) {
        param1->unk_00 |= 0x80;
    }

    return 0;
}

int CARD_Getinfo(u8* param1) {
    s32 iVar1;
    u8* ptr = param1;

    for (iVar1 = 0; iVar1 < 0x10; iVar1++) {
        *ptr++ = SD_CID[CARD_ExiChannel].data[0x0F - iVar1];
    }

    // not sizeof(CSD)?
    for (iVar1 = 0; iVar1 < 0x10; iVar1++) {
        *ptr++ = SD_CSD[CARD_ExiChannel].data[0x0F - iVar1];
    }

    return 0;
}
