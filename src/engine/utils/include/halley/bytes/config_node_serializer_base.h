#pragma once

#include <memory>
#include "halley/file_formats/config_file.h"
#include "halley/support/exception.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class UUID;
	class EntityFactoryContext;
    class Resources;
	class ConfigNode;

	namespace EntitySerialization {
        enum class Type : uint8_t {
        	Undefined,
        	Prefab,
	        SaveData,
        	Network,
        };

		inline int makeMask(Type t)
		{
			if (t == Type::Undefined) {
				return 0;
			}
			return 1 << (static_cast<int>(t) - 1);
		}
		
		template <typename T, typename ... Ts>
		[[nodiscard]] int makeMask(T v, Ts ... vs)
		{
			return static_cast<int>(v) | makeMask(vs...);
		}
	}

	template <>
	struct EnumNames<EntitySerialization::Type> {
		constexpr std::array<const char*, 4> operator()() const {
			return{{
				"Undefined",
				"Prefab",
				"SaveData",
				"Network"
			}};
		}
	};

	class EntitySerializationContext;
	
	class IDataInterpolator {
	public:
		virtual ~IDataInterpolator() = default;

		virtual void setEnabled(bool enabled) {}
		virtual bool isEnabled() const { return true; }

		virtual void update(Time t) {}
		
		virtual void deserialize(void* value, const void* defaultValue, const EntitySerializationContext& context, const ConfigNode& node) = 0;
		virtual std::optional<ConfigNode> prepareFieldForSerialization(const ConfigNode& fromValue, const ConfigNode& toValue) { return {}; } // Return nullopt if "toValue" is good to go
	};

	class IDataInterpolatorSetRetriever {
	public:
		virtual ~IDataInterpolatorSetRetriever() = default;
		virtual IDataInterpolator* tryGetInterpolator(const EntitySerializationContext& context, std::string_view componentName, std::string_view fieldName) const = 0;
		virtual ConfigNode createComponentDelta(const UUID& instanceUUID, const String& componentName, const ConfigNode& from, const ConfigNode& to) const = 0;
	};
	
	class EntitySerializationContext {
	public:
		Resources* resources = nullptr;
		const EntityFactoryContext* entityContext = nullptr;
		const IDataInterpolatorSetRetriever* interpolators = nullptr;
		int entitySerializationTypeMask = EntitySerialization::makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::SaveData);

		[[nodiscard]] bool matchType(int typeMask) const
		{
			return (entitySerializationTypeMask & typeMask) != 0;
		}
	};

	using ConfigNodeSerializationContext [[deprecated]] = EntitySerializationContext;

	template <typename T>
    class ConfigNodeSerializerEnumUtils {
	public:
	    static T parseEnum(const ConfigNode& node);
		static ConfigNode fromEnum(T value);
    };

    template <typename T>
    class ConfigNodeSerializer {
    public:
        ConfigNode serialize(const T& src, const EntitySerializationContext& context)
        {
        	if constexpr (std::is_enum_v<T>) {
        		return ConfigNodeSerializerEnumUtils<T>::fromEnum(src);
        	} else {
	        	throw Exception("ConfigNodeSerializer unimplemented type: " + String(typeid(T).name()), HalleyExceptions::Utils);
            }
        }
    	
        T deserialize(const EntitySerializationContext&, const ConfigNode& node)
        {
        	if constexpr (std::is_enum_v<T>) {
        		return ConfigNodeSerializerEnumUtils<T>::parseEnum(node);
        	} else {
	        	throw Exception("ConfigNodeSerializer unimplemented type: " + String(typeid(T).name()), HalleyExceptions::Utils);
            }
        }
    };
}
