#pragma once
#include "halley/editor_extensions/entity_validator.h"

namespace Halley {
	class EntityValidator;

	class EntityValidatorUI : public UIWidget {
	public:
		EntityValidatorUI(String id, UIFactory& factory);

		void setValidator(EntityValidator& validator);
		void setEntity(EntityData& entity);
		void refresh();

	private:
		UIFactory& factory;

		EntityValidator* validator = nullptr;
		EntityData* curEntity = nullptr;
		std::vector<IEntityValidator::Result> curResultSet;
	};
}
