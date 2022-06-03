#include "script_graph_editor.h"

#include "script_gizmo_ui.h"
#include "halley/tools/project/project.h"
#include "src/ui/infini_canvas.h"
#include "src/ui/project_window.h"
using namespace Halley;

ScriptGraphEditor::ScriptGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::ScriptGraph)
	, projectWindow(projectWindow)
{
}

void ScriptGraphEditor::onMakeUI()
{
	gizmoEditor = std::make_shared<ScriptGizmoUI>(factory, gameResources, *projectWindow.getEntityEditorFactory(), projectWindow.getScriptNodeTypes(), [=] ()
	{
		markModified();
	});
	
	if (scriptGraph) {
		gizmoEditor->load(*scriptGraph);
	}

	infiniCanvas = getWidgetAs<InfiniCanvas>("infiniCanvas");
	infiniCanvas->clear();
	infiniCanvas->add(gizmoEditor, 0, {}, UISizerAlignFlags::Centre, Vector2f());
	infiniCanvas->setMouseMirror(gizmoEditor);
	infiniCanvas->setZoomEnabled(false);
	infiniCanvas->setLeftClickScrollKey(KeyCode::Space);

	infiniCanvas->setZoomListener([=] (float zoom)
	{
		gizmoEditor->setZoom(zoom);
	});

	getWidget("toolbar")->add(gizmoEditor->makeUI());
}

void ScriptGraphEditor::reload()
{
}

void ScriptGraphEditor::refreshAssets()
{
}

void ScriptGraphEditor::save()
{
	if (modified) {
		modified = false;

		const auto assetPath = Path("comet/" + scriptGraph->getAssetId() + ".comet");
		const auto strData = scriptGraph->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
		project.setAssetSaveNotification(true);
	}
}

bool ScriptGraphEditor::isModified()
{
	return modified;
}

void ScriptGraphEditor::markModified()
{
	modified = true;
}

std::shared_ptr<const Resource> ScriptGraphEditor::loadResource(const String& assetId)
{
	if (project.isDLLLoaded()) {
		open();
	} else {
		pendingLoad = true;
	}
	
	return {};
}

void ScriptGraphEditor::open()
{
	Expects (project.isDLLLoaded());

	if (!hasUI) {
		factory.loadUI(*this, "halley/script_graph_editor");
		hasUI = true;
	}
	
	const auto assetPath = project.getImportAssetsDatabase().getPrimaryInputFile(assetType, assetId);
	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);

	if (!assetData.empty()) {
		auto config = YAMLConvert::parseConfig(assetData);
		scriptGraph = std::make_shared<ScriptGraph>(config.getRoot());
	} else {
		scriptGraph = std::make_shared<ScriptGraph>();
		scriptGraph->makeDefault();
		markModified();
	}
	scriptGraph->setAssetId(assetId);

	if (gizmoEditor) {
		gizmoEditor->load(*scriptGraph);
	}
}

void ScriptGraphEditor::update(Time time, bool moved)
{
	if (pendingLoad && project.isDLLLoaded()) {
		pendingLoad = false;
		open();
	}

	AssetEditor::update(time, moved);

	infiniCanvas->setScrollEnabled(!gizmoEditor->isHighlighted());
}
