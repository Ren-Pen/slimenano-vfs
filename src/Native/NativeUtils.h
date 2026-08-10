#ifndef SLIMENANO_VFS_SRC_NATIVE_NATIVE_UTILS_H
#define SLIMENANO_VFS_SRC_NATIVE_NATIVE_UTILS_H

#include <cstdio>

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

} // namespace slimenano::filesystem

#endif
