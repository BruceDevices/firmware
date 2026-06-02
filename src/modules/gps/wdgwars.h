/**
 * @file wdgwars.h
 * @brief WDGoWars connector — uploads wardriving CSV to wdgwars.pl
 * @version 0.1
 */

#ifndef __WDGWARS_H__
#define __WDGWARS_H__

#include <WiFiClientSecure.h>
#include <globals.h>

class WDGoWars {
public:
    WDGoWars();
    ~WDGoWars();

    bool upload(FS *fs, String filepath, bool auto_delete = true);
    bool upload_all(FS *fs, String folder, bool auto_delete = true);

private:
    const char *host = "wdgwars.pl";
    const char *upload_path = "/api/upload-csv";

    bool _check_api_key(void);
    bool _upload_file(File file, String upload_message);
    void _send_upload_headers(WiFiClientSecure &client, String filename, int filesize, String boundary);
    void _display_banner(void);
};

#endif
