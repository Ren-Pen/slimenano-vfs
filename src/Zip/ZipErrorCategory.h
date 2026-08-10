#ifndef SLIMENANO_VFS_SRC_ZIP_ZIP_CATEGORY_H
#define SLIMENANO_VFS_SRC_ZIP_ZIP_CATEGORY_H

#include <string>
#include <system_error>

#include <zip.h>

namespace slimenano::filesystem {

class ZipErrorCategory final : public std::error_category {

public:
    const char* name() const noexcept override { return "zip"; }

    std::string message(int ev) const override {
        zip_error_t e;
        zip_error_init_with_code(&e, ev);
        std::string msg = zip_error_strerror(&e);
        zip_error_fini(&e);
        return msg;
    }
};

const std::error_category& zip_error_category() noexcept;

std::error_code make_zip_error(zip_error_t& error);

} // namespace slimenano::filesystem

#endif
