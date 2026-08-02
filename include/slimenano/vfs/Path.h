/**
 * @file Path.h
 * @brief Virtual path type used throughout the library.
 */

#ifndef SLIMENANO_VFS_INCLUDE_VFS_PATH_H
#define SLIMENANO_VFS_INCLUDE_VFS_PATH_H

#include <cstddef>
#include <string>
#include <string_view>
#include <iterator>
#include <functional>

namespace slimenano::filesystem {

/**
 * @brief A normalized, library-defined virtual path.
 *
 * A Path is stored internally as a UTF-8 string using '/' as the separator;
 * backslashes are accepted on input and normalized to '/'. The input is
 * normalized on construction: duplicate separators are collapsed and '.'
 * and '..' segments are resolved.
 *
 * Paths are implicitly constructible from std::string, const char* and
 * std::string_view, and they compare with the spaceship operator, which
 * makes them usable as ordered container keys. A std::hash specialization
 * also allows them to be used with unordered containers.
 */
class Path {
public:
    /**
     * @brief Separator character used in the stored form of a path.
     *
     * Both '/' and '\\' are accepted as separators on input, but the stored
     * form always uses this character.
     */
    constexpr inline static char kPathSpliterator = '/';

    /** @brief Constructs an empty path. */
    Path() = default;

    /**
     * @brief Constructs a path from a UTF-8 string view.
     *
     * The input is normalized on construction.
     *
     * @param path The UTF-8 path to represent; may be absolute or relative.
     */
    Path(std::string_view path);

    /** @brief Constructs a path from a UTF-8 std::string. */
    Path(const std::string& path) : Path(std::string_view(path)) {}

    /** @brief Constructs a path from a null-terminated UTF-8 string. */
    Path(const char* path) : Path(std::string_view(path)) {}

    /**
     * @brief Returns the normalized path as a UTF-8 string view.
     *
     * @note The returned view aliases the Path's internal storage and is
     *       invalidated by any operation that assigns to the Path.
     *
     * @return The normalized path; "/" for the root and an empty view for an
     *         empty path.
     */
    std::string_view String() const noexcept;

    /**
     * @brief Returns the parent directory of the path.
     *
     * @return The parent directory; an empty path when the path has no
     *         parent. For the root path "/", the root itself is returned.
     */
    Path Parent() const;

    /**
     * @brief Resolves this path against a base path.
     *
     * An absolute path is returned unchanged; a relative path is appended to
     * @p base. A relative @p base is itself treated as relative to the root.
     *
     * @param base The base path to resolve against.
     *
     * @return The resolved absolute path.
     */
    Path ToAbsolute(const Path& base) const;

    /**
     * @brief Expresses this path relative to a base path.
     *
     * The common ancestor is determined segment by segment, and the result
     * is padded with ".." segments where needed.
     *
     * @param base The base path to make this path relative to.
     *
     * @return The relative path, or an empty path when the two paths are not
     *         both absolute or both relative.
     */
    Path ToRelative(const Path& base) const;

    /**
     * @brief Returns the final component of the path.
     *
     * @return The file or directory name; empty for an empty path and for
     *         the root path.
     */
    std::string_view Filename() const;

    /**
     * @brief Returns the filename without its final extension.
     *
     * @return The stem; empty when the path has no filename. A filename that
     *         begins with a dot (e.g. ".gitignore") or ends with a dot is
     *         returned unchanged.
     */
    std::string_view Stem() const;

    /**
     * @brief Returns the final extension of the filename without the dot.
     *
     * @return The extension, e.g. "txt" for "readme.txt"; empty when the
     *         filename has no extension, begins with a dot, or ends with a
     *         dot.
     */
    std::string_view Extension() const;

    /**
     * @brief Checks whether the path is absolute.
     *
     * @return true when the path starts with the root separator.
     */
    bool IsAbsolute() const noexcept;

    /**
     * @brief Checks whether the path is relative.
     *
     * @return true when the path is not absolute; an empty path counts as
     *         relative.
     */
    bool IsRelative() const noexcept;

    /**
     * @brief Checks whether the path is empty.
     *
     * @return true when the path contains no characters.
     */
    bool Empty() const noexcept;

    /**
     * @brief Joins two path components with a separator.
     *
     * An absolute right-hand side replaces the left-hand side; an empty
     * operand leaves the other side unchanged.
     *
     * @param rhs The path to append.
     *
     * @return The joined path.
     */
    Path operator/(const Path& rhs) const;

    /**
     * @brief Checks whether the path is non-empty.
     *
     * @return true when the path is not empty.
     */
    explicit operator bool() const noexcept;

    /**
     * @brief Compares two paths for equality.
     */
    bool operator==(const Path&) const = default;

    /**
     * @brief Orders two paths lexicographically.
     */
    auto operator<=>(const Path&) const = default;

    /**
     * @brief Forward iterator over the segments of a path.
     *
     * Dereferencing yields each segment as a std::string_view; for absolute
     * paths the root segment "/" is yielded first. Iteration is suitable for
     * range-based for loops: for (std::string_view segment : path) { ... }
     */
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::string_view;
        using reference = std::string_view;
        using difference_type = std::ptrdiff_t;
        using pointer = void;

        /** @brief Constructs the past-the-end iterator. */
        Iterator() = default;

        /**
         * @brief Dereferences the iterator.
         *
         * @return The current segment as a string view.
         */
        std::string_view operator*() const noexcept;

        /**
         * @brief Advances the iterator to the next segment; prefix form.
         *
         * @return A reference to the advanced iterator.
         */
        Iterator& operator++() noexcept;

        /**
         * @brief Advances the iterator to the next segment; postfix form.
         *
         * @return A copy of the iterator before advancement.
         */
        Iterator operator++(int) noexcept;

        /**
         * @brief Compares two iterators for equality.
         *
         * @param rhs The iterator to compare with.
         *
         * @return true when both iterators refer to the same position.
         */
        bool operator==(const Iterator&) const noexcept;

    private:
        friend class Path;

        /**
         * @brief Constructs an iterator positioned at segment @p begin.
         *
         * @param path  The path string to iterate over.
         * @param begin Byte offset of the first segment.
         */
        Iterator(std::string_view path, std::size_t begin) noexcept;

        /**
         * @brief Locates the end offset of the current segment.
         */
        void findEnd() noexcept;

        std::string_view m_path;
        std::size_t m_begin{std::string_view::npos};
        std::size_t m_end{std::string_view::npos};
    };

    /**
     * @brief Returns an iterator to the first segment of the path.
     *
     * For an empty path, begin() equals end().
     *
     * @return An iterator positioned at the first segment.
     */
    Iterator begin() const noexcept;

    /**
     * @brief Returns the past-the-end iterator.
     *
     * @return An iterator that compares equal to the end of the path.
     */
    Iterator end() const noexcept;

private:
    std::string m_path{};
};

} // namespace slimenano::filesystem

/**
 * @brief Hash specialization enabling Path to be used with unordered
 *        containers.
 *
 * The path is hashed by its string form.
 */
template <> struct std::hash<slimenano::filesystem::Path> {
    /**
     * @brief Hashes a Path.
     *
     * @param p The path to hash.
     *
     * @return A hash of the path's string form.
     */
    std::size_t operator()(const slimenano::filesystem::Path& p) const noexcept {
        return std::hash<std::string_view>{}(p.String());
    }
};

#endif
