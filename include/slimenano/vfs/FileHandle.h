/**
 * @file FileHandle.h
 * @brief Interface for accessing an open file.
 */

#ifndef SLIMENANO_VFS_INCLUDE_VFS_FILE_H
#define SLIMENANO_VFS_INCLUDE_VFS_FILE_H

#include <cstdint>
#include <cstddef>
#include <span>
#include <system_error>

namespace slimenano::filesystem {

/**
 * @brief Anchor point for FileHandle::Seek().
 */
enum class SeekOrigin : std::int8_t {
    /** @brief The offset is measured from the beginning of the file. */
    Begin = 0,
    /** @brief The offset is measured from the current position. */
    Current,
    /** @brief The offset is measured from the end of the file. */
    End
};

/**
 * @brief Interface for sequential and random access to an open file.
 *
 * A FileHandle is obtained from FileSystem::Open(). All positions are
 * byte-based. Every operation reports failure through its std::error_code
 * output parameter instead of throwing exceptions; a cleared error code
 * means success. The underlying file is released when the handle is
 * destroyed.
 *
 * Instances are non-copyable and non-movable.
 */
class FileHandle {

public:
    virtual ~FileHandle() = default;

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&&) noexcept = delete;
    FileHandle& operator=(FileHandle&&) noexcept = delete;

    /**
     * @brief Reads up to buffer.size() bytes from the file into @p buffer.
     *
     * @param buffer Destination span for the read data.
     * @param ec     On failure, set to an error code describing the problem.
     *
     * @return The number of bytes actually read; fewer than requested means
     *         end of file was reached. Returns zero on failure.
     */
    virtual std::size_t Read(std::span<std::byte> buffer, std::error_code& ec) = 0;
    /**
     * @brief Writes buffer.size() bytes from @p buffer to the file.
     *
     * @param buffer Source span of the data to write.
     * @param ec     On failure, set to an error code describing the problem.
     *
     * @return The number of bytes written, or zero on failure.
     */
    virtual std::size_t Write(std::span<const std::byte> buffer, std::error_code& ec) = 0;
    /**
     * @brief Moves the file position by @p offset relative to @p origin.
     *
     * @param offset Byte offset relative to @p origin; may be negative.
     * @param origin Anchor point for the offset.
     * @param ec     On failure, set to an error code describing the problem.
     *
     * @return The new position in bytes, or zero on failure.
     */
    virtual std::uint64_t Seek(std::int64_t offset, SeekOrigin origin, std::error_code& ec) = 0;
    /**
     * @brief Returns the current file position.
     *
     * @param ec On failure, set to an error code describing the problem.
     *
     * @return The position in bytes, or zero on failure.
     */
    [[nodiscard]] virtual std::uint64_t Tell(std::error_code& ec) = 0;
    /**
     * @brief Flushes buffered data to the underlying storage.
     *
     * @param ec On failure, set to an error code describing the problem.
     */
    virtual void Flush(std::error_code& ec) = 0;
    /**
     * @brief Closes the file.
     *
     * Subsequent operations on the handle fail with
     * std::errc::bad_file_descriptor. Closing an already closed handle is a
     * no-op.
     *
     * @param ec On failure, set to an error code describing the problem.
     */
    virtual void Close(std::error_code& ec) = 0;

protected:
    /** @brief Creates a handle; only derived classes can construct one. */
    FileHandle() = default;
};

} // namespace slimenano::filesystem

#endif
