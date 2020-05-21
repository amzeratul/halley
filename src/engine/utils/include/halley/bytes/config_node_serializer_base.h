#pragma once

#include <memory>
#include "halley/support/exception.h"

namespace Halley {
    class EntitySerializationContext;
    class Resources;
	class ConfigNode;

	template <typename T>
    class ConfigNodeSerializerEnumUtils {
	public:
	    static T parseEnum(const ConfigNode& node);
    };
	
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
        	if constexpr (std::is_enum_v<T>) {
        		return ConfigNodeSerializerEnumUtils<T>::parseEnum(node);
        	} else {
	        	throw Exception("ConfigNodeSerializer unimplemented type: " + String(typeid(T).name()), HalleyExceptions::Utils);
            }
        }
    };
}
