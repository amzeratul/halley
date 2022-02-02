#pragma once

#include <memory>
#include "halley/file_formats/config_file.h"
#include "halley/support/exception.h"

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
	
	class EntitySerializationContext {
	public:
		Resources* resources = nullptr;
		const EntityFactoryContext* entityContext = nullptr;
		int entitySerializationTypeMask = EntitySerialization::makeMask(EntitySerialization::Type::Prefab, EntitySerialization::Type::SaveData);

		[[nodiscard]] bool matchType(int typeMask) const
		{
			return (entitySerializationTypeMask & typeMask) != 0;
		}
	};

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
