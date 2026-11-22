import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class WebUploadTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = Path(cls.enterClassContext(tempfile.TemporaryDirectory()))
        source = (ROOT / 'src/core/wifi/webInterface.cpp').read_text()
        first = source.index('namespace {\nconstexpr char uploadPrefix')
        last = source.index('void notFound(', first)
        cpp = cls.temp / 'upload.cpp'
        cls.binary = cls.temp / 'upload'
        cpp.write_text(STUB + source[first:last] + CHECK)
        subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wl,--wrap=fsync',
                        '-Wl,--wrap=fwrite', str(cpp), '-o', str(cls.binary)], check=True)

    def run_case(self, name):
        with tempfile.TemporaryDirectory() as directory:
            subprocess.run([str(self.binary), name], cwd=directory, check=True)

    def test_interleaved_tabs_use_their_own_folders_and_drives(self):
        self.run_case('tabs')

    def test_open_failure_and_missing_sd_report_errors(self):
        self.run_case('unavailable')

    def test_short_write_and_failed_sync_keep_the_destination(self):
        self.run_case('failed-write')

    def test_disconnect_cleans_temporary_file_and_keeps_original(self):
        self.run_case('disconnect')

    def test_failed_replace_restores_original(self):
        self.run_case('rollback')

    def test_invalid_and_multiple_files_do_not_commit(self):
        self.run_case('invalid')

    def test_empty_file_and_legacy_encrypted_text(self):
        self.run_case('file-types')

    def test_browser_keeps_queued_destinations_and_recovers_from_error(self):
        source = (ROOT / 'embedded_resources/web_interface/index.js').read_text()
        upload = source[source.index('const _queueUpload'):source.index('async function runCommand')]
        subprocess.run(['node', '-e', JS_STUB + upload + JS_CHECK], check=True)


STUB = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <strings.h>
#include <unistd.h>

namespace hostfs = std::filesystem;
struct String : std::string {
    using std::string::string;
    String(std::string text) : std::string(std::move(text)) {}
    char *begin() { return data(); }
    bool startsWith(const char *prefix) const { return rfind(prefix, 0) == 0; }
    bool endsWith(const char *suffix) const {
        const size_t count = strlen(suffix);
        return size() >= count && compare(size() - count, count, suffix) == 0;
    }
    void remove(size_t index) { erase(index); }
    int indexOf(char value, size_t offset) const {
        const auto index = find(value, offset);
        return index == npos ? -1 : static_cast<int>(index);
    }
    int lastIndexOf(char value) const { return static_cast<int>(rfind(value)); }
    String substring(size_t from, size_t to = npos) const { return substr(from, to - from); }
};

struct File {
    hostfs::path path;
    explicit operator bool() const { return hostfs::exists(path); }
    bool isDirectory() const { return hostfs::is_directory(path); }
    void close() {}
};

constexpr int FILE_READ = 0;
struct FS {
    std::string root;
    bool mounted = true;
    bool blockReplace = false;
    const char *mountpoint() const { return mounted ? root.c_str() : nullptr; }
    File open(const String &path, int) { return {root + path}; }
    bool exists(const String &path) const { return hostfs::exists(root + path); }
    bool mkdir(const String &path) {
        std::error_code error;
        return hostfs::create_directory(root + path, error);
    }
    bool remove(const String &path) { return hostfs::remove(root + path); }
    bool rename(const String &from, const String &to) {
        if (exists(to)) return false;
        if (blockReplace && from.find(".bruce-upload-") != String::npos && !from.endsWith(".bak")) return false;
        std::error_code error;
        hostfs::rename(root + from, root + to, error);
        return !error;
    }
} LittleFS{"onboard"}, SD{"sd"};

struct AsyncWebParameter {
    String text;
    const String &value() const { return text; }
};

struct AsyncWebServerRequest {
    void *_tempObject = nullptr;
    std::function<void()> disconnected;
    std::map<std::string, AsyncWebParameter> query, body;
    int status = 0;
    bool authorized = true;

    AsyncWebServerRequest(const char *fs, const char *folder) : query{{"fs", {fs}}, {"folder", {folder}}} {}
    ~AsyncWebServerRequest() { free(_tempObject); }
    void onDisconnect(std::function<void()> fn) { disconnected = std::move(fn); }
    bool getResponse() const { return status != 0; }
    const AsyncWebParameter *getParam(const char *name, bool post = false) const {
        const auto &params = post ? body : query;
        const auto item = params.find(name);
        return item == params.end() ? nullptr : &item->second;
    }
    bool hasParam(const char *name, bool post) const { return getParam(name, post); }
    void send(int code, const char *, const char *) { status = code; }
};

