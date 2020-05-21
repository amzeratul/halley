#pragma once

#include <memory>
#include "halley/support/exception.h"

namespace Halley {
    class EntitySerializationContext;
    class Resources;
	class ConfigNode;
	
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
}
