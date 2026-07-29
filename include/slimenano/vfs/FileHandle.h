
#ifndef SLIMENANO_VFS_INCLUDE_VFS_FILE_H
#define SLIMENANO_VFS_INCLUDE_VFS_FILE_H

#include <cstdint>
#include <cstddef>
#include <span>
#include <system_error>

namespace slimenano::filesystem {

enum class SeekOrigin : std::int8_t {
    Begin = 0,
    Current,
    End
};

class FileHandle {

public:
    virtual ~FileHandle() = default;

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&&) noexcept = delete;
    FileHandle& operator=(FileHandle&&) noexcept = delete;

    virtual std::size_t Read(std::span<std::byte> buffer, std::error_code& ec) = 0;
    virtual std::size_t Write(std::span<const std::byte> buffer, std::error_code& ec) = 0;
    virtual std::uint64_t Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) = 0;
    virtual std::uint64_t Tell(std::error_code& ec) = 0;
    virtual void Flush(std::error_code& ec) = 0;
    virtual void Close(std::error_code& ec) = 0;

protected:
    FileHandle() = default;
};

} // namespace slimenano::filesystem

#endif
