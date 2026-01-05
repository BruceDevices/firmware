#include "sd_functions.h"
#include "display.h" // using displayRedStripe as error msg
#include "modules/badusb_ble/ducky_typer.h"
#include "modules/bjs_interpreter/interpreter.h"
#include "modules/gps/wigle.h"
#include "modules/ir/TV-B-Gone.h"
#include "modules/ir/custom_ir.h"
#include "modules/others/audio.h"
#include "modules/others/qrcode_menu.h"
#include "modules/rf/rf_send.h"
#include "mykeyboard.h" // using keyboard when calling rename
#include "passwords.h"
#include "scrollableTextArea.h"
#include <globals.h>

#include <MD5Builder.h>
#include <algorithm> // for std::sort
#include <esp_rom_crc.h>

// SPIClass sdcardSPI;
String fileToCopy;
std::vector<FileList> fileList;

/***************************************************************************************
** Function name: setupSdCard
** Description:   Start SD Card
***************************************************************************************/
bool setupSdCard() {
#ifndef USE_SD_MMC
    if (bruceConfigPins.SDCARD_bus.sck < 0) {
        sdcardMounted = false;
        return false;
    }
#endif
    // avoid unnecessary remounting
    if (sdcardMounted) return true;
    bool result = true;
    bool task = false; // devices that doesn't use InputHandler task
#ifdef USE_TFT_eSPI_TOUCH
    task = true;
#endif
#ifdef USE_SD_MMC
    if (!SD.begin("/sdcard", true)) {
        sdcardMounted = false;
        result = false;
    }
#else
    // Not using InputHandler (SdCard on default &SPI bus)
    if (task) {
        if (!SD.begin((int8_t)bruceConfigPins.SDCARD_bus.cs)) result = false;
        // Serial.println("Task not activated");
    }
    // SDCard in the same Bus as TFT, in this case we call the SPI TFT Instance
    else if (bruceConfigPins.SDCARD_bus.mosi == (gpio_num_t)TFT_MOSI &&
             bruceConfigPins.SDCARD_bus.mosi != GPIO_NUM_NC) {
        Serial.println("SDCard in the same Bus as TFT, using TFT SPI instance");
#if TFT_MOSI > 0 // condition for Headless and 8bit displays (no SPI bus)
        if (!SD.begin(bruceConfigPins.SDCARD_bus.cs, tft.getSPIinstance())) {
            result = false;
            Serial.println("SDCard in the same Bus as TFT, but failed to mount");
        }
#else
        goto NEXT; // destination for Headless and 8bit displays (no SPI bus)
#endif

    }
    // If not using TFT Bus, use a specific bus
    else {
    NEXT:
        sdcardSPI.begin(
            (int8_t)bruceConfigPins.SDCARD_bus.sck,
            (int8_t)bruceConfigPins.SDCARD_bus.miso,
            (int8_t)bruceConfigPins.SDCARD_bus.mosi,
            (int8_t)bruceConfigPins.SDCARD_bus.cs
        ); // start SPI communications
        delay(10);
        if (!SD.begin((int8_t)bruceConfigPins.SDCARD_bus.cs, sdcardSPI)) {
            result = false;
#if defined(ARDUINO_M5STICK_C_PLUS) || defined(ARDUINO_M5STICK_C_PLUS2)
            // If using Shared SPI, do not stop the bus if SDCard is not present
            // If using Legacy, release the pins from this SPI Bus
            if (bruceConfigPins.SDCARD_bus.miso != bruceConfigPins.CC1101_bus.miso) sdcardSPI.end();
#endif
        }
        Serial.println("SDCard in a different Bus, using sdcardSPI instance");
    }
#endif

    if (result == false) {
        Serial.println("SDCARD NOT mounted, check wiring and format");
        sdcardMounted = false;
        return false;
    } else {
        Serial.println("SDCARD mounted successfully");
        sdcardMounted = true;
        return true;
    }
}

/***************************************************************************************
** Function name: closeSdCard
** Description:   Turn Off SDCard, set sdcardMounted state to false
***************************************************************************************/
void closeSdCard() {
    SD.end();
    Serial.println("SD Card Unmounted...");
    sdcardMounted = false;
}

