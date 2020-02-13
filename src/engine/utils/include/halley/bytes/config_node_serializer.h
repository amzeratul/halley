#pragma once

#include "halley/file_formats/config_file.h"
#include "halley/maths/polygon.h"
#include "halley/maths/colour.h"

namespace Halley {
    template <typename T>
    class ConfigNodeDeserializer {
    public:
        T operator()(Resources&, const ConfigNode& node)
        {
			throw Exception("ConfigNodeDeserializer unimplemented type: " + String(typeid(T).name()), HalleyExceptions::Utils);
        }
    };

	template <>
    class ConfigNodeDeserializer<int> {
    public:
        int operator()(Resources&, const ConfigNode& node)
        {
			return node.asInt(0);
        }
    };

	template <>
    class ConfigNodeDeserializer<float> {
    public:
        float operator()(Resources&, const ConfigNode& node)
        {
			return node.asFloat(0);
        }
    };

	template <>
    class ConfigNodeDeserializer<Vector2i> {
    public:
        Vector2i operator()(Resources&, const ConfigNode& node)
        {
			return node.asVector2i(Vector2i());
        }
    };

	template <>
    class ConfigNodeDeserializer<Vector2f> {
    public:
        Vector2f operator()(Resources&, const ConfigNode& node)
        {
			return node.asVector2f(Vector2f());
        }
    };

	template <>
    class ConfigNodeDeserializer<Angle1f> {
    public:
        Angle1f operator()(Resources&, const ConfigNode& node)
        {
			return Angle1f::fromRadians(node.asFloat(0.0f));
        }
    };

	template <>
    class ConfigNodeDeserializer<Colour4f> {
    public:
        Colour4f operator()(Resources&, const ConfigNode& node)
        {
			return Colour4f::fromString(node.asString("#000000"));
        }
    };

	template <>
    class ConfigNodeDeserializer<Rect4i> {
    public:
        Rect4i operator()(Resources&, const ConfigNode& node)
        {
        	if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
				return Rect4i(seq[0].asInt(), seq[1].asInt(), seq[2].asInt(), seq[3].asInt());
			} else {
				return Rect4i();
			}
        }
    };

	template <>
    class ConfigNodeDeserializer<Rect4f> {
    public:
        Rect4f operator()(Resources&, const ConfigNode& node)
        {
			if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
				return Rect4f(seq[0].asFloat(0), seq[1].asFloat(0), seq[2].asFloat(0), seq[3].asFloat(0));
			} else {
				return Rect4f();
			}
        }
    };

	template <typename T>
    class ConfigNodeDeserializer<Maybe<T>> {
    public:
        Maybe<T> operator()(Resources& resources, const ConfigNode& node)
        {
        	if (node.getType() == ConfigNodeType::Undefined) {
				return Maybe<T>();
        	} else {
				return Maybe<T>(ConfigNodeDeserializer<T>()(resources, node));
			}
        }
    };

	template <typename T>
    class ConfigNodeDeserializer<std::vector<T>> {
    public:
        std::vector<T> operator()(Resources& resources, const ConfigNode& node)
        {
			std::vector<T> result;
        	if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
				result.reserve(seq.size());
        		for (auto& s: seq) {
					result.push_back(ConfigNodeDeserializer<T>()(resources, s));
				}
			}
			return result;
        }
    };
	
	template<>
	class ConfigNodeDeserializer<Polygon> {
	public:
		Polygon operator()(Resources& resources, const ConfigNode& node)
		{
			VertexList list;
			if (node.getType() == ConfigNodeType::Sequence) {
				list.reserve(node.asSequence().size());
				for (auto& n: node.asSequence()) {
					list.push_back(n.asVector2f());
				}
			}
			return Polygon(std::move(list));
		}
	};
	
	template<>
	class ConfigNodeDeserializer<String> {
	public:
		String operator()(Resources& resources, const ConfigNode& node)
		{
			return node.asString("");
		}
	};

	class ConfigNodeHelper {
	public:
		template <typename T>
		static void deserializeIfDefined(T& dst, Resources& resources, const ConfigNode& node)
		{
			if (node.getType() != ConfigNodeType::Undefined) {
				dst = ConfigNodeDeserializer<T>()(resources, node);
			}
		}
	};
}
