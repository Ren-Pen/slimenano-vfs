#if defined(_WIN32)
#include <windows.h>
#endif

#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <slimenano/vfs/vfs.h>

namespace {

namespace fs = slimenano::filesystem;

std::string Trim(std::string_view s) {
    const auto begin = s.find_first_not_of(" \t");
    if (begin == std::string_view::npos) {
        return {};
    }
    const auto end = s.find_last_not_of(" \t");
    return std::string(s.substr(begin, end - begin + 1));
}

std::pair<std::string, std::string> SplitCommand(const std::string& line) {
    const auto space = line.find(' ');
    if (space == std::string::npos) {
        return {line, {}};
    }
    return {line.substr(0, space), Trim(line.substr(space + 1))};
}

void PrintHelp() {
    std::cout << "Commands:\n"
              << "- cd <path>                     change cwd to <path>\n"
              << "- exit                          exit the program\n"
              << "- help                          show this message\n"
              << "- dir [path]                    list files under <path> (default: cwd)\n"
              << "- mount <FS@param;mountPoint>   mount a file system into vfs\n"
              << "- unmount <mountPoint>          unmount from vfs\n"
              << "\n"
              << "Supported FS:\n"
              << "- native   param = <directory>\n"
              << "- zip      param = <zip file>\n"
              << "\n"
              << "Example:\n"
              << "- mount native@/home/user/data;/data\n"
              << "- mount zip@/tmp/archive.zip;/z\n"
              << std::endl;
}

void PrintMountUsage() {
    std::cout << "Usage: mount <FS@param;mountPoint>\n"
              << "\n"
              << "Supported FS:\n"
              << "- native   param = <directory>\n"
              << "- zip      param = <zip file>\n"
              << "\n"
              << "Example: mount native@/home/user/data;/data\n"
              << std::endl;
}

void DoCd(fs::VirtualFileSystem& vfs, fs::Path& cwd, const std::string& arg) {
    if (arg.empty()) {
        cwd = fs::Path::Root();
        return;
    }

    const fs::Path nextCwd = cwd / arg;

    std::error_code ec;
    const bool isDirectory = vfs.IsDirectory(nextCwd, ec);
    if (ec) {
        std::cout << "Error: " << ec.message() << std::endl;
        return;
    }
    if (!isDirectory) {
        std::cout << '"' << nextCwd.String() << "\" is not a directory or does not exist" << std::endl;
        return;
    }
    cwd = nextCwd;
}

void DoDir(fs::VirtualFileSystem& vfs, const fs::Path& cwd, const std::string& arg) {
    const fs::Path listPath = arg.empty() ? cwd : (cwd / arg);

    std::error_code ec;
    const auto entries = vfs.List(listPath, ec);
    if (ec) {
        std::cout << "Error: " << ec.message() << std::endl;
        return;
    }

    std::printf("%-6s %12s %14s  %s\n", "TYPE", "SIZE", "MTIME", "NAME");
    for (const auto& name : entries) {
        std::error_code statEc;
        const auto stat = vfs.Stat(listPath / name, statEc);

        const char* type = "ERR";
        if (!statEc) {
            type = stat.type == fs::FileType::None ? "ERR" : (stat.IsDirectory() ? "DIR" : "FILE");
        }

        std::printf(
            "%-6s %12llu %14lld  %s\n",
            type,
            static_cast<unsigned long long>(stat.size),
            static_cast<long long>(stat.modifiedTime),
            name.c_str()
        );
    }
}

void DoMount(fs::VirtualFileSystem& vfs, const std::string& arg) {
    if (arg.empty()) {
        PrintMountUsage();
        return;
    }

    const auto atPos = arg.find('@');
    const auto semiPos = arg.find(';');
    if (atPos == std::string::npos || semiPos == std::string::npos || semiPos < atPos) {
        PrintMountUsage();
        return;
    }

    const std::string type = arg.substr(0, atPos);
    const std::string param = arg.substr(atPos + 1, semiPos - atPos - 1);
    const std::string mountPoint = arg.substr(semiPos + 1);

    if (param.empty() || mountPoint.empty()) {
        PrintMountUsage();
        return;
    }

    std::error_code ec;
    if (type == "native") {
        vfs.CreateAndMount<fs::NativeFileSystem>(fs::Path{mountPoint}, param, ec);
    } else if (type == "zip") {
        vfs.CreateAndMount<fs::ZipFileSystem>(fs::Path{mountPoint}, param, ec);
    } else {
        std::cout << "Unknown FS type: " << type << std::endl;
        PrintMountUsage();
        return;
    }

    if (ec) {
        std::cout << "[" << ec.category().name() << "] " << "Error: " << ec.message() << std::endl;
        return;
    }
    std::cout << "Mounted " << type << " (" << param << ") at " << mountPoint << std::endl;
}

void DoUnmount(fs::VirtualFileSystem& vfs, const std::string& arg) {
    if (arg.empty()) {
        std::cout << "Usage: unmount <mountPoint>" << std::endl;
        return;
    }
    vfs.Unmount(fs::Path{arg});
    std::cout << "Unmounted " << arg << std::endl;
}

} // namespace

int main() {

#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "Welcome to the demo vfs tester\n"
              << "Type \"help\" for a list of commands\n"
              << "Type \"exit\" to quit\n"
              << std::endl;

    fs::VirtualFileSystem vfs{};
    fs::Path cwd{fs::Path::Root()};

    while (true) {
        std::cout << cwd.String() << "> ";

        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;
            break;
        }

        const auto [cmd, arg] = SplitCommand(Trim(line));

        if (cmd.empty()) {
            continue;
        } else if (cmd == "exit") {
            break;
        } else if (cmd == "help") {
            PrintHelp();
        } else if (cmd == "cd") {
            DoCd(vfs, cwd, arg);
        } else if (cmd == "dir") {
            DoDir(vfs, cwd, arg);
        } else if (cmd == "mount") {
            DoMount(vfs, arg);
        } else if (cmd == "unmount") {
            DoUnmount(vfs, arg);
        } else {
            std::cout << "Unknown command: " << cmd << " (type \"help\")" << std::endl;
        }
    }

    return 0;
}
