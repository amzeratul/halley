#pragma once
#include "halley/editor_extensions/entity_validator.h"

namespace Halley {
	class EntityList;
	class EntityValidator;

	class EntityValidatorUI : public UIWidget {
	public:
		EntityValidatorUI(String id, UIFactory& factory);

		void setValidator(EntityValidator& validator);
		void setEntity(EntityData& entity, IEntityEditor& entityEditor, Resources& gameResources);
		void refresh();

	private:
		UIFactory& factory;

		EntityValidator* validator = nullptr;
		IEntityEditor* entityEditor = nullptr;
		Resources* gameResources = nullptr;

		EntityData* curEntity = nullptr;
		EntityData curEntityInstance;
		bool isPrefab = false;

		std::vector<IEntityValidator::Result> curResultSet;
	};

	class EntityValidatorListUI : public UIWidget {
	public:
		EntityValidatorListUI(String id, UIFactory& factory);

		void onMakeUI() override;

		void setList(std::weak_ptr<EntityList> entityList);
		void setInvalidEntities(std::vector<int> entities);

	private:
		UIFactory& factory;
		std::weak_ptr<EntityList> entityList;
		std::vector<int> invalidEntities;

		void move(int delta);
	};
}
