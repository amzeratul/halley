#pragma once

#include "script_gizmo_ui.h"
#include "../asset_editor.h"
#include "src/ui/infini_canvas.h"
#include "src/ui/scroll_background.h"

namespace Halley {
    class ScriptGraphEditor : public AssetEditor {
	public:
		ScriptGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow);
		~ScriptGraphEditor() override;

		void onActiveChanged(bool active) override;

        void reload() override;
        void refreshAssets() override;
		void onMakeUI() override;
		
		void save() override;
		bool isModified() override;
		void markModified();
		
	protected:
    	void update(Time t, bool moved) override;
        std::shared_ptr<const Resource> loadResource(const String& assetId) override;

    private:
		struct EntityEnumData {
			size_t connId;
			int64_t entityId;
			String name;
		};
		
		ProjectWindow& projectWindow;
		Resources& gameResources;
    	std::shared_ptr<ScriptGraph> scriptGraph;
		std::unique_ptr<ScriptState> scriptState;

		std::shared_ptr<ScriptGizmoUI> gizmoEditor;
		std::shared_ptr<InfiniCanvas> infiniCanvas;
		bool modified = false;
		bool pendingLoad = false;
		bool hasUI = false;

    	std::optional<uint32_t> scriptEnumHandle;
		std::optional<uint32_t> scriptStateHandle;
		std::optional<std::pair<size_t, int64_t>> curEntityId;
    	Vector<EntityEnumData> curEntities;

    	std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;

		void open();
		void setListeningToClient(bool listening);
		void setListeningToState(std::pair<size_t, int64_t> entityId);

		void onScriptEnum(size_t connId, ConfigNode data);
		void refreshScriptEnum();
		void onScriptState(size_t connId, ConfigNode data);
		void setCurrentInstance(std::pair<size_t, int64_t> entityId);
    };
}
