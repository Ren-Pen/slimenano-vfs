/**
 * @file VirtualFileSystem.cpp
 * @brief Implementation of the mount-point based virtual file system.
 */

#include <slimenano/vfs/VirtualFileSystem.h>

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <chrono>

namespace slimenano::filesystem {

namespace {

namespace chrono = std::chrono;

/**
 * @brief Root of the virtual tree.
 */
const Path kRoot{"/"};
} // namespace

/**
 * @brief Private implementation of the virtual file system: the mount tree.
 */
struct VirtualFileSystem::Impl {

    /**
     * @brief A node in the virtual directory tree.
     */
    struct Node {

        /**
         * @brief Constructs a node under a parent.
         *
         * @param parent Parent node; nullptr for the root.
         * @param path   Name of this node within the parent.
         */
        Node(const std::shared_ptr<Node>& parent, std::string_view path) :
            parent(parent), absolutePath(parent ? parent->absolutePath / path : path),
            creationTime(
                chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count()
            ) {};
        /** @brief Parent node; empty for the root. */
        std::weak_ptr<Node> parent;
        /** @brief Absolute virtual path of this node. */
        Path absolutePath;
        /** @brief Creation time, in milliseconds since the Unix epoch. */
        std::int64_t creationTime;
        /** @brief File systems mounted at this node, in mounting order. */
        std::vector<std::shared_ptr<FileSystem>> fileSystems{};
        /** @brief Child nodes, keyed by segment name. */
        std::map<std::string, std::shared_ptr<Node>, std::less<>> children{};
    };

    std::shared_ptr<Node> root = std::make_shared<Node>(nullptr, "/");

    /**
     * @brief Looks up the node for a virtual path.
     *
     * @param path Virtual path to look up.
     *
     * @return The node, or nullptr when the path does not exist in the tree.
     */
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

    /**
     * @brief Removes empty nodes from the tree, bottom-up.
     *
     * A node with no children and no mounted file systems is detached from
     * its parent, and the parent is then cleaned up recursively.
     *
     * @param ptr The node to clean up.
     */
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

    /**
     * @brief Result of resolving a virtual path.
     *
     * @p fs is the file system that owns the path (nullptr for a pure
     * virtual directory), @p path is the path relative to that file
     * system's mount point, and @p creationTime is the creation time of the
     * owning node.
     */
    struct VirtualFileNode {
        /** @brief File system owning the path; nullptr for a virtual directory. */
        std::shared_ptr<FileSystem> fs;
        /** @brief Path relative to the file system's mount point. */
        Path path;
        /** @brief Creation time of the owning node, in milliseconds. */
        std::int64_t creationTime;
    };

    /**
     * @brief Walks the tree to the node nearest a path.
     *
     * @param absolutePath The absolute path to walk.
     *
     * @return The nearest existing node and whether the full path was found
     *         exactly.
     */
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

    /**
     * @brief Resolves a path to its owning file system.
     *
     * Returns the file system mounted at the deepest ancestor node that has
     * one; a null file system is returned for pure virtual directories.
     *
     * @param path Virtual path to resolve.
     *
     * @return The resolved owner, or std::nullopt when no node exists.
     */
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

    /**
     * @brief Resolves a path to a file system where the entry exists.
     *
     * Like Resolve, but additionally asks each candidate mount whether the
     * entry exists and returns the first mount that reports it.
     *
     * @param path Virtual path to resolve.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The resolved owner, or std::nullopt when no entry exists.
     */
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

    /**
     * @brief Resolves a path for a write operation.
     *
     * Fails with std::errc::no_such_file_or_directory when the path does
     * not resolve, and std::errc::read_only_file_system when it resolves to
     * a pure virtual directory.
     *
     * @param path Virtual path to resolve.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The resolved owner, or std::nullopt on failure.
     */
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

    /**
     * @brief Collects the entries visible at a virtual path.
     *
     * Merges the child nodes of the exact node with the listings of every
     * candidate mount at or above the path. Names are deduplicated and
     * sorted.
     *
     * @param path Virtual path to list.
     * @param ec   On failure, set to an error code describing the problem.
     *
     * @return The merged entry names.
     */
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

/**
 * @brief Constructs an empty virtual file system.
 */
VirtualFileSystem::VirtualFileSystem() : m_impl(std::make_unique<Impl>()) {
}

/**
 * @brief Destroys the virtual file system.
 */
VirtualFileSystem::~VirtualFileSystem() = default;

/**
 * @brief Mounts a file system at a virtual directory.
 *
 * @param mountPoint Virtual directory at which to attach @p ptr.
 * @param ptr        The file system to mount.
 */
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

/**
 * @brief Checks whether at least one file system is mounted at a virtual
 *        directory.
 *
 * @param mountPoint The virtual directory to query.
 *
 * @return true when at least one file system is mounted there.
 */
bool VirtualFileSystem::IsMounted(const Path& mountPoint) const {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return false;
    }
    return !node->fileSystems.empty();
}

/**
 * @brief Checks whether a specific file system instance is mounted at a
 *        virtual directory.
 *
 * @param mountPoint The virtual directory to query.
 * @param ptr        The file system instance to look for.
 *
 * @return true when @p ptr is mounted at @p mountPoint.
 */
bool VirtualFileSystem::IsMounted(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr) const {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return false;
    }
    return std::ranges::find(node->fileSystems, ptr) != node->fileSystems.end();
}