/***************************************************************************************
** Function name: ToggleSDCard
** Description:   Turn Off or On the SDCard, return sdcardMounted state
***************************************************************************************/
bool ToggleSDCard() {
    if (sdcardMounted == true) {
        closeSdCard();
        sdcardMounted = false;
        return false;
    } else {
        sdcardMounted = setupSdCard();
        return sdcardMounted;
    }
}
/***************************************************************************************
** Function name: deleteFromSd
** Description:   delete file or folder
***************************************************************************************/
bool deleteFromSd(FS fs, String path) {
    File dir = fs.open(path);
    Serial.printf("Deleting: %s\n", path.c_str());
    if (!dir.isDirectory()) {
        dir.close();
        return fs.remove(path.c_str());
    }

    dir.rewindDirectory();
    bool success = true;

    bool isDir;
    String fileName = dir.getNextFileName(&isDir);
    while (fileName != "") {
        if (isDir) {
            success &= deleteFromSd(fs, fileName);
        } else {
            success &= fs.remove(fileName.c_str());
        }
        fileName = dir.getNextFileName(&isDir);
    }

    dir.close();
    // Apaga a própria pasta depois de apagar seu conteúdo
    success &= fs.rmdir(path.c_str());
    return success;
}

/***************************************************************************************
** Function name: renameFile
** Description:   rename file or folder
***************************************************************************************/
bool renameFile(FS fs, String path, String filename) {
    String newName = keyboard(filename, 76, "Type the new Name:");
    // Rename the file of folder
    if (fs.rename(path, path.substring(0, path.lastIndexOf('/')) + "/" + newName)) {
        // Serial.println("Renamed from " + filename + " to " + newName);
        return true;
    } else {
        // Serial.println("Fail on rename.");
        return false;
    }
}
/***************************************************************************************
** Function name: copyToFs
** Description:   copy file from SD or LittleFS to LittleFS or SD
***************************************************************************************/
bool copyToFs(FS from, FS to, String path, bool draw) {
    // Using Global Buffer
    bool result = false;
    if (!sdcardMounted) {
        if (!setupSdCard()) {
            sdcardMounted = false;
            Serial.println("SD Card not mounted");
            return false;
        }
    }

    if (!LittleFS.begin()) {
        Serial.println("LittleFS not mounted");
        return false;
    }

    File source = from.open(path, FILE_READ);
    if (!source) {
        Serial.println("Fail opening Source file");
        return false;
    }
    path = path.substring(path.lastIndexOf('/'));
    if (!path.startsWith("/")) path = "/" + path;
    File dest = to.open(path, FILE_WRITE);
    if (!dest) {
        Serial.println("Fail creating destination file");
        return false;
    }
    size_t bytesRead;
    int tot = source.size();
    int prog = 0;

    if (&to == &LittleFS && (LittleFS.totalBytes() - LittleFS.usedBytes()) < tot) {
        displayError("Not enought space", true);
        return false;
    }
    const int bufSize = 1024;
    static uint8_t buff[bufSize] = {0}; // static to keep this buffer off the task stack
    // tft.drawRect(5,tftHeight-12, (tftWidth-10), 9, bruceConfig.priColor);
    while ((bytesRead = source.read(buff, bufSize)) > 0) {
        if (dest.write(buff, bytesRead) != bytesRead) {
            // Serial.println("Falha ao escrever no arquivo de destino");
            source.close();
            dest.close();
            Serial.println("Error 5");
            return false;
        } else {
            prog += bytesRead;
            float rad = 360 * prog / tot;
            if (draw)
                tft.drawArc(
                    tftWidth / 2,
                    tftHeight / 2,
                    tftHeight / 4,
                    tftHeight / 5,
                    0,
                    int(rad),
                    ALCOLOR,
                    bruceConfig.bgColor,
                    true
                );
        }
    }
    if (prog == tot) result = true;
    else {
        displayError("Fail Copying File", true);
        return false;
    }

    return result;
}