bool checkUserWebAuth(AsyncWebServerRequest *request) {
    if (request->authorized) return true;
    request->send(401, "text/plain", "");
    return false;
}
bool setupSdCard() { return SD.mounted; }
void log_e(const char *, ...) {}
void log_w(const char *, ...) {}
String encryptString(String &text, const String &) { return "encoded:" + text; }

bool shortWrite, failSync;
int syncCalls, writeCalls;
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    ++syncCalls;
    return failSync ? -1 : __real_fsync(fd);
}
extern "C" size_t __real_fwrite(const void *, size_t, size_t, FILE *);
extern "C" size_t __wrap_fwrite(const void *data, size_t size, size_t count, FILE *file) {
    ++writeCalls;
    return __real_fwrite(data, size, shortWrite ? count / 2 : count, file);
}
'''


CHECK = r'''
void put(const FS &fs, const char *path, const char *value) {
    std::ofstream output(fs.root + path);
    output << value;
}

std::string get(const FS &fs, const char *path) {
    std::ifstream input(fs.root + path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool hasStagedFiles(const FS &fs) {
    for (const auto &entry : hostfs::recursive_directory_iterator(fs.root)) {
        if (entry.path().filename().string().find(".bruce-upload-") == 0) return true;
    }
    return false;
}

void part(AsyncWebServerRequest &request, const char *name, size_t index, std::string data, bool final) {
    handleUpload(&request, name, index, reinterpret_cast<uint8_t *>(data.data()), data.size(), final);
}

int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string test = argv[1];
    hostfs::create_directory(LittleFS.root);
    hostfs::create_directory(SD.root);

    if (test == "tabs") {
        AsyncWebServerRequest first("SD", "/First Folder/");
        AsyncWebServerRequest second("LittleFS", "/second");
        part(first, "nested/file.txt", 0, "one", false);
        part(second, "nested/file.txt", 0, "two", true);
        assert(!SD.exists("/First Folder/nested/file.txt") && !LittleFS.exists("/second/nested/file.txt"));
        finishUpload(&second);
        part(first, "nested/file.txt", 3, " more", true);
        finishUpload(&first);
        assert(first.status == 200 && second.status == 200);
        assert(get(SD, "/First Folder/nested/file.txt") == "one more");
        assert(get(LittleFS, "/second/nested/file.txt") == "two");
    } else if (test == "unavailable") {
        SD.mounted = false;
        AsyncWebServerRequest noSd("SD", "/");
        part(noSd, "file.txt", 0, "body", true);
        finishUpload(&noSd);
        assert(noSd.status == 503);
        put(LittleFS, "/regular-file", "original");
        AsyncWebServerRequest noDirectory("LittleFS", "/regular-file");
        part(noDirectory, "file.txt", 0, "body", true);
        finishUpload(&noDirectory);
        assert(noDirectory.status == 507 && get(LittleFS, "/regular-file") == "original");
    } else if (test == "failed-write") {
        put(LittleFS, "/file.txt", "original");
        shortWrite = true;
        AsyncWebServerRequest shortRequest("LittleFS", "/");
        part(shortRequest, "file.txt", 0, "partial body", true);
        finishUpload(&shortRequest);
        assert(shortRequest.status == 507 && writeCalls == 1 && syncCalls == 0);
        assert(get(LittleFS, "/file.txt") == "original");

        shortWrite = false;
        failSync = true;
        AsyncWebServerRequest syncRequest("LittleFS", "/");
        part(syncRequest, "file.txt", 0, "unsynced body", true);
        finishUpload(&syncRequest);
        assert(syncRequest.status == 507 && writeCalls == 2 && syncCalls == 1);
        assert(get(LittleFS, "/file.txt") == "original");
    } else if (test == "disconnect") {
        put(SD, "/file.txt", "original");
        {
            AsyncWebServerRequest interrupted("SD", "/");
            part(interrupted, "file.txt", 0, "unfinished", false);
            assert(hasStagedFiles(SD));
            interrupted.disconnected();
            assert(!hasStagedFiles(SD));
        }
        {
            AsyncWebServerRequest incompleteRequest("SD", "/");
            part(incompleteRequest, "file.txt", 0, "file part complete", true);
            assert(hasStagedFiles(SD));
        }
        assert(get(SD, "/file.txt") == "original");
    } else if (test == "rollback") {
        put(SD, "/file.txt", "original");
        SD.blockReplace = true;
        AsyncWebServerRequest failure("SD", "/");
        part(failure, "file.txt", 0, "new body", true);
        finishUpload(&failure);
        assert(failure.status == 500 && get(SD, "/file.txt") == "original");
        SD.blockReplace = false;
        AsyncWebServerRequest retry("SD", "/");
        part(retry, "file.txt", 0, "new body", true);
        finishUpload(&retry);
        assert(retry.status == 200 && get(SD, "/file.txt") == "new body");
    } else if (test == "invalid") {
        AsyncWebServerRequest unauthorized("SD", "/");
        unauthorized.authorized = false;
        part(unauthorized, "file.txt", 0, "body", true);
        finishUpload(&unauthorized);
        assert(unauthorized.status == 401);
        AsyncWebServerRequest badName("SD", "/");
        part(badName, "bad name.", 0, "body", true);
        finishUpload(&badName);
        assert(badName.status == 400);
        AsyncWebServerRequest reservedName("SD", "/");
        part(reservedName, ".BRUCE-UPLOAD-example", 0, "body", true);
        finishUpload(&reservedName);
        assert(reservedName.status == 400);
        AsyncWebServerRequest unknownDrive("Unknown", "/");
        part(unknownDrive, "file.txt", 0, "body", true);
        finishUpload(&unknownDrive);
        assert(unknownDrive.status == 400);
        AsyncWebServerRequest multiple("SD", "/");
        part(multiple, "a.txt", 0, "first file", true);
        part(multiple, "b.txt", 0, "second file", true);
        finishUpload(&multiple);
        assert(multiple.status == 400 && !SD.exists("/a.txt") && !SD.exists("/b.txt"));
        AsyncWebServerRequest latePassword("SD", "/");
        part(latePassword, "file.txt", 0, "text", true);
        latePassword.body["password"] = {"fake"};
        finishUpload(&latePassword);
        assert(latePassword.status == 400 && !SD.exists("/file.txt"));
    } else {
        assert(test == "file-types");
        AsyncWebServerRequest empty("LittleFS", "/");
        part(empty, "empty.txt", 0, "", true);
        finishUpload(&empty);
        assert(empty.status == 200 && LittleFS.exists("/empty.txt") && get(LittleFS, "/empty.txt").empty());
        const std::string bytes("a\0b", 3);
        AsyncWebServerRequest binary("SD", "/");
        part(binary, "binary.dat", 0, bytes, true);
        finishUpload(&binary);
        assert(binary.status == 200 && get(SD, "/binary.dat") == bytes);
        for (const char *name : {"first.txt", "second.txt"}) {
            AsyncWebServerRequest encrypted("SD", "/");
            (name[0] == 'f' ? encrypted.body : encrypted.query)["password"] = {"fake"};
            part(encrypted, name, 0, "text", true);
            finishUpload(&encrypted);
            const String filename = String("/") + name + ".enc";
            assert(encrypted.status == 200 && get(SD, filename.c_str()) == "encoded:text");
        }
    }
    assert(!hasStagedFiles(SD) && !hasStagedFiles(LittleFS));
}
'''


JS_STUB = r'''
const assert = require("node:assert/strict");
const requests = [];
const alerts = [];
const panel = { innerHTML: "", appendChild() {} };
function $() { return { querySelector: () => panel }; }
const Dialog = { show() {}, hide() {} };
const T = { uploadLoading: () => ({ querySelector: () => ({ setAttribute() {} }) }) };
const stringToId = (value) => value;
const IS_DEV = false;
function fetchSystemInfo() {}
function fetchFiles() {}
function alert(message) { alerts.push(message); }
let currentPath = "/initial";
let currentDrive = "SD";
class FormData {
  values = [];
  append(...value) { this.values.push(value); }
}
class XMLHttpRequest {
  upload = {};
  open(method, url) { this.url = new URL(url, "http://bruce"); }
  send(data) { this.data = data; requests.push(this); }
}
'''


JS_CHECK = r'''
const first = { name: "test.txt" };
const second = { name: "different.txt" };
appendFileToQueue([first], { fs: currentDrive, folder: currentPath });
appendFileToQueue([second], { fs: "LittleFS", folder: "/different # path" });
currentPath = "/after-navigation";
uploadFile();
assert.equal(requests.length, 1);
assert.equal(requests[0].url.searchParams.get("folder"), "/initial");
assert.equal(requests[0].url.searchParams.get("fs"), "SD");
assert.equal(requests[0].data.values.length, 1);
requests[0].status = 200;
requests[0].onload();
assert.equal(requests[1].url.searchParams.get("folder"), "/different # path");
assert.equal(requests[1].url.searchParams.get("fs"), "LittleFS");
appendFileToQueue([first], { fs: "SD", folder: "/" });
requests[1].status = 507;
requests[1].responseText = "Unable to write complete upload";
requests[1].onload();
assert.equal(_runningUpload, false);
assert.equal(_queueUpload.length, 0);
assert.equal(requests.length, 2);
assert.match(alerts[0], /Unable to write complete upload/);

appendFileToQueue([first], { fs: "SD", folder: "/" });
uploadFile();
requests[2].onerror();
assert.equal(_runningUpload, false);
assert.match(alerts[1], /Network connection lost/);
'''
