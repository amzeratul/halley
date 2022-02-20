#pragma once

#include "halley/core/api/save_data.h"

namespace Halley {
    class AndroidSaveData : public ISaveData {
    public:
        AndroidSaveData(SaveDataType type, const String& containerName);

        bool isReady() const override;

        Bytes getData(const String &path) override;

        Vector<String> enumerate(const String &root) override;

        void setData(const String &path, const Bytes &data, bool commit) override;

        void commit() override;
    };
}
