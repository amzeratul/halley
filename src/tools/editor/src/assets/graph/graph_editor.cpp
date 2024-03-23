#include "graph_editor.h"

#include "graph_gizmo_ui.h"
#include "src/ui/infini_canvas.h"
#include "src/ui/project_window.h"

using namespace Halley;

GraphEditor::GraphEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, std::shared_ptr<BaseGraph> graph, AssetType assetType, AssetEditor* assetEditor, std::shared_ptr<const Scene> scene, Callback callback)
	: UIWidget("GraphEditor", {}, UISizer())
	, factory(factory)
	, projectWindow(projectWindow)
	, gameResources(gameResources)
	, project(projectWindow.getProject())
	, assetEditor(assetEditor)
	, scene(std::move(scene))
	, callback(std::move(callback))
	, assetType(assetType)
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

std::shared_ptr<BaseGraph> GraphEditor::getGraph() const
{
	return graph;
}

void GraphEditor::initUndoStack()
{
	if (graph) {
		undoStack.loadInitialValue(graph->toConfigNode());
	} else {
		undoStack.clear();
	}
}

void GraphEditor::onMakeUI()
{
	entityEditorFactory = std::make_shared<EntityEditorFactory>(projectWindow.getEntityEditorFactoryRoot(), nullptr);

	gizmoEditor = createGizmoEditor();
	
	if (graph) {
		gizmoEditor->load(*graph);
	}
	const auto assetKey = getAssetKey();

	infiniCanvas = getWidgetAs<InfiniCanvas>("infiniCanvas");
	infiniCanvas->clear();
	infiniCanvas->add(gizmoEditor, 0, {}, UISizerAlignFlags::Top | UISizerAlignFlags::Left);
	infiniCanvas->setMouseMirror(gizmoEditor);
	infiniCanvas->setZoomEnabled(false);
	infiniCanvas->setLeftClickScrollKey(KeyCode::Space);

	infiniCanvas->setZoomListener([=] (float zoom)
	{
		gizmoEditor->setZoom(zoom);
	});
	infiniCanvas->setScrollPosition(projectWindow.getAssetSetting(assetKey, "position").asVector2f({}));
	infiniCanvas->setScrollListener([=] (Vector2f pos)
	{
		projectWindow.setAssetSetting(assetKey, "position", ConfigNode(pos));
	});

	getWidget("toolbarGizmo")->clear();
	getWidget("toolbarGizmo")->add(gizmoEditor->makeUI());

	getWidget("drillUpBar")->setActive(!!callback);
	
	const auto autoConnect = projectWindow.getSetting(EditorSettingType::Project, "autoConnectPins").asBool(true);
	gizmoEditor->setAutoConnectPins(autoConnect);
	bindData("autoConnectPins", autoConnect, [=](bool value)
	{
		projectWindow.setSetting(EditorSettingType::Project, "autoConnectPins", ConfigNode(value));
		gizmoEditor->setAutoConnectPins(value);
	});
	
	setHandle(UIEventType::ButtonClicked, "ok", [=](const UIEvent& event)
	{
		callback(true, graph);
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "cancel", [=](const UIEvent& event)
	{
		callback(false, graph);
		destroy();
	});

	setHandle(UIEventType::ButtonClicked, "undoButton", [=](const UIEvent& event)
	{
		undo();
	});

	setHandle(UIEventType::ButtonClicked, "redoButton", [=](const UIEvent& event)
	{
		redo();
	});

	setHandle(UIEventType::ButtonClicked, "centreViewButton", [=](const UIEvent& event)
	{
		centreView();
	});
	
	getWidget("saveButton")->setActive(assetEditor);
	if (assetEditor) {
		setHandle(UIEventType::ButtonClicked, "saveButton", [=](const UIEvent& event)
		{
			assetEditor->save();
		});
	}
}

void GraphEditor::onModified()
{
	setModified(true);
	if (graph) {
		undoStack.update(graph->toConfigNode());
	}
}

void GraphEditor::undo()
{
	if (graph && undoStack.canUndo()) {
		const auto assetId = graph->getAssetId();
		graph->load(undoStack.undo(), gameResources);
		graph->setAssetId(assetId);
	}
}

void GraphEditor::redo()
{
	if (graph && undoStack.canRedo()) {
		const auto assetId = graph->getAssetId();
		graph->load(undoStack.redo(), gameResources);
		graph->setAssetId(assetId);
	}
}

void GraphEditor::centreView()
{
	infiniCanvas->setScrollPosition({});
}

String GraphEditor::getAssetKey() const
{
	return graph ? (toString(assetType) + ":" + graph->getAssetId()) : "";
}

std::shared_ptr<UIWidget> GraphEditor::asWidget()
{
	return shared_from_this();
}

void GraphEditor::setModified(bool value)
{
	modified = value;

	if (value) {
		onWasModified();
	}
}

bool GraphEditor::isModified()
{
	return modified;
}

void GraphEditor::drillDownSave()
{
	callback(true, graph);
	setModified(false);
}

void GraphEditor::onWasModified()
{}
