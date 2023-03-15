#pragma once

#include <optional>

namespace Halley {
    class String;

    class ISerializationDictionary {
    public:
        virtual ~ISerializationDictionary() = default;
        virtual std::optional<size_t> stringToIndex(const String& string) = 0;
        virtual const String& indexToString(size_t index) = 0;
        virtual void notifyMissingString(const String& string) = 0;
    };
}
