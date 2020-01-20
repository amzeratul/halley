#pragma once

#include "halley/file_formats/config_file.h"

namespace Halley {
    template <typename T>
    class ConfigNodeDeserializer {
    public:
        T operator()(const ConfigNode& node)
        {
			return T();
        }
    };

	template <>
    class ConfigNodeDeserializer<int> {
    public:
        int operator()(const ConfigNode& node)
        {
			return node.asInt();
        }
    };

	template <>
    class ConfigNodeDeserializer<Vector2f> {
    public:
        Vector2f operator()(const ConfigNode& node)
        {
			return node.asVector2f();
        }
    };
}
