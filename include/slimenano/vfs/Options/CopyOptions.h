
#ifndef SLIMENANO_VFS_INCLUDE_VFS_OPTIONS_COPY_OPTIONS_H
#define SLIMENANO_VFS_INCLUDE_VFS_OPTIONS_COPY_OPTIONS_H

#include <cstdint>
#include <type_traits>

namespace slimenano::filesystem {

enum class CopyOption : std::uint8_t {
    None = 0,
    Recursive = 1 << 0,
    OverwriteExisting = 1 << 1,
    SkipExisting = 1 << 2,
    UpdateExisting = 1 << 3,
    DirectoriesOnly = 1 << 4
};

/**
 * @brief Combines two option flag sets.
 *
 * @param lhs The first flag set.
 * @param rhs The second flag set.
 *
 * @return The bitwise combination of both flag sets.
 */
constexpr CopyOption operator|(CopyOption lhs, CopyOption rhs) noexcept {

    return static_cast<CopyOption>(
        static_cast<std::underlying_type_t<CopyOption>>(lhs) | static_cast<std::underlying_type_t<CopyOption>>(rhs)
    );
}
/**
 * @brief Tests which flags are shared by two option flag sets.
 *
 * @param lhs The first flag set.
 * @param rhs The second flag set.
 *
 * @return The flags present in both sets.
 */
constexpr CopyOption operator&(CopyOption lhs, CopyOption rhs) noexcept {
    return static_cast<CopyOption>(
        static_cast<std::underlying_type_t<CopyOption>>(lhs) & static_cast<std::underlying_type_t<CopyOption>>(rhs)
    );
}

/**
 * @brief Complements an option flag set.
 *
 * @param rhs The flag set to complement.
 *
 * @return All flags except those present in @p rhs.
 */
constexpr CopyOption operator~(CopyOption rhs) noexcept {
    return static_cast<CopyOption>(~static_cast<std::underlying_type_t<CopyOption>>(rhs));
}

} // namespace slimenano::filesystem

#endif
