/**
 * @file NativeFileHandle.h
 * @brief Declaration of the native file handle implementation.
 */

#ifndef SLIMENANO_VFS_SRC_NATIVE_NATIVE_FILE_HANDLE_H
#define SLIMENANO_VFS_SRC_NATIVE_NATIVE_FILE_HANDLE_H

#include <slimenano/vfs/FileHandle.h>
#include <slimenano/vfs/Options/OpenOptions.h>

#include <cstdio>
#include <memory>

namespace slimenano::filesystem {

class NativeFileHandle final : public FileHandle {

    struct FileDeleter {
        void operator()(std::FILE* fp) const noexcept {
            if (fp) {
                std::fclose(fp);
            }
        }
    };

    using FilePtr = std::unique_ptr<std::FILE, FileDeleter>;

public:
    NativeFileHandle(std::string_view path, OpenOption options, std::error_code& ec);
    virtual ~NativeFileHandle() = default;

    std::size_t Read(std::span<std::byte> buffer, std::error_code& ec) override;

    std::size_t Write(std::span<const std::byte> buffer, std::error_code& ec) override;

    std::uint64_t Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) override;

    [[nodiscard]] std::uint64_t Tell(std::error_code& ec) override;

    void Flush(std::error_code& ec) override;

    void Close(std::error_code& ec) override;

private:
    FilePtr m_pFile{};
    bool m_readable{false};
    bool m_writable{false};
};

} // namespace slimenano::filesystem

#endif
