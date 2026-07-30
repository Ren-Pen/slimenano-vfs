#include <slimenano/vfs/NativeFileSystem.h>

#include <chrono>

namespace slimenano::filesystem {

namespace fs = std::filesystem;
namespace chrono = std::chrono;

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
    const auto nativePath = ToNativePath(path);
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
    const auto nativePath = ToNativePath(path);
    return fs::exists(nativePath, ec);
}

bool NativeFileSystem::IsDirectory(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(path);
    return fs::is_directory(nativePath, ec);
}

bool NativeFileSystem::IsRegularFile(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(path);
    return fs::is_regular_file(nativePath, ec);
}

bool NativeFileSystem::IsReadable(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(path);
    const auto status = fs::status(nativePath, ec);
    if (ec) {
        return false;
    }
    const auto perms = status.permissions();
    return (perms & fs::perms::owner_read) != fs::perms::none;
}

bool NativeFileSystem::IsWritable(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(path);
    const auto status = fs::status(nativePath, ec);
    if (ec) {
        return false;
    }
    const auto perms = status.permissions();
    return (perms & fs::perms::owner_write) != fs::perms::none;
}

std::uint64_t NativeFileSystem::Size(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(path);
    const auto size = fs::file_size(nativePath, ec);
    if (ec) {
        return 0;
    }
    return static_cast<std::uint64_t>(size);
}

std::vector<std::string> NativeFileSystem::List(const Path& path, std::error_code& ec) const {
    ec.clear();
    const auto nativePath = ToNativePath(path);
    auto iter = fs::directory_iterator{nativePath, ec};
    if (ec) {
        return {};
    }

    std::vector<std::string> names;
    for (const auto& entry : iter) {
        names.push_back(entry.path().filename().u8string());
    }
}

} // namespace slimenano::filesystem
