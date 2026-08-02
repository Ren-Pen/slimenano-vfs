/**
 * @file Path.cpp
 * @brief Implementation of the Path class.
 */

#include <slimenano/vfs/Path.h>

#include <cstddef>
#include <vector>

namespace slimenano::filesystem {

namespace {

/**
 * @brief Checks whether a character is a path separator.
 *
 * @param c The character to test.
 *
 * @return true for '/' and '\\'.
 */
constexpr bool isSeparator(char c) noexcept {
    return c == Path::kPathSpliterator || c == '\\';
}

/**
 * @brief Normalizes a raw path string.
 *
 * Collapses duplicate separators, resolves "." and ".." segments, keeps a
 * leading separator for absolute paths, and returns "/" for the root.
 *
 * @param path The raw path to normalize.
 *
 * @return The normalized path string.
 */
std::string normalize(std::string_view path) {
    if (path.empty()) {
        return {};
    }

    bool absolute = isSeparator(path.front());

    std::vector<std::string_view> segments;
    segments.reserve(8);
    std::size_t i = 0, n = path.size();
    while (i < n) {
        while (i < n && isSeparator(path[i])) {
            ++i;
        }
        auto start = i;
        while (i < n && !isSeparator(path[i])) {
            ++i;
        }
        if (i == start) {
            break;
        }

        std::string_view segment = path.substr(start, i - start);

        if (segment == ".") {
            continue;
        }

        if (segment == "..") {
            if (!segments.empty() && segments.back() != "..") {
                segments.pop_back();
            } else if (!absolute) {
                segments.push_back(segment);
            }
            continue;
        }

        segments.push_back(segment);
    }

    std::string normalized;
    normalized.reserve(path.size());
    if (absolute)
        normalized.push_back(Path::kPathSpliterator);
    for (std::size_t k = 0; k < segments.size(); ++k) {
        if (k) {
            normalized.push_back(Path::kPathSpliterator);
        }
        normalized.append(segments[k]);
    }
    if (normalized.empty()) {
        return absolute ? "/" : "";
    }
    return normalized;
}

} // namespace

/**
 * @brief Constructs the Path, normalizing @p path.
 *
 * @param path The UTF-8 path to represent.
 */
Path::Path(std::string_view path) : m_path(normalize(path)) {
}

/**
 * @brief Returns the normalized path string.
 *
 * @return The path as a UTF-8 string view.
 */
std::string_view Path::String() const noexcept {
    return m_path;
}

/**
 * @brief Returns the parent directory of the path.
 *
 * @return The parent directory; empty for a parentless path and "/" for the
 *         root.
 */
Path Path::Parent() const {
    if (m_path.empty()) {
        return {};
    }

    const auto pos = m_path.rfind(kPathSpliterator);
    if (pos == std::string::npos) {
        return {};
    }

    if (pos == 0) {
        return {"/"};
    }

    return {m_path.substr(0, pos)};
}

/**
 * @brief Resolves the path against a base path.
 *
 * @param base The base path to resolve against.
 *
 * @return The resolved path.
 */
Path Path::ToAbsolute(const Path& base) const {
    if (IsAbsolute()) {
        return *this;
    }

    if (base.IsRelative()) {
        return {"/" + base.m_path + "/" + m_path};
    }

    return {base.m_path + "/" + m_path};
}

/**
 * @brief Converts the path to a relative form against a base path.
 *
 * @param base The base path to convert against.
 *
 * @return The relative path, or an empty path when the paths differ in
 *         absolute-ness.
 */
Path Path::ToRelative(const Path& base) const {
    if (IsAbsolute() != base.IsAbsolute()) {
        return {};
    }

    auto thisIt = begin();
    auto baseIt = base.begin();

    while (thisIt != end() && baseIt != base.end() && *thisIt == *baseIt) {
        ++thisIt;
        ++baseIt;
    }

    std::string result;
    result.reserve(m_path.size() + base.m_path.size());
    for (; baseIt != base.end(); ++baseIt) {
        if (!result.empty()) {
            result.push_back(kPathSpliterator);
        }
        result += "..";
    }

    for (; thisIt != end(); ++thisIt) {
        if (!result.empty()) {
            result.push_back(kPathSpliterator);
        }
        result += *thisIt;
    }

    return {result};
}

/**
 * @brief Returns the final path component.
 *
 * @return The filename, or an empty view for an empty path or the root.
 */
std::string_view Path::Filename() const {
    if (m_path.empty() || m_path == "/") {
        return {};
    }

    const auto pos = m_path.rfind(kPathSpliterator);
    if (pos == std::string::npos) {
        return m_path;
    }

    return std::string_view(m_path).substr(pos + 1);
}

/**
 * @brief Returns the filename without its final extension.
 *
 * @return The stem, or an empty view when the path has no filename.
 */
std::string_view Path::Stem() const {
    const auto filename = Filename();
    if (filename.empty()) {
        return {};
    }

    const auto pos = filename.rfind('.');
    if (pos == std::string_view::npos || pos == 0 || pos + 1 == filename.size()) {
        return filename;
    }

    return filename.substr(0, pos);
}

/**
 * @brief Returns the final extension of the filename without the dot.
 *
 * @return The extension, or an empty view when there is none.
 */
std::string_view Path::Extension() const {
    const auto filename = Filename();
    if (filename.empty()) {
        return {};
    }

    const auto pos = filename.rfind('.');
    if (pos == std::string_view::npos || pos == 0 || pos + 1 == filename.size()) {
        return {};
    }
    return filename.substr(pos + 1);
}

/**
 * @brief Checks whether the path is absolute.
 *
 * @return true when the path starts with the root separator.
 */
bool Path::IsAbsolute() const noexcept {
    return !m_path.empty() && m_path.front() == kPathSpliterator;
}

/**
 * @brief Checks whether the path is relative.
 *
 * @return true when the path is not absolute.
 */
bool Path::IsRelative() const noexcept {
    return !IsAbsolute();
}

/**
 * @brief Checks whether the path is empty.
 *
 * @return true when the path contains no characters.
 */
bool Path::Empty() const noexcept {
    return m_path.empty();
}

/**
 * @brief Joins the path with @p rhs using a separator.
 *
 * @param rhs The path to append.
 *
 * @return The joined path.
 */
Path Path::operator/(const Path& rhs) const {
    if (rhs.IsAbsolute()) {
        return rhs;
    }

    if (m_path.empty()) {
        return rhs;
    }

    if (rhs.m_path.empty()) {
        return *this;
    }

    std::string result;
    result.reserve(m_path.size() + rhs.m_path.size() + 1);

    result += m_path;
    if (result.back() != kPathSpliterator) {
        result.push_back(kPathSpliterator);
    }
    result += rhs.m_path;

    return {result};
}

/**
 * @brief Checks whether the path is non-empty.
 *
 * @return true when the path is not empty.
 */
Path::operator bool() const noexcept {
    return !Empty();
}

/**
 * @brief Compares two iterators for equality.
 *
 * Iterators compare equal when they are positioned at the same segment.
 *
 * @param rhs The iterator to compare with.
 *
 * @return true when the iterators are at the same position.
 */
bool Path::Iterator::operator==(const Iterator& rhs) const noexcept {
    return m_begin == rhs.m_begin;
}

/**
 * @brief Locates the end offset of the current segment.
 */
void Path::Iterator::findEnd() noexcept {
    if (m_begin < m_path.size() && m_path[m_begin] == Path::kPathSpliterator) {
        m_end = m_begin + 1;
        return;
    }
    m_end = m_begin;
    while (m_end < m_path.size() && m_path[m_end] != Path::kPathSpliterator) {
        ++m_end;
    }
}

/**
 * @brief Constructs an iterator at a given position.
 *
 * @param path  The path string to iterate over.
 * @param begin Offset of the first segment.
 */
Path::Iterator::Iterator(std::string_view path, std::size_t begin) noexcept : m_path(path), m_begin(begin) {
    if (m_begin != std::string_view::npos) {
        findEnd();
    }
}

/**
 * @brief Returns the current segment.
 *
 * @return The segment as a string view.
 */
std::string_view Path::Iterator::operator*() const noexcept {
    return m_path.substr(m_begin, m_end - m_begin);
}

/**
 * @brief Advances the iterator to the next segment.
 *
 * @return The advanced iterator.
 */
Path::Iterator& Path::Iterator::operator++() noexcept {
    if (m_begin == std::string_view::npos) {
        return *this;
    }
    m_begin = m_end;
    while (m_begin < m_path.size() && m_path[m_begin] == Path::kPathSpliterator) {
        ++m_begin;
    }
    if (m_begin >= m_path.size()) {
        m_begin = m_end = std::string_view::npos;
        return *this;
    }
    findEnd();
    return *this;
}

/**
 * @brief Advances the iterator to the next segment (postfix form).
 *
 * @return The iterator before advancement.
 */
Path::Iterator Path::Iterator::operator++(int) noexcept {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

/**
 * @brief Returns an iterator to the first segment.
 *
 * @return The begin iterator, or end() for an empty path.
 */
Path::Iterator Path::begin() const noexcept {
    if (m_path.empty()) {
        return end();
    }
    if (m_path.front() == kPathSpliterator) {
        return Iterator(m_path, 0);
    }
    std::size_t pos = 0;
    while (pos < m_path.size() && m_path[pos] == kPathSpliterator) {
        ++pos;
    }
    if (pos >= m_path.size()) {
        return end();
    }
    return Iterator(m_path, pos);
}

/**
 * @brief Returns the past-the-end iterator.
 *
 * @return The end iterator.
 */
Path::Iterator Path::end() const noexcept {
    return Iterator(m_path, std::string_view::npos);
}

} // namespace slimenano::filesystem
