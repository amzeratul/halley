#pragma once

#include "graph_asset_editor.h"
#include "graph_editor.h"
#include "script_graph_gizmo_ui.h"
#include "../asset_editor.h"
#include "halley/tools/dll/project_dll.h"
#include "src/scene/entity_editor.h"
#include "src/assets/config_undo_stack.h"
#include "src/ui/popup_window.h"

namespace Halley {
	class ScriptGraphVariableInspector;

	class ScriptGraphEditor : public GraphEditor {
	public:
		ScriptGraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<ScriptGraph> scriptGraph,
			AssetEditor* assetEditor, std::shared_ptr<const Scene> scene = {}, Callback callback = {}, Vector<String> entityTargets = {});
		~ScriptGraphEditor() override;

		void init() override;
		void setGraph(std::shared_ptr<BaseGraph> graph) override;

		void onMakeUI() override;

		const Vector<String>& getScriptTargetIds() const;
		std::shared_ptr<const Scene> getScene() const;
		
		void openProperties();

	protected:
    	void update(Time t, bool moved) override;
		void onActiveChanged(bool active) override;
		void onWasModified() override;

		std::shared_ptr<GraphGizmoUI> createGizmoEditor() override;

    private:
		struct EntityEnumData {
			size_t connId;
			int64_t entityId;
			String name;
			int scriptIdx;
		};

    	std::shared_ptr<ScriptGraph> scriptGraph;
		std::unique_ptr<ScriptState> scriptState;
		std::shared_ptr<ScriptGizmoUI> scriptGizmoEditor;

		std::shared_ptr<ScriptGraphVariableInspector> variableInspector;
		bool autoAcquire = false;
		bool variableInspectorEnabled = false;

    	std::optional<uint32_t> scriptEnumHandle;
		std::optional<uint32_t> scriptStateHandle;
		std::optional<std::pair<size_t, int64_t>> curEntityId;
		std::optional<std::pair<size_t, int64_t>> entityIdBeforeSuspend;
    	Vector<EntityEnumData> curEntities;

    	std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		Vector<String> entityTargets;
		Vector<String> cutsceneIds;

		void setListeningToClient(bool listening);
		void setListeningToState(std::pair<size_t, int64_t> entityId);

		void onScriptEnum(size_t connId, ConfigNode data);
		void refreshScriptEnum();
		void onScriptState(size_t connId, ConfigNode data);
		void onCurNodeData(const ConfigNode& curNodeData);
		void setCurNodeData(const String& str);
		void onDebugDisplayData(const ConfigNode& node);
		void setCurrentInstance(std::pair<size_t, int64_t> entityId);
		void updateNodeUnderCursor();
		ConfigNode getCurrentNodeConfig();

		bool tryAutoAcquire();
	};

	class ScriptGraphProperties : public PopupWindow {
	public:
		using Callback = std::function<void()>;
		ScriptGraphProperties(UIFactory& factory, ScriptGraph& script, Callback callback);

		void onMakeUI() override;

	private:
		UIFactory& factory;
		ScriptGraph& scriptGraph;
		Callback callback;

		ConfigNode properties;
	};

	class ScriptGraphAssetEditor : public GraphAssetEditor {
	public:
		ScriptGraphAssetEditor(UIFactory& factory, Resources& gameResoures, ProjectWindow& projectWindow);

	protected:
		std::shared_ptr<BaseGraph> makeGraph() override;
		std::shared_ptr<GraphEditor> makeGraphEditor(std::shared_ptr<BaseGraph> graph) override;
	};
}
