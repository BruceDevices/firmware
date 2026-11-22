import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / 'src/core'
ARDUINOJSON = ROOT / '.pio/libdeps/lilygo-t-embed-cc1101/ArduinoJson/src'


class ConfigSaveTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = Path(cls.enterClassContext(tempfile.TemporaryDirectory()))
        storage = (CORE / 'config_storage.cpp').read_text()
        storage = '\n'.join(line for line in storage.splitlines() if not line.startswith('#include "'))
        header = (CORE / 'config_storage.h').read_text().split('enum class', 1)[1]
        cls.storage = 'enum class' + header + storage + '\n'
        config = (CORE / 'config.cpp').read_text()
        config = config[config.index('void BruceConfig::saveFile()'):config.index('void BruceConfig::factoryReset()')]
        pins = (CORE / 'configPins.cpp').read_text()
        pins = pins[pins.index('void BruceConfigPins::fromFile('):pins.index('void BruceConfigPins::validateConfig()')]
        cpp = cls.temp / 'config.cpp'
        cls.binary = cls.temp / 'config'
        cpp.write_text(STUB + cls.storage + config + pins + CHECK)
        subprocess.run(['g++', '-std=c++17', '-pthread', '-Wall', '-Wextra', '-Werror', '-Wl,--wrap=fflush',
                        '-Wl,--wrap=fsync', str(cpp), '-o', str(cls.binary)], check=True)

    def run_case(self, case):
        with tempfile.TemporaryDirectory() as directory:
            subprocess.run([str(self.binary), case], cwd=directory, check=True)

    def test_save_writes_credentials_to_storage_only(self):
        self.run_case('normal')

    def test_short_write_preserves_both_copies(self):
        self.run_case('short')

    def test_full_disk_detected_while_flushing_preserves_existing_file(self):
        self.run_case('full')

    def test_sync_failure_preserves_existing_file(self):
        self.run_case('sync')

    def test_failed_onboard_mirror_keeps_committed_sd_and_previous_onboard_copy(self):
        self.run_case('mirror')

    def test_restart_and_failed_rename_recover_the_previous_file(self):
        self.run_case('rename')

    def test_corrupt_sd_falls_back_to_onboard_and_temp_is_ignored(self):
        self.run_case('corrupt')

    def test_factory_reset_moves_previous_copy_out_of_recovery(self):
        self.run_case('reset')

    def test_pin_save_preserves_other_boards_and_unreadable_settings(self):
        self.run_case('pins')

    @unittest.skipUnless((ARDUINOJSON / 'ArduinoJson.h').is_file(), 'requires PlatformIO ArduinoJson dependency')
    def test_real_arduinojson_round_trip_and_fallback(self):
        cpp = self.temp / 'arduinojson.cpp'
        binary = cpp.with_suffix('')
        cpp.write_text(STUB.split('struct JsonObject', 1)[0] + '#include <ArduinoJson.h>\n' +
                       'void log_e(const char *, ...) {}\n' + self.storage + CHECK_ARDUINOJSON)
        subprocess.run(['g++', '-std=c++17', '-pthread', '-Wall', '-Wextra', '-Werror', '-I', str(ARDUINOJSON),
                        str(cpp), '-o', str(binary)], check=True)
        with tempfile.TemporaryDirectory() as directory:
            subprocess.run([str(binary)], cwd=directory, check=True)


