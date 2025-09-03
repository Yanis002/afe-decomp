#include "os/OSTime.h"
#include "types.h"

#include <dolphin.h>

extern void EXI_CmdWrite(u8* data, int n);
extern void EXI_ResRead(u8* data, int n);
extern void EXI_StopResRead(u8* data, int n);
extern void EXI_DataRead(u8* data, int n);
extern void EXI_CmdWrite0(u8* data, int n);
extern void EXI_DataRes(u8* data);
extern s32 EXI_CheckTimeOut(u32 arg0, u32 arg1);

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

typedef struct ARG {
    union {
        u8 data[0x04];
        u32 data_u32;
    };
} ARG;

typedef struct UnkARG {
    ARG arg;
    u8 _00;
} UnkARG;

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
ARG SD_ARG[CARD_NUM_CHANS];
int CARD_SectorSize[CARD_NUM_CHANS];
volatile u16 CARD_ErrStatus[CARD_NUM_CHANS];
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
u16 CARD_Command(u8 param1, int cmd);
u16 CARD_Response2();
u16 CARD_AppCommand();
u16 CARD_SetBlockLength(int param_1);

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
    u16 ret_code;
    int iVar1;

    func_CARD_In[CARD_ExiChannel] = param_1;
    func_CARD_Out[CARD_ExiChannel] = param_2;
    CARD_Size[CARD_ExiChannel] = 0;
    CARD_ErrStatus[CARD_ExiChannel] = 0;

    while ((iVar1 = EXIProbeEx(CARD_ExiChannel)) == 0) {}

    if (iVar1 == 1) {
        CARD_Reset();
    } else {
        return 0x90;
    }

    ret_code = CARD_ErrStatus[CARD_ExiChannel];
    return ret_code;
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

u16 CARD_ReadD() {
}

u16 CARD_WriteD() {
}

u16 CARD_SD_Status() {
    int iVar3;
    u16 ret;
    int pad;

    CARD_ErrStatus[CARD_ExiChannel] = 0;
    iVar3 = CARD_SectorSize[CARD_ExiChannel];
    CARD_SectorSize[CARD_ExiChannel] = 0x40;
    CARD_SetBlockLength(CARD_SectorSize[CARD_ExiChannel]);

    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    CARD_AppCommand();

    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    // ?
    CARD_ErrStatus[CARD_ExiChannel];

    SD_ARG[CARD_ExiChannel].data_u32 = 0;

    if (!CARD_Command(0x4D, SD_ARG[CARD_ExiChannel].data_u32) && !CARD_Response2()) {
        EXI_DataRead(SD_SDSTATUS[CARD_ExiChannel].data, CARD_SectorSize[CARD_ExiChannel] & 0xFFFF);
    }

    CARD_SectorSize[CARD_ExiChannel] = iVar3;
    CARD_SetBlockLength(CARD_SectorSize[CARD_ExiChannel]);

    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

u16 CARD_Command(u8 param1, int cmd) {
    SD_CMD[CARD_ExiChannel].data[0] = param1;
    SD_CMD[CARD_ExiChannel].data[1] = ((ARG*)&cmd)->data[0];
    SD_CMD[CARD_ExiChannel].data[2] = ((ARG*)&cmd)->data[1];
    SD_CMD[CARD_ExiChannel].data[3] = ((ARG*)&cmd)->data[2];
    SD_CMD[CARD_ExiChannel].data[4] = ((ARG*)&cmd)->data[3];

    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);
    return CARD_ErrStatus[CARD_ExiChannel];
}

u16 CARD_Response1(void) {
    EXI_ResRead(SD_RES[CARD_ExiChannel].data, 1);

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x40)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x1000;
    }

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x20)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x0100;
    }

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x08)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x0002;
    }

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x04)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x0001;
    }

    return CARD_ErrStatus[CARD_ExiChannel];
}

u16 CARD_Response2() {
    EXI_ResRead(SD_RES[CARD_ExiChannel].data, 2);

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x7C) || (SD_RES[CARD_ExiChannel].data[1] & 0x9E)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 8;
        CARD_Status[CARD_ExiChannel] = SD_RES[CARD_ExiChannel].data[0] << 8;
        CARD_Status[CARD_ExiChannel] += SD_RES[CARD_ExiChannel].data[1];
    }

    return CARD_ErrStatus[CARD_ExiChannel];
}

u16 CARD_StopResponse() {
    EXI_StopResRead(SD_RES[CARD_ExiChannel].data, 1);

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x40)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x1000;
    }

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x20)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x0100;
    }

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x08)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x0002;
    }

    if ((SD_RES[CARD_ExiChannel].data[0] & 0x04)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x0001;
    }

    return CARD_ErrStatus[CARD_ExiChannel];
}

u16 CARD_DataResponse() {
    u16 ret;

    EXI_DataRes(SD_RES[CARD_ExiChannel].data);

    if (((SD_RES[CARD_ExiChannel].data[0] >> 1) & 7) == 5) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x002;
    }

    if (((SD_RES[CARD_ExiChannel].data[0] >> 1) & 7) == 6) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x200;
    }

    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

