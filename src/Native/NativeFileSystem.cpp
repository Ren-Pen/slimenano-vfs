/**
 * @file NativeFileSystem.cpp
 * @brief Implementation of the native file system.
 */

#include <slimenano/vfs/NativeFileSystem.h>

#include "NativeFileHandle.h"

#include <filesystem>
#include <chrono>
#include <fstream>
#include <string>
#include <string_view>

namespace slimenano::filesystem {

namespace {

namespace fs = std::filesystem;
namespace chrono = std::chrono;

/**
 * @brief Converts a UTF-8 string view to a native filesystem path.
 *
 * The conversion goes through the C++20 char8_t path constructor so that
 * non-ASCII characters are preserved on Windows.
 *
 * @param utf8 The UTF-8 path to convert.
 *
 * @return The native path; empty when @p utf8 is empty.
 */
fs::path FromUtf8(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    return fs::path(std::u8string_view(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()));
}

/**
 * @brief Converts a native filesystem path to a UTF-8 string.
 *
 * @param native The native path to convert.
 *
 * @return The path as a UTF-8 string.
 */
std::string ToUtf8(const fs::path& native) {
    const auto utf8 = native.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

/**
 * @brief Maps a virtual path to a native path below a root directory.
 *
 * The virtual path is made absolute, and the leading root separator is
 * stripped before appending to the root.
 *
 * @param root Native root directory.
 * @param path Virtual path to map.
 *
 * @return The native path below @p root.
 */
fs::path ToNativePath(const fs::path& root, const Path& path) {
    const auto absolutePath = path.ToAbsolute("/");
    const auto rel = absolutePath.String().substr(1);
    return root / FromUtf8(rel);
}

} // namespace

struct NativeFileSystem::Impl {
    explicit Impl(fs::path root) : m_root(root) {}

    /** @brief Native path of the root directory. */
    fs::path m_root;
};

/**
 * @brief Constructs a native file system rooted at @p root.
 *
 * @param root UTF-8 path of the backing directory on disk.
 */
NativeFileSystem::NativeFileSystem(std::string_view root, std::error_code& ec) :
    m_pImpl(std::make_unique<Impl>(FromUtf8(root))) {
    ec.clear();
    auto is_dir = fs::is_directory(m_pImpl->m_root, ec);
    if (ec) {
        return;
    }
    if (!is_dir) {
        ec = std::make_error_code(std::errc::not_a_directory);
    }
}

NativeFileSystem::~NativeFileSystem() = default;

/**
 * @brief Returns metadata about the entry at @p path.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return The entry's metadata; an empty FileInfo when no entry exists at
 *         @p path.
 */
FileInfo NativeFileSystem::Stat(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    const auto status = fs::status(nativePath, ec);
    if (ec) {
        return {};
    }

    const auto type = status.type();

    if (type == std::filesystem::file_type::none) {
        ec = std::make_error_code(std::errc::io_error);
        return {};
    }

    if (type == std::filesystem::file_type::not_found) {
        return {};
    }

    FileInfo info{};

    switch (type) {
    case fs::file_type::regular:
        info.type = FileType::Regular;
        break;
    case fs::file_type::directory:
        info.type = FileType::Directory;
        break;
    default:
        // symlink / block / character / fifo / socket / unknown
        info.type = FileType::Other;
        break;
    }

    const auto perms = status.permissions();
    info.readable = (perms & fs::perms::owner_read) != fs::perms::none;
    info.writable = (perms & fs::perms::owner_write) != fs::perms::none;

    if (info.type == FileType::Regular) {
        std::error_code sizeEc;
        const auto sz = fs::file_size(nativePath, sizeEc);
        if (!sizeEc) {
            info.size = static_cast<std::uint64_t>(sz);
        }
    }

    std::error_code timeEc;
    const auto ftime = fs::last_write_time(nativePath, timeEc);
    if (!timeEc) {
        auto ctime = chrono::clock_cast<chrono::system_clock>(ftime);
        info.modifiedTime = chrono::duration_cast<chrono::milliseconds>(ctime.time_since_epoch()).count();
    }

    return info;
}

/**
 * @brief Checks whether an entry exists at @p path.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when an entry exists at @p path.
 */
bool NativeFileSystem::Exists(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    return fs::exists(nativePath, ec);
}

/**
 * @brief Checks whether the entry at @p path is a directory.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry at @p path is a directory.
 */
bool NativeFileSystem::IsDirectory(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    return fs::is_directory(nativePath, ec);
}

/**
 * @brief Checks whether the entry at @p path is a regular file.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry at @p path is a regular file.
 */
bool NativeFileSystem::IsRegularFile(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    return fs::is_regular_file(nativePath, ec);
}

/**
 * @brief Checks whether the entry at @p path can be read.
 *
 * Readability is derived from the owner read permission bit.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry is readable.
 */
bool NativeFileSystem::IsReadable(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    const auto status = fs::status(nativePath, ec);
    if (ec) {
        return false;
    }
    const auto perms = status.permissions();
    return (perms & fs::perms::owner_read) != fs::perms::none;
}

/**
 * @brief Checks whether the entry at @p path can be written.
 *
 * Writability is derived from the owner write permission bit.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry is writable.
 */
bool NativeFileSystem::IsWritable(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    const auto status = fs::status(nativePath, ec);
    if (ec) {
        return false;
    }
    const auto perms = status.permissions();
    return (perms & fs::perms::owner_write) != fs::perms::none;
}

/**
 * @brief Returns the size of the entry at @p path in bytes.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return The size in bytes; zero when the entry does not exist or is not a
 *         regular file.
 */
std::uint64_t NativeFileSystem::Size(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    const auto size = fs::file_size(nativePath, ec);
    if (ec) {
        return 0;
    }
    return static_cast<std::uint64_t>(size);
}

/**
 * @brief Lists the names of the entries directly contained in @p path.
 *
 * The names are returned in the iteration order of the underlying
 * directory, without sorting.
 *
 * @param path The virtual path of the directory to list.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return The entry names as UTF-8 strings, excluding "." and "..".
 */
std::vector<std::string> NativeFileSystem::List(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);
    auto iter = fs::directory_iterator{nativePath, ec};
    if (ec) {
        return {};
    }

