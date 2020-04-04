#pragma once
#include <functional>

#include "create_functions.h"
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/entity/entity.h"

namespace Halley {
	class World;
	class Resources;
	
	class EntityFactory {
	public:
		enum class UpdateMode {
			TransformOnly,
			UpdateAll,
			UpdateAllDeleteOld
		};
		
		explicit EntityFactory(World& world, Resources& resources);
		virtual ~EntityFactory();

		EntityRef createEntity(const char* prefabName);
		EntityRef createEntity(const String& prefabName);
		EntityRef createEntity(const ConfigNode& node);
		
		void updateEntity(EntityRef& entity, const ConfigNode& node, UpdateMode mode = UpdateMode::UpdateAll);
		void updateEntityTree(EntityRef& entity, const ConfigNode& node);
		
		template <typename T>
		CreateComponentFunctionResult createComponent(EntityRef& e, const ConfigNode& componentData)
		{
			CreateComponentFunctionResult result;
			result.componentId = T::componentIndex;
			
			auto comp = e.tryGetComponent<T>();
			if (comp) {
				comp->deserialize(context, componentData);
			} else {
				T component;
				component.deserialize(context, componentData);
				e.addComponent<T>(std::move(component));
				result.created = true;
			}

			return result;
		}

	private:
		World& world;
		ConfigNodeSerializationContext context;
		std::unique_ptr<EntitySerializationContext> entityContext;

		EntityRef createEntity(EntityRef* parent, const ConfigNode& node, bool populate);
		void doUpdateEntityTree(EntityRef& entity, const ConfigNode& node, bool refreshing);
	};

	class EntitySerializationContext {
	public:
		World& world;
		std::map<UUID, EntityId> uuids;

		EntitySerializationContext(World& world);
	};
}
