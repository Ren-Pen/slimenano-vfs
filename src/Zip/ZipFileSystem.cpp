#include <slimenano/vfs/ZipFileSystem.h>

#include <zip.h>

#include <slimenano/vfs/VirtualPathTree.h>

#include "ZipErrorCategory.h"
#include "ZipFileHandle.h"

#if defined(_WIN32)
#include "../Native/NativeUtils.h"
#endif

namespace slimenano::filesystem {

namespace {

struct ZipDeleter {
    void operator()(zip_t* za) const noexcept {
        if (za) {
            zip_close(za);
        }
    }
};

using ZipPtr = std::shared_ptr<zip_t>;

} // namespace

struct ZipFileSystem::Impl {

    struct ZipNodeInfo {
        zip_int64_t index{-1};
        FileInfo fileInfo{
            .type = FileType::Directory,
            .readable = true,
        };
    };

    Impl(std::string_view zip_path) : zipPath(zip_path) {}

    std::string zipPath;
    ZipPtr pZip{};

    VirtualPathTree<ZipNodeInfo> indexTree{};
};

ZipFileSystem::ZipFileSystem(std::string_view zipPath, std::error_code& ec) : m_pImpl(std::make_unique<Impl>(zipPath)) {

    if (m_pImpl->pZip) {
        ec = std::make_error_code(std::errc::already_connected);
        return;
    }

    ec.clear();

    zip_error_t error;
    zip_error_init(&error);

    zip_source_t* src;

#if defined(_WIN32)

    auto wzipPath = Utf8ToWchar(m_pImpl->zipPath);
    src = zip_source_win32w_create(wzipPath.c_str(), 0, -1, &error);
#else
    src = zip_source_file_create(m_pImpl->zipPath.c_str(), 0, -1, &error);

#endif

    if (!src) {
        ec = make_zip_error(error);
        zip_error_fini(&error);
        return;
    }

    zip_t* za = zip_open_from_source(src, ZIP_RDONLY | ZIP_CHECKCONS, &error);
    if (za == nullptr) {
        zip_source_free(src);
        ec = make_zip_error(error);
        zip_error_fini(&error);
        return;
    }

    zip_error_fini(&error);

    const auto numEntries = zip_get_num_entries(za, 0);
    for (zip_int64_t i = 0; i < numEntries; i++) {
        zip_stat_t st;
        zip_stat_init(&st);
        if (zip_stat_index(za, i, 0, &st) != 0) {
            ec = make_zip_error(*zip_get_error(za));
            zip_close(za);
            m_pImpl->indexTree = {};
            return;
        }

        std::string_view name{st.name};
        auto node = m_pImpl->indexTree.CreateNodeIfAbsent(Path{name});
        node->data.index = i;
        node->data.fileInfo.size = (st.valid & ZIP_STAT_SIZE) ? st.size : 0;
        node->data.fileInfo.modifiedTime = (st.valid & ZIP_STAT_MTIME) ? static_cast<std::int64_t>(st.mtime) * 1000 : 0;

        bool isDir = !name.empty() && name.back() == '/';
        node->data.fileInfo.type = isDir ? FileType::Directory : FileType::Regular;
    }

    m_pImpl->pZip = ZipPtr(za, ZipDeleter{});
}

ZipFileSystem::~ZipFileSystem() = default;

FileInfo ZipFileSystem::Stat(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        return {};
    }
    return node->data.fileInfo;
}

bool ZipFileSystem::Exists(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    return node != nullptr;
}

bool ZipFileSystem::IsDirectory(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        return false;
    }
    return node->data.fileInfo.IsDirectory();
};

bool ZipFileSystem::IsRegularFile(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        return false;
    }
    return node->data.fileInfo.IsRegularFile();
}

bool ZipFileSystem::IsReadable(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        return false;
    }
    return node->data.fileInfo.readable;
}

bool ZipFileSystem::IsWritable(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        return false;
    }
    return node->data.fileInfo.writable;
}

std::uint64_t ZipFileSystem::Size(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        return 0;
    }
    return node->data.fileInfo.size;
}

std::vector<std::string> ZipFileSystem::List(const Path& path, std::error_code& ec) const {
    ec.clear();
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return {};
    }

    if (!node->data.fileInfo.IsDirectory()) {
        ec = std::make_error_code(std::errc::not_a_directory);
        return {};
    }

    std::vector<std::string> names;
    names.reserve(node->children.size());
    for (const auto& [name, child] : node->children) {
        names.push_back(name);
    }
    return names;
}

void ZipFileSystem::CreateFile(const Path&, std::error_code& ec) {
    ec = std::make_error_code(std::errc::read_only_file_system);
}

void ZipFileSystem::CreateDirectory(const Path&, std::error_code& ec) {
    ec = std::make_error_code(std::errc::read_only_file_system);
}

void ZipFileSystem::CreateDirectories(const Path&, std::error_code& ec) {
    ec = std::make_error_code(std::errc::read_only_file_system);
}

void ZipFileSystem::Delete(const Path&, std::error_code& ec) {
    ec = std::make_error_code(std::errc::read_only_file_system);
}

void ZipFileSystem::DeleteAll(const Path&, std::error_code& ec) {
    ec = std::make_error_code(std::errc::read_only_file_system);
}

std::unique_ptr<FileHandle> ZipFileSystem::Open(const Path& path, OpenOption openOptions, std::error_code& ec) {
    ec.clear();
    if (!m_pImpl->pZip) {
        ec = std::make_error_code(std::errc::bad_file_descriptor);
        return {};
    }
    if ((openOptions & (OpenOption::Append | OpenOption::Truncate | OpenOption::Create)) != OpenOption::None) {
        ec = std::make_error_code(std::errc::read_only_file_system);
        return {};
    }
    auto node = m_pImpl->indexTree.FindNode(path);
    if (!node) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return {};
    }
    if (node->data.fileInfo.IsDirectory()) {
        ec = std::make_error_code(std::errc::is_a_directory);
        return {};
    }
    if ((openOptions & OpenOption::Read) == OpenOption::None) {
        ec = std::make_error_code(std::errc::invalid_argument);
        return {};
    }
    auto handle = std::make_unique<ZipFileHandle>(m_pImpl->pZip, node->data.index, ec);
    if (ec) {
        return {};
    }
    return handle;
}

} // namespace slimenano::filesystem
