#pragma once
#include <functional>
#include "entity.h"
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	class World;
	class Resources;
	
	class EntityFactory {
	public:
		explicit EntityFactory(World& world, Resources& resources);

		template <typename T>
		void createComponent(EntityRef& e, const ConfigNode& componentData)
		{
			T component;
			component.deserialize(resources, componentData);
			e.addComponent<T>(std::move(component));
		}

		Maybe<EntityRef> createEntity(const ConfigNode& node);
		void createScene(const ConfigNode& sceneNode);

	private:
		World& world;
		Resources& resources;
	};
}
