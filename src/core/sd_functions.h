#ifndef __SD_FUNCTIONS_H__
#define __SD_FUNCTIONS_H__

#include <FS.h>
#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>

struct FileList {
    String filename;
    bool folder;
    bool operation;
};

// extern SPIClass sdcardSPI;

bool setupLittleFS(uint8_t maxFiles = 3);

bool setupSdCard(uint8_t maxFiles = 3);

void closeSdCard();

bool ToggleSDCard();

bool deleteFromSd(FS fs, String path);

bool renameFile(FS fs, String path, String filename);

bool copyFile(FS fs, String path);

bool copyToFs(FS from, FS to, String path, bool draw = true);

bool pasteFile(FS fs, String path);

bool createFolder(FS fs, String path);

bool folderExists(FS fs, String path);

String readLineFromFile(File myFile);

String readSmallFile(FS &fs, const String &filepath);

char *readBigFile(FS *fs, const String &filepath, bool binary = false, size_t *fileSize = NULL);

String md5File(FS &fs, const String &filepath);

String crc32File(FS &fs, const String &filepath);

void readFs(FS &fs, const String &folder, const String &allowed_ext = "*");

bool sortList(const FileList &a, const FileList &b);

String loopSD(FS &fs, bool filePicker = false, const String &allowed_ext = "*", String rootPath = "/");

void viewFile(FS &fs, const String &filepath);

bool checkLittleFsSize();

bool checkLittleFsSizeNM(); // Don't display msg

bool getFsStorage(FS *&fs);

void fileInfo(FS &fs, const String &filepath);

File createNewFile(FS *&fs, String filepath, String filename);

#endif
