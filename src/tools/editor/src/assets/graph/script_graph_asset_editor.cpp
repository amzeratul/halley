#include "script_graph_asset_editor.h"

#include "script_graph_editor.h"
#include "halley/tools/project/project.h"
#include "src/ui/project_window.h"
using namespace Halley;

ScriptGraphAssetEditor::ScriptGraphAssetEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::ScriptGraph)
	, projectWindow(projectWindow)
	, gameResources(gameResources)
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		if (!dllListenerAdded) {
			dll.addReloadListener(*this);
			dllListenerAdded = true;
		}
	});
}

ScriptGraphAssetEditor::~ScriptGraphAssetEditor()
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		if (dllListenerAdded) {
			dll.removeReloadListener(*this);
		}
	});
}

void ScriptGraphAssetEditor::reload()
{
}

void ScriptGraphAssetEditor::refreshAssets()
{
}

void ScriptGraphAssetEditor::save()
{
	if (isModified()) {
		const auto scriptGraph = graphEditor->getScriptGraph();
		if (scriptGraph->getAssetId().isEmpty()) {
			throw Exception("Trying to save Comet script with no name", HalleyExceptions::Tools);
		}

		const auto strData = scriptGraph->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
		project.setAssetSaveNotification(true);

		graphEditor->setModified(false);
	}
}

bool ScriptGraphAssetEditor::isModified()
{
	return graphEditor && graphEditor->isModified();
}

void ScriptGraphAssetEditor::onProjectDLLStatusChange(ProjectDLL::Status status)
{
	if (status == ProjectDLL::Status::Unloaded) {
		if (graphEditor) {
			graphEditor->destroy();
			graphEditor = {};
		}
		clear();
		needsLoading = true;
	}
}

std::shared_ptr<const Resource> ScriptGraphAssetEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	if (project.isDLLLoaded()) {
		open();
	} else {
		pendingLoad = true;
	}
	
	return {};
}

void ScriptGraphAssetEditor::open()
{
	Expects (project.isDLLLoaded());

	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);
	std::shared_ptr<ScriptGraph> scriptGraph;
	bool modified = false;

	if (!assetData.empty()) {
		auto config = YAMLConvert::parseConfig(assetData);
		scriptGraph = std::make_shared<ScriptGraph>(config.getRoot());
	} else {
		scriptGraph = std::make_shared<ScriptGraph>();
		scriptGraph->makeDefault();
		modified = true;
	}
	scriptGraph->setAssetId(assetId);

	if (graphEditor) {
		graphEditor->setScriptGraph(std::move(scriptGraph));
	} else {
		graphEditor = std::make_shared<ScriptGraphEditor>(factory, gameResources, projectWindow, std::move(scriptGraph), this);
		add(graphEditor, 1);
	}
	if (modified) {
		graphEditor->setModified(true);
	}
}

void ScriptGraphAssetEditor::update(Time time, bool moved)
{
	if (pendingLoad && project.isDLLLoaded()) {
		pendingLoad = false;
		open();
	}

	AssetEditor::update(time, moved);
}
