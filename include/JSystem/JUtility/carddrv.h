#ifndef CARDDRV_H
#define CARDDRV_H

#include "types.h"
#include "JSystem/JUtility/JUTSDCard.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Test {
    u16 unk_00;
} Test;

typedef struct UnkARG {
    ARG arg;
    u8 _00;
} UnkARG;

typedef struct ReadWriteDParam5 {
    u16 unk_00;
    u16 unk_02;
    u32 unk_04;
} ReadWriteDParam5;

extern int CARD_IF_Reset();
extern u16 CARD_InitD(int param_1, int param_2);
extern u16 CARD_SelectedNo();
extern u16 CARD_Select(u16 param_1);
extern u16 CARD_Reset();
extern int CARD_Getstatus(Test* param1);
extern int CARD_Getinfo(u8* param1);
extern u16 CARD_ReadD(SDSTATUS* param1, u32 param2, int param3, int param4, ReadWriteDParam5* param5);
extern u16 CARD_WriteD(SDSTATUS* param1, u32 param2, int param3, int param4, ReadWriteDParam5* param5);
extern u16 CARD_SD_Status();
extern u16 CARD_Command(u8 param1, int cmd);
extern u16 CARD_Response1(void);
extern u16 CARD_Response2();
extern u16 CARD_StopResponse();
extern u16 CARD_DataResponse();
extern u16 CARD_SoftReset();
extern u16 CARD_AppCommand();
extern u16 CARD_SendOpCond();
extern u16 CARD_SendCSD();
extern u16 CARD_SendCID();
extern u16 CARD_SetBlockLength(int param_1);
extern u16 CARD_Term();

#ifdef __cplusplus
};
#endif

#endif
