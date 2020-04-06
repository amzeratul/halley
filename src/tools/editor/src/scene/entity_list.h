#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class EntityList final : public UIWidget {
	public:
		EntityList(String id, UIFactory& factory);
				
		void refreshList(const ISceneData& sceneData);

	private:
		UIFactory& factory;
		std::shared_ptr<UITreeList> list;
		
		void makeUI();
		void addEntities(const EntityTree& entity, int depth, const String& parentId);
	};
}
