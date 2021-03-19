#pragma once
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
	class EntityEditorFactories {
	public:
		static std::vector<std::unique_ptr<IComponentEditorFieldFactory>> getDefaultFactories();
	};
}
