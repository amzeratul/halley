#pragma once
#include <functional>
#include "entity.h"
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"

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
		std::vector<EntityEntry> createScene(const ConfigNode& sceneNode);

		template <typename F>
		std::vector<EntityEntry> createSceneWith(const ConfigNode& sceneNode, F f)
		{
			auto result = createScene(sceneNode);
			for (auto& r: result) {
				f(r);
			}
			return result;
		}

		template <typename T, typename F>
		std::vector<EntityEntry> createSceneWithComponentAdjust(const ConfigNode& sceneNode, F f)
		{
			auto result = createScene(sceneNode);
			for (auto& r: result) {
				if (r.entity.hasComponent<T>()) {
					f(r.entity.getComponent<T>());
				}
			}
			return result;
		}
		
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
	};
}
