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

		EntityRef createEntity(const char* prefabName);
		EntityRef createEntity(const String& prefabName);
		EntityRef createEntity(const ConfigNode& node);
		
		void updateEntity(EntityRef& entity, const ConfigNode& node, bool transformOnly = false);
		void updateEntityTree(EntityRef& entity, const ConfigNode& node);
		
		template <typename T>
		void createComponent(EntityRef& e, const ConfigNode& componentData)
		{
			auto comp = e.tryGetComponent<T>();
			if (comp) {
				comp->deserialize(context, componentData);
			} else {
				T component;
				component.deserialize(context, componentData);
				e.addComponent<T>(std::move(component));
			}
		}

	private:
		World& world;
		ConfigNodeSerializationContext context;
		std::unique_ptr<EntitySerializationContext> entityContext;

		EntityRef createEntity(EntityRef* parent, const ConfigNode& node, bool populate);
	};

	class EntitySerializationContext {
	public:
		World& world;
		std::map<UUID, EntityId> uuids;

		EntitySerializationContext(World& world);
	};
}
