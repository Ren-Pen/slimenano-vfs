/**
 * @file NativeFileSystem.h
 * @brief File system implementation backed by the native OS file system.
 */

#ifndef SLIMENANO_VFS_INCLUDE_VFS_NATIVE_FILE_SYSTEM_H
#define SLIMENANO_VFS_INCLUDE_VFS_NATIVE_FILE_SYSTEM_H

#include <slimenano/vfs/FileSystem.h>

namespace slimenano::filesystem {

/**
 * @brief FileSystem implementation backed by the native operating system
 *        file system.
 *
 * All paths handled by a NativeFileSystem are virtual paths relative to its
 * root: the root path "/" maps to the directory given at construction, and
 * every other virtual path is resolved below that directory. Conversion
 * between the library's UTF-8 Path representation and std::filesystem paths
 * is performed losslessly, so non-ASCII names are preserved.
 *
 * Instances are non-copyable and non-movable.
 */
class NativeFileSystem final : public FileSystem {

public:
    /**
     * @brief Constructs a native file system rooted at @p root.
     *
     * @param root UTF-8 path of the backing directory on disk.
     */
    explicit NativeFileSystem(std::string_view root, std::error_code& ec);
    ~NativeFileSystem();

    NativeFileSystem(const NativeFileSystem&) = delete;
    NativeFileSystem(NativeFileSystem&&) = delete;
    NativeFileSystem& operator=(const NativeFileSystem&) = delete;
    NativeFileSystem& operator=(NativeFileSystem&&) = delete;

    /**
     * @brief Returns metadata about the entry at @p path.
     *
     * Implements FileSystem::Stat. Permission and time information is taken
     * from the owner permission bits and the file system timestamps.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The entry's metadata; an empty FileInfo when no entry exists
     *         at @p path.
     */
    [[nodiscard]] FileInfo Stat(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Checks whether an entry exists at @p path.
     *
     * Implements FileSystem::Exists.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when an entry exists at @p path.
     */
    [[nodiscard]] bool Exists(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Checks whether the entry at @p path is a directory.
     *
     * Implements FileSystem::IsDirectory.
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
     * Implements FileSystem::IsReadable. Readability is derived from the
     * owner read permission bit.
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
     * Implements FileSystem::IsWritable. Writability is derived from the
     * owner write permission bit.
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
     * @brief Lists the names of the entries directly contained in @p path.
     *
     * Implements FileSystem::List. The names are returned in the iteration
     * order of the underlying directory, without sorting.
     *
     * @param path The virtual path of the directory to list.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The entry names as UTF-8 strings, excluding "." and "..".
     */
    [[nodiscard]] std::vector<std::string> List(const Path& path, std::error_code& ec) const override;

    /**
     * @brief Creates a new file at @p path.
     *
     * Implements FileSystem::CreateFile; does nothing when the file already
     * exists.
     *
     * @param path The virtual path of the file to create.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void CreateFile(const Path& path, std::error_code& ec) override;

    /**
     * @brief Creates a single directory at @p path.
     *
     * Implements FileSystem::CreateDirectory; does nothing when the
     * directory already exists.
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
     * Implements FileSystem::DeleteFile; does nothing when the file does not
     * exist.
     *
     * @param path The virtual path of the file to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void Delete(const Path& path, std::error_code& ec) override;

    /**
     * @brief Recursively deletes the directory at @p path and everything it
     *        contains.
     *
     * Implements FileSystem::DeleteDirectories.
     *
     * @param path The virtual path of the directory to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    void DeleteAll(const Path& path, std::error_code& ec) override;

    void Copy(const Path& src, const Path& to, CopyOption copyOptions, std::error_code& ec) override;

    void Rename(const Path& src, const Path& to, std::error_code& ec) override;

    /**
     * @brief Opens the file at @p path and returns a handle to it.
     *
     * Implements FileSystem::Open.
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
    std::unique_ptr<Impl> m_pImpl;
};

} // namespace slimenano::filesystem

#endif
