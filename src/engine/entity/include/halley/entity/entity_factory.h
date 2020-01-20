#pragma once
#include <functional>
#include "entity.h"
#include "halley/file_formats/config_file.h"

namespace Halley {
	class World;
	
	class EntityFactory {
	public:
		template <typename T>
		static void createComponent(EntityRef& e, const ConfigNode& componentData)
		{
			T component;
			component.deserialize(componentData);
			e.addComponent<T>(std::move(component));
		}

		explicit EntityFactory(World& world);
		EntityRef createEntity(const ConfigNode& node);

	private:
		World& world;
	};
}
