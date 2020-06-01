#include "halley/core/editor_extensions/scene_editor_interface.h"
using namespace Halley;


void SceneEditorOutputState::clear()
{
	fieldsChanged.clear();
	newSelection.reset();
	mouseOver.reset();
}

std::optional<String> EntityTree::findIdInScene(const String& id) const
{
	if (entityId == id) {
		return entityId;
	}

	if (std::find(prefabChildrenId.begin(), prefabChildrenId.end(), id) != prefabChildrenId.end()) {
		return entityId;
	}

	for (const auto& child: children) {
		auto result = child.findIdInScene(id);
		if (result) {
			return result;
		}
	}

	return {};
}
