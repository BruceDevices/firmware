#ifndef __CLIENTS_H__
#define __CLIENTS_H__

#include <WiFi.h>

void telnet_setup();

void ssh_setup(const String &host = "");

void ssh_setup_profile(const String &host, const String &port, const String &user, const String &pwd);

void ssh_connection_menu();

void ssh_loop(void *pvParameters);

char *stringTochar(const String &s);

#endif
