#pragma once
#include "entity_validator_ui.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_tree_list.h"

namespace Halley {
	class EntityIcons;
	class SceneEditorWindow;
	class UIFactory;
	class EntityTree;
	class ISceneData;

	class EntityList final : public UIWidget {
	public:
		EntityList(String id, UIFactory& factory);

		void update(Time t, bool moved) override;
		
		void setSceneEditorWindow(SceneEditorWindow& sceneEditor);
		void setSceneData(std::shared_ptr<ISceneData> sceneData);

		void refreshList();
		void refreshNames();

		void onEntityModified(const String& id, const EntityData& node);
		void onEntityAdded(const String& id, const String& parentId, int childIndex, const EntityData& data);
		void onEntityRemoved(const String& id, const String& newSelectionId);
		void select(const String& id);

		UUID getEntityUnderCursor() const;
		String getCurrentSelection() const;

		void setEntityValidatorList(std::shared_ptr<EntityValidatorListUI> validatorList);
		void validateAllEntities();

		UITreeList& getList();

	private:
		struct EntityInfo {
			String name;
			Sprite icon;
			bool valid = true;
		};

		UIFactory& factory;
		SceneEditorWindow* sceneEditorWindow;
		const EntityIcons* icons = nullptr;

		std::shared_ptr<UITreeList> list;
		std::shared_ptr<ISceneData> sceneData;
		std::shared_ptr<EntityValidatorListUI> validatorList;
		std::set<UUID> invalidEntities;
		bool needsToNotifyValidatorList = false;

		void makeUI();
		void addEntities(const EntityTree& entity, const String& parentId);
		void addEntity(const EntityData& data, const String& parentId, int childIndex);
		void addEntityTree(const String& parentId, int childIndex, const EntityData& data);
		EntityInfo getEntityInfo(const EntityData& data) const;

		void openContextMenu(const String& entityId);
		void onContextMenuAction(const String& actionId, const String& entityId);

		bool markAllValid();
		bool markValid(const UUID& uuid, bool valid);
		void notifyValidatorList();
		void onEntityModified(const String& id, const EntityData& node, bool onlyRefreshValidation);

		bool isEntityValid(const EntityData& entityData, bool recursive) const;
	};
}
