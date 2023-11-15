#pragma once
#include "entity_icons.h"
#include "entity_validator_ui.h"
#include "halley/game/scene_editor_interface.h"
#include "halley/tools/ecs/fields_schema.h"
#include "halley/ui/ui_widget.h"
#include "src/ui/select_asset_widget.h"

namespace Halley {
	class EntityEditorFactoryRoot;
	class EntityEditorFactory;
	class SceneEditorWindow;
	class ECSData;
	class UIFactory;
	class UIDropdown;
	class UITextInput;

	class EntityEditor final : public UIWidget, IEntityEditor {
	public:
		EntityEditor(String id, UIFactory& factory);

		void setEntityEditorFactory(EntityEditorFactoryRoot* entityEditorFactory);

		void onAddedToRoot(UIRoot& root) override;
		void onRemovedFromRoot(UIRoot& root) override;
		
		void update(Time t, bool moved) override;

		void setSceneEditorWindow(SceneEditorWindow& sceneEditor, const HalleyAPI& api);
		void setECSData(ECSData& data);

		bool loadEntity(const String& id, EntityData& data, const Prefab* prefabData, bool force, Resources& gameResources);
		void unloadEntity(bool becauseHasMultiple);
		void reloadEntity() override;
		void unloadIcons();
		void unloadValidator();
		void onFieldChangedByGizmo(const String& componentName, const String& fieldName) override;
		void onFieldChangedProcedurally(const String& componentName, const String& fieldName) override;

		void setDefaultName(const String& name, const String& prevName) override;
		void focusRenameEntity();

		void addComponent();
		void addComponent(const String& name, ConfigNode data) override;
		void deleteComponent(const String& name) override;
		void copyAllComponentsToClipboard();
		void copyComponentToClipboard(const String& name, bool append);
		void copyComponentsToClipboard(ConfigNode components);
		void pasteComponentsFromClipboard();
		bool isValidComponents(const ConfigNode& data);
		void pasteComponents(const ConfigNode& data);
		void pasteComponent(const String& name, ConfigNode data);

		void setHighlightedComponents(Vector<String> componentNames);

		IProjectWindow& getProjectWindow() const;

		Vector<EntityInfo> getEntities() const override;
		EntityInfo getEntityInfo(const UUID& uuid) const override;
		void goToEntity(const UUID& uuid) override;
		
	protected:
		bool onKeyPress(KeyboardKeyPress key) override;

	private:
		UIFactory& factory;
		ECSData* ecsData = nullptr;
		SceneEditorWindow* sceneEditor = nullptr;
		const HalleyAPI* api = nullptr;
		const EntityIcons* entityIcons = nullptr;
		std::shared_ptr<EntityEditorFactory> entityEditorFactory;
		
		std::shared_ptr<UIWidget> fields;
		std::shared_ptr<UITextInput> entityName;
		std::shared_ptr<UIDropdown> entityIcon;
		std::shared_ptr<UIDropdown> variant;
		std::shared_ptr<SelectAssetWidget> prefabName;

		EntityData* currentEntityData = nullptr;
		EntityData prevEntityData;

		String currentId;
		const Prefab* prefabData = nullptr;
		bool needToReloadUI = false;
		bool isPrefab = false;
		bool unloadedBecauseHasMultiple = false;
		Resources* gameResources = nullptr;

		int ecsDataRevision = 0;

		std::map<String, std::shared_ptr<UIWidget>> componentWidgets;
		Vector<String> highlightedComponents;

		std::shared_ptr<EntityValidatorUI> entityValidatorUI;

		void makeUI();
		void loadComponentData(const String& componentType, ConfigNode& data);

		void loadVariants();
		void setName(const String& name, bool markModified = true);
		String getName() const;
		void setPrefabName(const String& prefab);
		void setSelectable(bool selectable);
		void setSerializable(bool serializable);
		void setEntityEnabled(bool enabled);
		void editPrefab();
		void setIcon(const String& icon);
		void setVariant(const String& variant);

		void refreshEntityData();
		void onEntityUpdated(bool temporary = false) override;
		void setTool(const String& tool, const String& componentName, const String& fieldName) override;
		EntityData& getEntityData();
		const EntityData& getEntityData() const;
		ConfigNode& getComponentData(const String& componentName, const String& fieldName);

		std::set<String> getComponentsOnEntity() const;
		std::set<String> getComponentsOnPrefab() const;

		void setComponentColour(const String& name, UIWidget& component);
		ConfigNode serializeComponent(const String& name, const ConfigNode& data);
		ConfigNode getComponentsFromClipboard();
	};

	class EntityEditorFactoryRoot {
		friend class EntityEditorFactory;
	public:
		EntityEditorFactoryRoot(ProjectWindow& projectWindow, UIFactory& factory);
		
		void setGameResources(Resources& resources, const HalleyAPI& api);

		void addFieldFactories(Vector<std::unique_ptr<IComponentEditorFieldFactory>> factories);
		void addStandardFieldFactories();
		void clear();
		bool isEmpty() const;

	private:
		ProjectWindow& projectWindow;
		UIFactory& factory;
		Resources* gameResources = nullptr;
		const HalleyAPI* api = nullptr;
		HashMap<String, std::unique_ptr<IComponentEditorFieldFactory>> fieldFactories;
	};

	class EntityEditorFactory : public IEntityEditorFactory {
	public:
		EntityEditorFactory(EntityEditorFactoryRoot& root, IEntityEditorCallbacks* callbacks);

		std::shared_ptr<IUIElement> makeLabel(const String& label) const override;
		std::shared_ptr<UIWidget> makeNestedField(const String& label) const override;
		std::shared_ptr<IUIElement> makeField(const String& fieldType, ComponentFieldParameters parameters, ComponentEditorLabelCreation createLabel) const override;
		ConfigNode getDefaultNode(const String& fieldType) const override;

		void addFieldFactory(std::unique_ptr<IComponentEditorFieldFactory> factory);

	private:
		EntityEditorFactoryRoot& root;
		std::unique_ptr<ComponentEditorContext> context;
		HashMap<String, std::unique_ptr<IComponentEditorFieldFactory>> additionalFieldFactories;

		std::pair<String, Vector<String>> parseType(const String& type) const;
		std::unique_ptr<ComponentEditorContext> makeContext(IEntityEditorCallbacks* callbacks);
	};
}
