/**
 * @file FileSystem.h
 * @brief Abstract interface for a file system.
 */

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
#include <slimenano/vfs/Options/CopyOptions.h>
#include <slimenano/vfs/Options/OpenOptions.h>

namespace slimenano::filesystem {

/**
 * @brief Abstract interface for a file system.
 *
 * A FileSystem exposes a uniform, path-based API for querying and modifying
 * files and directories. Concrete implementations map these operations onto
 * a backing store, e.g. the native OS file system (see NativeFileSystem) or
 * a hierarchy of mounted sub-file-systems (see VirtualFileSystem).
 *
 * Every operation reports failure through its std::error_code output
 * parameter instead of throwing exceptions; a cleared error code means
 * success. Instances are non-copyable and non-movable.
 */
class FileSystem {
public:
    FileSystem() = default;
    virtual ~FileSystem() = default;

    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;
    FileSystem(FileSystem&&) noexcept = delete;
    FileSystem& operator=(FileSystem&&) noexcept = delete;

    /**
     * @brief Returns metadata about the entry at @p path.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The entry's metadata. When no entry exists at @p path, returns
     *         a FileInfo whose type is FileType::None.
     */
    [[nodiscard]] virtual FileInfo Stat(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Checks whether an entry exists at @p path.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when an entry exists at @p path.
     */
    [[nodiscard]] virtual bool Exists(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Checks whether the entry at @p path is a directory.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry at @p path is a directory.
     */
    [[nodiscard]] virtual bool IsDirectory(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Checks whether the entry at @p path is a regular file.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry at @p path is a regular file.
     */
    [[nodiscard]] virtual bool IsRegularFile(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Checks whether the entry at @p path can be read.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry is readable.
     */
    [[nodiscard]] virtual bool IsReadable(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Checks whether the entry at @p path can be written.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return true when the entry is writable.
     */
    [[nodiscard]] virtual bool IsWritable(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Returns the size of the entry at @p path in bytes.
     *
     * @param path The virtual path to query.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The size in bytes; zero when the entry does not exist or is
     *         not a regular file.
     */
    [[nodiscard]] virtual std::uint64_t Size(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Lists the names of the entries directly contained in @p path.
     *
     * @param path The virtual path of the directory to list.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The entry names as UTF-8 strings, excluding "." and "..".
     */
    [[nodiscard]] virtual std::vector<std::string> List(const Path& path, std::error_code& ec) const = 0;

    /**
     * @brief Creates a new file at @p path.
     *
     * Does nothing when a file already exists at @p path.
     *
     * @param path The virtual path of the file to create.
     * @param ec   On failure, set to an error code describing the problem.
     */
    virtual void CreateFile(const Path& path, std::error_code& ec) = 0;

    /**
     * @brief Creates a single directory at @p path.
     *
     * Does nothing when the directory already exists. Fails when a parent
     * directory does not exist.
     *
     * @param path The virtual path of the directory to create.
     * @param ec   On failure, set to an error code describing the problem.
     */
    virtual void CreateDirectory(const Path& path, std::error_code& ec) = 0;

    /**
     * @brief Creates the directory at @p path and any missing parent
     *        directories.
     *
     * @param path The virtual path of the directory to create.
     * @param ec   On failure, set to an error code describing the problem.
     */
    virtual void CreateDirectories(const Path& path, std::error_code& ec) = 0;

    /**
     * @brief Deletes the file at @p path.
     *
     * Does nothing when the file does not exist.
     *
     * @param path The virtual path of the file to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    virtual void DeleteFile(const Path& path, std::error_code& ec) = 0;

    /**
     * @brief Deletes the empty directory at @p path.
     *
     * Does nothing when the directory does not exist. Fails when the
     * directory is not empty.
     *
     * @param path The virtual path of the directory to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    virtual void DeleteDirectory(const Path& path, std::error_code& ec) = 0;

    /**
     * @brief Recursively deletes the directory at @p path and everything it
     *        contains.
     *
     * Does nothing when the directory does not exist.
     *
     * @param path The virtual path of the directory to delete.
     * @param ec   On failure, set to an error code describing the problem.
     */
    virtual void DeleteDirectories(const Path& path, std::error_code& ec) = 0;

    virtual void Copy(const Path& src, const Path& to, CopyOption copyOptions, std::error_code& ec) = 0;

    virtual void Rename(const Path& src, const Path& to, std::error_code& ec) = 0;

    /**
     * @brief Opens the file at @p path and returns a handle to it.
     *
     * Access is controlled by @p openOptions: read access requires
     * OpenOption::Read, write access requires OpenOption::Append,
     * OpenOption::Truncate or OpenOption::Create. With OpenOption::Create a
     * missing file is created; without it, opening a missing file fails with
     * std::errc::no_such_file_or_directory.
     *
     * @param path        The virtual path of the file to open.
     * @param openOptions Flags controlling access mode and creation.
     * @param ec          On failure, set to an error code describing the
     *                    problem.
     *
     * @return A handle to the open file, or nullptr on failure.
     */
    [[nodiscard]] virtual std::unique_ptr<FileHandle>
    Open(const Path& path, OpenOption openOptions, std::error_code& ec) = 0;

    /**
     * @brief Opens the file at @p path for reading.
     *
     * Convenience overload equivalent to Open(path, OpenOption::Read, ec).
     *
     * @param path The virtual path of the file to open.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return A handle to the open file, or nullptr on failure.
     */
    [[nodiscard]] std::unique_ptr<FileHandle> Open(const Path& path, std::error_code& ec) {
        return Open(path, OpenOption::Read, ec);
    }
};

} // namespace slimenano::filesystem

#endif
