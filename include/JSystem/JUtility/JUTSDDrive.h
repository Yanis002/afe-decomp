#ifndef JUTSDDRIVE_H
#define JUTSDDRIVE_H

#include "types.h"
#include "JSystem/JKernel/JKRFileFinder.h"

#define DRIVE_SLOT_A 0
#define DRIVE_SLOT_B 1
#define MAX_DRIVES 2

#define MAX_PATH_LEN 126

#define IsAvailable(n) ((n & 0xFFFF) == 0)

class JUTSDCardFinder : public JKRFileFinder {
    JUTSDCardFinder(const char* param1);

    virtual ~JUTSDCardFinder();
    virtual bool findNextFile(); // _0C

    // _00     = VTBL
    // _00-_14 = JKRFileFinder
    void* mUnk_14; // _14
    char mUnk_18[0x6C - 0x18];
    u16 mUnk_6C;
};

struct JUTSDDrive {
  public:
    static bool init();
    static int setup(int param1);
    static int mount(int param1);
    static int unmount(int param1);
    static int format(int param1, u16 param2, const char* param3);
    static int terminate(int param1);
    static int removeFile(int param1, const char* param2);
    static int renameFile(int param1, const char* param2, const char* param3);
    static int setCurrentDirectory(int param1, const char* param2);
    static int makeDirectory(int param1, const char* param2);
    static int expandPath(int param1, const char* param2, char* param3);

    static bool IsInitialized() {
        return sInitialized;
    }

    static int GetCurrentDrive() {
        return sCurrentDrive;
    }

    static void* GetDriveInfoPtr(int nDrive) {
        return sDriveInfoPtr[nDrive];
    }

    static char* GetCurrentPath() {
        return sCurrentPath;
    }

    static bool GetAvailable(int nDrive) {
        return sAvailable[nDrive];
    }

    static bool GetMounted(int nDrive) {
        return sMounted[nDrive];
    }

  private:
    int mUnk_00;
    int mUnk_04;
    int mUnk_08;
    int mUnk_0C;
    int mUnk_10;
    int mUnk_14;
    int mUnk_18;
    int mUnk_1C;
    int mUnk_20;
    int mUnk_24;
    int mUnk_28;
    int mUnk_2C;
    int mUnk_30;
    int mUnk_34;
    int mUnk_38;
    int mUnk_3C;

    static bool sInitialized;
    static int sCurrentDrive;
    static void* sDriveInfoPtr[MAX_DRIVES];
    static char sCurrentPath[MAX_PATH_LEN];
    static bool sAvailable[MAX_DRIVES];
    static bool sMounted[MAX_DRIVES];
};

void JUTSeekPathString(const char* param1, char** param2, char** param3, int* param4);
void JUTCutTailPath(char* param1);
void JUTAppendDirectory(char* param1, const char* param2);

#endif