STUB = r'''
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

namespace hostfs = std::filesystem;
struct String : std::string {
    using std::string::string;
    String(std::string value) : std::string(std::move(value)) {}
    String substring(size_t offset) const { return substr(offset); }
};

using File = std::ifstream;
constexpr int FILE_READ = 0;
struct FS {
    std::string root;
    bool blockPublish = false;
    bool blockRollback = false;
    const char *mountpoint() const { return root.c_str(); }
    File open(const String &path, int) { return File(root + path); }
    bool exists(const String &path) const { return hostfs::exists(root + path); }
    bool remove(const String &path) { return hostfs::remove(root + path); }
    bool rename(const String &source, const String &destination) {
        if (exists(destination)) return false; // FAT cannot replace an existing destination.
        const auto suffix = hostfs::path(source.c_str()).extension();
        if (blockPublish && suffix == ".tmp") return false;
        if (blockRollback && suffix == ".prev" && hostfs::path(destination.c_str()).extension() == ".conf") {
            return false;
        }
        hostfs::rename(root + source, root + destination);
        return true;
    }
} LittleFS{"onboard"}, SD{"sd"};

struct JsonObject { std::string &data; };
struct JsonObjectConst {};
struct JsonDocument {
    std::string data;
    void clear() { data.clear(); }
    bool overflowed() const { return false; }
    template <typename T> bool is() { return static_cast<const JsonDocument &>(*this).is<JsonObjectConst>(); }
    template <typename T> bool is() const {
        return std::is_same_v<T, JsonObjectConst> && data.size() >= 2 && data.front() == '{' && data.back() == '}';
    }
    template <typename T> T as() { return {data}; }
    template <typename T> T to() { data = "{}"; return as<T>(); }
    void remove(const char *) {
        const auto index = data.find(",\"current\":1");
        if (index != std::string::npos) data.erase(index, sizeof(",\"current\":1") - 1);
    }
};

bool deserializeJson(JsonDocument &document, File &file) {
    document.data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return document.data.empty();
}

size_t bytesToWrite = std::numeric_limits<size_t>::max();
int writeCalls;
template <typename Writer> size_t serializeJsonPretty(const JsonDocument &doc, Writer &writer) {
    ++writeCalls;
    return writer.write(reinterpret_cast<const uint8_t *>(doc.data.data()), std::min(bytesToWrite, doc.data.size()));
}
size_t measureJsonPretty(const JsonDocument &doc) { return doc.data.size(); }

bool failFlush;
int syncCalls, failSyncCall;
extern "C" int __real_fflush(FILE *file);
extern "C" int __wrap_fflush(FILE *file) { return failFlush ? EOF : __real_fflush(file); }
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    return ++syncCalls == failSyncCall ? -1 : __real_fsync(fd);
}
struct SerialStream {
    std::string output;
    size_t write(const uint8_t *data, size_t count) {
        output.append(reinterpret_cast<const char *>(data), count);
        return count;
    }
} Serial;
void log_e(const char *, ...) {}

bool sdcardMounted = true;
bool setupSdCard() { return sdcardMounted; }
const char *getMacAddress() { return "current"; }
class BruceConfig {
    static constexpr const char *filepath = "/bruce.conf";
public:
    JsonDocument toJson() { return {R"({"wifi":{"test":"fake-secret"}})"}; }
    void saveFile();
};
class BruceConfigPins {
    static constexpr const char *filepath = "/brucePins.conf";
public:
    void fromFile(bool);
    void saveFile();
    void factoryReset();
    void fromJson(JsonObject) {}
    void toJson(JsonObject object) {
        object.data.pop_back();
        if (object.data.size() > 1) object.data += ',';
        object.data += R"("current":2})";
    }
};
'''