    std::vector<std::string> names;
    for (const auto& entry : iter) {
        names.push_back(ToUtf8(entry.path().filename()));
    }
    return names;
}

/**
 * @brief Creates a new file at @p path.
 *
 * Does nothing when the file already exists.
 *
 * @param path The virtual path of the file to create.
 * @param ec   On failure, set to an error code describing the problem.
 */
void NativeFileSystem::CreateFile(const Path& path, std::error_code& ec) {
    ec.clear();
    const auto nativePath = ToNativePath(m_pImpl->m_root, path);

    std::error_code existsEc;
    if (fs::exists(nativePath, existsEc)) {
        return;
    }
    if (existsEc) {
        ec = existsEc;
        return;
    }
    std::ofstream stream{nativePath, std::ios::out | std::ios::binary};
    if (!stream) {
        ec = std::make_error_code(std::errc::io_error);
    }
}

/**
 * @brief Creates a single directory at @p path.
 *
 * Does nothing when the directory already exists.
 *
 * @param path The virtual path of the directory to create.
 * @param ec   On failure, set to an error code describing the problem.
 */
void NativeFileSystem::CreateDirectory(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::create_directory(ToNativePath(m_pImpl->m_root, path), ec);
}

/**
 * @brief Creates the directory at @p path and any missing parent
 *        directories.
 *
 * @param path The virtual path of the directory to create.
 * @param ec   On failure, set to an error code describing the problem.
 */
void NativeFileSystem::CreateDirectories(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::create_directories(ToNativePath(m_pImpl->m_root, path), ec);
}

/**
 * @brief Deletes the file at @p path.
 *
 * Does nothing when the file does not exist.
 *
 * @param path The virtual path of the file to delete.
 * @param ec   On failure, set to an error code describing the problem.
 */
void NativeFileSystem::DeleteFile(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::remove(ToNativePath(m_pImpl->m_root, path), ec);
}

/**
 * @brief Deletes the empty directory at @p path.
 *
 * Does nothing when the directory does not exist, and fails when it is not
 * empty.
 *
 * @param path The virtual path of the directory to delete.
 * @param ec   On failure, set to an error code describing the problem.
 */
void NativeFileSystem::DeleteDirectory(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::remove(ToNativePath(m_pImpl->m_root, path), ec);
}

/**
 * @brief Recursively deletes the directory at @p path and everything it
 *        contains.
 *
 * @param path The virtual path of the directory to delete.
 * @param ec   On failure, set to an error code describing the problem.
 */
void NativeFileSystem::DeleteDirectories(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::remove_all(ToNativePath(m_pImpl->m_root, path), ec);
}

void NativeFileSystem::Copy(const Path& src, const Path& to, CopyOption copyOptions, std::error_code& ec) {
    ec.clear();
    auto srcPath = ToNativePath(m_pImpl->m_root, src);
    auto toPath = ToNativePath(m_pImpl->m_root, to);
    fs::copy_options fsOpts = fs::copy_options::none;
    if ((copyOptions & CopyOption::Recursive) != CopyOption::None) {
        fsOpts |= fs::copy_options::recursive;
    }
    if ((copyOptions & CopyOption::OverwriteExisting) != CopyOption::None) {
        fsOpts |= fs::copy_options::overwrite_existing;
    }
    if ((copyOptions & CopyOption::SkipExisting) != CopyOption::None) {
        fsOpts |= fs::copy_options::skip_existing;
    }
    if ((copyOptions & CopyOption::UpdateExisting) != CopyOption::None) {
        fsOpts |= fs::copy_options::update_existing;
    }
    if ((copyOptions & CopyOption::DirectoriesOnly) != CopyOption::None) {
        fsOpts |= fs::copy_options::directories_only;
    }
    fs::copy(srcPath, toPath, fsOpts, ec);
}

void NativeFileSystem::Rename(const Path& src, const Path& to, std::error_code& ec) {
    ec.clear();
    auto srcPath = ToNativePath(m_pImpl->m_root, src);
    auto toPath = ToNativePath(m_pImpl->m_root, to);
    fs::rename(srcPath, toPath, ec);
}

/**
 * @brief Opens the file at @p path and returns a handle to it.
 *
 * @param path        The virtual path of the file to open.
 * @param openOptions Flags controlling access mode and creation.
 * @param ec          On failure, set to an error code describing the
 *                    problem.
 *
 * @return A handle to the open file, or nullptr on failure.
 */
std::unique_ptr<FileHandle> NativeFileSystem::Open(const Path& path, OpenOption openOptions, std::error_code& ec) {
    ec.clear();
    auto handle = std::make_unique<NativeFileHandle>(ToUtf8(ToNativePath(m_pImpl->m_root, path)), openOptions, ec);
    if (ec) {
        return {};
    }
    return handle;
}

} // namespace slimenano::filesystem
