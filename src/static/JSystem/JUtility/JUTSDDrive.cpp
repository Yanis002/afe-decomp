#include "JSystem/JUtility/JUTSDDrive.h"
#include "JSystem/JUtility/JUTFileSystem.h"

#include <string.h>

extern "C" int SDTerm(u16);

bool JUTSDDrive::sInitialized;
int JUTSDDrive::sCurrentDrive;
void* JUTSDDrive::sDriveInfoPtr[MAX_DRIVES];
char JUTSDDrive::sCurrentPath[MAX_PATH_LEN];
bool JUTSDDrive::sAvailable[MAX_DRIVES] = { false, false };
bool JUTSDDrive::sMounted[MAX_DRIVES] = { false, false };
u16 driveTable[MAX_DRIVES] = { DRIVE_SLOT_A, DRIVE_SLOT_B };

bool JUTSDDrive::init() {
    if (!IsAvailable(FS_CardIFReset())) {
        return false;
    }

    sInitialized = true;
    sAvailable[DRIVE_SLOT_A] = false;
    sAvailable[DRIVE_SLOT_B] = false;
    sMounted[DRIVE_SLOT_A] = false;
    sMounted[DRIVE_SLOT_B] = false;
    sCurrentDrive = DRIVE_SLOT_A;
    return true;
}

int JUTSDDrive::setup(int param1) {
    unsigned int uVar1 = FS_Init(0, 0, driveTable[param1]);
    unsigned int uVar2;
    unsigned int ret;

    if (!IsAvailable(uVar1)) {
        sAvailable[param1] = false;
        sMounted[param1] = false;

        uVar2 = JUTSDDrive::terminate(param1);
        ret = uVar1;

        if (!IsAvailable(uVar2)) {
            ret = uVar2;
        }

        return ret;
    }

    sAvailable[param1] = true;
    sMounted[param1] = false;
    strcpy(&sCurrentPath[param1 * 0x3F], "\\");
    return 0;
}

int JUTSDDrive::mount(int param1) {
    unsigned int uVar1 = FS_Mount(&sDriveInfoPtr[param1], driveTable[param1]);

    if (!IsAvailable(uVar1)) {
        return uVar1;
    }

    sMounted[param1] = true;
    return 0;
}

int JUTSDDrive::unmount(int param1) {
    unsigned int uVar1 = FS_Umount(sDriveInfoPtr[param1]);

    if (!IsAvailable(uVar1)) {
        return uVar1;
    }

    sMounted[param1] = false;
    return 0;
}

int JUTSDDrive::format(int param1, u16 param2, const char* param3) {
    return Format(param3, param2, driveTable[param1]);
}

int JUTSDDrive::terminate(int param1) {
    unsigned int uVar1 = SDTerm(driveTable[param1]);

    if (!IsAvailable(uVar1)) {
        return uVar1;
    }

    sAvailable[param1] = false;
    sMounted[param1] = false;
    return 0;
}

int JUTSDDrive::removeFile(int param1, const char* param2) {
    char acStack_48[64];

    expandPath(param1, param2, acStack_48);
    return Delete(sDriveInfoPtr[param1], acStack_48);
}

int JUTSDDrive::renameFile(int param1, const char* param2, const char* param3) {
    char acStack_48[64];
    char acStack_88[64];

    expandPath(param1, param2, acStack_48);
    expandPath(param1, param3, acStack_88);
    return Rename(sDriveInfoPtr[param1], acStack_48, acStack_88);
}

int JUTSDDrive::setCurrentDirectory(int param1, const char* param2) {
    char* __src;
    unsigned int uVar1;
    char acStack_58[64];

    __src = &sCurrentPath[param1 * 0x3f];
    strcpy(acStack_58, __src);
    JUTAppendDirectory(__src, param2);
    uVar1 = FS_Chdir(sDriveInfoPtr[param1], __src);

    if (!IsAvailable(uVar1)) {
        strcpy(__src, acStack_58);
        return uVar1;
    }

    return 0;
}

int JUTSDDrive::makeDirectory(int param1, const char* param2) {
    char acStack_48[64];

    JUTSDDrive::expandPath(param1, param2, acStack_48);
    return Mkdir(sDriveInfoPtr[param1], acStack_48);
}