u16 CARD_SoftReset() {
    UnkARG iVar2;
    u16 ret;

    CARD_ErrStatus[CARD_ExiChannel] = 0;

    SD_ARG[CARD_ExiChannel].data_u32 = 0;
    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0;
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];

    EXI_CmdWrite0(SD_CMD[CARD_ExiChannel].data, 5);

    CARD_Response1();

    SD_CMD[CARD_ExiChannel].data[0] = 0x0C;
    SD_CMD[CARD_ExiChannel].data[1] = 0x00;
    SD_CMD[CARD_ExiChannel].data[2] = 0x00;
    SD_CMD[CARD_ExiChannel].data[3] = 0x00;
    SD_CMD[CARD_ExiChannel].data[4] = 0x00;

    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);

    if (CARD_ErrStatus[CARD_ExiChannel] == 0) {
        CARD_StopResponse();
    }

    CARD_ErrStatus[CARD_ExiChannel] = 0;

    SD_ARG[CARD_ExiChannel].data_u32 = 0;
    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0;
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];
    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);

    if (CARD_ErrStatus[CARD_ExiChannel] == 0) {
        CARD_Response1();
    }

    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

u16 CARD_AppCommand() {
    UnkARG iVar2;
    u16 ret;

    SD_ARG[CARD_ExiChannel].data_u32 = 0;
    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0x37;
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];
    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);
    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    CARD_Response1();

    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

inline void test() {
    if (CARD_ErrStatus[CARD_ExiChannel] == 0) {
        CARD_Response1();
    }
}

u16 CARD_SendOpCond() {
    UnkARG iVar2;
    u16 ret;
    OSTick tick;
    s32 timeout;

    //! TODO: match with CARD_Command
    tick = OSGetTick();

    do {
        SD_ARG[CARD_ExiChannel].data_u32 = 0;
        iVar2.arg = SD_ARG[CARD_ExiChannel];
        iVar2._00 = 0x37;
        SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
        SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
        SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
        SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
        SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];
        EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);

        test();

        if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
            ret = CARD_ErrStatus[CARD_ExiChannel];
            return ret;
        }

        SD_ARG[CARD_ExiChannel].data_u32 = 0;
        iVar2.arg = SD_ARG[CARD_ExiChannel];
        iVar2._00 = 0x69;
        SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
        SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
        SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
        SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
        SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];
        EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);
        if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
            ret = CARD_ErrStatus[CARD_ExiChannel];
            return ret;
        }

        if (CARD_Response1() || !(SD_RES[CARD_ExiChannel].data[0] & 1)) {
            goto end;
        }

        timeout = EXI_CheckTimeOut(tick, 1500);
    } while (timeout == 0);

    SD_ARG[CARD_ExiChannel].data_u32 = 0;
    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0x37;
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];
    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);

    test();

    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    SD_ARG[CARD_ExiChannel].data_u32 = 0;
    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0x09;
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];
    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);
    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    if (!CARD_Response1() && (SD_RES[CARD_ExiChannel].data[0] & 1)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0x8000;
    }

end:
    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

u16 CARD_SendCSD() {
    UnkARG iVar2;
    u16 ret;
    int i;

    SD_ARG[CARD_ExiChannel].data_u32 = 0;

    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0x09;

    //! TODO: match with CARD_Command
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];

    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);

    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    if (!CARD_Response1()) {
        EXI_DataRead(SD_CSD[CARD_ExiChannel].data, 0x10);
    }

    for (i = 0; i < 0x10; i++) {}

    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

u16 CARD_SendCID() {
    UnkARG iVar2;
    u16 ret;
    int i;

    SD_ARG[CARD_ExiChannel].data_u32 = 0;

    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0x0A;

    //! TODO: match with CARD_Command
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];

    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);

    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    if (!CARD_Response1()) {
        EXI_DataRead(SD_CID[CARD_ExiChannel].data, 0x10);
    }

    for (i = 0; i < 0x10; i++) {}

    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

u16 CARD_SetBlockLength(int param_1) {
    UnkARG iVar2;
    u16 ret;

    SD_ARG[CARD_ExiChannel].data_u32 = param_1;

    iVar2.arg = SD_ARG[CARD_ExiChannel];
    iVar2._00 = 0x10;

    //! TODO: match with CARD_Command
    SD_CMD[CARD_ExiChannel].data[0] = iVar2._00;
    SD_CMD[CARD_ExiChannel].data[1] = iVar2.arg.data[0];
    SD_CMD[CARD_ExiChannel].data[2] = iVar2.arg.data[1];
    SD_CMD[CARD_ExiChannel].data[3] = iVar2.arg.data[2];
    SD_CMD[CARD_ExiChannel].data[4] = iVar2.arg.data[3];

    EXI_CmdWrite(SD_CMD[CARD_ExiChannel].data, 5);

    if (CARD_ErrStatus[CARD_ExiChannel] != 0) {
        ret = CARD_ErrStatus[CARD_ExiChannel];
        return ret;
    }

    // if (CARD_Command(iVar2.data[4], iVar2.data)) {
    //     ret = CARD_ErrStatus[CARD_ExiChannel];
    //     return ret;
    // }

    CARD_Response1();

    ret = CARD_ErrStatus[CARD_ExiChannel];
    return ret;
}

u16 CARD_Term() {
    u16 uVar2;

    CARD_ErrStatus[CARD_ExiChannel] = 0;

    if (EXIDetach(CARD_ExiChannel) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xC0;
        uVar2 = CARD_ErrStatus[CARD_ExiChannel];
        return uVar2;
    }

    return 0;
}
