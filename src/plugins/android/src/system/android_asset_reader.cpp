#include "android_asset_reader.h"
using namespace Halley;

AndroidAssetReader::AndroidAssetReader(AAssetManager *mgr, String path)
{
    asset = AAssetManager_open(mgr, path.mid(2).c_str(), AASSET_MODE_RANDOM);
}

AndroidAssetReader::~AndroidAssetReader()
{
    close();
}

size_t AndroidAssetReader::size() const
{
    return size_t(AAsset_getLength64(asset));
}

int AndroidAssetReader::read(gsl::span<gsl::byte> dst)
{
    int nRead = AAsset_read(asset, dst.data(), size_t(dst.size()));
    if (nRead > 0) {
        curPos += nRead;
    }
    return nRead;
}

void AndroidAssetReader::seek(int64_t pos, int whence)
{
    curPos = AAsset_seek64(asset, off_t(pos), whence);
}

size_t AndroidAssetReader::tell() const
{
    return curPos;
}

void AndroidAssetReader::close()
{
    if (asset) {
        AAsset_close(asset);
        asset = nullptr;
    }
}