/***************************************************************************************
** Function name: copyFile
** Description:   copy file address to memory
***************************************************************************************/
bool copyFile(FS fs, String path) {
    File file = fs.open(path, FILE_READ);
    if (!file.isDirectory()) {
        fileToCopy = path;
        file.close();
        return true;
    } else {
        displayRedStripe("Cannot copy Folder");
        file.close();
        return false;
    }
}

/***************************************************************************************
** Function name: pasteFile
** Description:   paste file to new folder
***************************************************************************************/
bool pasteFile(FS fs, String path) {
    // Using Global Buffer

    // Abrir o arquivo original
    File sourceFile = fs.open(fileToCopy, FILE_READ);
    if (!sourceFile) {
        // Serial.println("Falha ao abrir o arquivo original para leitura");
        return false;
    }

    // Criar o arquivo de destino
    File destFile = fs.open(path + "/" + fileToCopy.substring(fileToCopy.lastIndexOf('/') + 1), FILE_WRITE);
    if (!destFile) {
        // Serial.println("Falha ao criar o arquivo de destino");
        sourceFile.close();
        return false;
    }

    // Ler dados do arquivo original e escrever no arquivo de destino
    size_t bytesRead;
    int tot = sourceFile.size();
    int prog = 0;
    const int bufSize = 1024;
    uint8_t buff[1024] = {0};
    // tft.drawRect(5,tftHeight-12, (tftWidth-10), 9, bruceConfig.priColor);
    while ((bytesRead = sourceFile.read(buff, bufSize)) > 0) {
        if (destFile.write(buff, bytesRead) != bytesRead) {
            // Serial.println("Falha ao escrever no arquivo de destino");
            sourceFile.close();
            destFile.close();
            return false;
        } else {
            prog += bytesRead;
            float rad = 360 * prog / tot;
            tft.drawArc(
                tftWidth / 2,
                tftHeight / 2,
                tftHeight / 4,
                tftHeight / 5,
                0,
                int(rad),
                ALCOLOR,
                bruceConfig.bgColor,
                true
            );
            // tft.fillRect(7,tftHeight-10, (tftWidth-14)*prog/tot, 5, bruceConfig.priColor);
        }
    }

    // Fechar ambos os arquivos
    sourceFile.close();
    destFile.close();
    return true;
}

/***************************************************************************************
** Function name: createFolder
** Description:   create new folder
***************************************************************************************/
bool createFolder(FS fs, String path) {
    String foldername = keyboard("", 76, "Folder Name: ");
    if (!fs.mkdir(path + "/" + foldername)) {
        displayRedStripe("Couldn't create folder");
        return false;
    }
    return true;
}

/**********************************************************************
**  Function: readLineFromFile
**  Read the line of the config file until the ';'
**********************************************************************/
String readLineFromFile(File myFile) {
    String line = "";
    char character;

    while (myFile.available()) {
        character = myFile.read();
        if (character == ';') { break; }
        line += character;
    }
    return line;
}

/***************************************************************************************
** Function name: readSmallFile
** Description:   read a small (<3KB) file and return its contents as a single string
**                on any error returns an empty string
***************************************************************************************/
String readSmallFile(FS &fs, String filepath) {
    String fileContent = "";
    File file;

    file = fs.open(filepath, FILE_READ);
    if (!file) return "";

    size_t fileSize = file.size();
    if (fileSize > SAFE_STACK_BUFFER_SIZE || fileSize > ESP.getFreeHeap()) {
        displayError("File is too big", true);
        return "";
    }
    // TODO: if(psramFound()) -> use PSRAM instead

    fileContent = file.readString();

    file.close();
    return fileContent;
}

/***************************************************************************************
** Function name: readFile
** Description:   read file and return its contents as a char*
**                caller needs to call free()
***************************************************************************************/
char *readBigFile(FS &fs, String filepath, bool binary, size_t *fileSize) {
    File file = fs.open(filepath);
    if (!file) {
        Serial.printf("Could not open file: %s\n", filepath.c_str());
        return NULL;
    }

    size_t fileLen = file.size();
    char *buf = (char *)(psramFound() ? ps_malloc(fileLen + 1) : malloc(fileLen + 1));
    if (fileSize != NULL) { *fileSize = file.size(); }

    if (!buf) {
        Serial.printf("Could not allocate memory for file: %s\n", filepath.c_str());
        return NULL;
    }

    size_t bytesRead = 0;
    while (bytesRead < fileLen && file.available()) {
        size_t toRead = fileLen - bytesRead;
        if (toRead > 512) { toRead = 512; }
        file.read((uint8_t *)(buf + bytesRead), toRead);
        bytesRead += toRead;
    }
    buf[bytesRead] = '\0';
    file.close();

    return buf;
}

