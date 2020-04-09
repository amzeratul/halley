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

		void setSceneEditorWindow(SceneEditorWindow& sceneEditor);
		void setECSData(ECSData& data);
		void addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories);

		bool loadEntity(const String& id, ConfigNode& data, bool force);
		void reloadEntity();

	private:
		UIFactory& factory;
		ECSData* ecsData = nullptr;
		SceneEditorWindow* sceneEditor = nullptr;
		ComponentEditorContext context;
		
		std::shared_ptr<UIWidget> fields;
		std::shared_ptr<UITextInput> entityName;
		std::map<String, std::unique_ptr<IComponentEditorFieldFactory>> fieldFactories;

		String currentId;
		ConfigNode* currentEntityData = nullptr;
		bool needToReloadUI = false;

		void makeUI();
		void loadComponentData(const String& componentType, ConfigNode& data);
		std::shared_ptr<IUIElement> createEditField(const String& fieldType, const String& fieldName, ConfigNode& componentData, const String& defaultValue);

		void addComponent();
		void addComponent(const String& name);
		void deleteComponent(const String& name);
		void setName(const String& name);
		void onEntityUpdated();
		ConfigNode& getEntityData();
	};
}
