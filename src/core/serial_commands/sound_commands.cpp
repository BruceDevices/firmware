#include "sound_commands.h"
#include "core/sd_functions.h"
#include "modules/others/audio.h"
#include <globals.h>

uint32_t toneCallback(cmd *c) {
    Command cmd(c);

    Argument freqArg = cmd.getArgument("frequency");
    Argument durArg = cmd.getArgument("duration");
    String strFreq = freqArg.getValue();
    String strDur = durArg.getValue();
    strFreq.trim();
    strDur.trim();

    bool soundEnabled = bruceConfig.soundEnabled;
    bruceConfig.soundEnabled = true;

    unsigned long frequency = std::stoul(strFreq.c_str());
    unsigned long duration = std::stoul(strDur.c_str());

    serialDevice->println((int)500UL);
    serialDevice->println((int)frequency);
    serialDevice->println((int)duration);

    _tone(frequency, duration);

    bruceConfig.soundEnabled = soundEnabled;
    return true;
}

uint32_t playCallback(cmd *c) {
    // RTTTL player
    // music_player
    // mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6

    // File player
    // music_player boot.wav

    Command cmd(c);

    Argument arg = cmd.getArgument("song");
    String song = arg.getValue();
    song.trim();

    bool soundEnabled = bruceConfig.soundEnabled;
    bruceConfig.soundEnabled = true;

    if (song.indexOf(":") != -1) return playAudioRTTTLString(song);

    if (song.indexOf(".") != -1) {
        if (!song.startsWith("/")) song = "/" + song;

        FS *fs;
        if (!getFsStorage(fs)) return false;

        if (!(*fs).exists(song)) {
            serialDevice->println("Song file does not exist");
            return false;
        }

        return playAudioFile(fs, song);
    }

    bruceConfig.soundEnabled = soundEnabled;
    return false;
}

uint32_t ttsCallback(cmd *c) {
    // tts hello world

    Command cmd(c);

    Argument arg = cmd.getArgument(0);
    String text = arg.getValue();
    text.trim();

    bool soundEnabled = bruceConfig.soundEnabled;
    bruceConfig.soundEnabled = true;

    bool r = tts(text);

    bruceConfig.soundEnabled = soundEnabled;
    return r;
}

void createSoundCommands(SimpleCLI *cli) {
    Command toneCmd = cli->addCommand("tone,beep", toneCallback);
    toneCmd.addPosArg("frequency", "500UL");
    toneCmd.addPosArg("duration", "500UL");

#ifdef HAS_SPEAKER
    Command playCmd = cli->addCommand("play,music_player", playCallback);
    playCmd.addPosArg("song");

    Command ttsCmd = cli->addSingleArgCmd("tts,say", ttsCallback);
#endif

    // TODO: webradio
    // https://github.com/earlephilhower/ESP8266Audio/tree/master/examples/WebRadio
}
