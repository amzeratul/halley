#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class ECSData;
	class UIFactory;

	class EntityEditor final : public UIWidget {
	public:
		EntityEditor(String id, UIFactory& factory);

		void setSceneData(ISceneData& sceneData, ECSData& data);
		void showEntity(const String& id);

	private:
		UIFactory& factory;
		ISceneData* sceneData = nullptr;
		ECSData* ecsData = nullptr;
		std::shared_ptr<UIWidget> fields;

		void makeUI();
		void loadComponentData(const String& componentType, const ConfigNode& data);
	};
}
