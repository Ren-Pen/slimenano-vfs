/**
 * @file VirtualFileSystem.h
 * @brief Mount-point based virtual file system combining multiple
 *        file systems.
 */

#ifndef SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_FILE_SYSTEM_H
#define SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_FILE_SYSTEM_H

#include <type_traits>
#include <utility>

#include <slimenano/vfs/FileSystem.h>

namespace slimenano::filesystem {

/**
 * @brief A mount-point based virtual file system combining multiple
 *        FileSystem implementations behind a single virtual tree.
 *
 * Sub-file-systems are attached to virtual directories with Mount().
 * Operations on a virtual path are dispatched to the file system mounted at
 * the deepest ancestor mount point of the path; when several file systems
 * are mounted at the same point, the most recently mounted one is tried
 * first. Paths that do not resolve to any mounted file system are treated as
 * virtual directories: they exist, count as directories, and may be created
 * implicitly when something is mounted below them, but they are read-only.
 */
class VirtualFileSystem final : public FileSystem {

public:
    /**
     * @brief Constructs an empty virtual file system containing only the
     *        root directory.
     */
    VirtualFileSystem();

    /**
     * @brief Destroys the virtual file system, releasing its references to
     *        the mounted sub-file-systems.
     */
    virtual ~VirtualFileSystem();

    /**
     * @brief Mounts a file system at a virtual directory.
     *
     * Intermediate directories on the path are created implicitly. Mounting
     * a null pointer is ignored, and mounting the same file system instance
     * twice at the same point is a no-op.
     *
     * @param mountPoint Virtual directory at which to attach @p ptr.
     * @param ptr        The file system to mount.
     */
    void Mount(const Path& mountPoint, std::shared_ptr<FileSystem> ptr);

    /**
     * @brief Creates a FileSystem implementation and mounts it at a virtual
     *        directory.
     *
     * @tparam T         Concrete FileSystem implementation to instantiate.
     * @tparam Args      Constructor argument types of T.
     * @param mountPoint Virtual directory at which to attach the file
     *                   system.
     * @param ec         On failure, set to an error code describing the
     *                   problem.
     * @param args       Arguments forwarded to T's constructor.
     *
     * @return The created file system, or nullptr when construction or
     *         initialization fails.
     */
    template <typename T, typename... Args>
    std::shared_ptr<FileSystem> CreateAndMount(std::error_code& ec, const Path& mountPoint, Args&&... args) {
        ec.clear();
        static_assert(std::is_base_of_v<FileSystem, T>, "T must derive from FileSystem");
        std::shared_ptr<T> ptr;
        if constexpr (std::is_constructible_v<T, Args..., std::error_code&>) {
            ptr = std::make_shared<T>(std::forward<Args>(args)..., ec);
            if (ec) {
                return nullptr;
            }
        } else {
            ptr = std::make_shared<T>(std::forward<Args>(args)...);
        }

        Mount(mountPoint, ptr);
        return ptr;
    }

    /**
     * @brief Checks whether at least one file system is mounted at a virtual
     *        directory.
     *
     * @param mountPoint The virtual directory to query.
     *
     * @return true when at least one file system is mounted there.
     */
    [[nodiscard]] bool IsMounted(const Path& mountPoint) const;

    /**
     * @brief Checks whether a specific file system instance is mounted at a
     *        virtual directory.
     *
     * @param mountPoint The virtual directory to query.
     * @param ptr        The file system instance to look for.
     *
     * @return true when @p ptr is mounted at @p mountPoint.
     */
    [[nodiscard]] bool IsMounted(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr) const;

    /**
     * @brief Detaches all file systems mounted at a virtual directory.
     *
     * Empty virtual directories left behind are pruned from the tree.
     *
     * @param mountPoint The virtual directory to unmount.
     */
    void Unmount(const Path& mountPoint);

    /**
     * @brief Detaches a specific file system instance from a virtual
     *        directory.
     *
     * @param mountPoint The virtual directory to unmount.
     * @param ptr        The file system instance to detach.
     */
    void Unmount(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr);

    /**
     * @brief Returns the file systems mounted at a virtual directory.
     *
     * @param mountPoint The virtual directory to query.
     *
     * @return The mounted file systems in mounting order; empty when nothing
     *         is mounted there.
     */
    [[nodiscard]] std::vector<std::shared_ptr<FileSystem>> GetMountedFileSystems(const Path& mountPoint) const;

