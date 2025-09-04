#include "os.h"
#include "types.h"
#include "JSystem/JUtility/carddrv.h"

#include <dolphin.h>

u8 SD_DUMMY[0x80];
u8 EXI_ClrData[CARD_NUM_CHANS];

s32 EXI_CheckTimeOut(u32 arg0, u32 arg1);
u16 EXI_MakeCRC16(u8* param_1, u16 param_2);

asm void EXI_Null(s32 chan /*, OSContext* context*/) {
}

asm void EXI_Unlock(s32 chan /*, OSContext* context*/) {
    li r4, 1
    lhz r0, CARD_ExiChannel
    slwi r0, r0, 2
    li r3, CARD_UnlockFlag
    stwx r4, r3, r0
}

#pragma peephole on

void EXI_AlarmFunc(OSAlarm* alarm, OSContext* context) {
    OSSignalSemaphore(&CARD_Sem[CARD_ExiChannel]);
}

u16 EXI_LockAndSelect(u32 nFreq) {
    CARD_UnlockFlag[CARD_ExiChannel] = 0;

    if (EXILock(CARD_ExiChannel, 0, (EXICallback)EXI_Unlock) == 0) {
        while (CARD_UnlockFlag[CARD_ExiChannel] == 0) {}

        if (EXILock(CARD_ExiChannel, 0, (EXICallback)EXI_Null) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xA0;
            return CARD_ErrStatus[CARD_ExiChannel];
        }
    }

    if (EXISelect(CARD_ExiChannel, 0, nFreq) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xB0;

        if (EXIUnlock(CARD_ExiChannel) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xD0;
        }

        return CARD_ErrStatus[CARD_ExiChannel];
    }

    return 0;
}

u16 EXI_DeselectAndUnlock() {
    if (EXIDeselect(CARD_ExiChannel) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xE0;
    }

    if (EXIUnlock(CARD_ExiChannel) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xD0;
        CARD_ErrStatus[CARD_ExiChannel];
    }

    return CARD_ErrStatus[CARD_ExiChannel];
}

static inline void EXI_UnknownInline3(RES* pRES, OSTick tick, int time) {
    while (((pRES->data[0] & 0x80) != 0)) {
        pRES->data[0] = EXI_ClrData[CARD_ExiChannel];
    
        if (EXIImmEx(CARD_ExiChannel, pRES, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
            break;
        }
    
        if ((pRES->data[0] & 0x80) == 0) {
            break;
        }

        if (EXI_CheckTimeOut(tick, time) == 0) {
            continue;
        }

        pRES->data[0] = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, pRES, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        } else if ((pRES->data[0] & 0x80) != 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0x4000;
        }

        break;
    }
}

static inline void EXI_UnknownInline4(RES* pRES, OSTick tick, int time) {
    while (pRES->data[0] != 0xFF) {
        pRES->data[1] = pRES->data[0];
        pRES->data[0] = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, pRES, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
            break;
        } 
        
        if (pRES->data[0] == 0xFF) {
            break;
        }

        if (EXI_CheckTimeOut(tick, 0x5DCU) == 0) {
            continue;
        }

        pRES->data[1] = pRES->data[0];
        pRES->data[0] = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, pRES, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        } else if (pRES->data[0] != 0xFF) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0x4000;
        }

        break;
    }
}

