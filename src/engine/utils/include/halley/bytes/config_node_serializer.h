#pragma once

#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/maths/colour.h"
#include "halley/maths/rect.h"
#include "config_node_serializer_base.h"
#include <set>
#include <unordered_set>


#include "halley/core/resources/resource_reference.h"

namespace Halley {
	template <>
	class ConfigNodeSerializer<bool> {
	public:
		ConfigNode serialize(bool value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		int deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
		{
			return node.asBool(false);
		}
	};

	template <>
    class ConfigNodeSerializer<int> {
    public:
        ConfigNode serialize(int value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		int deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asInt(0);
        }
    };

	template <>
    class ConfigNodeSerializer<float> {
    public:
        ConfigNode serialize(float value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		float deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asFloat(0);
        }
    };

	template <>
    class ConfigNodeSerializer<Vector2i> {
    public:
        ConfigNode serialize(Vector2i value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector2i deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asVector2i(Vector2i());
        }
    };

	template <>
    class ConfigNodeSerializer<Vector2f> {
    public:
        ConfigNode serialize(Vector2f value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector2f deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return node.asVector2f(Vector2f());
        }
    };

	template <>
    class ConfigNodeSerializer<Angle1f> {
    public:
        ConfigNode serialize(Angle1f value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value.toRadians());
		}
		