    /**
     * @brief Checks whether an entry exists at @p path.
     *
     * Implements FileSystem::Exists; virtual directories always exist.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when an entry exists at @p path.
     */
    [[nodiscard]] bool Exists(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Returns metadata about the entry at @p path.
     *
     * Implements FileSystem::Stat. Virtual directories report themselves as
     * readable, non-writable directories.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The entry's metadata; an empty FileInfo when no entry exists
     *         at @p path.
     */
    [[nodiscard]] FileInfo Stat(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Checks whether the entry at @p path is a directory.
     *
     * Implements FileSystem::IsDirectory; virtual directories count as
     * directories.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry at @p path is a directory.
     */
    [[nodiscard]] bool IsDirectory(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Checks whether the entry at @p path is a regular file.
     *
     * Implements FileSystem::IsRegularFile.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry at @p path is a regular file.
     */
    [[nodiscard]] bool IsRegularFile(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Checks whether the entry at @p path can be read.
     *
     * Implements FileSystem::IsReadable; virtual directories are readable.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry is readable.
     */
    [[nodiscard]] bool IsReadable(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Checks whether the entry at @p path can be written.
     *
     * Implements FileSystem::IsWritable; virtual directories are not
     * writable.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry is writable.
     */
    [[nodiscard]] bool IsWritable(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Returns the size of the entry at @p path in bytes.
     *
     * Implements FileSystem::Size.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The size in bytes; zero when the entry does not exist or is
     *         not a regular file.
     */
    [[nodiscard]] std::uint64_t Size(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Lists the entries visible at @p path.
     *
     * Implements FileSystem::List. The result merges the entries of all
     * candidate mounts with the virtual child directories, deduplicated and
     * sorted.
     *
     * @param path The virtual path of the directory to list.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The merged entry names.
     */
    [[nodiscard]] std::vector<std::string> List(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Creates a new file at @p path.
     *
     * Implements FileSystem::CreateFile.
     *
     * @param path The virtual path of the file to create.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void CreateFile(const Path& path, std::error_code& ec) override;

    /**
     * @brief Creates a single directory at @p path.
     *
     * Implements FileSystem::CreateDirectory.
     *
     * @param path The virtual path of the directory to create.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void CreateDirectory(const Path& path, std::error_code& ec) override;

    /**
     * @brief Creates the directory at @p path and any missing parent
     *        directories.
     *
     * Implements FileSystem::CreateDirectories.
     *
     * @param path The virtual path of the directory to create.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void CreateDirectories(const Path& path, std::error_code& ec) override;

    /**
     * @brief Deletes the file at @p path.
     *
     * Implements FileSystem::DeleteFile.
     *
     * @param path The virtual path of the file to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void DeleteFile(const Path& path, std::error_code& ec) override;

    /**
     * @brief Deletes the empty directory at @p path.
     *
     * Implements FileSystem::DeleteDirectory.
     *
     * @param path The virtual path of the directory to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void DeleteDirectory(const Path& path, std::error_code& ec) override;

    /**
     * @brief Recursively deletes the directory at @p path and everything it
     *        contains.
     *
     * Implements FileSystem::DeleteDirectories.
     *
     * @param path The virtual path of the directory to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void DeleteDirectories(const Path& path, std::error_code& ec) override;

    void Copy(const Path& src, const Path& to, CopyOption copyOptions, std::error_code& ec) override;

    void Rename(const Path& src, const Path& to, std::error_code& ec) override;

    /**
     * @brief Opens the file at @p path and returns a handle to it.
     *
     * Implements FileSystem::Open. Read requests are resolved against
     * entries that actually exist; write requests may also target pure
     * virtual directories, which then fail with
     * std::errc::read_only_file_system.
     *
     * @param path        The virtual path of the file to open.
     * @param openOptions Flags controlling access mode and creation.
     * @param ec          On failure, set to an error code describing the
     *                    problem.
     *
     * @return A handle to the open file, or nullptr on failure.
     */
    [[nodiscard]] std::unique_ptr<FileHandle>
    Open(const Path& path, OpenOption openOptions, std::error_code& ec) override;

    /**
     * @brief Brings the base-class convenience overload Open(path, ec) into
     *        scope for this class.
     */
    using FileSystem::Open;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace slimenano::filesystem

#endif
