#ifndef SLIMENANO_VFS_INCLUDE_VFS_FILE_INFO_H
#define SLIMENANO_VFS_INCLUDE_VFS_FILE_INFO_H

#include <cstdint>

namespace slimenano::filesystem {

enum class FileType : std::int8_t {
    None = 0,
    Regular,
    Directory,
    Other
};

struct FileInfo {
    FileType type{FileType::None};
    bool readable{false};
    bool writable{false};
    std::uint64_t size{0};

    [[nodiscard]] bool Exists() const noexcept { return type != FileType::None; }
    [[nodiscard]] bool IsDirectory() const noexcept { return type == FileType::Directory; }
    [[nodiscard]] bool IsRegularFile() const noexcept { return type == FileType::Regular; }
};

} // namespace slimenano::filesystem

#endif
