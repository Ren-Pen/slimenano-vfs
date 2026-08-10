#include "ZipErrorCategory.h"
#include "ZipFileHandle.h"

namespace slimenano::filesystem {

ZipFileHandle::ZipFileHandle(std::shared_ptr<zip_t> pZip, zip_int64_t index, std::error_code& ec) :
    m_pZip(std::move(pZip)) {
    ec.clear();
    zip_file_t* fp = zip_fopen_index(m_pZip.get(), index, 0);
    if (!fp) {
        ec = make_zip_error(*zip_get_error(m_pZip.get()));
        return;
    }
    m_pFile.reset(fp);
}

std::size_t ZipFileHandle::Read(std::span<std::byte> buffer, std::error_code& ec) {
    ec.clear();
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    if (buffer.empty()) {
        return 0;
    }
    zip_int64_t n = zip_fread(m_pFile.get(), buffer.data(), buffer.size());
    if (n < 0) {
        ec = make_zip_error(*zip_file_get_error(m_pFile.get()));
        return 0;
    }
    return static_cast<std::size_t>(n);
}

std::size_t ZipFileHandle::Write(std::span<const std::byte>, std::error_code& ec) {
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    ec = std::make_error_code(std::errc::read_only_file_system);
    return 0;
}

std::uint64_t ZipFileHandle::Seek(std::int64_t, SeekOrigin, std::error_code& ec) {
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    ec = std::make_error_code(std::errc::operation_not_supported);
    return 0;
}

std::uint64_t ZipFileHandle::Tell(std::error_code& ec) {
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return 0;
    }
    ec = std::make_error_code(std::errc::operation_not_supported);
    return 0;
}

void ZipFileHandle::Flush(std::error_code& ec) {
    ec.clear();
    if (!m_pFile) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
    }
}

void ZipFileHandle::Close(std::error_code& ec) {
    ec.clear();
    if (m_pFile) {
        auto* fp = m_pFile.release();
        auto r = zip_fclose(fp);
        if (r) {
            ec = std::error_code(r, zip_error_category());
        }
    }
    m_pZip.reset();
}

} // namespace slimenano::filesystem
