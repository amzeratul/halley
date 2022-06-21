#pragma once
#include "halley/core/game/scene_editor_interface.h"
#include "halley/tools/ecs/ecs_data.h"

namespace Halley {
	class EntityEditorFactories {
	public:
		static Vector<std::unique_ptr<IComponentEditorFieldFactory>> getDefaultFactories();
		static Vector<std::unique_ptr<IComponentEditorFieldFactory>> getECSFactories(const ECSData& ecsData);
	};
}
