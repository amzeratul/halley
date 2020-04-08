#pragma once
#include "halley/ui/ui_widget.h"

namespace Halley {
	class UIFactory;

	class EntityList final : public UIWidget {
	public:
		EntityList(String id, UIFactory& factory);

		void setSceneData(std::shared_ptr<ISceneData> sceneData);
		void refreshList();

	private:
		UIFactory& factory;
		std::shared_ptr<UITreeList> list;
		std::shared_ptr<ISceneData> sceneData;
		
		void makeUI();
		void addEntities(const EntityTree& entity, int depth, const String& parentId);
	};
}
