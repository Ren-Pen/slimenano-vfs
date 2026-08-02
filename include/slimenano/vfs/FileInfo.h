/**
 * @file FileInfo.h
 * @brief Metadata describing a file-system entry.
 */

#ifndef SLIMENANO_VFS_INCLUDE_VFS_FILE_INFO_H
#define SLIMENANO_VFS_INCLUDE_VFS_FILE_INFO_H

#include <cstdint>

namespace slimenano::filesystem {

/**
 * @brief Kind of a file-system entry.
 */
enum class FileType : std::int8_t {
    /** @brief No entry exists at the queried path. */
    None = 0,
    /** @brief A regular file. */
    Regular,
    /** @brief A directory. */
    Directory,
    /** @brief Any other kind of entry (symlink, device, pipe, socket, ...). */
    Other
};

/**
 * @brief Metadata about a file-system entry, produced by FileSystem::Stat().
 *
 * Time fields are in milliseconds since the Unix epoch. The permission
 * fields reflect the current user's read/write access to the entry.
 */
struct FileInfo {
    /** @brief Kind of the entry; FileType::None means no entry exists. */
    FileType type{FileType::None};
    /** @brief Whether the entry is readable by the current user. */
    bool readable{false};
    /** @brief Whether the entry is writable by the current user. */
    bool writable{false};
    /** @brief Size of the entry in bytes; meaningful for regular files. */
    std::uint64_t size{0};

    /** @brief Creation time, in milliseconds since the Unix epoch. */
    std::int64_t createdTime{0};
    /** @brief Last modification time, in milliseconds since the Unix epoch. */
    std::int64_t modifiedTime{0};
    /** @brief Last access time, in milliseconds since the Unix epoch. */
    std::int64_t accessedTime{0};

    /**
     * @brief Checks whether an entry exists at the queried path.
     *
     * @return true when type is not FileType::None.
     */
    [[nodiscard]] bool Exists() const noexcept { return type != FileType::None; }
    /**
     * @brief Checks whether the entry is a directory.
     *
     * @return true when type is FileType::Directory.
     */
    [[nodiscard]] bool IsDirectory() const noexcept { return type == FileType::Directory; }
    /**
     * @brief Checks whether the entry is a regular file.
     *
     * @return true when type is FileType::Regular.
     */
    [[nodiscard]] bool IsRegularFile() const noexcept { return type == FileType::Regular; }
};

} // namespace slimenano::filesystem

#endif
