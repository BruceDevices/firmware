#ifndef __SERIAL_CMDS_H__
#define __SERIAL_CMDS_H__

#include <Arduino.h>

extern TaskHandle_t serialcmdsTaskHandle;

void startSerialCommandsHandlerTask(bool initQueues = false);

bool parseSerialCommand(const String &command, bool waitForResponse = true);
#endif
