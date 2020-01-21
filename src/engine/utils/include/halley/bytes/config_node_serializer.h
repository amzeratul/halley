#pragma once

#include "halley/file_formats/config_file.h"

namespace Halley {
    template <typename T>
    class ConfigNodeDeserializer {
    public:
        T operator()(const ConfigNode& node)
        {
			throw Exception("ConfigNodeDeserializer unimplemented type: " + String(typeid(T).name()), HalleyExceptions::Utils);
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
    class ConfigNodeDeserializer<float> {
    public:
        float operator()(const ConfigNode& node)
        {
			return node.asFloat();
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

	template <>
    class ConfigNodeDeserializer<Colour4f> {
    public:
        Colour4f operator()(const ConfigNode& node)
        {
			return Colour4f::fromString(node.asString());
        }
    };

	template <typename T>
    class ConfigNodeDeserializer<Maybe<T>> {
    public:
        Maybe<T> operator()(const ConfigNode& node)
        {
        	if (node.getType() == ConfigNodeType::Undefined) {
				return Maybe<T>();
        	} else {
				return Maybe<T>(ConfigNodeDeserializer<T>()(node));
			}
        }
    };
}