/**
 * @brief Detaches all file systems mounted at a virtual directory.
 *
 * @param mountPoint The virtual directory to unmount.
 */
void VirtualFileSystem::Unmount(const Path& mountPoint) {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return;
    }
    node->fileSystems.clear();
    m_impl->cleanupNode(node);
}

/**
 * @brief Detaches a specific file system instance from a virtual directory.
 *
 * @param mountPoint The virtual directory to unmount.
 * @param ptr        The file system instance to detach.
 */
void VirtualFileSystem::Unmount(const Path& mountPoint, const std::shared_ptr<FileSystem>& ptr) {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return;
    }
    std::erase(node->fileSystems, ptr);
    m_impl->cleanupNode(node);
}

/**
 * @brief Returns the file systems mounted at a virtual directory.
 *
 * @param mountPoint The virtual directory to query.
 *
 * @return The mounted file systems in mounting order; empty when nothing is
 *         mounted there.
 */
std::vector<std::shared_ptr<FileSystem>> VirtualFileSystem::GetMountedFileSystems(const Path& mountPoint) const {
    auto node = m_impl->findNode(mountPoint);
    if (!node) {
        return {};
    }
    return node->fileSystems;
}

/**
 * @brief Validates the virtual file system.
 *
 * The virtual file system is always valid, so the error code is simply
 * cleared.
 *
 * @param ec On success, cleared.
 */
void VirtualFileSystem::Initialize(std::error_code& ec) const {
    ec.clear();
}

/**
 * @brief Checks whether an entry exists at @p path.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when an entry exists; virtual directories always exist.
 */
bool VirtualFileSystem::Exists(const Path& path, std::error_code& ec) const {
    return m_impl->ResolveIfExist(path, ec).has_value();
}

/**
 * @brief Returns metadata about the entry at @p path.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return The entry's metadata, or an empty FileInfo when the entry does
 *         not exist.
 */
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

/**
 * @brief Checks whether the entry at @p path is a directory.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry is a directory; virtual directories count as
 *         directories.
 */
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

/**
 * @brief Checks whether the entry at @p path is a regular file.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry is a regular file.
 */
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

/**
 * @brief Checks whether the entry at @p path can be read.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry is readable; virtual directories are
 *         readable.
 */
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

/**
 * @brief Checks whether the entry at @p path can be written.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return true when the entry is writable; virtual directories are not
 *         writable.
 */
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

/**
 * @brief Returns the size of the entry at @p path in bytes.
 *
 * @param path The virtual path to query.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return The size in bytes; zero when the entry does not exist or is not a
 *         regular file.
 */
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

/**
 * @brief Lists the entries visible at @p path.
 *
 * @param path The virtual path to list.
 * @param ec   On failure, set to an error code describing the problem.
 *
 * @return The merged, deduplicated entry names.
 */
std::vector<std::string> VirtualFileSystem::List(const Path& path, std::error_code& ec) const {
    return m_impl->ResolveList(path, ec);
}

/**
 * @brief Creates a new file at @p path.
 *
 * @param path The virtual path of the file to create.
 * @param ec   On failure, set to an error code describing the problem.
 */
void VirtualFileSystem::CreateFile(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->CreateFile(vfNode->path, ec);
    }
}

/**
 * @brief Creates a single directory at @p path.
 *
 * @param path The virtual path of the directory to create.
 * @param ec   On failure, set to an error code describing the problem.
 */
void VirtualFileSystem::CreateDirectory(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->CreateDirectory(vfNode->path, ec);
    }
}

/**
 * @brief Creates the directory at @p path and any missing parent
 *        directories.
 *
 * @param path The virtual path of the directory to create.
 * @param ec   On failure, set to an error code describing the problem.
 */
void VirtualFileSystem::CreateDirectories(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->CreateDirectories(vfNode->path, ec);
    }
}

/**
 * @brief Deletes the file at @p path.
 *
 * @param path The virtual path of the file to delete.
 * @param ec   On failure, set to an error code describing the problem.
 */
void VirtualFileSystem::DeleteFile(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->DeleteFile(vfNode->path, ec);
    }
}

/**
 * @brief Deletes the empty directory at @p path.
 *
 * @param path The virtual path of the directory to delete.
 * @param ec   On failure, set to an error code describing the problem.
 */
void VirtualFileSystem::DeleteDirectory(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->DeleteDirectory(vfNode->path, ec);
    }
}

/**
 * @brief Recursively deletes the directory at @p path and everything it
 *        contains.
 *
 * @param path The virtual path of the directory to delete.
 * @param ec   On failure, set to an error code describing the problem.
 */
void VirtualFileSystem::DeleteDirectories(const Path& path, std::error_code& ec) {
    if (auto vfNode = m_impl->ResolveForWrite(path, ec)) {
        vfNode->fs->DeleteDirectories(vfNode->path, ec);
    }
}

/**
 * @brief Opens the file at @p path and returns a handle to it.
 *
 * Read requests are resolved against entries that actually exist; write
 * requests may also target pure virtual directories, which then fail with
 * std::errc::read_only_file_system.
 *
 * @param path        The virtual path of the file to open.
 * @param openOptions Flags controlling access mode and creation.
 * @param ec          On failure, set to an error code describing the
 *                    problem.
 *
 * @return A handle to the open file, or nullptr on failure.
 */
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
