#pragma once

#include "script_gizmo_ui.h"
#include "../asset_editor.h"
#include "halley/tools/dll/project_dll.h"
#include "src/scene/entity_editor.h"
#include "src/ui/infini_canvas.h"
#include "src/assets/config_undo_stack.h"
#include "src/ui/popup_window.h"

namespace Halley {
	class ScriptGraphVariableInspector;

	class ScriptGraphEditor : public DrillDownAssetWindow {
	public:
		using Callback = std::function<void(bool, std::shared_ptr<ScriptGraph>)>;

		ScriptGraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<ScriptGraph> scriptGraph,
			AssetEditor* assetEditor, std::shared_ptr<const Scene> scene = {}, Callback callback = {}, Vector<String> entityTargets = {});
		~ScriptGraphEditor() override;

		void setScriptGraph(std::shared_ptr<ScriptGraph> graph);

		void onActiveChanged(bool active) override;
		void setModified(bool modified);
		bool isModified() override;
		void drillDownSave() override;
		std::shared_ptr<ScriptGraph> getScriptGraph();

		void onMakeUI() override;

		const Vector<String>& getScriptTargetIds() const;
		std::shared_ptr<const Scene> getScene() const;

		void onModified();
		void undo();
		void redo();
		void centreView();
		void openProperties();

	protected:
    	void update(Time t, bool moved) override;

    private:
		struct EntityEnumData {
			size_t connId;
			int64_t entityId;
			String name;
			int scriptIdx;
		};

		UIFactory& factory;
		ProjectWindow& projectWindow;
		Resources& gameResources;
		Project& project;
		AssetEditor* assetEditor = nullptr;
		std::shared_ptr<const Scene> scene;
		Callback callback;

		ConfigUndoStack undoStack;

    	std::shared_ptr<ScriptGraph> scriptGraph;
		std::unique_ptr<ScriptState> scriptState;

		std::shared_ptr<ScriptGizmoUI> gizmoEditor;
		std::shared_ptr<InfiniCanvas> infiniCanvas;
		std::shared_ptr<ScriptGraphVariableInspector> variableInspector;
		bool modified = false;
		bool autoAcquire = false;
		bool variableInspectorEnabled = false;

    	std::optional<uint32_t> scriptEnumHandle;
		std::optional<uint32_t> scriptStateHandle;
		std::optional<std::pair<size_t, int64_t>> curEntityId;
		std::optional<std::pair<size_t, int64_t>> entityIdBeforeSuspend;
    	Vector<EntityEnumData> curEntities;

    	std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
		std::shared_ptr<EntityEditorFactory> entityEditorFactory;
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

		void initUndoStack();
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
}
