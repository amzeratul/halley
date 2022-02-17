#pragma once

#include <memory>
#include "halley/file_formats/config_file.h"
#include "halley/support/exception.h"
#include "halley/time/halleytime.h"

namespace Halley {
	class EntityFactoryContext;
    class Resources;
	class ConfigNode;

	namespace EntitySerialization {
        enum class Type {
        	Undefined = 0,
        	Prefab = 1,
	        SaveData = 2
        };

		inline int makeMask(Type t)
		{
			return static_cast<int>(t);
		}
		
		template <typename T, typename ... Ts>
		[[nodiscard]] int makeMask(T v, Ts ... vs)
		{
			return static_cast<int>(v) | makeMask(vs...);
		}
	}

	class EntitySerializationContext;
	
	class IDataInterpolator {
	public:
		virtual ~IDataInterpolator() = default;

		virtual void setEnabled(bool enabled) { this->enabled = enabled; }
		virtual bool isEnabled() const { return enabled; }

		virtual void update(Time t) {}
		
		virtual void deserialize(const EntitySerializationContext& context, const ConfigNode& node) = 0;

	private:
		bool enabled = true;
	};

	class IDataInterpolatorSet {
	public:
		virtual ~IDataInterpolatorSet() = default;
		virtual IDataInterpolator* tryGetInterpolator(const EntitySerializationContext& context, std::string_view componentName, std::string_view fieldName) const = 0;
	};
	
	class EntitySerializationContext {
	public:
		Resources* resources = nullptr;
		const EntityFactoryContext* entityContext = nullptr;
		const IDataInterpolatorSet* interpolators = nullptr;
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
