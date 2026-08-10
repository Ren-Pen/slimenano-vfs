#ifndef SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_PATH_TREE_H
#define SLIMENANO_VFS_INCLUDE_VFS_VIRTUAL_PATH_TREE_H

#include <memory>
#include <map>
#include <concepts>
#include <functional>

#include <slimenano/vfs/Path.h>

namespace slimenano::filesystem {

template <typename T> class VirtualPathTree {

public:
    struct Node {
        Node(const std::shared_ptr<Node>& parent, std::string_view path) :
            parent(parent), absolutePath(parent ? parent->absolutePath / path : path) {}

        std::weak_ptr<Node> parent;
        Path absolutePath;
        std::map<std::string, std::shared_ptr<Node>, std::less<>> children{};

        T data{};
    };

    template <typename Pred>
        requires std::predicate<Pred, const T&>
    void CleanupNode(const std::shared_ptr<Node>& ptr, Pred empty) {
        if (!empty(ptr->data) || !ptr->children.empty()) {
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

        CleanupNode(parent, empty);
    }

    std::pair<std::shared_ptr<Node>, bool> ResolveNode(const Path& path) const {
        auto absolutePath = path.ToAbsolute(Path::Root());
        auto node = m_root;
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

    std::shared_ptr<Node> FindNode(const Path& path) const {
        auto [node, exact] = ResolveNode(path);
        return exact ? node : nullptr;
    }

    std::shared_ptr<Node> CreateNodeIfAbsent(const Path& path) {
        Path absolutePath = path.ToAbsolute(Path::Root());

        auto node = m_root;

        for (auto segment : absolutePath) {
            if (segment == "/") {
                continue;
            }
            auto it = node->children.find(segment);
            if (it == node->children.end()) {
                it = node->children.try_emplace(std::string{segment}, std::make_shared<Node>(node, segment)).first;
            }
            node = it->second;
        }

        return node;
    }

private:
    std::shared_ptr<Node> m_root = std::make_shared<Node>(nullptr, "/");
};

} // namespace slimenano::filesystem

#endif