CHECK = r'''
void put(const FS &fs, const String &path, const std::string &value) {
    std::ofstream out(fs.root + path);
    out << value;
}

std::string get(const FS &fs, const String &path) {
    std::ifstream file(fs.root + path);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string test = argv[1];
    hostfs::create_directory(LittleFS.root);
    hostfs::create_directory(SD.root);
    const char *path = "/bruce.conf";
    const std::string previous = R"({"previous":true})";
    const std::string latest = R"({"latest":true})";
    BruceConfig config;
    JsonDocument loaded;

    if (test == "normal") {
        config.saveFile();
        assert(get(LittleFS, path) == config.toJson().data && get(SD, path) == config.toJson().data);
        assert(Serial.output.find("fake-secret") == std::string::npos);
        put(LittleFS, path, previous);
        assert(loadConfigFile(path, loaded, true) == ConfigReadResult::Loaded && loaded.data == config.toJson().data);
    } else if (test == "short") {
        put(LittleFS, path, previous);
        put(SD, path, previous);
        bytesToWrite = 7;
        config.saveFile();
        assert(writeCalls == 1 && syncCalls == 0);
        assert(get(LittleFS, path) == previous && get(SD, path) == previous);
        assert(!SD.exists("/bruce.conf.tmp"));
    } else if (test == "full" || test == "sync") {
        put(LittleFS, path, previous);
        sdcardMounted = false;
        failFlush = test == "full";
        failSyncCall = 1;
        config.saveFile();
        assert(writeCalls == 1 && syncCalls == (failFlush ? 0 : 1));
        assert(get(LittleFS, path) == previous && !LittleFS.exists("/bruce.conf.tmp"));
    } else if (test == "mirror") {
        put(LittleFS, path, previous);
        put(SD, path, previous);
        failSyncCall = 2;
        config.saveFile();
        assert(syncCalls == 2 && get(SD, path) == config.toJson().data && get(LittleFS, path) == previous);
        assert(loadConfigFile(path, loaded, true) == ConfigReadResult::Loaded && loaded.data == config.toJson().data);
        assert(loadConfigFile(path, loaded, false) == ConfigReadResult::Loaded && loaded.data == previous);
    } else if (test == "rename") {
        put(LittleFS, path, previous);
        LittleFS.blockPublish = LittleFS.blockRollback = true;
        saveConfigFile(path, JsonDocument{latest}, false);
        assert(!LittleFS.exists(path));
        assert(loadConfigFile(path, loaded, false) == ConfigReadResult::Loaded && loaded.data == previous);
        LittleFS.blockPublish = LittleFS.blockRollback = false;
        saveConfigFile(path, JsonDocument{latest}, false);
        assert(get(LittleFS, path) == latest && !LittleFS.exists("/bruce.conf.prev"));

        // Simulate restart immediately after publishing, before deleting the previous copy.
        put(LittleFS, "/bruce.conf.prev", previous);
        assert(loadConfigFile(path, loaded, false) == ConfigReadResult::Loaded && loaded.data == latest);
        saveConfigFile(path, JsonDocument{previous}, false);
        assert(get(LittleFS, path) == previous && !LittleFS.exists("/bruce.conf.prev"));

        put(LittleFS, path, "{broken");
        put(LittleFS, "/bruce.conf.prev", previous);
        assert(loadConfigFile(path, loaded, false) == ConfigReadResult::Loaded && loaded.data == previous);
        saveConfigFile(path, JsonDocument{latest}, false);
        assert(get(LittleFS, path) == latest && !LittleFS.exists("/bruce.conf.prev"));
    } else if (test == "corrupt") {
        put(LittleFS, path, previous);
        put(SD, path, "[]");
        put(SD, "/bruce.conf.tmp", latest);
        assert(loadConfigFile(path, loaded, true) == ConfigReadResult::Loaded && loaded.data == previous);
        LittleFS.remove(path);
        assert(loadConfigFile(path, loaded, true) == ConfigReadResult::Unreadable && loaded.data.empty());
        SD.remove(path);
        assert(loadConfigFile(path, loaded, true) == ConfigReadResult::Missing && loaded.data.empty());
    } else if (test == "reset") {
        put(SD, "/bruce.conf.prev", previous);
        put(LittleFS, path, latest);
        put(LittleFS, "/bruce.conf.prev", previous);
        put(LittleFS, "/bak.bruce.conf", previous);
        backupConfigFile(path, true);
        assert(loadConfigFile(path, loaded, true) == ConfigReadResult::Missing);
        assert(get(SD, "/bak.bruce.conf") == previous && get(LittleFS, "/bak.bruce.conf") == latest);
    } else {
        assert(test == "pins");
        BruceConfigPins pins;
        const char *pinPath = "/brucePins.conf";
        put(SD, pinPath, R"({"other":9,"current":1})");
        pins.saveFile();
        assert(get(SD, pinPath) == R"({"other":9,"current":2})");
        assert(get(LittleFS, pinPath) == get(SD, pinPath));
        put(SD, pinPath, "{broken");
        put(LittleFS, pinPath, "{truncated");
        pins.saveFile();
        assert(get(SD, pinPath) == "{broken" && get(LittleFS, pinPath) == "{truncated");
    }
}
'''


CHECK_ARDUINOJSON = r'''
int main() {
    hostfs::create_directory(LittleFS.root);
    hostfs::create_directory(SD.root);
    JsonDocument settings;
    settings["wifi"]["test"] = "credential-mock";
    settings["brightness"] = 75;
    saveConfigFile("/bruce.conf", settings, true);

    JsonDocument loaded;
    assert(loadConfigFile("/bruce.conf", loaded, true) == ConfigReadResult::Loaded);
    assert(loaded["wifi"]["test"] == "credential-mock");

    std::ofstream(SD.root + "/bruce.conf") << "{\"incomplete\":";
    assert(loadConfigFile("/bruce.conf", loaded, true) == ConfigReadResult::Loaded);
    assert(loaded["brightness"] == 75);
    saveConfigFile("/bruce.conf", loaded, true);
    assert(!SD.exists("/bruce.conf.prev"));
    assert(loadConfigFile("/bruce.conf", loaded, true) == ConfigReadResult::Loaded);
    assert(loaded["wifi"]["test"] == "credential-mock");
}
'''
