#ifndef SLIMENANO_VFS_INCLUDE_VFS_FILE_SYSTEM_H
#define SLIMENANO_VFS_INCLUDE_VFS_FILE_SYSTEM_H

#include <cstdint>
#include <vector>
#include <memory>
#include <system_error>
#include <string>

#include <slimenano/vfs/Path.h>
#include <slimenano/vfs/FileInfo.h>
#include <slimenano/vfs/FileHandle.h>
#include <slimenano/vfs/Options/OpenOptions.h>

namespace slimenano::filesystem {

class FileSystem {
public:
    FileSystem() = default;
    virtual ~FileSystem() = default;

    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;
    FileSystem(FileSystem&&) noexcept = delete;
    FileSystem& operator=(FileSystem&&) noexcept = delete;

    virtual void Initialize(std::error_code& ec) const = 0;

    [[nodiscard]] virtual FileInfo Stat(const Path& path, std::error_code& ec) const = 0;
    [[nodiscard]] virtual bool Exists(const Path& path, std::error_code& ec) const = 0;
    [[nodiscard]] virtual bool IsDirectory(const Path& path, std::error_code& ec) const;
    [[nodiscard]] virtual bool IsRegularFile(const Path& path, std::error_code& ec) const;
    [[nodiscard]] virtual bool IsReadable(const Path& path, std::error_code& ec) const;
    [[nodiscard]] virtual bool IsWritable(const Path& path, std::error_code& ec) const;
    [[nodiscard]] virtual std::uint64_t Size(const Path& path, std::error_code& ec) const;
    [[nodiscard]] virtual std::vector<std::string> List(const Path& path, std::error_code& ec) const = 0;

    virtual void CreateFile(const Path& path, std::error_code& ec) = 0;
    virtual void CreateDirectory(const Path& path, std::error_code& ec) = 0;
    virtual void CreateDirectories(const Path& path, std::error_code& ec) = 0;
    virtual void DeleteFile(const Path& path, std::error_code& ec) = 0;
    virtual void DeleteDirectory(const Path& path, std::error_code& ec) = 0;
    virtual void DeleteDirectories(const Path& path, std::error_code& ec) = 0;

    [[nodiscard]] virtual std::unique_ptr<FileHandle>
    Open(const Path& path, OpenOption openOptions, std::error_code& ec) = 0;

    [[nodiscard]] std::unique_ptr<FileHandle> Open(const Path& path, std::error_code& ec) {
        return Open(path, OpenOption::Read, ec);
    }
};

} // namespace slimenano::filesystem

#endif
