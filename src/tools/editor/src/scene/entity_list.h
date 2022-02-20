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

		void onEntityModified(const String& id, const EntityData* prevData, const EntityData& newData);
		void onEntitiesAdded(gsl::span<const EntityChangeOperation> changes);
		void onEntitiesRemoved(gsl::span<const String> ids, const String& newSelectionId);

		void select(const String& id, UIList::SelectionMode mode = UIList::SelectionMode::Normal);
		void select(gsl::span<const String> ids, UIList::SelectionMode mode = UIList::SelectionMode::Normal);

		UUID getEntityUnderCursor() const;
		String getCurrentSelection() const;
		Vector<String> getCurrentSelections() const;

		void setEntityValidatorList(std::shared_ptr<EntityValidatorListUI> validatorList);
		void validateAllEntities();
		IEntityValidator::Severity getValidationSeverity() const;
		bool isWaitingToValidate() const;
		void forceValidationIfWaiting();

		UITreeList& getList();

	private:
		struct EntityInfo {
			String name;
			Sprite icon;
			IEntityValidator::Severity severity;
		};

		UIFactory& factory;
		SceneEditorWindow* sceneEditorWindow;
		const EntityIcons* icons = nullptr;

		std::shared_ptr<UITreeList> list;
		std::shared_ptr<ISceneData> sceneData;
		std::shared_ptr<EntityValidatorListUI> validatorList;
		std::map<UUID, IEntityValidator::Severity> invalidEntities;
		IEntityValidator::Severity validationSeverity;
		bool needsToNotifyValidatorList = false;
		bool needsToValidateAllEntities = true;
		Time validationTimeout = 0;

		void makeUI();
		void addEntities(const EntityTree& entity, const String& parentId);
		void addEntity(const EntityData& data, const String& parentId, int childIndex);
		void addEntityTree(const String& parentId, int childIndex, const EntityData& data);
		EntityInfo getEntityInfo(const EntityData& data) const;

		void openContextMenu(Vector<String> entityIds);
		void onContextMenuAction(const String& actionId, gsl::span<const String> entityIds);

		bool markAllValid();
		bool markValid(const UUID& uuid, IEntityValidator::Severity severity);
		void notifyValidatorList();
		void doValidateAllEntities();
		void validateEntityTree(const EntityTree& entityTree);
		void onEntityModified(const String& id, const EntityData& node, bool onlyRefreshValidation);

		IEntityValidator::Severity getEntitySeverity(const EntityData& entityData, bool recursive) const;
	};
}
