#include <slimenano/vfs/Path.h>

#include <gtest/gtest.h>

#include <iterator>
#include <string_view>
#include <vector>

using slimenano::filesystem::Path;

static_assert(std::forward_iterator<Path::Iterator>);

namespace {

std::vector<std::string_view> Segments(const Path& p) {
    std::vector<std::string_view> out;
    for (auto seg : p) {
        out.push_back(seg);
    }
    return out;
}

using SV = std::vector<std::string_view>;

} // namespace

// ---------------------------------------------------------------------------
// Segment enumeration
// ---------------------------------------------------------------------------

TEST(PathIterator, AbsoluteMultiSegment) {
    EXPECT_EQ(Segments(Path("/foo/bar")), (SV{"/", "foo", "bar"}));
}

TEST(PathIterator, AbsoluteSingleSegment) {
    EXPECT_EQ(Segments(Path("/foo")), (SV{"/", "foo"}));
}

TEST(PathIterator, RootOnly) {
    EXPECT_EQ(Segments(Path("/")), (SV{"/"}));
}

TEST(PathIterator, RelativeMultiSegment) {
    EXPECT_EQ(Segments(Path("foo/bar")), (SV{"foo", "bar"}));
}

TEST(PathIterator, RelativeSingleSegment) {
    EXPECT_EQ(Segments(Path("foo")), (SV{"foo"}));
}

TEST(PathIterator, DeepPath) {
    EXPECT_EQ(Segments(Path("/a/b/c/d")), (SV{"/", "a", "b", "c", "d"}));
}

TEST(PathIterator, EmptyYieldsNothing) {
    EXPECT_TRUE(Segments(Path("")).empty());
}

// ---------------------------------------------------------------------------
// Iterator mechanics
// ---------------------------------------------------------------------------

TEST(PathIterator, PostfixReturnsOldValue) {
    Path p("/a/b");
    auto it = p.begin();
    auto old = it++;
    EXPECT_EQ(*old, "/");
    EXPECT_EQ(*it, "a");
}

TEST(PathIterator, MultiPassIsStable) {
    Path p("/x/y");
    EXPECT_EQ(Segments(p), Segments(p));
}

TEST(PathIterator, IncrementPastEndIsSafe) {
    Path p("/z");
    auto e = p.end();
    ++e;
    EXPECT_EQ(e, p.end());
}

TEST(PathIterator, BeginEqualsEndForEmpty) {
    Path p("");
    EXPECT_EQ(p.begin(), p.end());
}

TEST(PathIterator, RangeForCountsRoot) {
    int count = 0;
    for ([[maybe_unused]] auto seg : Path("/usr/local/bin")) {
        ++count;
    }
    EXPECT_EQ(count, 4);
}
