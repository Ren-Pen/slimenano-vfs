#ifndef SLIMENANO_VFS_INCLUDE_VFS_OPTIONS_OPEN_OPTIONS_H
#define SLIMENANO_VFS_INCLUDE_VFS_OPTIONS_OPEN_OPTIONS_H

#include <cstdint>
#include <type_traits>

namespace slimenano::filesystem {

enum class OpenOption : std::uint8_t {
    None = 0,          //
    Read = 1 << 0,     // Enable read
    Append = 1 << 1,   // Enable write, append mode; write position defaults to end of file
    Truncate = 1 << 2, // Enable write, truncate existing file to zero bytes
    Create = 1 << 3    // Enable write, create the file if it does not exist, otherwise open the existing file
};

constexpr OpenOption operator|(OpenOption lhs, OpenOption rhs) noexcept {

    return static_cast<OpenOption>(
        static_cast<std::underlying_type_t<OpenOption>>(lhs) | static_cast<std::underlying_type_t<OpenOption>>(rhs)
    );
}
constexpr OpenOption operator&(OpenOption lhs, OpenOption rhs) noexcept {
    return static_cast<OpenOption>(
        static_cast<std::underlying_type_t<OpenOption>>(lhs) & static_cast<std::underlying_type_t<OpenOption>>(rhs)
    );
}

constexpr OpenOption operator~(OpenOption rhs) noexcept {
    return static_cast<OpenOption>(~static_cast<std::underlying_type_t<OpenOption>>(rhs));
}

} // namespace slimenano::filesystem

#endif
