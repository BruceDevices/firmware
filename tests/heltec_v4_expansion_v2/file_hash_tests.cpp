#include "src/core/file_hash_stream.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char *expression, int line) {
    if (!condition) throw std::runtime_error(std::string("line ") + std::to_string(line) + ": " + expression);
}
#define REQUIRE(expression) require((expression), #expression, __LINE__)

struct TestFile {
    std::vector<uint8_t> bytes;
    size_t offset = 0;
    size_t readLimit = 512;
    size_t failAt = size_t(-1);
    int error = 0;
    bool overreport = false;
    bool changeSize = false;
    std::vector<size_t> requests;

    size_t size() const { return bytes.size() + (changeSize && offset != 0 ? 1 : 0); }
    int read(uint8_t *destination, size_t wanted) {
        requests.push_back(wanted);
        if (overreport) return int(wanted + 1);
        if (offset >= failAt) return error;
        const size_t available = std::min(bytes.size() - offset, failAt - offset);
        const size_t count = std::min(wanted, std::min(readLimit, available));
        std::copy_n(bytes.data() + offset, count, destination);
        offset += count;
        return int(count);
    }
};

struct HashSink {
    std::vector<uint8_t> bytes;
    size_t calls = 0;
    void add(const uint8_t *data, size_t length) {
        REQUIRE(length > 0 && length <= 512);
        bytes.insert(bytes.end(), data, data + length);
        ++calls;
    }
};

TestFile makeFile(size_t length) {
    TestFile file;
    file.bytes.resize(length);
    for (size_t index = 0; index < length; ++index) file.bytes[index] = uint8_t(index * 37);
    return file;
}

void emptyFileIsAValidEmptyHashInput() {
    TestFile file;
    HashSink hash;
    size_t pauses = 0;
    REQUIRE(feedFileToHash(file, hash, [&]() { ++pauses; }));
    REQUIRE(hash.bytes.empty() && hash.calls == 0 && pauses == 0);
    REQUIRE(file.requests.empty());
}

void binaryBytesAndChunkBoundaries() {
    for (size_t length : {size_t(1), size_t(511), size_t(512), size_t(513), size_t(1024), size_t(1025)}) {
        TestFile file = makeFile(length);
        HashSink hash;
        size_t pauses = 0;
        REQUIRE(feedFileToHash(file, hash, [&]() { ++pauses; }));
        REQUIRE(hash.bytes == file.bytes); // Includes NUL and every binary byte value.
        REQUIRE(hash.calls == (length + 511) / 512);
        REQUIRE(pauses == hash.calls);
        for (size_t requested : file.requests) REQUIRE(requested <= 512);
    }
}

void multiMegabyteFileReachesHashExactly() {
    TestFile file = makeFile(3800123);
    file.bytes.back() = 0;
    HashSink hash;
    size_t pauses = 0;
    REQUIRE(feedFileToHash(file, hash, [&]() { ++pauses; }));
    REQUIRE(hash.bytes == file.bytes);
    REQUIRE(file.offset == file.bytes.size());
    REQUIRE(pauses == (file.bytes.size() + 511) / 512);
}

void shortReadsDoNotSkipBytes() {
    TestFile file = makeFile(1031);
    file.readLimit = 7;
    HashSink hash;
    size_t pauses = 0;
    REQUIRE(feedFileToHash(file, hash, [&]() { ++pauses; }));
    REQUIRE(hash.bytes == file.bytes);
    REQUIRE(pauses == (file.bytes.size() + 6) / 7);
}

void prematureEofOrReadErrorCannotPass() {
    for (int error : {0, -1}) {
        TestFile file = makeFile(2048);
        file.failAt = 700;
        file.error = error;
        HashSink hash;
        size_t pauses = 0;
        REQUIRE(!feedFileToHash(file, hash, [&]() { ++pauses; }));
        REQUIRE(hash.bytes.size() == 700);
        REQUIRE(pauses == 2);
    }
}

void invalidReadLengthCannotPass() {
    TestFile file = makeFile(1024);
    file.overreport = true;
    HashSink hash;
    size_t pauses = 0;
    REQUIRE(!feedFileToHash(file, hash, [&]() { ++pauses; }));
    REQUIRE(hash.bytes.empty() && pauses == 0);
}

void changingFileSizeCannotPass() {
    TestFile file = makeFile(1024);
    file.changeSize = true;
    HashSink hash;
    REQUIRE(!feedFileToHash(file, hash, []() {}));
    REQUIRE(hash.bytes == file.bytes);
}
} // namespace

int main() {
    try {
        emptyFileIsAValidEmptyHashInput();
        binaryBytesAndChunkBoundaries();
        multiMegabyteFileReachesHashExactly();
        shortReadsDoNotSkipBytes();
        prematureEofOrReadErrorCannotPass();
        invalidReadLengthCannotPass();
        changingFileSizeCannotPass();
        std::cout << "PASS 7 streaming hash cases: binary, large/empty files, bounded reads, yields, and failures\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL streaming file hash: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
