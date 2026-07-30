#include <slimenano/vfs/VirtualFileSystem.h>

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <chrono>

namespace slimenano::filesystem {

namespace {

namespace chrono = std::chrono;

const Path kRoot{"/"};
} // namespace

struct VirtualFileSystem::Impl {

    struct Node {

        Node(const std::shared_ptr<Node>& parent, std::string_view path) :
            parent(parent), absolutePath(parent ? parent->absolutePath / path : path),
            creationTime(
                chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()
            ) {};
        std::weak_ptr<Node> parent;
        Path absolutePath;
        std::int64_t creationTime;
        std::vector<std::shared_ptr<FileSystem>> fileSystems{};
        std::map<std::string, std::shared_ptr<Node>, std::less<>> children{};
    };

    std::shared_ptr<Node> root = std::make_shared<Node>(nullptr, "/");

    std::shared_ptr<Node> findNode(const Path& path) const {
        Path absolutePath = path.ToAbsolute(kRoot);
        auto node = root;

        for (auto segment : absolutePath) {
            if (segment == "/") {
                continue;
            }
            auto it = node->children.find(segment);
            if (it == node->children.end()) {
                return {};
            }
            node = it->second;
        }

        return node;
    }

    void cleanupNode(const std::shared_ptr<Node>& ptr) {
        if (!ptr->fileSystems.empty() || !ptr->children.empty()) {
            return;
        }
        auto parent = ptr->parent.lock();
        if (!parent) {
            return;
        }

        std::erase_if(parent->children, [&](const auto& kv) {
            return kv.second == ptr;
        });
        ptr->parent = {};

        cleanupNode(parent);
    }

    struct VirtualFileNode {
        std::shared_ptr<FileSystem> fs;
        Path path;
        std::int64_t creationTime;
    };

    std::pair<std::shared_ptr<Node>, bool> ResolveNode(const Path& absolutePath) const {
        auto node = root;
        bool exact = true;
        for (auto segment : absolutePath) {
            if (segment == "/") {
                continue;
            }
            auto it = node->children.find(segment);
            if (it == node->children.end()) {
                exact = false;
                break;
            }
            node = it->second;
        }

        return {node, exact};
    }

    std::optional<VirtualFileNode> Resolve(const Path& path) const {
        auto absolutePath = path.ToAbsolute(kRoot);
        auto [node, exact] = ResolveNode(absolutePath);

        if (exact && node->fileSystems.empty()) {
            return VirtualFileNode{nullptr, absolutePath, node->creationTime};
        }

        while (node) {
            if (!node->fileSystems.empty()) {
                return VirtualFileNode{
                    node->fileSystems.back(), absolutePath.ToRelative(node->absolutePath), node->creationTime
                };
            }
            node = node->parent.lock();
        }
        return std::nullopt;
    }

    std::optional<VirtualFileNode> ResolveIfExist(const Path& path, std::error_code& ec) const {
        ec.clear();
        auto absolutePath = path.ToAbsolute(kRoot);
        auto [node, exact] = ResolveNode(absolutePath);

        if (exact && node->fileSystems.empty()) {
            return VirtualFileNode{nullptr, absolutePath, node->creationTime};
        }

        while (node) {
            if (!node->fileSystems.empty()) {
                auto rel = absolutePath.ToRelative(node->absolutePath);
                for (auto it = node->fileSystems.rbegin(); it != node->fileSystems.rend(); ++it) {
                    std::error_code fsEc;
                    if ((*it)->Exists(rel, fsEc)) {
                        ec.clear();
                        return VirtualFileNode{*it, rel, node->creationTime};
                    }
                    if (!ec && fsEc) {
                        ec = fsEc;
                    }
                }
            }
            node = node->parent.lock();
        }
        return std::nullopt;
    }

    std::optional<VirtualFileNode> ResolveForWrite(const Path& path, std::error_code& ec) const {
        ec.clear();
        auto vfNode = Resolve(path);
        if (!vfNode) {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
            return std::nullopt;
        }
        if (!vfNode->fs) {
            ec = std::make_error_code(std::errc::read_only_file_system);
            return std::nullopt;
        }
        return vfNode;
    }

    std::vector<std::string> ResolveList(const Path& path, std::error_code& ec) const {
        ec.clear();
        std::set<std::string> names;

        auto absolutePath = path.ToAbsolute(kRoot);
        auto [node, exact] = ResolveNode(absolutePath);

        if (exact) {
            for (const auto& [name, child] : node->children) {
                names.insert(name);
            }
        }

        while (node) {
            if (!node->fileSystems.empty()) {
                auto rel = absolutePath.ToRelative(node->absolutePath);
                for (auto it = node->fileSystems.rbegin(); it != node->fileSystems.rend(); ++it) {
                    std::error_code fsEc;
                    const auto& fs = *it;
                    if (fs->IsDirectory(rel, fsEc)) {
                        auto entries = fs->List(rel, fsEc);
                        if (!fsEc) {
                            for (auto& e : entries) {
                                names.emplace(std::move(e));
                            }
                        } else if (!ec) {
                            ec = fsEc;
                        }
                    }
                }
            }
            node = node->parent.lock();
        }

        return {names.begin(), names.end()};
    }
};

VirtualFileSystem::VirtualFileSystem() : m_impl(std::make_unique<Impl>()) {
}
VirtualFileSystem::~VirtualFileSystem() = default;

void VirtualFileSystem::Mount(const Path& mountPoint, std::shared_ptr<FileSystem> ptr) {

    if (!ptr) {
        return;
    }

    Path absoluteMountPoint = mountPoint.ToAbsolute(kRoot);

    auto node = m_impl->root;

    for (auto segment : absoluteMountPoint) {
        if (segment == "/") {
            continue;
        }
        auto it = node->children.find(segment);
        if (it == node->children.end()) {
            it = node->children.try_emplace(std::string{segment}, std::make_shared<Impl::Node>(node, segment)).first;
        }
        node = it->second;
    }

    if (std::ranges::find(node->fileSystems, ptr) == node->fileSystems.end()) {
        node->fileSystems.emplace_back(std::move(ptr));
    }
}

bool VirtualFileSystem::IsMounted(const Path& mountPoint) const {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return false;
    }
    return !node->fileSystems.empty();
}

