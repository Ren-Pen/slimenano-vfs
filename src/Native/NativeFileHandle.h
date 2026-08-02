/**
 * @file NativeFileHandle.h
 * @brief Declaration of the native file handle implementation.
 */

#ifndef SLIMENANO_VFS_SRC_NATIVE_FILE_HANDLE_H
#define SLIMENANO_VFS_SRC_NATIVE_FILE_HANDLE_H

#include <slimenano/vfs/FileHandle.h>
#include <slimenano/vfs/Options/OpenOptions.h>

#include <filesystem>
#include <fstream>

namespace slimenano::filesystem {

/**
 * @brief FileHandle implementation backed by a std::fstream.
 *
 * Wraps a binary fstream opened according to the OpenOption flags; whether
 * the stream supports reading or writing is recorded in the member flags.
 */
class NativeFileHandle final : public FileHandle {
public:
    /**
     * @brief Opens the native file for the requested access.
     *
     * Fails with std::errc::invalid_argument when neither read nor write
     * access is requested, std::errc::no_such_file_or_directory when the
     * file does not exist and OpenOption::Create is not set, and
     * std::errc::io_error on other I/O failures.
     *
     * @param path    Native path of the file to open.
     * @param options Access flags for the file.
     * @param ec      On failure, set to an error code describing the
     *                problem.
     */
    NativeFileHandle(std::filesystem::path path, OpenOption options, std::error_code& ec);

    /**
     * @brief Reads up to buffer.size() bytes from the file into @p buffer.
     *
     * @param buffer Destination span for the read data.
     * @param ec     On failure, set to an error code describing the
     *               problem.
     *
     * @return The number of bytes actually read; fewer than requested means
     *         end of file was reached. Returns zero on failure.
     */
    std::size_t Read(std::span<std::byte> buffer, std::error_code& ec) override;

    /**
     * @brief Writes buffer.size() bytes from @p buffer to the file.
     *
     * @param buffer Source span of the data to write.
     * @param ec     On failure, set to an error code describing the
     *               problem.
     *
     * @return The number of bytes written, or zero on failure.
     */
    std::size_t Write(std::span<const std::byte> buffer, std::error_code& ec) override;

    /**
     * @brief Moves the file position by @p offset relative to @p origin.
     *
     * When the handle is both readable and writable, both stream positions
     * are moved.
     *
     * @param offset Byte offset relative to @p origin; may be negative.
     * @param origin Anchor point for the offset.
     * @param ec     On failure, set to an error code describing the
     *               problem.
     *
     * @return The new position in bytes, or zero on failure.
     */
    std::uint64_t Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) override;

    /**
     * @brief Returns the current file position.
     *
     * @param ec On failure, set to an error code describing the problem.
     *
     * @return The position in bytes, or zero on failure.
     */
    std::uint64_t Tell(std::error_code& ec) override;

    /**
     * @brief Flushes buffered data to the underlying storage.
     *
     * @param ec On failure, set to an error code describing the problem.
     */
    void Flush(std::error_code& ec) override;

    /**
     * @brief Closes the file.
     *
     * Subsequent operations on the handle fail with
     * std::errc::bad_file_descriptor. Closing an already closed handle is a
     * no-op.
     *
     * @param ec On failure, set to an error code describing the problem.
     */
    void Close(std::error_code& ec) override;

private:
    /** @brief Underlying binary stream. */
    std::fstream m_stream;
    /** @brief Whether the handle supports reading. */
    bool m_readable = false;
    /** @brief Whether the handle supports writing. */
    bool m_writable = false;
    /** @brief Whether the handle has been closed. */
    bool m_closed = false;
};

} // namespace slimenano::filesystem

#endif
