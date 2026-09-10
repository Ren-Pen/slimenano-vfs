#include <slimenano/vfs/ZipFileSystem.h>

#include <gtest/gtest.h>
#include <zip.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace slimenano::filesystem {
namespace {

namespace fs = std::filesystem;

// Builds a fixed-structure archive on disk and opens it through
// ZipFileSystem. The archive contains, by design:
//   hello.txt            regular file, content "12345"
//   中文文件.txt          regular file with a UTF-8 name, content "nihao"
//   dir/                 explicit directory entry
//   dir/nested.txt       regular file inside the explicit directory
//   implicit/deep.txt    regular file whose "implicit/" parent has NO entry
//   empty.txt            zero-length regular file
class ZipFileSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::error_code ec;
        m_dir = fs::temp_directory_path() / "slimenano-vfs-zip-test";
        fs::remove_all(m_dir, ec);
        fs::create_directories(m_dir, ec);
        ASSERT_FALSE(ec) << ec.message();

        m_zipPath = m_dir / "test.zip";
        BuildArchive(m_zipPath.string());

        const auto u8Path = m_zipPath.u8string();
        m_fs = std::make_unique<ZipFileSystem>(
            std::string(reinterpret_cast<const char*>(u8Path.data()), u8Path.size()), ec
        );
        ASSERT_FALSE(ec) << ec.message();
    }

    void TearDown() override {
        std::error_code ec;
        m_fs.reset();
        fs::remove_all(m_dir, ec);
    }

    // Creates the archive described in the class comment at @p path.
    static void BuildArchive(const std::string& path) {
        std::error_code rmEc;
        fs::remove(path, rmEc);

        int err = 0;
        zip_t* za = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
        ASSERT_NE(za, nullptr) << "zip_open create failed, err=" << err;

        // zip_source_buffer does NOT copy: the backing storage must stay
        // alive until zip_close. Keep every payload in this vector.
        std::vector<std::string> storage;
        storage.reserve(8);

        auto addFile = [&](const char* name, std::string content) {
            storage.push_back(std::move(content));
            const std::string& held = storage.back();
            zip_source_t* s = zip_source_buffer(za, held.data(), held.size(), 0);
            ASSERT_NE(s, nullptr) << "zip_source_buffer failed for " << name;
            const zip_int64_t idx = zip_file_add(za, name, s, ZIP_FL_ENC_UTF_8);
            if (idx < 0) {
                zip_source_free(s);
                FAIL() << "zip_file_add failed for " << name << ": " << zip_strerror(za);
            }
        };

        addFile("hello.txt", "12345");
        addFile("中文文件.txt", "nihao"); // 中文文件.txt
        ASSERT_GE(zip_dir_add(za, "dir", ZIP_FL_ENC_UTF_8), 0) << zip_strerror(za);
        addFile("dir/nested.txt", "nested-content");
        addFile("implicit/deep.txt", "deep"); // no explicit "implicit/" entry
        addFile("empty.txt", "");

        ASSERT_EQ(zip_close(za), 0) << "zip_close failed";
    }

    // Reads the whole handle into a byte vector using @p chunk-sized reads.
    static std::vector<std::byte> ReadAll(FileHandle& h, std::size_t chunk, std::error_code& ec) {
        std::vector<std::byte> out;
        std::vector<std::byte> buf(chunk);
        for (;;) {
            const std::size_t n = h.Read(std::span<std::byte>(buf.data(), buf.size()), ec);
            if (ec || n == 0) {
                break;
            }
            out.insert(out.end(), buf.begin(), buf.begin() + n);
        }
        return out;
    }

    static std::vector<std::byte> Bytes(std::string_view s) {
        std::vector<std::byte> v(s.size());
        std::memcpy(v.data(), s.data(), s.size());
        return v;
    }

    fs::path m_dir;
    fs::path m_zipPath;
    std::unique_ptr<ZipFileSystem> m_fs;
};

// ---------------------------------------------------------------------------
// Construction / initialization
// ---------------------------------------------------------------------------

TEST_F(ZipFileSystemTest, OpenNonexistentArchiveReportsError) {
    std::error_code ec;
    const auto missing = (m_dir / "does-not-exist.zip").string();
    ZipFileSystem badFs(missing, ec);
    EXPECT_TRUE(ec) << "opening a missing archive should fail";
}

// ---------------------------------------------------------------------------
// Stat
// ---------------------------------------------------------------------------