bool VirtualFileSystem::IsMounted(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr) const {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return false;
    }
    return std::ranges::find(node->fileSystems, ptr) != node->fileSystems.end();
}

void VirtualFileSystem::Unmount(const Path& mountPoint) {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return;
    }
    node->fileSystems.clear();
    m_impl->cleanupNode(node);
}

void VirtualFileSystem::Unmount(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr) {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return;
    }
    std::erase(node->fileSystems, ptr);
    m_impl->cleanupNode(node);
}

std::vector<std::shared_ptr<FileSystem>> VirtualFileSystem::GetMountedFileSystems(const Path& mountPoint) const {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return {};
    }
    return node->fileSystems;
}

void VirtualFileSystem::Initialize(std::error_code& ec) const {
    ec.clear();
}

bool VirtualFileSystem::Exists(const Path& path, std::error_code& ec) const {
    return m_impl->ResolveIfExist(path, ec).has_value();
}

FileInfo VirtualFileSystem::Stat(const Path& path, std::error_code& ec) const {
    auto vfNode = m_impl->ResolveIfExist(path, ec);
    if (!vfNode) {
        return {};
    }
    if (!vfNode->fs) {
        return {
            .type = FileType::Directory,
            .readable = true,
            .writable = false,
            .size = 0,
            .createdTime = vfNode->creationTime,
            .modifiedTime = vfNode->creationTime,
            .accessedTime = 0
        };
    }
    return vfNode->fs->Stat(vfNode->path, ec);
}

bool VirtualFileSystem::IsDirectory(const Path& path, std::error_code& ec) const {
    auto vfNode = m_impl->ResolveIfExist(path, ec);
    if (!vfNode) {
        return false;
    }
    if (!vfNode->fs) {
        return true;
    }
    return vfNode->fs->IsDirectory(vfNode->path, ec);
}

bool VirtualFileSystem::IsRegularFile(const Path& path, std::error_code& ec) const {
    auto vfNode = m_impl->ResolveIfExist(path, ec);
    if (!vfNode) {
        return false;
    }
    if (!vfNode->fs) {
        return false;
    }
    return vfNode->fs->IsRegularFile(vfNode->path, ec);
}

bool VirtualFileSystem::IsReadable(const Path& path, std::error_code& ec) const {
    auto vfNode = m_impl->ResolveIfExist(path, ec);
    if (!vfNode) {
        return false;
    }
    if (!vfNode->fs) {
        return true;
    }
    return vfNode->fs->IsReadable(vfNode->path, ec);
}

bool VirtualFileSystem::IsWritable(const Path& path, std::error_code& ec) const {
    auto vfNode = m_impl->ResolveIfExist(path, ec);
    if (!vfNode) {
        return false;
    }
    if (!vfNode->fs) {
        return false;
    }
    return vfNode->fs->IsWritable(vfNode->path, ec);
}

std::uint64_t VirtualFileSystem::Size(const Path& path, std::error_code& ec) const {
    auto vfNode = m_impl->ResolveIfExist(path, ec);
    if (!vfNode) {
        return 0;
    }
    if (!vfNode->fs) {
        return 0;
    }
    return vfNode->fs->Size(vfNode->path, ec);
}

std::vector<std::string> VirtualFileSystem::List(const Path& path, std::error_code& ec) const {
    return m_impl->ResolveList(path, ec);
}

void VirtualFileSystem::CreateFile(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->CreateFile(vfNode->path, ec);
    }
}

void VirtualFileSystem::CreateDirectory(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->CreateDirectory(vfNode->path, ec);
    }
}

void VirtualFileSystem::CreateDirectories(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->CreateDirectories(vfNode->path, ec);
    }
}

void VirtualFileSystem::DeleteFile(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->DeleteFile(vfNode->path, ec);
    }
}

void VirtualFileSystem::DeleteDirectory(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->DeleteDirectory(vfNode->path, ec);
    }
}

void VirtualFileSystem::DeleteDirectories(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->DeleteDirectories(vfNode->path, ec);
    }
}

std::unique_ptr<FileHandle> VirtualFileSystem::Open(const Path& path, OpenOption openOptions, std::error_code& ec) {
    ec.clear();

    bool isWrite = (openOptions & (OpenOption::Create | OpenOption::Append | OpenOption::Truncate)) != OpenOption::None;

    std::optional<Impl::VirtualFileNode> vfNode;
    if (isWrite) {
        vfNode = m_impl->Resolve(path);
    } else {
        vfNode = m_impl->ResolveIfExist(path, ec);
    }
    if (!vfNode) {
        if (!ec) {
            ec = std::make_error_code(std::errc::no_such_file_or_directory);
        }
        return nullptr;
    }
    if (!vfNode->fs) {
        ec = std::make_error_code(std::errc::read_only_file_system);
        return nullptr;
    }
    return vfNode->fs->Open(vfNode->path, openOptions, ec);
}

} // namespace slimenano::filesystem
