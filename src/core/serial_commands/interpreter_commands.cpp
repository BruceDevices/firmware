#include "interpreter_commands.h"
#include "core/sd_functions.h"
#include "helpers.h"
#include "modules/bjs_interpreter/interpreter.h"

uint32_t jsFileCallback(cmd *c) {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    Command cmd(c);

    Argument arg = cmd.getArgument("filepath");
    String filepath = arg.getValue();
    filepath.trim();

    if (!filepath.startsWith("/")) filepath = "/" + filepath;

    FS *fs;
    if (!getFsStorage(fs)) return false;

    run_bjs_script_headless(*fs, filepath);

#endif
    return true;
}

uint32_t jsBufferCallback(cmd *c) {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

    Command cmd(c);

    Argument arg = cmd.getArgument("fileSize");
    String strFileSize = arg.getValue();
    strFileSize.trim();

    int fileSize = strFileSize.toInt() + 2;
    char *txt = _readFileFromSerial(fileSize);

    bool r = run_bjs_script_headless(txt);
    return r;
#else
    return true;
#endif
}

void createInterpreterCommands(SimpleCLI *cli) {
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
    Command jsCmd = cli->addCompositeCmd("js,run,interpret/er");

    Command fileCmd = jsCmd.addCommand(
        "run_from_file", jsFileCallback
    ); // TODO: remove "run_from_file" for flipper0-compatiblity
       // https://docs.flipper.net/development/cli/#GjMyY
    fileCmd.addPosArg("filepath");

    Command bufferCmd = jsCmd.addCommand("run_from_buffer", jsBufferCallback);
    bufferCmd.addPosArg("fileSize", "0"); // optional arg
#endif
}
