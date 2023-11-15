#pragma once

#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/maths/colour.h"
#include "halley/maths/rect.h"
#include "halley/utils/type_traits.h"
#include "config_node_serializer_base.h"
#include <set>


#include "halley/resources/resource_reference.h"
#include "halley/support/logger.h"

namespace Halley {
	namespace Detail {
		template<class T> using VoidDeserializer = decltype(std::declval<ConfigNodeSerializer<T>>().deserialize(std::declval<const EntitySerializationContext&>(), std::declval<const ConfigNode&>(), std::declval<T&>()));
	}

	template <typename T>
	class ConfigNodeHelper {
	public:
		static ConfigNode serialize(const T& value, const EntitySerializationContext& context)
		{
			return ConfigNodeSerializer<T>().serialize(value, context);
		}
		
		static void deserialize(T& dst, const T& defaultValue, const EntitySerializationContext& context, const ConfigNode& node)
		{
			if (node.getType() == ConfigNodeType::Undefined || node.getType() == ConfigNodeType::Del) {
				dst = defaultValue;
			} else {
				if constexpr (is_detected<Detail::VoidDeserializer, T>::value) {
					ConfigNodeSerializer<T>().deserialize(context, node, dst);
				} else {
					dst = ConfigNodeSerializer<T>().deserialize(context, node);
				}
			}
		}
		
		static void deserialize(T& dst, const EntitySerializationContext& context, const ConfigNode& node)
		{
			if (node.getType() == ConfigNodeType::Undefined || node.getType() == ConfigNodeType::Del) {
				dst = T();
			} else {
				if constexpr (is_detected<Detail::VoidDeserializer, T>::value) {
					ConfigNodeSerializer<T>().deserialize(context, node, dst);
				} else {
					dst = ConfigNodeSerializer<T>().deserialize(context, node);
				}
			}
		}
	};

	template <>
	class ConfigNodeSerializer<bool> {
	public:
		ConfigNode serialize(bool value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		int deserialize(const EntitySerializationContext&, const ConfigNode& node)
		{
			return node.asBool(false);
		}
	};

	template <>
    class ConfigNodeSerializer<int8_t> {
    public:
        ConfigNode serialize(int8_t value, const EntitySerializationContext& context)
		{
			return ConfigNode(static_cast<int>(value));
		}
		
		int8_t deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return static_cast<int8_t>(node.asInt(0));
        }
    };

	template <>
    class ConfigNodeSerializer<uint8_t> {
    public:
        ConfigNode serialize(uint8_t value, const EntitySerializationContext& context)
		{
			return ConfigNode(static_cast<int>(value));
		}
		
