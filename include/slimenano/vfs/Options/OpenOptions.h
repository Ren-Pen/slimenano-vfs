/**
 * @file OpenOptions.h
 * @brief Open options for file-system open operations.
 */

#ifndef SLIMENANO_VFS_INCLUDE_VFS_OPTIONS_OPEN_OPTIONS_H
#define SLIMENANO_VFS_INCLUDE_VFS_OPTIONS_OPEN_OPTIONS_H

#include <cstdint>
#include <type_traits>

namespace slimenano::filesystem {

/**
 * @brief Flags controlling how FileSystem::Open() opens a file.
 *
 * Flags can be combined with operator|(), e.g.
 * OpenOption::Read | OpenOption::Create. Read access is requested with
 * OpenOption::Read; write access is requested with OpenOption::Append,
 * OpenOption::Truncate or OpenOption::Create. When no flag requests any
 * access at all, the open operation fails with std::errc::invalid_argument.
 */
enum class OpenOption : std::uint8_t {
    /**
     * @brief No access requested.
     *
     * Combining None with other flags has no effect; on its own it makes the
     * open operation fail.
     */
    None = 0,
    /**
     * @brief Enables reading from the file.
     */
    Read = 1 << 0,
    /**
     * @brief Enables writing in append mode; the write position starts at
     *        the end of the file.
     */
    Append = 1 << 1,
    /**
     * @brief Enables writing and truncates the file to zero bytes when it is
     *        opened.
     */
    Truncate = 1 << 2,
    /**
     * @brief Enables writing and creates the file if it does not exist yet;
     *        an existing file is opened as-is.
     */
    Create = 1 << 3
};

/**
 * @brief Combines two option flag sets.
 *
 * @param lhs The first flag set.
 * @param rhs The second flag set.
 *
 * @return The bitwise combination of both flag sets.
 */
constexpr OpenOption operator|(OpenOption lhs, OpenOption rhs) noexcept {

    return static_cast<OpenOption>(
        static_cast<std::underlying_type_t<OpenOption>>(lhs) | static_cast<std::underlying_type_t<OpenOption>>(rhs)
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
constexpr OpenOption operator&(OpenOption lhs, OpenOption rhs) noexcept {
    return static_cast<OpenOption>(
        static_cast<std::underlying_type_t<OpenOption>>(lhs) & static_cast<std::underlying_type_t<OpenOption>>(rhs)
    );
}

/**
 * @brief Complements an option flag set.
 *
 * @param rhs The flag set to complement.
 *
 * @return All flags except those present in @p rhs.
 */
constexpr OpenOption operator~(OpenOption rhs) noexcept {
    return static_cast<OpenOption>(~static_cast<std::underlying_type_t<OpenOption>>(rhs));
}

} // namespace slimenano::filesystem

#endif
