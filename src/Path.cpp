#include <slimenano/vfs/Path.h>

#include <cstddef>
#include <vector>

namespace slimenano::filesystem {

namespace {

constexpr bool isSeparator(char c) noexcept {
    return c == Path::kPathSpliterator || c == '\\';
}

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

Path::Path(std::string_view path) : m_path(normalize(path)) {
}

std::string_view Path::String() const noexcept {
    return m_path;
}

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

Path Path::ToAbsolute(const Path& base) const {
    if (IsAbsolute()) {
        return *this;
    }

    if (base.IsRelative()) {
        return {"/" + base.m_path + "/" + m_path};
    }

    return {base.m_path + "/" + m_path};
}

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

bool Path::IsAbsolute() const noexcept {
    return !m_path.empty() && m_path.front() == kPathSpliterator;
}
bool Path::IsRelative() const noexcept {
    return !IsAbsolute();
}

bool Path::Empty() const noexcept {
    return m_path.empty();
}

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

Path::operator bool() const noexcept {
    return !Empty();
}

bool Path::Iterator::operator==(const Iterator& rhs) const noexcept {
    return m_begin == rhs.m_begin;
}

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

Path::Iterator::Iterator(std::string_view path, std::size_t begin) noexcept : m_path(path), m_begin(begin) {
    if (m_begin != std::string_view::npos) {
        findEnd();
    }
}

std::string_view Path::Iterator::operator*() const noexcept {
    return m_path.substr(m_begin, m_end - m_begin);
}

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

Path::Iterator Path::Iterator::operator++(int) noexcept {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

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

Path::Iterator Path::end() const noexcept {
    return Iterator(m_path, std::string_view::npos);
}

} // namespace slimenano::filesystem
