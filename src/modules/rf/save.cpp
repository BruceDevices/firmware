#include "save.h"
bool rf_raw_save(RawRecording recorded) {
    FS *fs = nullptr;
    if (!getFsStorage(fs) || fs == nullptr) {
        displayError("No space left on device, Insert SD Card", true);
        return false;
    }

    char filename[32];
    int index = 0;

    if (!fs->exists("/BruceRF")) {
        if (!fs->mkdir("/BruceRF")) {
            displayError("Error creating directory", true);
            return false;
        }
    }

    do { snprintf(filename, sizeof(filename), "/BruceRF/raw_%d.sub", index++); } while (fs->exists(filename));

    File file = fs->open(filename, FILE_WRITE, true);
    if (!file) {
        displayError("Error creating file", true);
        return false;
    }

    file.write((const uint8_t *)"Filetype: Bruce SubGhz File\n", 28);
    file.write((const uint8_t *)"Version 1\n", 10);

    char line[64];
    int len = snprintf(line, sizeof(line), "Frequency: %d\n", (int)(recorded.frequency * 1000000));
    file.write((const uint8_t *)line, len);

    file.write((const uint8_t *)"Preset: 0\n", 10);
    file.write((const uint8_t *)"Protocol: RAW\n", 14);
    file.write((const uint8_t *)"RAW_Data: ", 10);

    uint16_t values = 0;
    for (size_t i = 0; i < recorded.codes.size(); ++i) {
        size_t count = recorded.codeLengths[i];
        for (size_t j = 0; j < count; ++j) {
            // RAW_Data must keep maximun 512 values per line
            // https://github.com/flipperdevices/flipperzero-firmware/blob/dev/documentation/file_formats/SubGhzFileFormats.md#raw-files
            // check after each addition
            auto &code = recorded.codes[i][j];
            if (code.duration0 > 0) {
                if (code.level0 != 1) file.write((const uint8_t *)"-", 1);
                len = snprintf(line, sizeof(line), "%d ", code.duration0);
                file.write((const uint8_t *)line, len);
                values++;
                if (values % 512 == 0) file.write((const uint8_t *)"\nRAW_Data: ", 11);
            }
            if (code.duration1 > 0) {
                if (code.level1 != 1) file.write((const uint8_t *)"-", 1);
                len = snprintf(line, sizeof(line), "%d ", code.duration1);
                file.write((const uint8_t *)line, len);
                values++;
                if (values % 512 == 0) file.write((const uint8_t *)"\nRAW_Data: ", 11);
            }
        }

        if (i < recorded.codes.size() - 1) {
            len = snprintf(line, sizeof(line), "%d ", (int)(recorded.gaps[i] * -1000));
            file.write((const uint8_t *)line, len);
            values++;
            if (values % 512 == 0) file.write((const uint8_t *)"\nRAW_Data: ", 11);
        }

        file.flush();
    }

    file.close();
    displaySuccess(filename, true);
    return true;
}