JUTSDCardFinder::JUTSDCardFinder(const char* param1) {
    unsigned int uVar1;
    char acStack_58[64];
    int currentDrive = JUTSDDrive::GetCurrentDrive();
    char* currentPath = JUTSDDrive::GetCurrentPath();

    strcpy(acStack_58, &currentPath[currentDrive * 0x3F]);
    JUTAppendDirectory(acStack_58, param1);

    uVar1 = Opendir(JUTSDDrive::GetDriveInfoPtr(currentDrive), &mUnk_14, acStack_58);
    mUnk_6C = uVar1;
    mIsAvailable = IsAvailable(uVar1);
}

JUTSDCardFinder::~JUTSDCardFinder() {
    u32 uVar1 = 0;

    if (mIsAvailable) {
        uVar1 = Closedir(mUnk_14);
    }

    mUnk_6C = uVar1;
}

bool JUTSDCardFinder::findNextFile() {
    unsigned int uVar1;
    unsigned int uVar2;
    bool uVar3;

    uVar2 = Readdir(this->mUnk_14, &this->mUnk_18);
    uVar1 = uVar2 & 0xFFFF;
    this->mUnk_6C = uVar2;

    if (uVar1 == 0xA030) {
        return false;
    }

    if (!IsAvailable(uVar1)) {
        return false;
    }

    //! TODO: fake match?
    this->mIsDir = (this->mUnk_5C >> 4) & 1;
    return true;
}

int JUTSeekPathString(const char* param1, char** param2, char** param3, int* param4) {
    char* param;
    int iVar2;

    if (param1 == NULL || *param1 == '\0') {
        return JUT_PATH_NONE;
    }

    param = (char*)param1;
    *param3 = (char*)param1;

    for (iVar2 = 0; *param != '\0'; param++, iVar2++) {
        if (*param == '\\' || *param == '/') {
            break;
        }
    }

    if (iVar2 == 0) {
        iVar2 = 1;
        param++;
    }

    while (*param == '\\' || *param == '/') {
        param++;
    }

    *param2 = *param != '\0' ? param : NULL;
    *param4 = iVar2;

    if ((*param3)[0] == '.') {
        if (iVar2 == 1) {
            return JUT_PATH_CUR_DIR;
        }

        if ((*param3)[1] == '.') {
            return JUT_PATH_PARENT_DIR;
        }
    }

    return JUT_PATH_SUCCESS;
}

void JUTCutTailPath(char* param1) {
    char* pcVar1;
    char* pcVar2;

    pcVar2 = NULL;

    for (pcVar1 = param1; *pcVar1 != '\0'; pcVar1 = pcVar1 + 1) {
        if (*pcVar1 == '\\') {
            pcVar2 = pcVar1;
        }
    }

    if (pcVar2 == NULL) {
        *param1 = '\\';
        param1[1] = '\0';
        return;
    }

    if (pcVar2 == param1) {
        param1[1] = '\0';
        return;
    }

    *pcVar2 = '\0';
}

char* JUTAppendDirectory(char* param1, const char* param2) {
    char* sp8;
    char* spC;
    int sp10;
    int result = JUTSeekPathString(param2, &sp8, &spC, &sp10);

    if (result == JUT_PATH_NONE) {
        return param1;
    }

    if (result != JUT_PATH_CUR_DIR) {
        if (result == JUT_PATH_PARENT_DIR) {
            JUTCutTailPath(param1);
        } else {
            if (*spC == '\\' || *spC == '/') {
                *param1 = '\0';
                strncat(param1, spC, sp10);

                if (*param1 == '/') {
                    *param1 = '\\';
                }
            } else {
                if (param1[0] != '\\' || param1[1] != '\0') {
                    strcat(param1, "\\");
                }

                strncat(param1, spC, sp10);
            }
        }
    }

    JUTAppendDirectory(param1, sp8);
    return param1;
}

int JUTSDDrive::expandPath(int param1, const char* param2, char* param3) {
    char* currentPath = JUTSDDrive::GetCurrentPath();

    //! TODO: is param1 a struct of size 0x3F?
    strcpy(param3, &currentPath[param1 * 0x3F]);
    JUTAppendDirectory(param3, param2);
    return 0;
}
