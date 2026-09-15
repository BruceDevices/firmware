/**
 * @file wigle.h
 * @author Rennan Cockles - https://github.com/rennancockles
 * @brief Wigle connector
 * @version 0.1
 */

#ifndef __WIGLE_H__
#define __WIGLE_H__

#include <WiFiClientSecure.h>
#include <globals.h>

class Wigle {
public:
    /////////////////////////////////////////////////////////////////////////////////////
    // Constructor
    /////////////////////////////////////////////////////////////////////////////////////
    Wigle();
    ~Wigle();

    /////////////////////////////////////////////////////////////////////////////////////
    // Operations
    /////////////////////////////////////////////////////////////////////////////////////
    bool get_user(void);
    bool upload(FS *fs, const String &filepath, bool auto_delete = true);
    bool upload_all(FS *fs, const String &filepath, bool auto_delete = true);
    void send_upload_headers(
        WiFiClientSecure &client, const String &filename, int filesize, const String &boundary
    );
    void display_banner(void);
    void dump_wigle_info(void);

private:
    String wigle_user;
    String auth_header;
    const char *host = "api.wigle.net";

    bool _check_token(void);
    bool _upload_file(File file, const String &upload_message);
};

#endif
