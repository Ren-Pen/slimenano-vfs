#include <iostream>
#include <cstdio>
#include <slimenano/vfs/vfs.h>

namespace {
namespace fs = slimenano::filesystem;
}

int main() {

    std::cout << "Welcome to use the demo vfs tester" << std::endl;
    std::cout << "Input \"help\" can get some helps" << std::endl;
    std::cout << "Input \"exit\" for stop" << std::endl;

    fs::VirtualFileSystem vfs{};
    fs::Path cwd{fs::Path::Root()};

    while (true) {

        std::cout << cwd.String() << ">";
        std::string line;
        std::getline(std::cin, line);
        if (line == "exit") {
            break;
        } else if (line == "help") {
            std::cout << "Commands:" << std::endl;
            std::cout << "- cd <path>                           change cwd to <path>" << std::endl;
            std::cout << "- exit                                exit the program" << std::endl;
            std::cout << "- help                                show this message" << std::endl;
            std::cout << "- list [path]                         list files under the <path>" << std::endl;
            std::cout << "- mount <FS@param>;<mountPoint>       mount a file system into vfs" << std::endl;
            std::cout << "- unmount <mountPoint>                unmount from vfs" << std::endl;
            std::cout << std::endl;
            std::cout << "Support FS:" << std::endl;
            std::cout << "- native                              Native File System" << std::endl;
            std::cout << "    <path>                            directory" << std::endl;
            std::cout << "- zip                                 ZIP File System" << std::endl;
            std::cout << "    <path>                            zip file" << std::endl;
            std::cout << std::endl;
        } else if (line.starts_with("cd")) {
            auto pos = line.find_first_not_of("cd ");
            if (pos == std::string::npos) {
                cwd = fs::Path::Root();
                continue;
            }
            fs::Path nextCwd = cwd / line.substr(pos);

            std::error_code ec;
            bool isDirectory = vfs.IsDirectory(nextCwd, ec);
            if (ec) {
                std::cout << "Error: " << ec.message() << std::endl;
                continue;
            }
            if (!isDirectory) {
                std::cout << '"' << nextCwd.String() << '"' << " is not a directory" << std::endl;
                continue;
            }
            cwd = nextCwd;
        } else if (line.starts_with("list")) {
            auto pos = line.find_first_not_of("list ");
            fs::Path listPath = (pos == std::string::npos) ? cwd : (cwd / line.substr(pos));
            std::error_code ec;
            auto list = vfs.List(listPath, ec);
            if (ec) {
                std::cout << "Error: " << ec.message() << std::endl;
                continue;
            }
            for (auto& name : list) {

                auto stat = vfs.Stat(listPath / name, ec);

                std::printf(
                    "%10s%10lld%10lld%10s\n",
                    stat.type == fs::FileType::None ? "ERR" : (stat.IsDirectory() ? "DIR" : "FILE"),
                    stat.size,
                    stat.modifiedTime,
                    name.c_str()
                );
            }
        } else if (line.starts_with("mount")) {
            auto pos = line.find_first_not_of("mount");
            if (pos == std::string::npos) {
                std::cout << "Usage: mount <FS@param>;<mountPoint>       mount a file system into vfs" << std::endl;
                std::cout << std::endl;
                std::cout << "Support FS:" << std::endl;
                std::cout << "- native                              Native File System" << std::endl;
                std::cout << "    <path>                            directory" << std::endl;
                std::cout << "- zip                                 ZIP File System" << std::endl;
                std::cout << "    <path>                            zip file" << std::endl;
                std::cout << std::endl;
                continue;
            }
            auto mountParam = line.substr(pos);
            auto typePos = mountParam.find_first_of("@");
            if (typePos == std::string::npos) {
                std::cout << "Usage: mount <FS@param>;<mountPoint>       mount a file system into vfs" << std::endl;
                std::cout << std::endl;
                std::cout << "Support FS:" << std::endl;
                std::cout << "- native                              Native File System" << std::endl;
                std::cout << "    <path>                            directory" << std::endl;
                std::cout << "- zip                                 ZIP File System" << std::endl;
                std::cout << "    <path>                            zip file" << std::endl;
                std::cout << std::endl;
                continue;
            }
            auto type = mountParam.substr(0, typePos);
            auto pathParam = mountParam.substr(typePos + 1);
            auto pathPos = pathParam.find_first_of(":");
            if (pathPos == std::string::npos) {
                std::cout << "Usage: mount <FS@param>;<mountPoint>       mount a file system into vfs" << std::endl;
                std::cout << std::endl;
                std::cout << "Support FS:" << std::endl;
                std::cout << "- native                              Native File System" << std::endl;
                std::cout << "    <path>                            directory" << std::endl;
                std::cout << "- zip                                 ZIP File System" << std::endl;
                std::cout << "    <path>                            zip file" << std::endl;
                std::cout << std::endl;
                continue;
            }
            auto param = pathParam.substr(0, pathPos);
            auto mountPoint = pathParam.substr(pathPos + 1);

            if (type == "native") {
                std::error_code ec;
                vfs.CreateAndMount<fs::NativeFileSystem>(fs::Path{mountPoint}, param, ec);
                if (ec) {
                    std::cout << "Error: " << ec.message() << std::endl;
                    continue;
                }
            } else if (type == "zip") {
                std::error_code ec;
                vfs.CreateAndMount<fs::ZipFileSystem>(fs::Path{mountPoint}, param, ec);
                if (ec) {
                    std::cout << "Error: " << ec.message() << std::endl;
                    continue;
                }
            } else {
                std::cout << "Unknown FS type: " << type << std::endl;
                std::cout << "Support FS:" << std::endl;
                std::cout << "- native                              Native File System" << std::endl;
                std::cout << "    <path>                            directory" << std::endl;
                std::cout << "- zip                                 ZIP File System" << std::endl;
                std::cout << "    <path>                            zip file" << std::endl;
                std::cout << std::endl;
                continue;
            }
        }
    }

    return 0;
}
