#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class EntityEditor final : public UIWidget {
	public:
		EntityEditor(String id, UIFactory& factory);

		void setSceneData(ISceneData& sceneData);
		void showEntity(const String& id);

	private:
		UIFactory& factory;
		ISceneData* sceneData = nullptr;

		void makeUI();
	};
}
