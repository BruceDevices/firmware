#pragma once

#include "vfs_api.h"
#include "ff.h"
#include <atomic>
#include <mutex>
#include <utility>

namespace fs {

class SdOpenFile;

// The gate is shared by SD and every copy of its fs::FS handle.
class SdFsImpl final : public FSImpl, public std::enable_shared_from_this<SdFsImpl> {
    friend class SDFS;
    friend class SdOpenFile;

    FSImplPtr _vfs;
    FATFS *_fat = nullptr; // Owned by the registered VFS context.
    size_t _openFiles = 0;
    std::atomic_bool _rawAccess{false};

    FileImplPtr keepOpen(FileImplPtr file);

public:
    std::recursive_mutex mutex;

    explicit SdFsImpl(FSImplPtr vfs) : _vfs(std::move(vfs)) {}

    void setMountpoint(const char *path) {
        _mountpoint = path;
        _vfs->mountpoint(path);
    }

    FileImplPtr open(const char *path, const char *mode, bool create) override {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (_rawAccess) return {};
        return keepOpen(_vfs->open(path, mode, create));
    }
    bool exists(const char *path) override {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return !_rawAccess && _vfs->exists(path);
    }
    bool rename(const char *from, const char *to) override {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return !_rawAccess && _vfs->rename(from, to);
    }
    bool remove(const char *path) override {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return !_rawAccess && _vfs->remove(path);
    }
    bool mkdir(const char *path) override {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return !_rawAccess && _vfs->mkdir(path);
    }
    bool rmdir(const char *path) override {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        return !_rawAccess && _vfs->rmdir(path);
    }
};

// Count real handles rather than fs::File wrappers. Copies share one tracked handle,
// while a child file can live longer than the directory it was opened from.
class SdOpenFile final : public FileImpl {
    std::shared_ptr<SdFsImpl> _fs;
    FileImplPtr _file;
    bool _open = true;

    template <typename Operation>
    decltype(auto) withFile(Operation operation) const {
        std::lock_guard<std::recursive_mutex> lock(_fs->mutex);
        return operation(*_file);
    }

    void closeLocked() {
        if (_open) {
            _file->close();
            _open = false;
        }
    }

public:
    SdOpenFile(std::shared_ptr<SdFsImpl> fs, FileImplPtr file) : _fs(std::move(fs)), _file(std::move(file)) {
        ++_fs->_openFiles;
    }
    ~SdOpenFile() override {
        std::lock_guard<std::recursive_mutex> lock(_fs->mutex);
        closeLocked();
        // A closed File may still have aliases with operations in progress.
        // Release the lease only when the last alias has been destroyed.
        --_fs->_openFiles;
    }

    void close() override {
        std::lock_guard<std::recursive_mutex> lock(_fs->mutex);
        closeLocked();
    }
    FileImplPtr openNextFile(const char *mode) override {
        std::lock_guard<std::recursive_mutex> lock(_fs->mutex);
        if (!_open) return {};
        return _fs->keepOpen(_file->openNextFile(mode));
    }
    size_t write(const uint8_t *buffer, size_t size) override {
        return withFile([&](FileImpl &file) { return file.write(buffer, size); });
    }
    size_t read(uint8_t *buffer, size_t size) override {
        return withFile([&](FileImpl &file) { return file.read(buffer, size); });
    }
    void flush() override { withFile([](FileImpl &file) { file.flush(); }); }
    bool seek(uint32_t position, SeekMode mode) override {
        return withFile([&](FileImpl &file) { return file.seek(position, mode); });
    }
    size_t position() const override { return withFile([](FileImpl &file) { return file.position(); }); }
    size_t size() const override { return withFile([](FileImpl &file) { return file.size(); }); }
    bool setBufferSize(size_t size) override {
        return withFile([&](FileImpl &file) { return file.setBufferSize(size); });
    }
    time_t getLastWrite() override { return withFile([](FileImpl &file) { return file.getLastWrite(); }); }
    const char *path() const override { return withFile([](FileImpl &file) { return file.path(); }); }
    const char *name() const override { return withFile([](FileImpl &file) { return file.name(); }); }
    boolean isDirectory() override { return withFile([](FileImpl &file) { return file.isDirectory(); }); }
    boolean seekDir(long position) override {
        return withFile([&](FileImpl &file) { return file.seekDir(position); });
    }
    String getNextFileName() override { return withFile([](FileImpl &file) { return file.getNextFileName(); }); }
    String getNextFileName(bool *isDir) override {
        return withFile([&](FileImpl &file) { return file.getNextFileName(isDir); });
    }
    void rewindDirectory() override { withFile([](FileImpl &file) { file.rewindDirectory(); }); }
    operator bool() override { return withFile([](FileImpl &file) { return static_cast<bool>(file); }); }
};

inline FileImplPtr SdFsImpl::keepOpen(FileImplPtr file) {
    if (!file || !*file) return {};
    return std::make_shared<SdOpenFile>(shared_from_this(), std::move(file));
}

} // namespace fs
