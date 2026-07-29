#ifndef SLIMENANO_VFS_INCLUDE_VFS_PATH_H
#define SLIMENANO_VFS_INCLUDE_VFS_PATH_H

#include <cstddef>
#include <string>
#include <string_view>
#include <iterator>
#include <functional>

namespace slimenano::filesystem {

class Path {
public:
    constexpr inline static char kPathSpliterator = '/';

    Path() = default;
    Path(std::string_view path);
    Path(const std::string& path) : Path(std::string_view(path)) {}
    Path(const char* path) : Path(std::string_view(path)) {}

    std::string_view String() const noexcept;

    Path Parent() const;
    Path ToAbsolute(const Path& base) const;
    Path ToRelative(const Path& base) const;

    std::string_view Filename() const;
    std::string_view Stem() const;
    std::string_view Extension() const;

    bool IsAbsolute() const noexcept;
    bool IsRelative() const noexcept;

    bool Empty() const noexcept;

    Path operator/(const Path& rhs) const;
    explicit operator bool() const noexcept;

    bool operator==(const Path&) const = default;
    auto operator<=>(const Path&) const = default;

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::string_view;
        using reference = std::string_view;
        using difference_type = std::ptrdiff_t;
        using pointer = void;

        Iterator() = default;

        std::string_view operator*() const noexcept;
        Iterator& operator++() noexcept;
        Iterator operator++(int) noexcept;
        bool operator==(const Iterator&) const noexcept;

    private:
        friend class Path;

        Iterator(std::string_view path, std::size_t begin) noexcept;

        void findEnd() noexcept;

        std::string_view m_path;
        std::size_t m_begin{std::string_view::npos};
        std::size_t m_end{std::string_view::npos};
    };

    Iterator begin() const noexcept;
    Iterator end() const noexcept;

private:
    std::string m_path{};
};

} // namespace slimenano::filesystem

template <> struct std::hash<slimenano::filesystem::Path> {
    std::size_t operator()(const slimenano::filesystem::Path& p) const noexcept {
        return std::hash<std::string_view>{}(p.String());
    }
};

#endif
