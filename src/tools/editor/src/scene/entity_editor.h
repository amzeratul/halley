#pragma once
#include "halley/tools/ecs/fields_schema.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorWindow;
	class ECSData;
	class UIFactory;

	class EntityEditor final : public UIWidget {
	public:
		EntityEditor(String id, UIFactory& factory);

		void update(Time t, bool moved) override;

		void setSceneEditor(SceneEditorWindow& sceneEditor);
		void setSceneData(ISceneData& sceneData, ECSData& data);
		void showEntity(const String& id);
		void addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories);

	private:
		UIFactory& factory;
		ISceneData* sceneData = nullptr;
		ECSData* ecsData = nullptr;
		SceneEditorWindow* sceneEditor = nullptr;
		ComponentEditorContext context;
		
		std::shared_ptr<UIWidget> fields;
		std::map<String, std::unique_ptr<IComponentEditorFieldFactory>> fieldFactories;

		String currentId;
		ConfigNode currentEntityData;
		bool needToReloadUI = false;

		void makeUI();
		void loadComponentData(const String& componentType, ConfigNode& data);
		std::shared_ptr<IUIElement> createEditField(const String& fieldType, const String& fieldName, ConfigNode& componentData, const String& defaultValue);

		void addComponent();
		void addComponent(const String& name);
		void deleteComponent(const String& name);
		void onEntityUpdated();
	};
}
