#include "android_asset_reader.h"
using namespace Halley;

AndroidAssetReader::AndroidAssetReader(AAssetManager *mgr, String path)
{
    asset = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_RANDOM);
}

AndroidAssetReader::~AndroidAssetReader()
{
    close();
}

size_t AndroidAssetReader::size() const
{
    return size_t(AAsset_getLength(asset));
}

int AndroidAssetReader::read(gsl::span<gsl::byte> dst)
{
    return AAsset_read(asset, dst.data(), size_t(dst.size()));
}

void AndroidAssetReader::seek(int64_t pos, int whence)
{
    AAsset_seek(asset, off_t(pos), whence);
}

size_t AndroidAssetReader::tell() const
{
    return size_t(AAsset_getLength(asset) - AAsset_getRemainingLength(asset));
}

void AndroidAssetReader::close()
{
    if (asset) {
        AAsset_close(asset);
        asset = nullptr;
    }
}
