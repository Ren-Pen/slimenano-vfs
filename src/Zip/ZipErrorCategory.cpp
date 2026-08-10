#include "ZipErrorCategory.h"

namespace slimenano::filesystem {

const std::error_category& zip_error_category() noexcept {
    static const ZipErrorCategory instance;
    return instance;
}

std::error_code make_zip_error(zip_error_t& error) {
    const int zipCode = zip_error_code_zip(&error);
    return std::error_code(zipCode, zip_error_category());
}

} // namespace slimenano::filesystem
