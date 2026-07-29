
#ifndef SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_FILE_SYSTEM_H
#define SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_FILE_SYSTEM_H

#include <type_traits>
#include <memory>
#include <utility>
#include <string>
#include <system_error>
#include <vector>

#include <slimenano/vfs/Path.h>
#include <slimenano/vfs/FileSystem.h>

namespace slimenano::filesystem {

class VirtualFileSystem final : public FileSystem {

public:
    VirtualFileSystem();
    virtual ~VirtualFileSystem();

    void Mount(const Path& mountPoint, std::shared_ptr<FileSystem> ptr);

    template <typename T, typename... Args>
    std::shared_ptr<FileSystem> CreateAndMount(const Path& mountPoint, Args&&... args) {
        static_assert(std::is_base_of_v<FileSystem, T>, "T must derive from FileSystem");
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        if (!ptr) {
            return nullptr;
        }

        if (!ptr->Initialize()) {
            return nullptr;
        }

        Mount(mountPoint, ptr);
        return ptr;
    }

    [[nodiscard]] bool IsMounted(const Path& mountPoint) const;
    [[nodiscard]] bool IsMounted(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr) const;
    void Unmount(const Path& mountPoint);
    void Unmount(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr);
    [[nodiscard]] std::vector<std::shared_ptr<FileSystem>> GetMountedFileSystems(const Path& mountPoint) const;

    [[nodiscard]] bool Initialize() const override;
    [[nodiscard]] bool Exists(const Path& path, std::error_code& ec) const override;
    [[nodiscard]] FileInfo Stat(const Path& path, std::error_code& ec) const override;
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
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace slimenano::filesystem

#endif