/***************************************************************************************
** Function name: getFileSize
** Description:   get a file size without opening
***************************************************************************************/
size_t getFileSize(FS &fs, String filepath) {
    File file = fs.open(filepath, FILE_READ);
    if (!file) return 0;
    size_t fileSize = file.size();
    file.close();
    return fileSize;
}

String md5File(FS &fs, String filepath) {
    if (!fs.exists(filepath)) return "";
    String txt = readSmallFile(fs, filepath);
    MD5Builder md5;
    md5.begin();
    md5.add(txt);
    md5.calculate();
    return (md5.toString());
}

String crc32File(FS &fs, String filepath) {
    if (!fs.exists(filepath)) return "";
    String txt = readSmallFile(fs, filepath);
    // derived from
    // https://techoverflow.net/2022/08/05/how-to-compute-crc32-with-ethernet-polynomial-0x04c11db7-on-esp32-crc-h/
    uint32_t romCRC =
        (~esp_rom_crc32_le((uint32_t)~(0xffffffff), (const uint8_t *)txt.c_str(), txt.length())) ^ 0xffffffff;

    char s[18] = {0};
    char crcBytes[4] = {0};
    memcpy(crcBytes, &romCRC, sizeof(uint32_t));
    snprintf(s, sizeof(s), "%02X%02X%02X%02X\n", crcBytes[3], crcBytes[2], crcBytes[1], crcBytes[0]);
    return (String(s));
}

/***************************************************************************************
** Function name: sortList
** Description:   sort files/folders by name
***************************************************************************************/
bool sortList(const FileList &a, const FileList &b) {
    if (a.folder != b.folder) {
        return a.folder > b.folder; // true if a is a folder and b is not
    }
    // Order items alphabetically
    String fa = a.filename.c_str();
    fa.toUpperCase();
    String fb = b.filename.c_str();
    fb.toUpperCase();
    return fa < fb;
}

/***************************************************************************************
** Function name: checkExt
** Description:   check file extension
***************************************************************************************/
bool checkExt(String ext, String pattern) {
    ext.toUpperCase();
    pattern.toUpperCase();
    if (ext == pattern) return true;

    // If the pattern is a list of extensions (e.g., "TXT|JPG|PNG"), split and check
    int start = 0;
    int end = pattern.indexOf('|');
    while (end != -1) {
        String currentExt = pattern.substring(start, end);
        if (ext == currentExt) { return true; }
        start = end + 1;
        end = pattern.indexOf('|', start);
    }

    // Check the last extension in the list
    String lastExt = pattern.substring(start);
    return ext == lastExt;
}

/***************************************************************************************
** Function name: readFs
** Description:   read files/folders from a folder
***************************************************************************************/
void readFs(FS fs, String folder, String allowed_ext) {
    int allFilesCount = 0;
    fileList.clear();
    FileList object;

    File root = fs.open(folder);
    if (!root || !root.isDirectory()) { return; }

    while (true) {
        bool isDir;
        String fullPath = root.getNextFileName(&isDir);
        String nameOnly = fullPath.substring(fullPath.lastIndexOf("/") + 1);
        if (fullPath == "") { break; }
        // Serial.printf("Path: %s (isDir: %d)\n", fullPath.c_str(), isDir);

        if (isDir) {
            object.filename = nameOnly;
            object.folder = true;
            object.operation = false;
            fileList.push_back(object);
        } else {
            int dotIndex = nameOnly.lastIndexOf(".");
            String ext = dotIndex >= 0 ? nameOnly.substring(dotIndex + 1) : "";
            if (allowed_ext == "*" || checkExt(ext, allowed_ext)) {
                object.filename = nameOnly;
                object.folder = false;
                object.operation = false;
                fileList.push_back(object);
            }
        }
    }
    root.close();

    // Sort folders/files
    std::sort(fileList.begin(), fileList.end(), sortList);

    Serial.println("Files listed with: " + String(fileList.size()) + " files/folders found");

    // Adds Operational btn at the botton
    object.filename = "> Back";
    object.folder = false;
    object.operation = true;

    fileList.push_back(object);
}