TEST_F(ZipFileSystemTest, StatRegularFile) {
    std::error_code ec;
    const auto info = m_fs->Stat(Path{"/hello.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(info.type, FileType::Regular);
    EXPECT_EQ(info.size, 5u);
}

TEST_F(ZipFileSystemTest, StatUtf8File) {
    std::error_code ec;
    const auto info = m_fs->Stat(Path{"/中文文件.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(info.type, FileType::Regular);
    EXPECT_EQ(info.size, 5u);
}

TEST_F(ZipFileSystemTest, StatExplicitDirectory) {
    std::error_code ec;
    const auto info = m_fs->Stat(Path{"/dir"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(info.type, FileType::Directory);
}

TEST_F(ZipFileSystemTest, StatImplicitDirectory) {
    std::error_code ec;
    const auto info = m_fs->Stat(Path{"/implicit"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(info.type, FileType::Directory);
}

TEST_F(ZipFileSystemTest, StatMissingReturnsNoneWithoutError) {
    std::error_code ec;
    const auto info = m_fs->Stat(Path{"/nope.txt"}, ec);
    EXPECT_FALSE(ec);
    EXPECT_EQ(info.type, FileType::None);
}

TEST_F(ZipFileSystemTest, StatEmptyFileHasZeroSize) {
    std::error_code ec;
    const auto info = m_fs->Stat(Path{"/empty.txt"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(info.type, FileType::Regular);
    EXPECT_EQ(info.size, 0u);
}

// ---------------------------------------------------------------------------
// Exists / type queries
// ---------------------------------------------------------------------------

TEST_F(ZipFileSystemTest, ExistsForFileDirAndImplicitDir) {
    std::error_code ec;
    EXPECT_TRUE(m_fs->Exists(Path{"/hello.txt"}, ec));
    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_fs->Exists(Path{"/dir"}, ec));
    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_fs->Exists(Path{"/implicit"}, ec));
    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_fs->Exists(Path{"/dir/nested.txt"}, ec));
    EXPECT_FALSE(ec);
}

TEST_F(ZipFileSystemTest, ExistsFalseForMissing) {
    std::error_code ec;
    EXPECT_FALSE(m_fs->Exists(Path{"/nope.txt"}, ec));
    EXPECT_FALSE(ec);
}

TEST_F(ZipFileSystemTest, IsDirectoryAndIsRegularFile) {
    std::error_code ec;
    EXPECT_TRUE(m_fs->IsDirectory(Path{"/dir"}, ec));
    EXPECT_TRUE(m_fs->IsDirectory(Path{"/implicit"}, ec));
    EXPECT_FALSE(m_fs->IsDirectory(Path{"/hello.txt"}, ec));

    EXPECT_TRUE(m_fs->IsRegularFile(Path{"/hello.txt"}, ec));
    EXPECT_FALSE(m_fs->IsRegularFile(Path{"/dir"}, ec));
}

TEST_F(ZipFileSystemTest, SizeMatchesStat) {
    std::error_code ec;
    EXPECT_EQ(m_fs->Size(Path{"/dir/nested.txt"}, ec), 14u);
    EXPECT_FALSE(ec);
}

// ---------------------------------------------------------------------------
// List
// ---------------------------------------------------------------------------

TEST_F(ZipFileSystemTest, ListRootReturnsSortedTopLevel) {
    std::error_code ec;
    const auto names = m_fs->List(Path{"/"}, ec);
    ASSERT_FALSE(ec) << ec.message();

    // Top level: hello.txt, 中文文件.txt, dir, implicit, empty.txt
    EXPECT_NE(std::find(names.begin(), names.end(), "hello.txt"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "dir"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "implicit"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "empty.txt"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), std::string("中文文件.txt")), names.end());

    // Names come from a std::map, so they must be sorted and unique.
    EXPECT_TRUE(std::is_sorted(names.begin(), names.end()));
    EXPECT_EQ(std::adjacent_find(names.begin(), names.end()), names.end());
}

TEST_F(ZipFileSystemTest, ListExplicitDirectory) {
    std::error_code ec;
    const auto names = m_fs->List(Path{"/dir"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "nested.txt");
}

TEST_F(ZipFileSystemTest, ListImplicitDirectory) {
    std::error_code ec;
    const auto names = m_fs->List(Path{"/implicit"}, ec);
    ASSERT_FALSE(ec) << ec.message();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "deep.txt");
}

TEST_F(ZipFileSystemTest, ListOnFileFails) {
    std::error_code ec;
    const auto names = m_fs->List(Path{"/hello.txt"}, ec);
    EXPECT_TRUE(ec);
    EXPECT_EQ(ec, std::errc::not_a_directory);
    EXPECT_TRUE(names.empty());
}

TEST_F(ZipFileSystemTest, ListMissingFails) {
    std::error_code ec;
    const auto names = m_fs->List(Path{"/nope"}, ec);
    EXPECT_TRUE(ec);
    EXPECT_EQ(ec, std::errc::no_such_file_or_directory);
    EXPECT_TRUE(names.empty());
}

// ---------------------------------------------------------------------------
// Open + Read
// ---------------------------------------------------------------------------

TEST_F(ZipFileSystemTest, OpenAndReadWholeFile) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);
    ASSERT_FALSE(ec) << ec.message();

    const auto data = ReadAll(*handle, 64, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(data, Bytes("12345"));
}

TEST_F(ZipFileSystemTest, OpenAndReadInSmallChunks) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/dir/nested.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);
    ASSERT_FALSE(ec) << ec.message();

    // 1-byte chunks must still reassemble the full content.
    const auto data = ReadAll(*handle, 1, ec);
    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(data, Bytes("nested-content"));
}

TEST_F(ZipFileSystemTest, ReadEmptyFileYieldsImmediateEof) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/empty.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);
    ASSERT_FALSE(ec) << ec.message();

    std::array<std::byte, 8> buf{};
    const std::size_t n = handle->Read(buf, ec);
    EXPECT_FALSE(ec);
    EXPECT_EQ(n, 0u);
}

TEST_F(ZipFileSystemTest, OpenDirectoryFails) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/dir"}, OpenOption::Read, ec);
    EXPECT_EQ(handle, nullptr);
    EXPECT_EQ(ec, std::errc::is_a_directory);
}

