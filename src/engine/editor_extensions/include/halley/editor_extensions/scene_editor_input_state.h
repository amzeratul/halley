#pragma once

#include <halley/maths/vector2.h>
#include <halley/maths/rect.h>

#include "halley/ui/widgets/ui_list.h"

namespace Halley {
   	struct SceneEditorInputState {
		// Filled on editor side
		bool leftClickPressed = false;
		bool leftClickReleased = false;
		bool leftClickHeld = false;
		bool middleClickPressed = false;
   		bool middleClickReleased = false;
		bool middleClickHeld = false;
		bool rightClickPressed = false;
   		bool rightClickReleased = false;
		bool rightClickHeld = false;
		bool shiftHeld = false;
		bool ctrlHeld = false;
   		bool altHeld = false;
		bool spaceHeld = false;
		std::optional<Vector2f> rawMousePos;
		Rect4f viewRect;

		// Filled on SceneEditor side
        std::optional<Vector2f> mousePos;
		std::optional<Rect4f> selectionBox;
		bool deselect = false;
    };

	struct SceneEditorOutputState {
		struct FieldChange {
			String componentName;
			String fieldName;
			UUID entityId;
			EntityData oldData;
			EntityData* newData = nullptr;
		};
		
		std::vector<FieldChange> fieldsChanged;
		std::vector<UUID> newSelection;
		UIList::SelectionMode selectionMode;
		std::optional<UUID> mouseOver;

		void clear()
		{
			fieldsChanged.clear();
			newSelection.clear();
			selectionMode = UIList::SelectionMode::Normal;
			mouseOver.reset();
		}
	};
}
