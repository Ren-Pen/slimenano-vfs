#include <gtest/gtest.h>
#include <slimenano/vfs/Path.h>

using slimenano::filesystem::Path;

struct ParentCase {
    std::string path;
    std::string parent;
};

class PathParentTest : public ::testing::TestWithParam<ParentCase> {};

TEST_P(PathParentTest, Parent) {
    EXPECT_EQ(Path(GetParam().path).Parent().String(), GetParam().parent);
}

INSTANTIATE_TEST_SUITE_P(
    Parent,
    PathParentTest,
    ::testing::Values(
        // empty / single segment
        ParentCase{"", ""},
        ParentCase{"a", ""},
        ParentCase{"foo", ""},

        // relative multi segment
        ParentCase{"a/b", "a"},
        ParentCase{"a/b/c", "a/b"},
        ParentCase{"foo/bar", "foo"},

        // absolute
        ParentCase{"/", "/"},
        ParentCase{"/a", "/"},
        ParentCase{"/a/b", "/a"},
        ParentCase{"/foo/bar", "/foo"},

        // TODO: verify dot-dot behavior against current Parent() implementation.
        // Parent() is a pure rfind('/') split and does not understand ".." semantics;
        // these expectations came from the old implementation and may be wrong.
        ParentCase{"../a", ".."},
        ParentCase{"..", ""}
    )
);
