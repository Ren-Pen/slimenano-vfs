/**
 * @file NativeFileHandle.cpp
 * @brief Implementation of the native file handle.
 */

#include "NativeFileHandle.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace slimenano::filesystem {

namespace {

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

#define PortableFSeek _fseeki64
#define PortableFTell _ftelli64

#else

std::FILE* PortableFOpen(std::string_view path, std::string_view mode) {
    return std::fopen(std::string(path).c_str(), std::string(mode).c_str());
}

#define PortableFSeek fseeko
#define PortableFTell ftello

#endif
} // namespace

NativeFileHandle::NativeFileHandle(std::string_view path, OpenOption options, std::error_code& ec) {

    m_readable = (options & OpenOption::Read) != OpenOption::None;
    m_writable = (options & (OpenOption::Append | OpenOption::Truncate | OpenOption::Create)) != OpenOption::None;
    const bool append = (options & OpenOption::Append) != OpenOption::None;
    const bool truncate = (options & OpenOption::Truncate) != OpenOption::None;
    const bool create = (options & OpenOption::Create) != OpenOption::None;

    if (!m_readable && !m_writable) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return;
    }

    bool exists = false;
    {
        FilePtr existsFp(PortableFOpen(path, "rb"));
        if (existsFp) {
            exists = true;
        }
    }
    if (!exists && !create) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return;
    }

    std::string mode;
    if (m_readable && !m_writable) {
        mode = "rb";
    } else if (m_writable && !m_readable) {
        mode = append ? "ab" : (truncate ? "wb" : (exists ? "r+b" : "wb"));
    } else if (m_writable && m_readable) {
        mode = append ? "a+b" : (truncate ? "w+b" : (exists ? "r+b" : "w+b"));
    } else {
        ec = std::make_error_code(std::errc::invalid_argument);
        return;
    }

    auto* fp = PortableFOpen(path, mode);
    if (!fp) {
        ec = std::make_error_code(std::errc::io_error);
        return;
    }
    m_pFile.reset(fp);
}

std::size_t NativeFileHandle::Read(std::span<std::byte> buffer, std::error_code& ec) {
    ec.clear();
    if (!m_pFile || !m_readable) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    if (buffer.empty()) {
        return 0;
    }

    std::size_t n = std::fread(buffer.data(), sizeof(buffer[0]), buffer.size(), m_pFile.get());
    if (n < buffer.size()) {
        if (std::ferror(m_pFile.get())) {
            ec = std::make_error_code(std::errc::io_error);
            std::clearerr(m_pFile.get());
            return 0;
        }
    }

    return n;
}

std::size_t NativeFileHandle::Write(std::span<const std::byte> buffer, std::error_code& ec) {
    ec.clear();
    if (!m_pFile || !m_writable) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    if (buffer.empty()) {
        return 0;
    }

    std::size_t n = fwrite(buffer.data(), sizeof(buffer[0]), buffer.size(), m_pFile.get());
    if (n < buffer.size()) {
        ec = std::make_error_code(std::errc::io_error);
        std::clearerr(m_pFile.get());
    }
    return n;
}

std::uint64_t NativeFileHandle::Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) {
    ec.clear();
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    int whence;
    switch (origin) {

    case SeekOrigin::Begin:
        whence = SEEK_SET;
        break;
    case SeekOrigin::Current:
        whence = SEEK_CUR;
        break;
    case SeekOrigin::End:
        whence = SEEK_END;
        break;
    default:
        ec = std::make_error_code(std::errc::invalid_argument);
        return 0;
    }

    if (PortableFSeek(m_pFile.get(), offset, whence) != 0) {
        ec = std::make_error_code(std::errc::io_error);
        return 0;
    }

    return Tell(ec);
}

std::uint64_t NativeFileHandle::Tell(std::error_code& ec) {
    ec.clear();
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    const auto pos = PortableFTell(m_pFile.get());
    if (pos < 0) {
        ec = std::make_error_code(std::errc::io_error);
        return 0;
    }
    return static_cast<std::uint64_t>(pos);
}

void NativeFileHandle::Flush(std::error_code& ec) {
    ec.clear();
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return;
    }
    if (fflush(m_pFile.get())) {
        ec = std::make_error_code(std::errc::io_error);
    }
}

void NativeFileHandle::Close(std::error_code& ec) {
    ec.clear();
    if (!m_pFile) {
        return;
    }

    auto* fp = m_pFile.release();

    if (fclose(fp)) {
        ec = std::make_error_code(std::errc::io_error);
    }
}

} // namespace slimenano::filesystem
