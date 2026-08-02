#include <slimenano/vfs/NativeFileSystem.h>

#include "NativeFileHandle.h"

#include <chrono>
#include <fstream>
#include <string>
#include <string_view>

namespace slimenano::filesystem {

namespace fs = std::filesystem;
namespace chrono = std::chrono;

namespace {

fs::path FromUtf8(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    return fs::path(std::u8string_view(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()));
}

std::string ToUtf8(const fs::path& native) {
    const auto utf8 = native.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

fs::path ToNativePath(const fs::path& root, const Path& path) {
    const auto absolutePath = path.ToAbsolute("/");
    const auto rel = absolutePath.String().substr(1);
    return root / FromUtf8(rel);
}

} // namespace

NativeFileSystem::NativeFileSystem(std::string_view root) : m_root(FromUtf8(root)) {
}

void NativeFileSystem::Initialize(std::error_code& ec) const {
    ec.clear();
    auto is_dir = fs::is_directory(m_root, ec);
    if (ec) {
        return;
    }
    if (!is_dir) {
        ec = std::make_error_code(std::errc::not_a_directory);
    }
}

FileInfo NativeFileSystem::Stat(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
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

bool NativeFileSystem::Exists(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
    return fs::exists(nativePath, ec);
}

bool NativeFileSystem::IsDirectory(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
    return fs::is_directory(nativePath, ec);
}

bool NativeFileSystem::IsRegularFile(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
    return fs::is_regular_file(nativePath, ec);
}

bool NativeFileSystem::IsReadable(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
    const auto status = fs::status(nativePath, ec);
    if (ec) {
        return false;
    }
    const auto perms = status.permissions();
    return (perms & fs::perms::owner_read) != fs::perms::none;
}

bool NativeFileSystem::IsWritable(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
    const auto status = fs::status(nativePath, ec);
    if (ec) {
        return false;
    }
    const auto perms = status.permissions();
    return (perms & fs::perms::owner_write) != fs::perms::none;
}

std::uint64_t NativeFileSystem::Size(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
    const auto size = fs::file_size(nativePath, ec);
    if (ec) {
        return 0;
    }
    return static_cast<std::uint64_t>(size);
}

std::vector<std::string> NativeFileSystem::List(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);
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

void NativeFileSystem::CreateFile(const Path& path, std::error_code& ec) {
    ec.clear();
    const auto nativePath = ToNativePath(m_root, path);

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

void NativeFileSystem::CreateDirectory(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::create_directory(ToNativePath(m_root, path), ec);
}

void NativeFileSystem::CreateDirectories(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::create_directories(ToNativePath(m_root, path), ec);
}

void NativeFileSystem::DeleteFile(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::remove(ToNativePath(m_root, path), ec);
}

void NativeFileSystem::DeleteDirectory(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::remove(ToNativePath(m_root, path), ec);
}

void NativeFileSystem::DeleteDirectories(const Path& path, std::error_code& ec) {
    ec.clear();
    fs::remove_all(ToNativePath(m_root, path), ec);
}

std::unique_ptr<FileHandle> NativeFileSystem::Open(const Path& path, OpenOption openOptions, std::error_code& ec) {
    ec.clear();
    auto handle = std::make_unique<NativeFileHandle>(ToNativePath(m_root, path), openOptions, ec);
    if (ec) {
        return nullptr;
    }
    return handle;
}

} // namespace slimenano::filesystem
