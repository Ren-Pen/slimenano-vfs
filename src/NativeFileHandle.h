#ifndef SLIMENANO_VFS_SRC_NATIVE_FILE_HANDLE_H
#define SLIMENANO_VFS_SRC_NATIVE_FILE_HANDLE_H

#include <slimenano/vfs/FileHandle.h>
#include <slimenano/vfs/Options/OpenOptions.h>

#include <filesystem>
#include <fstream>

namespace slimenano::filesystem {

class NativeFileHandle final : public FileHandle {
public:
    NativeFileHandle(std::filesystem::path path, OpenOption options, std::error_code& ec);

    std::size_t Read(std::span<std::byte> buffer, std::error_code& ec) override;
    std::size_t Write(std::span<const std::byte> buffer, std::error_code& ec) override;
    std::uint64_t Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) override;
    std::uint64_t Tell(std::error_code& ec) override;
    void Flush(std::error_code& ec) override;
    void Close(std::error_code& ec) override;

private:
    std::fstream m_stream;
    bool m_readable = false;
    bool m_writable = false;
    bool m_closed = false;
};

} // namespace slimenano::filesystem

#endif