TEST_F(ZipFileSystemTest, OpenMissingFails) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/nope.txt"}, OpenOption::Read, ec);
    EXPECT_EQ(handle, nullptr);
    EXPECT_EQ(ec, std::errc::no_such_file_or_directory);
}

TEST_F(ZipFileSystemTest, OpenWithWriteFlagsIsReadOnly) {
    std::error_code ec;
    for (const OpenOption opt : {OpenOption::Create, OpenOption::Append, OpenOption::Truncate}) {
        auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::Read | opt, ec);
        EXPECT_EQ(handle, nullptr);
        EXPECT_EQ(ec, std::errc::read_only_file_system);
    }
}

TEST_F(ZipFileSystemTest, OpenWithoutReadFlagFails) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::None, ec);
    EXPECT_EQ(handle, nullptr);
    EXPECT_EQ(ec, std::errc::invalid_argument);
}

// ---------------------------------------------------------------------------
// Write operations are all rejected (read-only archive)
// ---------------------------------------------------------------------------

TEST_F(ZipFileSystemTest, MutatingOperationsAreReadOnly) {
    std::error_code ec;

    m_fs->CreateFile(Path{"/x.txt"}, ec);
    EXPECT_EQ(ec, std::errc::read_only_file_system);

    m_fs->CreateDirectory(Path{"/x"}, ec);
    EXPECT_EQ(ec, std::errc::read_only_file_system);

    m_fs->CreateDirectories(Path{"/x/y"}, ec);
    EXPECT_EQ(ec, std::errc::read_only_file_system);

    m_fs->Delete(Path{"/hello.txt"}, ec);
    EXPECT_EQ(ec, std::errc::read_only_file_system);

    m_fs->DeleteAll(Path{"/dir"}, ec);
    EXPECT_EQ(ec, std::errc::read_only_file_system);
}

// ---------------------------------------------------------------------------
// FileHandle behaviour
// ---------------------------------------------------------------------------

TEST_F(ZipFileSystemTest, HandleWriteIsRejected) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);

    const std::array<std::byte, 3> payload{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::size_t n = handle->Write(payload, ec);
    EXPECT_EQ(n, 0u);
    EXPECT_EQ(ec, std::errc::read_only_file_system);
}

TEST_F(ZipFileSystemTest, HandleSeekAndTellUnsupported) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);

    handle->Seek(0, SeekOrigin::Begin, ec);
    EXPECT_EQ(ec, std::errc::operation_not_supported);

    (void)handle->Tell(ec);
    EXPECT_EQ(ec, std::errc::operation_not_supported);
}

TEST_F(ZipFileSystemTest, HandleFlushSucceeds) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);

    handle->Flush(ec);
    EXPECT_FALSE(ec);
}

TEST_F(ZipFileSystemTest, OperationsAfterCloseFail) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);

    handle->Close(ec);
    EXPECT_FALSE(ec) << ec.message();

    std::array<std::byte, 4> buf{};
    handle->Read(buf, ec);
    EXPECT_EQ(ec, std::errc::bad_file_descriptor);

    handle->Seek(0, SeekOrigin::Begin, ec);
    EXPECT_EQ(ec, std::errc::bad_file_descriptor);

    (void)handle->Tell(ec);
    EXPECT_EQ(ec, std::errc::bad_file_descriptor);
}

TEST_F(ZipFileSystemTest, DoubleCloseIsNoop) {
    std::error_code ec;
    auto handle = m_fs->Open(Path{"/hello.txt"}, OpenOption::Read, ec);
    ASSERT_NE(handle, nullptr);

    handle->Close(ec);
    EXPECT_FALSE(ec) << ec.message();
    handle->Close(ec);
    EXPECT_FALSE(ec) << ec.message();
}

} // namespace
} // namespace slimenano::filesystem
