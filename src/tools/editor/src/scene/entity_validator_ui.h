#pragma once
#include "halley/editor_extensions/entity_validator.h"

namespace Halley {
	class EntityValidator;

	class EntityValidatorUI : public UIWidget {
	public:
		EntityValidatorUI(String id, UIFactory& factory);

		void setValidator(EntityValidator& validator);
		void setEntity(EntityData& entity, IEntityEditor& entityEditor);
		void refresh();

	private:
		UIFactory& factory;

		EntityValidator* validator = nullptr;
		EntityData* curEntity = nullptr;
		IEntityEditor* entityEditor = nullptr;
		std::vector<IEntityValidator::Result> curResultSet;
	};
}
