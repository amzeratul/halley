#pragma once
#include "halley/core/scene_editor/scene_editor_interface.h"
#include "halley/tools/ecs/fields_schema.h"
#include "halley/ui/ui_widget.h"

namespace Halley {
	class SceneEditorWindow;
	class ECSData;
	class UIFactory;
	class UIDropdown;
	class UITextInput;

	class EntityEditor final : public UIWidget, IEntityEditor {
	public:
		EntityEditor(String id, UIFactory& factory);

		void update(Time t, bool moved) override;

		void setSceneEditorWindow(SceneEditorWindow& sceneEditor);
		void setECSData(ECSData& data);
		void addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories);

		bool loadEntity(const String& id, ConfigNode& data, const ConfigNode* prefabData, bool force, Resources& gameResources);
		void unloadEntity();
		void reloadEntity();
		void onFieldChangedByGizmo(const String& componentName, const String& fieldName);

		std::shared_ptr<IUIElement> makeLabel(const String& label) override;
		void createField(IUISizer& parent, const String& fieldType, const ComponentFieldParameters& parameters, bool createLabel) override;
		ConfigNode getDefaultNode(const String& fieldType) override;
		
	private:
		UIFactory& factory;
		ECSData* ecsData = nullptr;
		SceneEditorWindow* sceneEditor = nullptr;
		std::unique_ptr<ComponentEditorContext> context;
		
		std::shared_ptr<UIWidget> fields;
		std::shared_ptr<UITextInput> entityName;
		std::shared_ptr<UIDropdown> prefabName;
		std::map<String, std::unique_ptr<IComponentEditorFieldFactory>> fieldFactories;

		String currentId;
		ConfigNode* currentEntityData = nullptr;
		const ConfigNode* prefabData = nullptr;
		bool needToReloadUI = false;
		bool isPrefab = false;
		Resources* gameResources = nullptr;

		void makeUI();
		void loadComponentData(const String& componentType, ConfigNode& data, const std::vector<String>& componentNames);
		std::pair<String, std::vector<String>> parseType(const String& type);

		void addComponent();
		void addComponent(const String& name);
		void deleteComponent(const String& name);

		void setName(const String& name);
		void setPrefabName(const String& name);

		void onEntityUpdated() override;
		void setTool(SceneEditorTool tool, const String& componentName, const String& fieldName, ConfigNode options) override;
		ConfigNode& getEntityData();

		void updatePrefabNames();
	};
}
