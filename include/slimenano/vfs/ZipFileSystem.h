#ifndef SLIMENANO_VFS_INCLUDE_VFS_ZIP_FILE_SYSTEM_H
#define SLIMENANO_VFS_INCLUDE_VFS_ZIP_FILE_SYSTEM_H

#include <slimenano/vfs/FileSystem.h>

namespace slimenano::filesystem {

class ZipFileSystem final : public FileSystem {
public:
    explicit ZipFileSystem(std::string_view zip_path, std::error_code& ec);
    ~ZipFileSystem();

    ZipFileSystem(const ZipFileSystem&) = delete;
    ZipFileSystem(ZipFileSystem&&) = delete;
    ZipFileSystem& operator=(const ZipFileSystem&) = delete;
    ZipFileSystem& operator=(ZipFileSystem&&) = delete;

    [[nodiscard]] FileInfo Stat(const Path& path, std::error_code& ec) const override;

    [[nodiscard]] bool Exists(const Path& path, std::error_code& ec) const override;

    [[nodiscard]] bool IsDirectory(const Path& path, std::error_code& ec) const override;

    [[nodiscard]] bool IsRegularFile(const Path& path, std::error_code& ec) const override;

    [[nodiscard]] bool IsReadable(const Path& path, std::error_code& ec) const override;

    [[nodiscard]] bool IsWritable(const Path& path, std::error_code& ec) const override;

    [[nodiscard]] std::uint64_t Size(const Path& path, std::error_code& ec) const override;

    [[nodiscard]] std::vector<std::string> List(const Path& path, std::error_code& ec) const override;

    void CreateFile(const Path& path, std::error_code& ec) override;

    void CreateDirectory(const Path& path, std::error_code& ec) override;

    void CreateDirectories(const Path& path, std::error_code& ec) override;

    void DeleteFile(const Path& path, std::error_code& ec) override;

    void DeleteDirectory(const Path& path, std::error_code& ec) override;

    void DeleteDirectories(const Path& path, std::error_code& ec) override;

    void Copy(const Path& src, const Path& to, CopyOption copyOptions, std::error_code& ec) override;

    void Rename(const Path& src, const Path& to, std::error_code& ec) override;

    [[nodiscard]] std::unique_ptr<FileHandle>
    Open(const Path& path, OpenOption openOptions, std::error_code& ec) override;

    using FileSystem::Open;

private:
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};

} // namespace slimenano::filesystem

#endif
