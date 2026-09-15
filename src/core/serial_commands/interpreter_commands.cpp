#include "interpreter_commands.h"
#include "core/sd_functions.h"
#include "helpers.h"
#include "modules/bjs_interpreter/interpreter.h"

static uint32_t jsCallback(cmd *c) {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    Command cmd(c);

    const int argc = cmd.countArgs();
    if (argc <= 0) return true;

    String first = cmd.getArgument(0).getValue();
    first.trim();
    if (first.length() == 0) return true;

    if (first == "exit") {
        interpreter_state = -1;
        return true;
    }

    if (first == "run_from_buffer") {
        int fileSize = 0;
        if (argc >= 2) {
            String strFileSize = cmd.getArgument(1).getValue();
            strFileSize.trim();
            fileSize = strFileSize.toInt();
        }

        char *txt = _readFileFromSerial(fileSize + 2);
        return run_bjs_script_headless(txt);
    }

    String filepath;
    if (first == "run_from_file") {
        if (argc < 2) return false;

        filepath = cmd.getArgument(1).getValue();
    } else {
        filepath = first;
    }

    // Fallback: `js <path>` runs the script file.
    filepath.trim();
    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;

    if (sdcardMounted && SD.exists(filepath)) {
        fs = &SD;
    } else {
        fs = &LittleFS;
    }

    run_bjs_script_headless(*fs, filepath);
    return true;
#else
    return true;
#endif
}

void createInterpreterCommands(SimpleCLI *cli) {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    // Boundless so we can support both:
    // - subcommands: `js exit`, `js run_from_file <path>`, `js run_from_buffer <size>`
    // - fallback: `js <path>` for flipper0-compatiblity https://docs.flipper.net/development/cli/#GjMyY
    cli->addBoundlessCmd("js,run,interpret/er", jsCallback);
#endif
}
