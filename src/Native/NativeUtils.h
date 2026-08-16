#ifndef SLIMENANO_VFS_SRC_NATIVE_NATIVE_UTILS_H
#define SLIMENANO_VFS_SRC_NATIVE_NATIVE_UTILS_H

#include <cstdio>
#include <string_view>
#include <filesystem>
#include <slimenano/vfs/Path.h>

#if defined(_WIN32)
#include <string>
#endif

namespace slimenano::filesystem {

#if defined(_WIN32)

#define PortableFSeek _fseeki64
#define PortableFTell _ftelli64

#else

#define PortableFSeek fseeko
#define PortableFTell ftello

#endif

#if defined(_WIN32)
std::wstring Utf8ToWchar(std::string_view str);
#endif

std::FILE* PortableFOpen(std::string_view path, std::string_view mode);

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
std::filesystem::path Utf8ToNativePath(std::string_view utf8);

/**
 * @brief Converts a native filesystem path to a UTF-8 string.
 *
 * @param native The native path to convert.
 *
 * @return The path as a UTF-8 string.
 */
std::string NativePathToUtf8(const std::filesystem::path& native);

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
std::filesystem::path ToNativePath(const std::filesystem::path& root, const Path& path);

} // namespace slimenano::filesystem

#endif
