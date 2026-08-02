#ifndef SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_FILE_SYSTEM_H
#define SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_FILE_SYSTEM_H

#include <slimenano/vfs/FileSystem.h>

#include <filesystem>

namespace slimenano::filesystem {

class NativeFileSystem final : public FileSystem {

public:
    explicit NativeFileSystem(std::string_view root);
    ~NativeFileSystem() = default;

    NativeFileSystem(const NativeFileSystem&) = delete;
    NativeFileSystem(NativeFileSystem&&) = delete;
    NativeFileSystem& operator=(const NativeFileSystem&) = delete;
    NativeFileSystem& operator=(NativeFileSystem&&) = delete;

    void Initialize(std::error_code& ec) const override;

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

    [[nodiscard]] std::unique_ptr<FileHandle>
    Open(const Path& path, OpenOption openOptions, std::error_code& ec) override;

private:
    std::filesystem::path m_root;
};

} // namespace slimenano::filesystem

#endif