u16 EXI_ResRead(RES *arg0, u16 arg1) {
    RES *sp8;
    u16 spE;

    if (EXI_LockAndSelect(CARD_ExiFreq[CARD_ExiChannel])) {
        return CARD_ErrStatus[CARD_ExiChannel];
    }

    // !spE;
    spE = 0;

    sp8 = arg0;
    sp8->data[0] = EXI_ClrData[CARD_ExiChannel];

    if (EXIImmEx(CARD_ExiChannel, sp8->data, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    EXI_UnknownInline3(sp8, OSGetTick(), 500);

    if (arg1 > sizeof(u8)) {
        *++sp8->data = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, sp8->data, arg1 - 1, 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        }
    }

    return EXI_DeselectAndUnlock();
}

u16 EXI_StopResRead(RES *arg0, u16 arg1) {
    RES *sp8;
    s16 spE;
    OSTick tick;

    if (EXI_LockAndSelect(CARD_ExiFreq[CARD_ExiChannel])) {
        return CARD_ErrStatus[CARD_ExiChannel];
    }

    spE = 0;
    sp8 = arg0;

    sp8->data[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    sp8->data[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    tick = OSGetTick();
    EXI_UnknownInline3(sp8, tick, 1500);
    EXI_UnknownInline4(sp8, tick, 1500);

    sp8->data[0] = sp8->data[1];

    if (arg1 > sizeof(u8)) {
        *++sp8->data = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, (u8*)sp8, arg1 - 1, 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        }
    }

    return EXI_DeselectAndUnlock();
}

u16 EXI_DataRes(RES* arg0) {
    RES *sp8;
    s16 spE;
    OSTick tick;
    OSTick tick2;

    if (EXI_LockAndSelect(CARD_ExiFreq[CARD_ExiChannel])) {
        return CARD_ErrStatus[CARD_ExiChannel];
    }

    spE = 0;
    sp8 = arg0;

    sp8->data[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    tick = OSGetTick();
    while (sp8->data[0] & 0x10) {
        sp8->data[0] = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
            return EXI_DeselectAndUnlock();
        }

        if (sp8->data[0] & 0x10) {
            if (EXI_CheckTimeOut(tick, 1500) == 0) {
                continue;
            }
            
            sp8->data[0] = EXI_ClrData[CARD_ExiChannel];

            if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
                return EXI_DeselectAndUnlock();
            }

            if (sp8->data[0] & 0x10) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0x4000;
            }
        }

        break;
    }

    *++sp8->data = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    tick2 = OSGetTick();
    while (sp8->data[0] == 0) {
        sp8->data[0] = EXI_ClrData[CARD_ExiChannel];
    
        if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
            return EXI_DeselectAndUnlock();
        }

        if (sp8->data[0] == 0) {
            if (EXI_CheckTimeOut(tick2, 1500) == 0) {
                continue;
            }

            sp8->data[0] = EXI_ClrData[CARD_ExiChannel];
            
            if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
                return EXI_DeselectAndUnlock();
            }
            
            if (sp8->data[0] == 0) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0x4000;
            }
        }

        break;
    }

    return EXI_DeselectAndUnlock();
}

u16 EXI_MultiWriteStop(void) {
    int tick;
    int pad;
    int i;
    u8 sp8[32];

    for (i = 0; i < 0x20; i++) {
        sp8[i] = EXI_ClrData[CARD_ExiChannel];
    }

    if (EXI_LockAndSelect(CARD_ExiFreq[CARD_ExiChannel])) {
        return CARD_ErrStatus[CARD_ExiChannel];
    }

    sp8[0] = 0xFD;
    if ((s32) CARD_WP_Flag[CARD_ExiChannel] != 0) {
        sp8[0] = 2;
    }

    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 1) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    sp8[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    sp8[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    sp8[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    sp8[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    OSGetTick();
    tick = OSGetTick();
    !tick;

    while (sp8[0] == 0) {
        sp8[0] = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
            return EXI_DeselectAndUnlock();
        }

        if (sp8[0] == 0) {
            if (EXI_CheckTimeOut(tick, 1500) == 0) {
                continue;
            }

            sp8[0] = EXI_ClrData[CARD_ExiChannel];

            if (EXIImmEx(CARD_ExiChannel, sp8, sizeof(u8), 2) == 0) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
                return EXI_DeselectAndUnlock();
            }

            if (sp8[0] == 0) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0x4000;
            }
        }

        break;
    }

    return EXI_DeselectAndUnlock();
}