		uint8_t deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return static_cast<uint8_t>(node.asInt(0));
        }
    };

	template <>
    class ConfigNodeSerializer<int16_t> {
    public:
        ConfigNode serialize(int16_t value, const EntitySerializationContext& context)
		{
			return ConfigNode(static_cast<int>(value));
		}
		
		int16_t deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return static_cast<int16_t>(node.asInt(0));
        }
    };

	template <>
    class ConfigNodeSerializer<uint16_t> {
    public:
        ConfigNode serialize(uint16_t value, const EntitySerializationContext& context)
		{
			return ConfigNode(static_cast<int>(value));
		}
		
		uint16_t deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return static_cast<uint16_t>(node.asInt(0));
        }
    };

	template <>
    class ConfigNodeSerializer<int32_t> {
    public:
        ConfigNode serialize(int32_t value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		int32_t deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asInt(0);
        }
    };

	template <>
    class ConfigNodeSerializer<int64_t> {
    public:
        ConfigNode serialize(int64_t value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		int64_t deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asInt64(0);
        }
    };

	template <>
    class ConfigNodeSerializer<uint32_t> {
    public:
        ConfigNode serialize(uint32_t value, const EntitySerializationContext& context)
		{
			return ConfigNode(static_cast<int64_t>(value));
		}
		
		uint32_t deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return static_cast<uint32_t>(node.asInt64(0));
        }
    };

	template <>
    class ConfigNodeSerializer<float> {
    public:
        ConfigNode serialize(float value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		float deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asFloat(0);
        }
    };

	template <>
    class ConfigNodeSerializer<Vector2i> {
    public:
        ConfigNode serialize(Vector2i value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector2i deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asVector2i(Vector2i());
        }
    };

	template <>
    class ConfigNodeSerializer<Vector2f> {
    public:
        ConfigNode serialize(Vector2f value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector2f deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asVector2f(Vector2f());
        }
    };

	template <>
    class ConfigNodeSerializer<Vector3i> {
    public:
        ConfigNode serialize(Vector3i value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector3i deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asVector3i(Vector3i());
        }
    };

	template <>
    class ConfigNodeSerializer<Vector3f> {
    public:
        ConfigNode serialize(Vector3f value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector3f deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asVector3f(Vector3f());
        }
    };

	template <>
    class ConfigNodeSerializer<Vector4i> {
    public:
        ConfigNode serialize(Vector4i value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector4i deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asVector4i(Vector4i());
        }
    };

	template <>
    class ConfigNodeSerializer<Vector4f> {
    public:
        ConfigNode serialize(Vector4f value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}
		
		Vector4f deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return node.asVector4f(Vector4f());
        }
    };

	template <>
    class ConfigNodeSerializer<Angle1f> {
    public:
        ConfigNode serialize(Angle1f value, const EntitySerializationContext& context)
		{
			return ConfigNode(value.toDegrees());
		}
		
		Angle1f deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return Angle1f::fromDegrees(node.asFloat(0.0f));
        }
    };

	template <>
    class ConfigNodeSerializer<Colour4f> {
    public:
        ConfigNode serialize(Colour4f value, const EntitySerializationContext& context)
		{
			return ConfigNode(value.toString());
		}
		
		Colour4f deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
			return Colour4f::fromString(node.asString("#000000"));
        }
    };

	template <>
    class ConfigNodeSerializer<Rect4i> {
    public:
        ConfigNode serialize(Rect4i value, const EntitySerializationContext& context)
		{
        	Vector<int> seq = { value.getX(), value.getY(), value.getWidth(), value.getHeight() };
        	return ConfigNode(seq);
		}
		
		Rect4i deserialize(const EntitySerializationContext&, const ConfigNode& node)
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
        ConfigNode serialize(Rect4f value, const EntitySerializationContext& context)
		{
        	Vector<float> seq = { value.getX(), value.getY(), value.getWidth(), value.getHeight() };
        	return ConfigNode(seq);
		}
		
		Rect4f deserialize(const EntitySerializationContext&, const ConfigNode& node)
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
        ConfigNode serialize(const std::optional<T>& value, const EntitySerializationContext& context)
		{
        	if (value) {
				return ConfigNodeSerializer<T>().serialize(value.value(), context);
			} else {
				return ConfigNode();
			}
		}
		
        std::optional<T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
        {
        	if (node.getType() == ConfigNodeType::Undefined || node.getType() == ConfigNodeType::Del) {
				return std::optional<T>();
        	} else {
				return std::optional<T>(ConfigNodeSerializer<T>().deserialize(context, node));
			}
        }
    };

	template <typename T>
    class ConfigNodeSerializer<Vector<T>> {
    public:
        ConfigNode serialize(const Vector<T>& values, const EntitySerializationContext& context)
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
		
        Vector<T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
        {
			Vector<T> result;
        	if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
				result.reserve(seq.size());
        		for (auto& s: seq) {
					result.push_back(ConfigNodeSerializer<T>().deserialize(context, s));
				}
			}
			return result;
        }

		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, Vector<T>& target)
        {
			if (node.getType() == ConfigNodeType::Sequence) {
				auto seq = node.asSequence();
		        target.resize(seq.size());
				for (size_t i = 0; i < target.size(); ++i) {
					ConfigNodeHelper<T>::deserialize(target[i], context, seq[i]);
				}
			} else {
				target.clear();
			}
        }
	};

	template <typename T>
    class ConfigNodeSerializer<std::shared_ptr<T>> {
    public:
        ConfigNode serialize(const std::shared_ptr<T>& value, const EntitySerializationContext& context)
		{
			return ConfigNodeSerializer<T>().serialize(*value, context);
		}
		
        std::shared_ptr<T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
        {
			return std::make_shared<T>(ConfigNodeSerializer<T>().deserialize(context, node));
        }
		
        void deserialize(const EntitySerializationContext& context, const ConfigNode& node, std::shared_ptr<T>& target)
        {
			if (target) {
				ConfigNodeHelper<T>::deserialize(*target, context, node);
			} else {
				target = std::make_shared<T>(ConfigNodeSerializer<T>().deserialize(context, node));
			}
        }
	};

	template <typename A, typename B>
	class ConfigNodeSerializer<std::pair<A, B>>
	{
	public:
		ConfigNode serialize(const std::pair<A, B>& values, const EntitySerializationContext& context)
		{
			auto serializerA = ConfigNodeSerializer<A>();
			auto serializerB = ConfigNodeSerializer<B>();
			ConfigNode::SequenceType result = ConfigNode::SequenceType();

			result.emplace_back(serializerA.serialize(values.first, context));
			result.emplace_back(serializerB.serialize(values.second, context));
			
			return result;
		}

		std::pair<A, B> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			auto serializerA = ConfigNodeSerializer<A>();
			auto serializerB = ConfigNodeSerializer<B>();

			std::pair<A, B> result = std::pair<A, B>();

			if (node.getType() == ConfigNodeType::Sequence) {
				auto& seqNode = node.asSequence();

				if (seqNode.size() > 0) {
					result.first = serializerA.deserialize(context, seqNode.at(0));
				}

				if (seqNode.size() > 1) {
					result.second = serializerB.deserialize(context, seqNode.at(1));
				}
			}
			
			return result;
		}
	};

	template <typename T>
	class ConfigNodeSerializer<std::set<T>> {
	public:
		ConfigNode serialize(const std::set<T>& values, const EntitySerializationContext& context)
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
		
        std::set<T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
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
	class ConfigNodeSerializer<HashSet<T>> {
	public:
		ConfigNode serialize(const HashSet<T>& values, const EntitySerializationContext& context)
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

		HashSet<T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			HashSet<T> result;
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
		ConfigNode serialize(const ResourceReference<T>& value, const EntitySerializationContext& context)
		{
			return ConfigNode(value ? value->getAssetId() : "");
		}
		
        ResourceReference<T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			const auto assetId = node.hasKey("asset") ? node["asset"].asString("") : (node.getType() == ConfigNodeType::String ? node.asString("") : "");
			if (assetId.isEmpty()) {
				return {};
			} else {
				if (context.resources->exists<T>(assetId)) {
					return ResourceReference<T>(context.resources->get<T>(assetId));
				} else {
					Logger::logWarning("ResourceReference<" + String(typeid(T).name()) + "> could not load " + assetId, true);
					return {};
				}				
			}
		}
	};
	
	template<>
	class ConfigNodeSerializer<String> {
	public:
		ConfigNode serialize(const String& value, const EntitySerializationContext& context)
		{
			return ConfigNode(value);
		}

		String deserialize(const EntitySerializationContext&, const ConfigNode& node)
		{
			return node.asString("");
		}
	};

	template <typename T>
	class ConfigNodeSerializer<std::map<String, T>>
	{
	public:
		ConfigNode serialize(const std::map<String, T>& values, const EntitySerializationContext& context)
		{
        	auto serializer = ConfigNodeSerializer<T>();
        	ConfigNode result = ConfigNode::MapType();
        	for (auto& [key, value]: values) {
        		result[key] = serializer.serialize(value, context);
        	}
        	return result;
		}

		std::map<String, T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			std::map<String, T> result;
			deserialize(context, node, result);
			return result;
		}
		
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, std::map<String, T>& result)
		{
			if (node.getType() == ConfigNodeType::Map || node.getType() == ConfigNodeType::DeltaMap) {
				if (node.getType() == ConfigNodeType::Map) {
					result.clear();
				}
				for (const auto& s: node.asMap()) {
					if (s.second.getType() == ConfigNodeType::Del) {
						result.erase(s.first);
					} else {
						ConfigNodeHelper<T>().deserialize(result[s.first], T(), context, s.second);
					}
				}
			}
		}
	};

	template <typename K, typename V>
	class ConfigNodeSerializer<std::map<K, V>>
	{
	public:
		ConfigNode serialize(const std::map<K, V>& values, const EntitySerializationContext& context)
		{
        	auto serializer = ConfigNodeSerializer<V>();
        	ConfigNode result = ConfigNode::MapType();
        	for (auto& [key, value]: values) {
        		result[toString(key)] = serializer.serialize(value, context);
        	}
        	return result;
		}

		std::map<K, V> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			std::map<K, V> result;
			deserialize(context, node, result);
			return result;
		}
		
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, std::map<K, V>& result)
		{
			if (node.getType() == ConfigNodeType::Map || node.getType() == ConfigNodeType::DeltaMap) {
				if (node.getType() == ConfigNodeType::Map) {
					result.clear();
				}
				for (const auto& s: node.asMap()) {
					if (s.second.getType() == ConfigNodeType::Del) {
						result.erase(fromString<K>(s.first));
					} else {
						ConfigNodeHelper<V>().deserialize(result[fromString<K>(s.first)], V(), context, s.second);
					}
				}
			}
		}
	};

	template <typename K, typename V>
	class ConfigNodeSerializer<HashMap<K, V>>
	{
	public:
		ConfigNode serialize(const HashMap<K, V>& values, const EntitySerializationContext& context)
		{
        	auto serializer = ConfigNodeSerializer<V>();
        	ConfigNode result = ConfigNode::MapType();
        	for (auto& [key, value]: values) {
        		result[toString(key)] = serializer.serialize(value, context);
        	}
        	return result;
		}

		HashMap<K, V> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			HashMap<K, V> result;
			deserialize(context, node, result);
			return result;
		}
		
		void deserialize(const EntitySerializationContext& context, const ConfigNode& node, HashMap<K, V>& result)
		{
			if (node.getType() == ConfigNodeType::Map || node.getType() == ConfigNodeType::DeltaMap) {
				if (node.getType() == ConfigNodeType::Map) {
					result.clear();
				}
				for (const auto& s: node.asMap()) {
					if (s.second.getType() == ConfigNodeType::Del) {
						result.erase(fromString<K>(s.first));
					} else {
						ConfigNodeHelper<V>().deserialize(result[fromString<K>(s.first)], V(), context, s.second);
					}
				}
			}
		}
	};

	template <typename T>
	class ConfigNodeSerializer<OptionalLite<T>> {
	public:
        ConfigNode serialize(const OptionalLite<T>& value, const EntitySerializationContext& context)
		{
        	if (value) {
				return ConfigNodeSerializer<T>::serialize(value.value(), context);
			} else {
				return ConfigNode();
			}
		}

		OptionalLite<T> deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			if (node.getType() == ConfigNodeType::Undefined) {
				return OptionalLite<T>();
			} else {
				return OptionalLite<T>(ConfigNodeSerializer<T>().deserialize(context, node));
			}
		}
	};

	template <typename T>
	class ConfigNodeHelper<std::optional<T>> {
	public:
		static ConfigNode serialize(const std::optional<T>& src, const EntitySerializationContext& context)
		{
			if (src) {
				return ConfigNodeHelper<T>::serialize(src.value(), context);
			} else {
				return ConfigNode();
			}
		}

		static void deserialize(std::optional<T>& dst, const EntitySerializationContext& context, const ConfigNode& node)
		{
			dst = ConfigNodeSerializer<std::optional<T>>().deserialize(context, node);
		}

		static void deserialize(std::optional<T>& dst, const std::optional<T>& defaultValue, const EntitySerializationContext& context, const ConfigNode& node)
		{
			dst = ConfigNodeSerializer<std::optional<T>>().deserialize(context, node);
		}
	};

	template <typename T>
	class ConfigNodeHelper<OptionalLite<T>> {
	public:
		static ConfigNode serialize(const OptionalLite<T>& src, const EntitySerializationContext& context)
		{
			if (src) {
				return ConfigNodeHelper<T>::serialize(src.value(), context);
			} else {
				return ConfigNode();
			}
		}
		
		static void deserialize(OptionalLite<T>& dst, const EntitySerializationContext& context, const ConfigNode& node)
		{
			dst = ConfigNodeSerializer<OptionalLite<T>>().deserialize(context, node);
		}
		
		static void deserialize(OptionalLite<T>& dst, const OptionalLite<T>& defaultValue, const EntitySerializationContext& context, const ConfigNode& node)
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
	        static auto test(T &&t, U &&u) -> decltype(t != u, void(), std::true_type{});
	        static auto test(...) -> std::false_type;
	        using type = decltype(test(std::declval<L>(), std::declval<R>()));
	    };
	}

	template<typename L, typename R = L>
	struct HasOperatorDifferent : Detail::HasOperatorDifferent<L, R>::type {};

	template <typename T, typename Interpolator = void>
	class EntityConfigNodeSerializer {
	public:
		static void serialize(const T& value, const T& defaultValue, const EntitySerializationContext& context, ConfigNode& node, std::string_view componentName, std::string_view name, int serializationMask)
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

		static void deserialize(T& value, const T& defaultValue, const EntitySerializationContext& context, const ConfigNode& node, std::string_view componentName, std::string_view fieldName, int serializationMask)
		{
			if (context.matchType(serializationMask) && node.getType() != ConfigNodeType::Noop) {
				const bool delta = node.getType() == ConfigNodeType::DeltaMap;
				const auto& fieldNode = node[fieldName];
				if (fieldNode.getType() != ConfigNodeType::Noop && (fieldNode.getType() != ConfigNodeType::Undefined || !delta)) {
					auto* interpolator = context.interpolators ? context.interpolators->tryGetInterpolator(context, componentName, fieldName) : nullptr;
					if (interpolator) {
						interpolator->deserialize(&value, &defaultValue, context, node[fieldName]);
					} else {
						ConfigNodeHelper<T>::deserialize(value, defaultValue, context, node[fieldName]);
					}
				}
			}
		}
	};

	template<>
	class ConfigNodeSerializer<ConfigNode>
	{
	public:
		ConfigNode serialize(const ConfigNode& item, const EntitySerializationContext& context)
		{
			return ConfigNode(item);
		}
		
		ConfigNode deserialize(const EntitySerializationContext& context, const ConfigNode& node)
		{
			return ConfigNode(node);
		}
	};
}