/*********************************************************************
**  Function: loopSD
**  Where you choose what to do with your SD Files
**********************************************************************/
String loopSD(FS &fs, bool filePicker, String allowed_ext, String rootPath) {
    delay(10);
    if (!fs.exists(rootPath)) {
        Serial.println("loopSD-> 1st exist test failed");
        rootPath = "/";
        if (!fs.exists(rootPath)) {
            Serial.println("loopSD-> 2nd exist test failed");
            if (&fs == &SD) sdcardMounted = false;
            return "";
        }
    }

    Opt_Coord coord;
    String result = "";
    const short PAGE_JUMP_SIZE = (tftHeight - 20) / (LH * FM);
    bool reload = false;
    bool redraw = true;
    int index = 0;
    int maxFiles = 0;
    String Folder = rootPath;
    String PreFolder = rootPath;
    tft.drawPixel(0, 0, 0);
    tft.fillScreen(bruceConfig.bgColor); // TODO: Does only the T-Embed CC1101 need this?
    tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
    if (&fs == &SD) {
        if (!setupSdCard()) {
            displayError("Fail Mounting SD", true);
            return "";
        }
    }
    bool exit = false;
    // returnToMenu=true;  // make sure menu is redrawn when quitting in any point

    readFs(fs, Folder, allowed_ext);

    maxFiles = fileList.size() - 1; // discount the >back operator
    LongPress = false;
    unsigned long LongPressTmp = millis();
    while (1) {
        delay(10);
        // if(returnToMenu) break; // stop this loop and retur to the previous loop
        if (exit) break; // stop this loop and retur to the previous loop

        if (redraw) {
            if (strcmp(PreFolder.c_str(), Folder.c_str()) != 0 || reload) {
                index = 0;
                tft.fillScreen(bruceConfig.bgColor);
                tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                Serial.println("reload to read: " + Folder);
                readFs(fs, Folder, allowed_ext);
                PreFolder = Folder;
                maxFiles = fileList.size() - 1;
                if (strcmp(PreFolder.c_str(), Folder.c_str()) != 0 || index > maxFiles) index = 0;
                reload = false;
            }
            if (fileList.size() < 2) readFs(fs, Folder, allowed_ext);

            coord = listFiles(index, fileList);
#if defined(HAS_TOUCH)
            TouchFooter();
#endif
            redraw = false;
        }
        displayScrollingText(fileList[index].filename, coord);

        // !PrevPress enables EscPress on 3Btn devices to be used in Serial Navigation
        // This condition is important for StickCPlus, Core and other 3 Btn devices
        if (EscPress && PrevPress) EscPress = false;
        char pressed_letter;
        if (check(EscPress)) goto BACK_FOLDER;

#ifdef HAS_KEYBOARD
        pressed_letter = checkLetterShortcutPress();

        // check letter shortcuts
        if (pressed_letter > 0) {
            // Serial.println(pressed_letter);
            if (tolower(fileList[index].filename.c_str()[0]) == pressed_letter) {
                // already selected, go to the next
                index += 1;
                // check if index is still valid
                if (index <= maxFiles && tolower(fileList[index].filename.c_str()[0]) == pressed_letter) {
                    redraw = true;
                    continue;
                }
            }
            // else look again from the start
            for (int i = 0; i < maxFiles; i++) {
                if (tolower(fileList[i].filename.c_str()[0]) == pressed_letter) { // check if 1st char matches
                    index = i;
                    redraw = true;
                    break; // quit on 1st match
                }
            }
        }
#endif

        if (check(PrevPress) || check(UpPress)) {
            if (index == 0) index = maxFiles;
            else if (index > 0) index--;
            redraw = true;
        }
        /* DW Btn to next item */
        if (check(NextPress) || check(DownPress)) {
            if (index == maxFiles) index = 0;
            else index++;
            redraw = true;
        }
        if (check(NextPagePress)) {
            index += PAGE_JUMP_SIZE;
            if (index > maxFiles) index = maxFiles - 1; // check bounds
            redraw = true;
            continue;
        }
        if (check(PrevPagePress)) {
            index -= PAGE_JUMP_SIZE;
            if (index < 0) index = 0; // check bounds
            redraw = true;
            continue;
        }
        /* Select to install */
        if (LongPress || SelPress) {
            if (!LongPress) {
                LongPress = true;
                LongPressTmp = millis();
            }
            if (LongPress && millis() - LongPressTmp < 500) goto WAITING;
            LongPress = false;

            if (check(SelPress)) {
                if (fileList[index].folder == true && fileList[index].operation == false) {
                    options = {
                        {"New Folder", [=]() { createFolder(fs, Folder); }                                 },
                        {"Rename",
                         [=]() {
                             renameFile(fs, Folder + fileList[index].filename, fileList[index].filename);
                         }                                                                                 },
                        {"Delete",     [=]() { deleteFromSd(fs, Folder + "/" + fileList[index].filename); }},
                        {"Close Menu", [&]() { yield(); }                                                  },
                        {"Main Menu",  [&]() { exit = true; }                                              },
                    };
                    loopOptions(options);
                    tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                    reload = true;
                    redraw = true;
                } else if (fileList[index].folder == false && fileList[index].operation == false) {
                    goto Files;
                } else {
                    options = {
                        {"New Folder", [=]() { createFolder(fs, Folder); }},
                    };
                    if (fileToCopy != "") options.push_back({"Paste", [=]() { pasteFile(fs, Folder); }});
                    options.push_back({"Close Menu", [&]() { yield(); }});
                    options.push_back({"Main Menu", [&]() { exit = true; }});
                    loopOptions(options);
                    tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                    reload = true;
                    redraw = true;
                }
            } else {
            Files:
                if (fileList[index].folder == true && fileList[index].operation == false) {
                    Folder = Folder + (Folder == "/" ? "" : "/") +
                             fileList[index].filename; // Folder=="/"? "":"/" +
                    // Debug viewer
                    Serial.println(Folder);
                    redraw = true;
                } else if (fileList[index].folder == false && fileList[index].operation == false) {
                    // Save the file/folder info to Clear memory to allow other functions to work better
                    String filepath = Folder + (Folder == "/" ? "" : "/") + fileList[index].filename; //
                    String filename = fileList[index].filename;
                    // Debug viewer
                    Serial.println(filepath + " --> " + filename);
                    fileList.clear(); // Clear memory to allow other functions to work better

                    options = {
                        {"View File",  [=]() { viewFile(fs, filepath); }            },
                        {"File Info",  [=]() { fileInfo(fs, filepath); }            },
                        {"Rename",     [=]() { renameFile(fs, filepath, filename); }},
                        {"Copy",       [=]() { copyFile(fs, filepath); }            },
                        {"Delete",     [=]() { deleteFromSd(fs, filepath); }        },
                        {"New Folder", [=]() { createFolder(fs, Folder); }          },
                    };
                    if (fileToCopy != "") options.push_back({"Paste", [=]() { pasteFile(fs, Folder); }});
                    if (&fs == &SD)
                        options.push_back({"Copy->LittleFS", [=]() { copyToFs(SD, LittleFS, filepath); }});
                    if (&fs == &LittleFS && sdcardMounted)
                        options.push_back({"Copy->SD", [=]() { copyToFs(LittleFS, SD, filepath); }});

                    // custom file formats commands added in front
                    if (filepath.endsWith(".jpg") || filepath.endsWith(".gif") || filepath.endsWith(".bmp") ||
                        filepath.endsWith(".png"))
                        options.insert(options.begin(), {"View Image", [&]() {
                                                             drawImg(fs, filepath, 0, 0, true, -1);
                                                             delay(750);
                                                             while (!check(AnyKeyPress))
                                                                 vTaskDelay(10 / portTICK_PERIOD_MS);
                                                         }});
					if (filepath.endsWith(".ir")) {
                        options.insert(options.begin(), {"IR Choose cmd", [&]() {
                                                             delay(200);
                                                             chooseCmdIrFile(&fs, filepath);
                                                         }});
                        options.insert(options.begin(), {"IR Tx SpamAll", [&]() {
                                                             delay(200);
                                                             txIrFile(&fs, filepath);
                                                         }});
					}
                    if (filepath.endsWith(".sub"))
                        options.insert(options.begin(), {"Subghz Tx", [&]() {
                                                             delay(200);
                                                             txSubFile(&fs, filepath);
                                                         }});
                    if (filepath.endsWith(".csv")) {
                        options.insert(options.begin(), {"Wigle Upload", [&]() {
                                                             delay(200);
                                                             Wigle wigle;
                                                             wigle.upload(&fs, filepath);
                                                         }});
                        options.insert(options.begin(), {"Wigle Up All", [&]() {
                                                             delay(200);
                                                             Wigle wigle;
                                                             wigle.upload_all(&fs, Folder);
                                                         }});
                    }
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
                    if (filepath.endsWith(".bjs") || filepath.endsWith(".js")) {
                        options.insert(options.begin(), {"JS Script Run", [&]() {
                                                             delay(200);
                                                             run_bjs_script_headless(fs, filepath);
                                                             exit = true;
                                                         }});
                    }
#endif
#if defined(USB_as_HID)
                    if (filepath.endsWith(".txt")) {
                        options.push_back({"BadUSB Run", [&]() {
                                               ducky_startKb(hid_usb, false);
                                               key_input(fs, filepath, hid_usb);
                                               delete hid_usb;
                                               hid_usb = nullptr;
                                               // TODO: reinit serial port
                                           }});
                        options.push_back({"USB HID Type", [&]() {
                                               String t = readSmallFile(fs, filepath);
                                               displayRedStripe("Typing");
                                               key_input_from_string(t);
                                           }});
                    }
                    if (filepath.endsWith(".enc")) { // encrypted files
                        options.insert(
                            options.begin(), {"Decrypt+Type", [&]() {
                                                  String plaintext = readDecryptedFile(fs, filepath);
                                                  if (plaintext.length() == 0)
                                                      return displayError(
                                                          "Decryption failed", true
                                                      ); // file is too big or cannot read, or cancelled
                                                  // else
                                                  plaintext.trim(); // remove newlines
                                                  key_input_from_string(plaintext);
                                              }}
                        );
                    }
#endif
                    if (filepath.endsWith(".enc")) { // encrypted files
                        options.insert(
                            options.begin(), {"Decrypt+Show", [&]() {
                                                  String plaintext = readDecryptedFile(fs, filepath);
                                                  delay(200);
                                                  if (plaintext.length() == 0)
                                                      return displayError("Decryption failed", true);
                                                  plaintext.trim(); // remove newlines
                                                                    // if(plaintext.length()<..)
                                                  displaySuccess(plaintext, true);
                                                  // else
                                                  // TODO: show in the text viewer
                                              }}
                        );
                    }
#if defined(HAS_NS4168_SPKR)
                    if (isAudioFile(filepath))
                        options.insert(options.begin(), {"Play Audio", [&]() {
                                                             delay(200);
                                                             check(AnyKeyPress);
                                                             playAudioFile(&fs, filepath);
                                                         }});
#endif
                    // generate qr codes from small files (<3K)
                    size_t filesize = getFileSize(fs, filepath);
                    // Serial.println(filesize);
                    if (filesize < SAFE_STACK_BUFFER_SIZE && filesize > 0) {
                        options.push_back({"QR code", [&]() {
                                               delay(200);
                                               qrcode_display(readSmallFile(fs, filepath));
                                           }});
                        options.push_back({"CRC32", [&]() {
                                               delay(200);
                                               displaySuccess(crc32File(fs, filepath), true);
                                           }});
                        options.push_back({"MD5", [&]() {
                                               delay(200);
                                               displaySuccess(md5File(fs, filepath), true);
                                           }});
                    }
                    options.push_back({"Close Menu", [&]() { yield(); }});
                    options.push_back({"Main Menu", [&]() { exit = true; }});
                    if (!filePicker) loopOptions(options);
                    else {
                        result = filepath;
                        break;
                    }
                    tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
                    reload = true;
                    redraw = true;
                } else {
                BACK_FOLDER:
                    if (Folder == "/") break;
                    Folder = Folder.substring(0, Folder.lastIndexOf('/'));
                    if (Folder == "") Folder = "/";
                    Serial.println("Going to folder: " + Folder);
                    index = 0;
                    redraw = true;
                }
                redraw = true;
            }
        WAITING:
            delay(10);
        }
    }
    fileList.clear();
    return result;
}

