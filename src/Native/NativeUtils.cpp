#include "NativeUtils.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace slimenano::filesystem {

#if defined(_WIN32)

std::wstring Utf8ToWchar(std::string_view str) {
    if (str.empty()) {
        return {};
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

    if (size == 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(size), L'\0');

    MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);

    return result;
}

std::FILE* PortableFOpen(std::string_view path, std::string_view mode) {
    std::FILE* fp = nullptr;
    const std::wstring wpath = Utf8ToWchar(path);
    const std::wstring wmode(mode.begin(), mode.end());
    const errno_t err = _wfopen_s(&fp, wpath.c_str(), wmode.c_str());
    if (err != 0) {
        return nullptr;
    }
    return fp;
}

#else

std::FILE* PortableFOpen(std::string_view path, std::string_view mode) {
    return std::fopen(std::string(path).c_str(), std::string(mode).c_str());
}

#endif

} // namespace slimenano::filesystem
