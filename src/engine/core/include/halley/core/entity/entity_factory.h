#pragma once
#include <functional>
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/entity/entity.h"

namespace Halley {
	class World;
	class Resources;
	
	class EntityFactory {
	public:
		explicit EntityFactory(World& world, Resources& resources);
		virtual ~EntityFactory();
		
		EntityRef createEntity(const ConfigNode& node, bool populate = true);
		void updateEntity(EntityRef& entity, const ConfigNode& node);
		void updateEntityTree(EntityRef& entity, const ConfigNode& node);
		
		template <typename T>
		void createComponent(EntityRef& e, const ConfigNode& componentData)
		{
			auto comp = e.tryGetComponent<T>();
			if (comp) {
				comp->deserialize(resources, componentData);
			} else {
				T component;
				component.deserialize(resources, componentData);
				e.addComponent<T>(std::move(component));
			}
		}

	private:
		World& world;
		Resources& resources;

		void createChildEntity(EntityRef& parent, const ConfigNode& node, bool populate);
	};
}