/*********************************************************************
**  Function: viewFile
**  Display file content
**********************************************************************/
void viewFile(FS fs, String filepath) {
    File file = fs.open(filepath, FILE_READ);
    if (!file) return;

    ScrollableTextArea area = ScrollableTextArea("VIEW FILE");
    area.fromFile(file);

    file.close();

    area.show();
}

/*********************************************************************
**  Function: checkLittleFsSize
**  Check if there are more then 4096 bytes available for storage
**********************************************************************/
bool checkLittleFsSize() {
    if ((LittleFS.totalBytes() - LittleFS.usedBytes()) < 4096) {
        displayError("LittleFS is Full", true);
        return false;
    } else return true;
}

/*********************************************************************
**  Function: checkLittleFsSize
**  Check if there are more then 4096 bytes available for storage
**********************************************************************/
bool checkLittleFsSizeNM() { return (LittleFS.totalBytes() - LittleFS.usedBytes()) >= 4096; }

/*********************************************************************
**  Function: getFsStorage
**  Function will return true and FS will point to SDFS if available
**  and LittleFS otherwise. If LittleFS is full it wil return false.
**********************************************************************/
bool getFsStorage(FS *&fs) {
    // don't try to mount SD Card if not previously mounted
    if (sdcardMounted) fs = &SD;
    else if (checkLittleFsSize()) fs = &LittleFS;
    else return false;

    return true;
}

