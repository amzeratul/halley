#pragma once

#include <android/asset_manager.h>
#include "halley/resources/resource_data.h"

namespace Halley {
    class AndroidAssetReader : public ResourceDataReader {
    public:
        AndroidAssetReader(AAssetManager* mgr, String path);
        ~AndroidAssetReader();

        size_t size() const override;
        int read(gsl::span<gsl::byte> dst) override;
        void seek(int64_t pos, int whence) override;
        size_t tell() const override;
        void close() override;

    private:
        AAssetManager* mgr = nullptr;
        AAsset* asset = nullptr;
    };
}
