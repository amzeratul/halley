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
		ProjectWindow& projectWindow;
    	std::shared_ptr<ScriptGraph> scriptGraph;

		std::shared_ptr<ScriptGizmoUI> gizmoEditor;
		std::shared_ptr<InfiniCanvas> infiniCanvas;
		bool modified = false;
		bool pendingLoad = false;
		bool hasUI = false;

    	std::optional<uint32_t> scriptEnumHandle;
		std::optional<uint32_t> scriptStateHandle;
		int64_t curEntityId = -1;

		void open();
		void setListeningToClient(bool listening);
		void setListeningToState(int64_t entityId);

		void onScriptEnum(ConfigNode data);
		void onScriptState(ConfigNode data);
		void setCurrentInstance(int64_t entityId);
    };
}
