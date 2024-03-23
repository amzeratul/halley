#pragma once
#include "src/assets/asset_editor.h"
#include "src/assets/config_undo_stack.h"

namespace Halley {
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

		GraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<BaseGraph> graph, AssetEditor* assetEditor, std::shared_ptr<const Scene> scene = {}, Callback callback = {});
		virtual void init();

		void setGraph(std::shared_ptr<BaseGraph> graph);

	protected:
		UIFactory& factory;
		ProjectWindow& projectWindow;
		Resources& gameResources;
		Project& project;
		AssetEditor* assetEditor = nullptr;
		std::shared_ptr<const Scene> scene;
		Callback callback;

		ConfigUndoStack undoStack;

	private:
		std::shared_ptr<BaseGraph> graph;

		void initUndoStack();
	};
}
