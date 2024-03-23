#pragma once
#include "src/assets/asset_editor.h"
#include "src/assets/config_undo_stack.h"

namespace Halley {
	class EntityEditorFactory;
	class InfiniCanvas;
	class GraphGizmoUI;
	class ScriptGraphVariableInspector;

	class IGraphEditor {
	public:
		virtual ~IGraphEditor() = default;

		virtual void setModified(bool modified) = 0;
		virtual void onModified() = 0;
		virtual void undo() = 0;
		virtual void redo() = 0;
	};

	class GraphEditor: public UIWidget, public IDrillDownAssetWindow, public IGraphEditor {
	public:
		using Callback = std::function<void(bool, std::shared_ptr<BaseGraph>)>;

		GraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<BaseGraph> graph, AssetType assetType, AssetEditor* assetEditor, std::shared_ptr<const Scene> scene = {}, Callback callback = {});
		virtual void init();

		virtual void setGraph(std::shared_ptr<BaseGraph> graph);
		std::shared_ptr<BaseGraph> getGraph() const;

		void onMakeUI() override;
		
		void setModified(bool modified) override;
		bool isModified() override;
		void drillDownSave() override;

		void onModified() override;
		void undo() override;
		void redo() override;
		void centreView();

		String getAssetKey() const;

		std::shared_ptr<UIWidget> asWidget() override;

	protected:
		UIFactory& factory;
		ProjectWindow& projectWindow;
		Resources& gameResources;
		Project& project;
		AssetEditor* assetEditor = nullptr;
		std::shared_ptr<const Scene> scene;
		Callback callback;
		AssetType assetType;

		ConfigUndoStack undoStack;

		std::shared_ptr<GraphGizmoUI> gizmoEditor;
		std::shared_ptr<InfiniCanvas> infiniCanvas;
		bool modified = false;
		std::shared_ptr<EntityEditorFactory> entityEditorFactory;

		virtual std::shared_ptr<GraphGizmoUI> createGizmoEditor() = 0;
		virtual void onWasModified();

	private:
		std::shared_ptr<BaseGraph> graph;

		void initUndoStack();
	};
}
