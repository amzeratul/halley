#pragma once
#include <functional>
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/entity/entity.h"

namespace Halley {
	class World;
	class Resources;

	class EntityEntry {
	public:
		String name;
		EntityRef entity;
	};
	
	class EntityFactory {
	public:
		explicit EntityFactory(World& world, Resources& resources);
		virtual ~EntityFactory();
		
		EntityEntry createEntity(const ConfigNode& node);
		
		template <typename T>
		void createComponent(EntityRef& e, const ConfigNode& componentData)
		{
			T component;
			component.deserialize(resources, componentData);
			e.addComponent<T>(std::move(component));
		}

	private:
		World& world;
		Resources& resources;

		void createChildEntity(const ConfigNode& node, EntityRef& parent);
	};
}
