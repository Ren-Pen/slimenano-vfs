/**
 * @file NativeFileHandle.cpp
 * @brief Implementation of the native file handle.
 */

#include "NativeFileHandle.h"

#include <limits>

namespace slimenano::filesystem {

namespace {
namespace fs = std::filesystem;

inline void
doSeek(std::fstream& stream, StreamMode mode, std::ios::off_type offset, std::ios::seekdir dir, std::error_code& ec) {
    ec.clear();
    if (mode == StreamMode::None) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return;
    } else if (mode == StreamMode::Read) {
        stream.seekg(offset, dir);
    } else {
        stream.seekp(offset, dir);
    }
    if (stream.fail()) {
        ec = std::make_error_code(std::errc::io_error);
        stream.clear();
    }
}

} // namespace

void NativeFileHandle::SwitchStreamMode(StreamMode mode, std::error_code& ec) {
    ec.clear();
    if (m_curMode == mode) {
        return;
    }
    doSeek(m_stream, mode, m_pos, std::ios::beg, ec);
    if (ec) {
        ResetStreamMode();
        return;
    }
    m_curMode = mode;
}

void NativeFileHandle::ResetStreamMode() {
    m_curMode = StreamMode::None;
}

/**
 * @brief Opens the native file.
 *
 * Derives the stream mode from @p options, creates the file first when
 * OpenOption::Create is set and the file does not exist, and reports all
 * failures through @p ec.
 *
 * @param path    Native path of the file to open.
 * @param options Access flags for the file.
 * @param ec      On failure, set to an error code describing the problem.
 */
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
        if (append) {
            mode |= std::ios::ate;
        }
        if (truncate) {
            mode |= std::ios::trunc;
        }
    }

    m_stream.open(path, mode);
    if (!m_stream.is_open()) {
        ec = std::make_error_code(std::errc::io_error);
        return;
    }

    if (append) {
        const auto pos = m_stream.tellp();
        if (m_stream.fail() || pos < 0) {
            m_stream.close();
            ec = std::make_error_code(std::errc::io_error);
            return;
        }
        m_pos = pos;
    }
}

/**
 * @brief Reads bytes from the file into @p buffer.
 *
 * @param buffer Destination span for the read data.
 * @param ec     On failure, set to an error code describing the problem.
 *
 * @return The number of bytes read; fewer than requested means end of file
 *         was reached. Returns zero on error.
 */
std::size_t NativeFileHandle::Read(std::span<std::byte> buffer, std::error_code& ec) {
    ec.clear();
    if (m_closed || !m_readable) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    if (buffer.empty()) {
        return 0;
    }
    if (buffer.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        ec = std::make_error_code(std::errc::value_too_large);
        return 0;
    }

    SwitchStreamMode(StreamMode::Read, ec);
    if (ec) {
        return 0;
    }

    m_stream.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

    const auto state = m_stream.rdstate();
    if (state & std::ios::badbit) {
        ec = std::make_error_code(std::errc::io_error);
        ResetStreamMode();
        m_stream.clear();
        return 0;
    }
    if (state != std::ios::goodbit) {
        m_stream.clear();
    }
    const auto count = m_stream.gcount();
    m_pos += count;
    return static_cast<std::size_t>(count);
}

/**
 * @brief Writes bytes from @p buffer to the file.
 *
 * @param buffer Source span of the data to write.
 * @param ec     On failure, set to an error code describing the problem.
 *
 * @return The number of bytes written, or zero on failure.
 */
std::size_t NativeFileHandle::Write(std::span<const std::byte> buffer, std::error_code& ec) {
    ec.clear();
    if (m_closed || !m_writable) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    if (buffer.empty()) {
        return 0;
    }
    if (buffer.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        ec = std::make_error_code(std::errc::value_too_large);
        return 0;
    }

    SwitchStreamMode(StreamMode::Write, ec);
    if (ec) {
        return 0;
    }

    m_stream.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));

    if (m_stream.fail()) {
        ec = std::make_error_code(std::errc::io_error);
        ResetStreamMode();
        m_stream.clear();
        return 0;
    }
    const auto count = buffer.size();
    m_pos += static_cast<std::ios::off_type>(count);
    return count;
}

/**
 * @brief Moves the file position relative to @p origin.
 *
 * When the handle is both readable and writable, both stream positions are
 * moved.
 *
 * @param offset Byte offset relative to @p origin; may be negative.
 * @param origin Anchor point for the offset.
 * @param ec     On failure, set to an error code describing the problem.
 *
 * @return The new position in bytes, or zero on failure.
 */
std::uint64_t NativeFileHandle::Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) {
    ec.clear();
    if (m_closed || (!m_readable && !m_writable)) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }

    StreamMode mode = m_curMode;

    if (mode == StreamMode::None) {
        if (m_readable) {
            mode = StreamMode::Read;
        } else if (m_writable) {
            mode = StreamMode::Write;
        }
    }

    if (origin == SeekOrigin::Begin) {
        if (offset < 0) {
            ec = std::make_error_code(std::errc::invalid_seek);
            return 0;
        }
        doSeek(m_stream, mode, offset, std::ios::beg, ec);
        if (ec) {
            ResetStreamMode();
            return 0;
        }
        m_pos = static_cast<std::ios::off_type>(offset);
    } else {

        decltype(m_pos) newOffset = static_cast<decltype(m_pos)>(offset);
        const auto dir = origin == SeekOrigin::End ? std::ios::end : std::ios::beg;

        if (origin == SeekOrigin::Current) {
            if (offset > 0 && m_pos > std::numeric_limits<decltype(m_pos)>::max() - offset) {
                ec = std::make_error_code(std::errc::value_too_large);
                return 0;
            }
            if (offset < 0 && m_pos < std::numeric_limits<decltype(m_pos)>::min() - offset) {
                ec = std::make_error_code(std::errc::value_too_large);
                return 0;
            }

            newOffset += m_pos;
            if (newOffset < 0) {
                ec = std::make_error_code(std::errc::invalid_seek);
                return 0;
            }
        }

        doSeek(m_stream, mode, newOffset, dir, ec);
        if (ec) {
            ResetStreamMode();
            return 0;
        }

        decltype(m_pos) newPos{0};
        if (mode == StreamMode::Read) {
            newPos = m_stream.tellg();
        } else {
            newPos = m_stream.tellp();
        }
        if (m_stream.fail() || newPos < 0) {
            ec = std::make_error_code(std::errc::io_error);
            ResetStreamMode();
            m_stream.clear();
            return 0;
        }
        m_pos = newPos;
    }

    m_curMode = mode;

    return m_pos;
}

/**
 * @brief Returns the current file position.
 *
 * @param ec On failure, set to an error code describing the problem.
 *
 * @return The position in bytes, or zero on failure.
 */
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

/**
 * @brief Flushes buffered data to the underlying storage.
 *
 * @param ec On failure, set to an error code describing the problem.
 */
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

/**
 * @brief Closes the file.
 *
 * Subsequent operations on the handle fail with
 * std::errc::bad_file_descriptor. Closing an already closed handle is a
 * no-op.
 *
 * @param ec On failure, set to an error code describing the problem.
 */
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
