#pragma once
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_tree_list.h"

namespace Halley {
	class SceneEditorWindow;
	class UIFactory;
	class EntityTree;
	class ISceneData;

	class EntityList final : public UIWidget {
	public:
		EntityList(String id, UIFactory& factory);

		void setSceneEditorWindow(SceneEditorWindow& sceneEditor);
		void setSceneData(std::shared_ptr<ISceneData> sceneData);
		void refreshList();

		void onEntityModified(const String& id, const ConfigNode& node);
		void onEntityAdded(const String& id, const String& parentId, const ConfigNode& data);
		void onEntityRemoved(const String& id, const String& parentId);

	protected:
		bool onKeyPress(KeyboardKeyPress key) override;
		
	private:
		UIFactory& factory;
		SceneEditorWindow* sceneEditor;

		std::shared_ptr<UITreeList> list;
		std::shared_ptr<ISceneData> sceneData;

		void makeUI();
		void addEntities(const EntityTree& entity, const String& parentId);
		void addEntity(const String& name, const String& id, const String& parentId, const String& prefab);
		String getEntityName(const ConfigNode& data) const;
		String getEntityName(const String& name, const String& prefab) const;
	};
}
