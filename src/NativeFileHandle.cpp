#include "NativeFileHandle.h"

#include <system_error>

namespace slimenano::filesystem {

namespace fs = std::filesystem;

NativeFileHandle::NativeFileHandle(std::filesystem::path path, OpenOption options, std::error_code& ec) {

    m_readable = (options & OpenOption::Read) != OpenOption::None;
    m_writable = (options & (OpenOption::Append | OpenOption::Truncate | OpenOption::Create)) != OpenOption::None;
    const bool append = (options & OpenOption::Append) != OpenOption::None;
    const bool truncate = (options & OpenOption::Truncate) != OpenOption::None;
    const bool create = (options & OpenOption::Create) != OpenOption::None;

    if (!m_readable && !m_writable) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return;
    }

    std::error_code existsEc;
    const bool exists = fs::exists(path, existsEc);
    if (existsEc) {
        ec = existsEc;
        return;
    }
    if (!exists && !create) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return;
    }

    if (!exists) {
        std::ofstream touch{path, std::ios::out | std::ios::binary};
        if (!touch) {
            ec = std::make_error_code(std::errc::io_error);
            return;
        }
    }

    std::ios::openmode mode = std::ios::binary;
    if (m_readable) {
        mode |= std::ios::in;
    }
    if (m_writable) {
        mode |= std::ios::out;
    }
    if (append) {
        mode |= std::ios::app;
    }
    if (truncate) {
        mode |= std::ios::trunc;
    }

    m_stream.open(path, mode);
    if (!m_stream.is_open()) {
        ec = std::make_error_code(std::errc::io_error);
    }
}

std::size_t NativeFileHandle::Read(std::span<std::byte> buffer, std::error_code& ec) {
    ec.clear();
    if (m_closed || !m_readable) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    if (!buffer.empty()) {
        m_stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    }
    const auto count = m_stream.gcount();
    const auto state = m_stream.rdstate();
    if (state != std::ios::goodbit) {
        const bool eof = (state & std::ios::eofbit) != 0;
        m_stream.clear();
        if (!eof) {
            ec = std::make_error_code(std::errc::io_error);
        }
    }
    return static_cast<std::size_t>(count);
}

std::size_t NativeFileHandle::Write(std::span<const std::byte> buffer, std::error_code& ec) {
    ec.clear();
    if (m_closed || !m_writable) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    if (!buffer.empty()) {
        m_stream.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    }
    if (m_stream.fail()) {
        m_stream.clear();
        ec = std::make_error_code(std::errc::io_error);
        return 0;
    }
    return buffer.size();
}

std::uint64_t NativeFileHandle::Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) {
    ec.clear();
    if (m_closed) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    const auto dir = origin == SeekOrigin::Begin ? std::ios::beg
                     : origin == SeekOrigin::End ? std::ios::end
                                                 : std::ios::cur;
    if (m_readable) {
        m_stream.seekg(offset, dir);
    }
    if (m_writable) {
        m_stream.seekp(offset, dir);
    }
    if (m_stream.fail()) {
        m_stream.clear();
        ec = std::make_error_code(std::errc::io_error);
        return 0;
    }
    return Tell(ec);
}

std::uint64_t NativeFileHandle::Tell(std::error_code& ec) {
    ec.clear();
    if (m_closed) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    const auto pos = m_writable ? m_stream.tellp() : m_stream.tellg();
    if (pos < 0) {
        ec = std::make_error_code(std::errc::io_error);
        return 0;
    }
    return static_cast<std::uint64_t>(pos);
}

void NativeFileHandle::Flush(std::error_code& ec) {
    ec.clear();
    if (m_closed) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return;
    }
    m_stream.flush();
    if (m_stream.fail()) {
        m_stream.clear();
        ec = std::make_error_code(std::errc::io_error);
    }
}

void NativeFileHandle::Close(std::error_code& ec) {
    ec.clear();
    if (m_closed) {
        return;
    }
    m_stream.close();
    m_closed = true;
    if (m_stream.fail()) {
        ec = std::make_error_code(std::errc::io_error);
    }
}

} // namespace slimenano::filesystem
