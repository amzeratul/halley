#include "android_save_data.h"
using namespace Halley;

AndroidSaveData::AndroidSaveData(SaveDataType type, const String &containerName)
{
    // TODO
}

bool AndroidSaveData::isReady() const
{
    return true;
}

Bytes AndroidSaveData::getData(const String& path)
{
    // TODO
    return Halley::Bytes();
}

Vector<String> AndroidSaveData::enumerate(const String &root)
{
    // TODO
    return Vector<String>();
}

void AndroidSaveData::setData(const String &path, const Bytes &data, bool commit)
{
    // TODO
}

void AndroidSaveData::commit()
{
    // TODO
}
