#ifndef QR_CODE_MENU_H
#define QR_CODE_MENU_H

#include <Arduino.h>

void qrcode_display(const String &qrcodeUrl);
void pix_qrcode();
void qrcode_menu();
void custom_qrcode_menu();
void display_custom_qrcode();
void save_and_display_qrcode();
void remove_custom_qrcode();

#endif