/*********************************************************************
**  Function: fileInfo
**  Display file info
**********************************************************************/
void fileInfo(FS fs, String filepath) {
    File file = fs.open(filepath, FILE_READ);
    if (!file) return;

    int bytesize = file.size();
    float filesize = bytesize;
    String unit = "B";

    time_t modifiedTime = file.getLastWrite();

    if (filesize >= 1000000) {
        filesize /= 1000000.0;
        unit = "MB";
    } else if (filesize >= 1000) {
        filesize /= 1000.0;
        unit = "kB";
    }

    drawMainBorderWithTitle("FILE INFO");
    padprintln("");
    padprintln("Path: " + filepath);
    padprintln("");
    padprintf("Bytes: %d\n", bytesize);
    padprintln("");
    padprintf("Size: %.02f %s\n", filesize, unit.c_str());
    padprintln("");
    padprintf("Modified: %s\n", ctime(&modifiedTime));

    file.close();
    delay(100);

    while (!check(EscPress) && !check(SelPress)) { delay(100); }

    return;
}

/*********************************************************************
**  Function: createNewFile
**  Function will save a file into FS. If file already exists it will
**  append a version number to the file name.
**********************************************************************/
File createNewFile(FS *&fs, String filepath, String filename) {
    int extIndex = filename.lastIndexOf('.');
    String name = filename.substring(0, extIndex);
    String ext = filename.substring(extIndex);

    if (filepath.endsWith("/")) filepath = filepath.substring(0, filepath.length() - 1);
    if (!(*fs).exists(filepath)) (*fs).mkdir(filepath);

    name = filepath + "/" + name;

    if ((*fs).exists(name + ext)) {
        int i = 1;
        name += "_";
        while ((*fs).exists(name + String(i) + ext)) i++;
        name += String(i);
    }

    Serial.println("Creating file: " + name + ext);
    File file = (*fs).open(name + ext, FILE_WRITE);
    return file;
}
