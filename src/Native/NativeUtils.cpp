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

std::filesystem::path ToNativePath(const std::filesystem::path& root, const Path& path) {
    const auto absolutePath = path.ToAbsolute("/");
    const auto rel = absolutePath.String().substr(1);
    return root / Utf8ToNativePath(rel);
}

std::filesystem::path Utf8ToNativePath(std::string_view utf8) {
    if (utf8.empty()) {
        return {};
    }
    return std::filesystem::path(std::u8string_view(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()));
}

std::string NativePathToUtf8(const std::filesystem::path& native) {
    const auto utf8 = native.u8string();
    return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
}

} // namespace slimenano::filesystem
