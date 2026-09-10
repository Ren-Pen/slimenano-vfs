/**
 * @file NativeFileHandle.cpp
 * @brief Implementation of the native file handle.
 */

#include "NativeFileHandle.h"
#include "NativeUtils.h"

namespace slimenano::filesystem {

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

    bool exists = std::filesystem::exists(Utf8ToNativePath(path), ec);
    if (ec) {
        return;
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
