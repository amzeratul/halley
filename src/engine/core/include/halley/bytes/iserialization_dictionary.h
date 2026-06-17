#pragma once

#include <optional>
#include <string_view>

namespace Halley {
    class String;

    class ISerializationDictionary {
    public:
        virtual ~ISerializationDictionary() = default;
        virtual std::optional<size_t> stringToIndex(std::string_view string) = 0;
        virtual const String& indexToString(size_t index) = 0;
        virtual void notifyMissingString(std::string_view string) = 0;
    };
}
