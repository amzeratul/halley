#pragma once
#include "halley/resources/resource_data.h"
#include <cstdio>

namespace Halley {
    class Win32DataReader final : public ResourceDataReader {
    public:
		Win32DataReader(String path, int64_t start, int64_t end);
	    size_t size() const override;
	    int read(gsl::span<gsl::byte> dst) override;
	    void seek(int64_t pos, int whence) override;
	    size_t tell() const override;
	    void close() override;

    private:
		FILE* fp = nullptr;
		int64_t start;
		int64_t end;
		int64_t pos = 0;
    };
}
