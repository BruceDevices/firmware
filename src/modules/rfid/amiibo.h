/**
 * @file amiibo.h
 * @author Rennan Cockles (https://github.com/rennancockles)
 * @brief Emulate amiibo using an amiibolink
 * @version 0.2
 * @date 2024-10-11
 */

#ifndef __AMIIBO_H__
#define __AMIIBO_H__
#ifndef LITE_VERSION
#include <amiibolink.h>

class Amiibo {
public:
    typedef struct {
        String uid;
        String bcc;
        String sak;
        String atqa;
        String picc_type;
    } PrintableUID;

    enum AppMode {
        START_MODE,
        AMIIBO_UPLOAD,
        CHANGE_UID_MODE,
    };

    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    Amiibo();
    ~Amiibo();

    /////////////////////////////////////////////////////////////////////////////////////
    // Life Cycle
    /////////////////////////////////////////////////////////////////////////////////////
    void setup();
    bool connect();

private:
    Amiibolink amiibolink = Amiibolink(true);
    AppMode currentMode;
    PrintableUID printableUID;
    String strDump = "";

    void displayBanner(AppMode mode = START_MODE);

    void selectMode();
    void uploadAmiibo();
    void changeUIDMode();

    bool openDumpFile();
    bool checkEmulationTagType();
    void delayWithReturn(uint32_t ms);
};

#endif
#endif
