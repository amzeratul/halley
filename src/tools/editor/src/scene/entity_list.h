#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class EntityList final : public UIWidget {
	public:
		EntityList(String id, UIFactory& factory);

		void clearExceptions();
		void addException(EntityId entityId);
		
		void refreshList(const World& world);

	private:
		UIFactory& factory;
		std::shared_ptr<UIList> list;
		std::set<EntityId> exceptions;

		void makeUI();
	};
}