u16 EXI_DataRead(RES* arg0, u16 arg1) {
    OSTick temp_r25;
    int var_r28;
    u8 *var_r31;
    u16 sp12[CARD_NUM_CHANS];
    u8 sp10[CARD_NUM_CHANS];

    if (EXI_LockAndSelect(CARD_ExiFreq[CARD_ExiChannel])) {
        return CARD_ErrStatus[CARD_ExiChannel];
    }

    var_r31 = arg0->data;
    for (var_r28 = 0; var_r28 < arg1; var_r28++) {
        *var_r31++ = EXI_ClrData[CARD_ExiChannel];
    }

    sp12[0] = sp12[1] = 0;

    if (EXIImmEx(CARD_ExiChannel, arg0, sizeof(u8), 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    temp_r25 = OSGetTick();
    while (arg0->data[0] != 0xFE) {
        arg0->data[0] = EXI_ClrData[CARD_ExiChannel];

        if (EXIImmEx(CARD_ExiChannel, arg0, sizeof(u8), 2) == 0) {
            CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
            return EXI_DeselectAndUnlock();
        }

        if (arg0->data[0] != 0xFE) {
            if (EXI_CheckTimeOut(temp_r25, 1500) == 0) {
                continue;
            }

            arg0->data[0] = EXI_ClrData[CARD_ExiChannel];

            if (EXIImmEx(CARD_ExiChannel, arg0, sizeof(u8), 2) == 0) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
                return EXI_DeselectAndUnlock();
            }

            if (arg0->data[0] != 0xFE) {
                CARD_ErrStatus[CARD_ExiChannel] |= 0x4000;
            }
        }

        break;
    }

    arg0->data[0] = EXI_ClrData[CARD_ExiChannel];
    if (EXIImmEx(CARD_ExiChannel, arg0, arg1, 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    OSSetAlarm(&CARD_Alarm[CARD_ExiChannel], OSMicrosecondsToTicks(1), EXI_AlarmFunc);
    OSWaitSemaphore(&CARD_Sem[CARD_ExiChannel]);

    sp10[0] = EXI_ClrData[CARD_ExiChannel];
    sp10[1] = EXI_ClrData[CARD_ExiChannel];

    if (EXIImmEx(CARD_ExiChannel, &sp10, sizeof(u8) * 2, 2) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xF0;
        return EXI_DeselectAndUnlock();
    }

    sp12[0] = (sp10[0] << 8) & 0xFF00;
    sp12[0] += sp10[1];

    if (EXIDeselect(CARD_ExiChannel) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xE0;
    }

    if (EXIUnlock(CARD_ExiChannel) == 0) {
        CARD_ErrStatus[CARD_ExiChannel] |= 0xD0;
        CARD_ErrStatus[CARD_ExiChannel];
    }

    if (sp12[0] != EXI_MakeCRC16(arg0->data, arg1)) {
        CARD_ErrStatus[CARD_ExiChannel] |= 2;
    }

    return CARD_ErrStatus[CARD_ExiChannel];
}

u16 EXI_MakeCRC16(u8* param_1, u16 param_2) {
    u16 uVar1;
    uint uVar2;
    int iVar3;
    uint local_20;

    local_20 = 0;
    for (uVar1 = 0; uVar1 < param_2; uVar1++) {
        uVar2 = 0x80;
        iVar3 = 0;

        while (iVar3 < 8) {
            if ((uVar2 & ((uint)*param_1 ^ (int)(local_20 & 0xffff) >> (iVar3 + 8U & 0x3f) & 0xffU)) == 0) {
                local_20 <<= 1;

                if (((local_20 ^ 0x20) & 0x20) != 0) {
                    local_20 |= 0x20;
                } else {
                    local_20 &= ~0x20;
                }

                if (((local_20 ^ 0x1000) & 0x1000) != 0) {
                    local_20 |= 0x1000;
                } else {
                    local_20 &= ~0x1000;
                }
            } else {
                local_20 <<= 1;
                if ((local_20 & 0x20) != 0) {
                    local_20 |= 0x20;
                } else {
                    local_20 &= ~0x20;
                }

                if ((local_20 & 0x1000) != 0) {
                    local_20 |= 0x1000;
                } else {
                    local_20 &= ~0x1000;
                }
            }

            uVar2 = (int)uVar2 >> 1;
            iVar3++;
        }

        local_20 &= 0xffff;
        param_1++;
    }
    return local_20;
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
