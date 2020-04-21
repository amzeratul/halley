#pragma once

#include "halley/file_formats/config_file.h"
#include "halley/maths/polygon.h"
#include "halley/maths/colour.h"
#include "halley/data_structures/maybe.h"
#include <set>

namespace Halley {
	class EntitySerializationContext;
	
	class ConfigNodeSerializationContext {
	public:
		Resources* resources = nullptr;
		std::shared_ptr<EntitySerializationContext> entityContext;
	};

	
    template <typename T>
    class ConfigNodeSerializer {
    public:
        T deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			throw Exception("ConfigNodeSerializer unimplemented type: " + String(typeid(T).name()), HalleyExceptions::Utils);
        }
    };

	template <>
	class ConfigNodeSerializer<bool> {
	public:
		int deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
		{
			return node.asBool(false);
		}
	};

	template <>
    class ConfigNodeSerializer<int> {
    public:
        int deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asInt(0);
        }
    };

	template <>
    class ConfigNodeSerializer<float> {
    public:
        float deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asFloat(0);
        }
    };

	template <>
    class ConfigNodeSerializer<Vector2i> {
    public:
        Vector2i deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asVector2i(Vector2i());
        }
    };

	template <>
    class ConfigNodeSerializer<Vector2f> {
    public:
        Vector2f deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asVector2f(Vector2f());
        }
    };

	template <>
    class ConfigNodeSerializer<Angle1f> {
    public:
        Angle1f deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return Angle1f::fromRadians(node.asFloat(0.0f));
        }
    };

	template <>
    class ConfigNodeSerializer<Colour4f> {
    public:
        Colour4f deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return Colour4f::fromString(node.asString("#000000"));
        }
    };

	template <>
    class ConfigNodeSerializer<Rect4i> {
    public:
        Rect4i deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
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
    class ConfigNodeSerializer<Rect4f> {
    public:
        Rect4f deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
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
    class ConfigNodeSerializer<std::optional<T>> {
    public:
        std::optional<T> deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
        {
        	if (node.getType() == ConfigNodeType::Undefined) {
				return std::optional<T>();
        	} else {
				return std::optional<T>(ConfigNodeSerializer<T>().deserialize(context, node));
			}
        }
    };

	template <typename T>
    class ConfigNodeSerializer<std::vector<T>> {
    public:
        std::vector<T> deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
        {
			std::vector<T> result;
        	if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
				result.reserve(seq.size());
        		for (auto& s: seq) {
					result.push_back(ConfigNodeSerializer<T>().deserialize(context, s));
				}
			}
			return result;
        }
	};

	template <typename T>
	class ConfigNodeSerializer<std::set<T>> {
	public:
		std::set<T> deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			std::set<T> result;
			if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
				for (auto& s : seq) {
					result.insert(ConfigNodeSerializer<T>().deserialize(context, s));
				}
			}
			return result;
		}
	};
	
	template<>
	class ConfigNodeSerializer<Polygon> {
	public:
		Polygon deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
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
	class ConfigNodeSerializer<String> {
	public:
		String deserialize(ConfigNodeSerializationContext&, const ConfigNode& node)
		{
			return node.asString("");
		}
	};

	template <typename T>
	class ConfigNodeSerializer<std::map<String, T>>
	{
	public:
		std::map<String, T> deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			std::map<String, T> result;
			if (node.getType() == ConfigNodeType::Map) {
				auto map = node.asMap();
				for (auto& s : map) {
					result[s.first] = ConfigNodeSerializer<T>().deserialize(context, s.second);
				}
			}
			return result;
		}
	};

	template <typename T>
	class ConfigNodeSerializer<OptionalLite<T>> {
	public:
		OptionalLite<T> deserialize(ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			if (node.getType() == ConfigNodeType::Undefined) {
				return OptionalLite<T>();
			} else {
				return OptionalLite<T>(ConfigNodeSerializer<T>().deserialize(context, node));
			}
		}
	};

	template <typename T>
	class ConfigNodeHelper {
	public:
		static void deserialize(T& dst, ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			if (node.getType() != ConfigNodeType::Undefined) {
				dst = ConfigNodeSerializer<T>().deserialize(context, node);
			}
		}
	};

	template <typename T>
	class ConfigNodeHelper<std::optional<T>> {
	public:
		static void deserialize(std::optional<T>& dst, ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			dst = ConfigNodeSerializer<std::optional<T>>().deserialize(context, node);
		}
	};

	template <typename T>
	class ConfigNodeHelper<OptionalLite<T>> {
	public:
		static void deserialize(OptionalLite<T>& dst, ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			dst = ConfigNodeSerializer<OptionalLite<T>>().deserialize(context, node);
		}
	};
}