		Angle1f deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return Angle1f::fromRadians(node.asFloat(0.0f));
        }
    };

	template <>
    class ConfigNodeSerializer<Colour4f> {
    public:
        ConfigNode serialize(Colour4f value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value.toString());
		}
		
		Colour4f deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
        {
			return Colour4f::fromString(node.asString("#000000"));
        }
    };

	template <>
    class ConfigNodeSerializer<Rect4i> {
    public:
        ConfigNode serialize(Rect4i value, const ConfigNodeSerializationContext& context)
		{
        	std::vector<int> seq = { value.getX(), value.getY(), value.getWidth(), value.getHeight() };
        	return ConfigNode(seq);
		}
		
		Rect4i deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
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
        ConfigNode serialize(Rect4f value, const ConfigNodeSerializationContext& context)
		{
        	std::vector<float> seq = { value.getX(), value.getY(), value.getWidth(), value.getHeight() };
        	return ConfigNode(seq);
		}
		
		Rect4f deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
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
        ConfigNode serialize(const std::optional<T>& value, const ConfigNodeSerializationContext& context)
		{
        	if (value) {
				return ConfigNodeSerializer<T>::serialize(value.value(), context);
			} else {
				return ConfigNode();
			}
		}
		
        std::optional<T> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
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
        ConfigNode serialize(const std::vector<T>& values, const ConfigNodeSerializationContext& context)
		{
        	auto serializer = ConfigNodeSerializer<T>();
        	ConfigNode result = ConfigNode::SequenceType();
            auto& seq = result.asSequence();
        	seq.reserve(values.size());
        	for (auto& value: values) {
        		seq.push_back(serializer.serialize(value, context));
        	}
        	return result;
		}
		
        std::vector<T> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
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
		ConfigNode serialize(const std::set<T>& values, const ConfigNodeSerializationContext& context)
		{
        	auto serializer = ConfigNodeSerializer<T>();
        	ConfigNode result = ConfigNode::SequenceType();
            auto& seq = result.asSequence();
        	seq.reserve(values.size());
        	for (auto& value: values) {
        		seq.push_back(serializer.serialize(value, context));
        	}
        	return result;
		}
		
        std::set<T> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
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

	template <typename T>
	class ConfigNodeSerializer<std::unordered_set<T>> {
	public:
		ConfigNode serialize(const std::unordered_set<T>& values, const ConfigNodeSerializationContext& context)
		{
			auto serializer = ConfigNodeSerializer<T>();
			ConfigNode result = ConfigNode::SequenceType();
			auto& seq = result.asSequence();
			seq.reserve(values.size());
			for (auto& value : values) {
				seq.push_back(serializer.serialize(value, context));
			}
			return result;
		}

		std::unordered_set<T> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			std::unordered_set<T> result;
			if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
				for (auto& s : seq) {
					result.insert(ConfigNodeSerializer<T>().deserialize(context, s));
				}
			}
			return result;
		}
	};
	
	template <typename T>
	class ConfigNodeSerializer<ResourceReference<T>> {
	public:
		ConfigNode serialize(const ResourceReference<T>& value, const ConfigNodeSerializationContext& context)
		{
        	ConfigNode result = ConfigNode::MapType();
			result["asset"] = value.getId();
			return result;
		}
		
        ResourceReference<T> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			const auto assetId = node["asset"].asString("");
			return ResourceReference<T>(assetId.isEmpty() ? std::shared_ptr<const T>() : context.resources->get<T>(assetId));
		}
	};
	
	template<>
	class ConfigNodeSerializer<String> {
	public:
		ConfigNode serialize(const String& value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(value);
		}

		String deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node)
		{
			return node.asString("");
		}
	};

	template <typename T>
	class ConfigNodeSerializer<std::map<String, T>>
	{
	public:
		ConfigNode serialize(const std::map<String, T>& values, const ConfigNodeSerializationContext& context)
		{
        	auto serializer = ConfigNodeSerializer<T>();
        	ConfigNode result = ConfigNode::MapType();
        	for (auto& [key, value]: values) {
        		result[key] = serializer.serialize(value, context);
        	}
        	return result;
		}
		
		std::map<String, T> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
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

	template <typename K, typename V>
	class ConfigNodeSerializer<std::map<K, V>>
	{
	public:
		ConfigNode serialize(const std::map<K, V>& values, const ConfigNodeSerializationContext& context)
		{
        	auto serializer = ConfigNodeSerializer<V>();
        	ConfigNode result = ConfigNode::MapType();
        	for (auto& [key, value]: values) {
        		result[toString(key)] = serializer.serialize(value, context);
        	}
        	return result;
		}
		
		std::map<K, V> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			std::map<K, V> result;
			if (node.getType() == ConfigNodeType::Map) {
				auto map = node.asMap();
				for (auto& s : map) {
					result[fromString<K>(s.first)] = ConfigNodeSerializer<V>().deserialize(context, s.second);
				}
			}
			return result;
		}
	};

	template <typename T>
	class ConfigNodeSerializer<OptionalLite<T>> {
	public:
        ConfigNode serialize(const OptionalLite<T>& value, const ConfigNodeSerializationContext& context)
		{
        	if (value) {
				return ConfigNodeSerializer<T>::serialize(value.value(), context);
			} else {
				return ConfigNode();
			}
		}

		OptionalLite<T> deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			if (node.getType() == ConfigNodeType::Undefined) {
				return OptionalLite<T>();
			} else {
				return OptionalLite<T>(ConfigNodeSerializer<T>().deserialize(context, node));
			}
		}
	};

	template <class, class = void_t<>> struct HasInPlaceDeserializer : std::false_type {};
	template <class T> struct HasInPlaceDeserializer<T, decltype(std::declval<ConfigNodeSerializer<T>>().deserialize(std::declval<const ConfigNodeSerializationContext&>(), std::declval<const ConfigNode&>(), std::declval<T&>()))> : std::true_type { };

	template <typename T>
	class ConfigNodeHelper {
	public:
		static ConfigNode serialize(const T& value, const ConfigNodeSerializationContext& context)
		{
			return ConfigNodeSerializer<T>().serialize(value, context);
		}
		
		static void deserialize(T& dst, const T& defaultValue, const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			if (node.getType() == ConfigNodeType::Undefined || node.getType() == ConfigNodeType::Del) {
				dst = defaultValue;
			} else {
				if constexpr (HasInPlaceDeserializer<T>::value) {
					ConfigNodeSerializer<T>().deserialize(context, node, dst);
				} else {
					dst = ConfigNodeSerializer<T>().deserialize(context, node);
				}
			}
		}
	};

	template <typename T>
	class ConfigNodeHelper<std::optional<T>> {
	public:
		static ConfigNode serialize(const std::optional<T>& src, const ConfigNodeSerializationContext& context)
		{
			if (src) {
				return ConfigNodeHelper<T>::serialize(src.value(), context);
			} else {
				return ConfigNode();
			}
		}

		static void deserialize(std::optional<T>& dst, const std::optional<T>& defaultValue, const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			dst = ConfigNodeSerializer<std::optional<T>>().deserialize(context, node);
		}
	};

	template <typename T>
	class ConfigNodeHelper<OptionalLite<T>> {
	public:
		static ConfigNode serialize(const OptionalLite<T>& src, const ConfigNodeSerializationContext& context)
		{
			if (src) {
				return ConfigNodeHelper<T>::serialize(src.value(), context);
			} else {
				return ConfigNode();
			}
		}
		
		static void deserialize(OptionalLite<T>& dst, const OptionalLite<T>& defaultValue, const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			dst = ConfigNodeSerializer<OptionalLite<T>>().deserialize(context, node);
		}
	};

	template <typename T>
	T ConfigNodeSerializerEnumUtils<T>::parseEnum(const ConfigNode& node)
	{
		return fromString<T>(node.asString());
	}

	template <typename T>
	ConfigNode ConfigNodeSerializerEnumUtils<T>::fromEnum(T value)
	{
		return ConfigNode(toString(value));
	}

	namespace Detail
	{
	    template<typename L, typename R>
	    struct HasOperatorDifferent
	    {
	        template<typename T = L, typename U = R>
	        static auto test(T &&t, U &&u) -> decltype(t == u, void(), std::true_type{});
	        static auto test(...) -> std::false_type;
	        using type = decltype(test(std::declval<L>(), std::declval<R>()));
	    };
	}

	template<typename L, typename R = L>
	struct HasOperatorDifferent : Detail::HasOperatorDifferent<L, R>::type {};

	template <typename T>
	class EntityConfigNodeSerializer {
	public:
		static void serialize(const T& value, const T& defaultValue, const ConfigNodeSerializationContext& context, ConfigNode& node, const String& name, int serializationMask)
		{
			if (context.matchType(serializationMask)) {
				bool canWrite;
				if constexpr (HasOperatorDifferent<T>::value) {
					canWrite = value != defaultValue;
				} else {
					canWrite = true;
				}
				
				if (canWrite) {
					auto result = Halley::ConfigNodeHelper<T>::serialize(value, context);
					if (result.getType() != ConfigNodeType::Undefined) {
						node[name] = std::move(result);
					}
				}
			} else {
				node[name] = ConfigNode(ConfigNode::NoopType());
			}
		}

		static void deserialize(T& value, const T& defaultValue, const ConfigNodeSerializationContext& context, const ConfigNode& node, const String& name, int serializationMask)
		{
			if (context.matchType(serializationMask) && node.getType() != ConfigNodeType::Noop) {
				const bool delta = node.getType() == ConfigNodeType::DeltaMap;
				const auto& fieldNode = node[name];
				if (fieldNode.getType() != ConfigNodeType::Noop && (fieldNode.getType() != ConfigNodeType::Undefined || !delta)) {
					ConfigNodeHelper<T>::deserialize(value, defaultValue, context, node[name]);
				}
			}
		}
	};

	template<>
	class ConfigNodeSerializer<ConfigNode>
	{
	public:
		ConfigNode serialize(const ConfigNode& item, const ConfigNodeSerializationContext& context)
		{
			return ConfigNode(item);
		}
		
		ConfigNode deserialize(const ConfigNodeSerializationContext& context, const ConfigNode& node)
		{
			return ConfigNode(node);
		}
	};
}

