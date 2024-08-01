#if defined(_WIN32) || defined(WITH_GDK)

#include "os_winbase.h"

using namespace Halley;

static bool hasDirectory(const String& directory)
{
    const DWORD res = GetFileAttributes(directory.c_str());
    return res != INVALID_FILE_ATTRIBUTES && (res & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

void OSWinBase::createDirectories(const Path& path)
{
    const size_t n = path.getNumberPaths();
    for (size_t i = 1; i < n; ++i) {
        Path curPath = path.getFront(i);
        String nativePath = curPath.getNativeString(false);
        if (!hasDirectory(nativePath)) {
            if (!CreateDirectory(nativePath.c_str(), nullptr)) {
                throw Exception("Unable to create directory: " + curPath + " (trying to make " + path + ")", HalleyExceptions::OS);
            }
        }
    }
}

Vector<Path> OSWinBase::enumerateDirectory(const Path& rootPath)
{
    std::list<String> dirsToList;
    dirsToList.emplace_back(".");

    Vector<Path> result;

    WIN32_FIND_DATAW ffd;
    while (!dirsToList.empty()) {
        const auto curDir = dirsToList.front();
        const auto curPath = (rootPath / curDir).getString();
        const auto pathStr = (curPath + "/*").replaceAll("/", "\\");
        dirsToList.pop_front();

        const auto handle = FindFirstFileW(pathStr.getUTF16().c_str(), &ffd);
        if (handle != INVALID_HANDLE_VALUE) {
            do {
                String curFile = String(ffd.cFileName);

                const DWORD attrib = GetFileAttributesW((rootPath / curDir / curFile).getString().getUTF16().c_str());
                if ((attrib & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                    if (curFile != "." && curFile != "..") {
                        dirsToList.emplace_back(curDir + "/" + curFile);
                    }
                } else {
                    auto res = (Path(curDir) / curFile);
                    result.emplace_back(res);
                }
            } while (FindNextFileW(handle, &ffd) != 0);
        }
    }

    return result;
}

bool OSWinBase::isDebuggerAttached() const
{
#ifdef DEV_BUILD
    return IsDebuggerPresent() == TRUE;
#else
    return false;
#endif
}

#endif
