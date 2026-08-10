#ifndef SLIMENANO_VFS_SRC_ZIP_ZIP_HANDLE_H
#define SLIMENANO_VFS_SRC_ZIP_ZIP_HANDLE_H

#include <memory>

#include <zip.h>

#include <slimenano/vfs/FileHandle.h>

namespace slimenano::filesystem {

class ZipFileHandle final : public FileHandle {

public:
    struct ZipFileDeleter {
        void operator()(zip_file_t* fp) const noexcept {
            if (fp) {
                zip_fclose(fp);
            }
        }
    };

    using ZipFilePtr = std::unique_ptr<zip_file_t, ZipFileDeleter>;

    ZipFileHandle(std::shared_ptr<zip_t> pZip, zip_int64_t index, std::error_code& ec);
    virtual ~ZipFileHandle() = default;

    ZipFileHandle(const ZipFileHandle&) = delete;
    ZipFileHandle& operator=(const ZipFileHandle&) = delete;
    ZipFileHandle(ZipFileHandle&&) noexcept = delete;
    ZipFileHandle& operator=(ZipFileHandle&&) noexcept = delete;

    std::size_t Read(std::span<std::byte> buffer, std::error_code& ec) override;
    std::size_t Write(std::span<const std::byte> buffer, std::error_code& ec) override;
    std::uint64_t Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) override;
    [[nodiscard]] std::uint64_t Tell(std::error_code& ec) override;
    void Flush(std::error_code& ec) override;
    void Close(std::error_code& ec) override;

private:
    std::shared_ptr<zip_t> m_pZip;
    ZipFilePtr m_pFile{};
};

} // namespace slimenano::filesystem

#endif
