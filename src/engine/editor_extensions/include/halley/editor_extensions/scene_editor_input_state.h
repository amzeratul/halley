#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>

namespace Halley {
   	struct SceneEditorInputState {
		// Filled on editor side
		bool leftClickPressed = false;
		bool leftClickReleased = false;
		bool leftClickHeld = false;
		bool middleClickPressed = false;
		bool middleClickHeld = false;
		bool rightClickPressed = false;
		bool rightClickHeld = false;
		bool shiftHeld = false;
		bool ctrlHeld = false;
   		bool altHeld = false;
		bool spaceHeld = false;
		Vector2f rawMousePos;
		Rect4f viewRect;

		// Filled on SceneEditor side
        Vector2f mousePos;
		std::optional<Rect4f> selectionBox;
		bool deselect = false;
    };

	struct SceneEditorOutputState {
		std::vector<std::pair<String, String>> fieldsChanged;
		std::optional<UUID> newSelection;
		std::optional<UUID> mouseOver;

		void clear()
		{
			fieldsChanged.clear();
			newSelection.reset();
			mouseOver.reset();
		}
	};
}
