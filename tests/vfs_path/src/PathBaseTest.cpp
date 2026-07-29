#include <gtest/gtest.h>
#include <slimenano/vfs/Path.h>

#include <string>

using slimenano::filesystem::Path;

// ---------------------------------------------------------------------------
// Filename
// ---------------------------------------------------------------------------

struct FilenameCase {
    std::string path;
    std::string filename;
};

class PathFilenameTest : public ::testing::TestWithParam<FilenameCase> {};

TEST_P(PathFilenameTest, Filename) {
    EXPECT_EQ(Path(GetParam().path).Filename(), GetParam().filename);
}

INSTANTIATE_TEST_SUITE_P(
    Filename,
    PathFilenameTest,
    ::testing::Values(
        FilenameCase{"", ""},
        FilenameCase{"/", ""},
        FilenameCase{"a", "a"},
        FilenameCase{"foo", "foo"},
        FilenameCase{"a/b", "b"},
        FilenameCase{"/a/b", "b"},
        FilenameCase{"/foo", "foo"},
        FilenameCase{"foo/bar.txt", "bar.txt"},
        FilenameCase{"/foo/bar.txt", "bar.txt"}
    )
);

// ---------------------------------------------------------------------------
// Stem / Extension
// ---------------------------------------------------------------------------

struct StemExtCase {
    std::string path;
    std::string stem;
    std::string extension;
};

class PathStemExtTest : public ::testing::TestWithParam<StemExtCase> {};

TEST_P(PathStemExtTest, StemAndExtension) {
    const Path p(GetParam().path);
    EXPECT_EQ(p.Stem(), GetParam().stem);
    EXPECT_EQ(p.Extension(), GetParam().extension);
}

INSTANTIATE_TEST_SUITE_P(
    StemExt,
    PathStemExtTest,
    ::testing::Values(
        StemExtCase{"noext", "noext", ""},
        StemExtCase{"a", "a", ""},
        StemExtCase{"file.txt", "file", "txt"},
        StemExtCase{"/path/to/file.txt", "file", "txt"},
        StemExtCase{"archive.tar.gz", "archive.tar", "gz"},

        // leading-dot files are treated as having no extension
        StemExtCase{".gitignore", ".gitignore", ""},
        StemExtCase{".bashrc", ".bashrc", ""},

        // trailing dot: no extension
        StemExtCase{"file.", "file.", ""},
        StemExtCase{"a.", "a.", ""}
    )
);

// ---------------------------------------------------------------------------
// IsAbsolute / IsRelative
// ---------------------------------------------------------------------------

struct FlagCase {
    std::string path;
    bool absolute;
};

class PathFlagTest : public ::testing::TestWithParam<FlagCase> {};

TEST_P(PathFlagTest, AbsoluteAndRelativeAreComplementary) {
    const Path p(GetParam().path);
    EXPECT_EQ(p.IsAbsolute(), GetParam().absolute);
    EXPECT_EQ(p.IsRelative(), !GetParam().absolute);
}

INSTANTIATE_TEST_SUITE_P(
    Flags,
    PathFlagTest,
    ::testing::Values(
        FlagCase{"/", true},
        FlagCase{"/a", true},
        FlagCase{"/foo", true},
        FlagCase{"", false},
        FlagCase{"a", false},
        FlagCase{"foo", false},
        FlagCase{"../a", false}
    )
);

// ---------------------------------------------------------------------------
// Empty / operator bool
// ---------------------------------------------------------------------------

struct EmptyCase {
    std::string path;
    bool empty;
};

class PathEmptyTest : public ::testing::TestWithParam<EmptyCase> {};

TEST_P(PathEmptyTest, EmptyMatchesBool) {
    const Path p(GetParam().path);
    EXPECT_EQ(p.Empty(), GetParam().empty);
    EXPECT_EQ(static_cast<bool>(p), !GetParam().empty);
}

INSTANTIATE_TEST_SUITE_P(
    Empty,
    PathEmptyTest,
    ::testing::Values(
        EmptyCase{"", true},
        EmptyCase{"a/..", true},
        EmptyCase{"foo", false},
        EmptyCase{"/", false},
        EmptyCase{"a", false}
    )
);
