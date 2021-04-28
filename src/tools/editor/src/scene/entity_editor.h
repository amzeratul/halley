#pragma once
#include "entity_icons.h"
#include "halley/core/game/scene_editor_interface.h"
#include "halley/tools/ecs/fields_schema.h"
#include "halley/ui/ui_widget.h"
#include "src/ui/select_asset_widget.h"

namespace Halley {
	class EntityEditorFactory;
	class SceneEditorWindow;
	class ECSData;
	class UIFactory;
	class UIDropdown;
	class UITextInput;

	class EntityEditor final : public UIWidget, IEntityEditor {
	public:
		EntityEditor(String id, UIFactory& factory);

		void setEntityEditorFactory(std::shared_ptr<EntityEditorFactory> entityEditorFactory);

		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;
		
		void update(Time t, bool moved) override;
		
		void setSceneEditorWindow(SceneEditorWindow& sceneEditor);
		void setECSData(ECSData& data);

		bool loadEntity(const String& id, EntityData& data, const Prefab* prefabData, bool force, Resources& gameResources);
		void unloadEntity();
		void reloadEntity();
		void onFieldChangedByGizmo(const String& componentName, const String& fieldName);

		void setDefaultName(const String& name, const String& prevName) override;

		void addComponent();
		void addComponent(const String& name);
		void deleteComponent(const String& name);

		void setHighlightedComponents(std::vector<String> componentNames);

	protected:
		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		UIFactory& factory;
		ECSData* ecsData = nullptr;
		SceneEditorWindow* sceneEditor = nullptr;
		const EntityIcons* entityIcons = nullptr;
		std::shared_ptr<EntityEditorFactory> entityEditorFactory;
		
		std::shared_ptr<UIWidget> fields;
		std::shared_ptr<UITextInput> entityName;
		std::shared_ptr<UIDropdown> entityIcon;
		std::shared_ptr<SelectAssetWidget> prefabName;

		EntityData* currentEntityData = nullptr;
		EntityData prevEntityData;

		String currentId;
		const Prefab* prefabData = nullptr;
		bool needToReloadUI = false;
		bool isPrefab = false;
		Resources* gameResources = nullptr;

		int ecsDataRevision = 0;

		std::map<String, std::shared_ptr<UIWidget>> componentWidgets;
		std::vector<String> highlightedComponents;

		void makeUI();
		void loadComponentData(const String& componentType, ConfigNode& data);

		void setName(const String& name);
		String getName() const;
		void setPrefabName(const String& prefab);
		void editPrefab();
		void setIcon(const String& icon);

		void onEntityUpdated() override;
		void setTool(const String& tool, const String& componentName, const String& fieldName) override;
		EntityData& getEntityData();
		const EntityData& getEntityData() const;

		std::set<String> getComponentsOnEntity() const;
		std::set<String> getComponentsOnPrefab() const;

		void setComponentColour(const String& name, UIWidget& component);
	};

	class EntityEditorFactory : public IEntityEditorFactory {
	public:
		EntityEditorFactory(UIFactory& factory);
		
		void setEntityEditor(IEntityEditor& entityEditor);
		void setGameResources(Resources& resources);

		void addFieldFactories(std::vector<std::unique_ptr<IComponentEditorFieldFactory>> factories);
		void resetFieldFactories();
	
		std::shared_ptr<IUIElement> makeLabel(const String& label) const override;
		std::shared_ptr<IUIElement> makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const override;
		ConfigNode getDefaultNode(const String& fieldType) const override;

	private:
		UIFactory& factory;
		IEntityEditor* entityEditor = nullptr;
		Resources* gameResources = nullptr;
		std::map<String, std::unique_ptr<IComponentEditorFieldFactory>> fieldFactories;

		mutable std::unique_ptr<ComponentEditorContext> context;

		std::pair<String, std::vector<String>> parseType(const String& type) const;
		void makeContext();
	};
}
