#include "graph_editor.h"

#include "src/ui/project_window.h"

using namespace Halley;

GraphEditor::GraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<BaseGraph> graph, AssetEditor* assetEditor, std::shared_ptr<const Scene> scene, Callback callback)
	: UIWidget("GraphEditor", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, gameResources(gameResources)
	, project(projectWindow.getProject())
	, assetEditor(assetEditor)
	, scene(std::move(scene))
	, callback(std::move(callback))
	, undoStack(32)
	, graph(std::move(graph))
{
}

void GraphEditor::init()
{
	factory.loadUI(*this, "halley/graph_editor");
	initUndoStack();
}

void GraphEditor::setGraph(std::shared_ptr<BaseGraph> graph)
{
	this->graph = std::move(graph);
	initUndoStack();
}

void GraphEditor::initUndoStack()
{
	if (graph) {
		undoStack.loadInitialValue(graph->toConfigNode());
	} else {
		undoStack.clear();
	}
}
