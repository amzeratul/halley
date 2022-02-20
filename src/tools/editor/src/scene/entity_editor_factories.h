#pragma once
#include "halley/core/game/scene_editor_interface.h"

namespace Halley {
	class EntityEditorFactories {
	public:
		static Vector<std::unique_ptr<IComponentEditorFieldFactory>> getDefaultFactories();
	};
}
