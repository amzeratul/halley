#include "halley/core/editor_extensions/scene_editor_interface.h"
#include "editor_extensions/scene_editor_input_state.h"
using namespace Halley;


void SceneEditorOutputState::clear()
{
	fieldsChanged.clear();
	newSelection.reset();
	mouseOver.reset();
}

bool EntityTree::contains(const String& id) const
{
	if (entityId == id) {
		return true;
	}

	for (const auto& child: children) {
		if (child.contains(id)) {
			return true;
		}
	}

	return false;
}
