#ifndef __IR_UTILS_H
#define __IR_UTILS_H
#include <FS.h>
#include <globals.h>

void setup_ir_pin(int pin, uint8_t mode);
String pickDirectory(FS &fs, String rootPath = "/");
void loadScFile(FS *fs, String filepath, bool &exitAll);

#endif
