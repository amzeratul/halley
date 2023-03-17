#pragma once
#include "halley/file/path.h"

using namespace Halley;

class FreeDeleter {
public:
    void operator()(void* ptr) const
    {
        free(ptr);
    }
};

class ZipFile {
public:
    ZipFile();
    ZipFile(Path path, bool inMemory);
    ~ZipFile();

    bool open(Path path, bool inMemory);
    bool open(Bytes bytes);
    void close();

    size_t getNumFiles() const;
    String getFileName(size_t idx) const;
    Vector<String> getFileNames() const;
    Bytes extractFile(size_t idx) const;

    void printDiagnostics() const;

    static bool isZipFile(const Path& path);
    static Bytes readFile(const Path& path);

private:
    Path path;
	bool isOpen = false;

	void* fileHandle = nullptr;
    std::unique_ptr<void, FreeDeleter> archive = nullptr;
    Bytes compressedData;
};
