#include "graph_asset_editor.h"
#include "graph_editor.h"
#include "halley/tools/project/project.h"
#include "src/ui/project_window.h"
using namespace Halley;

GraphAssetEditor::GraphAssetEditor(UIFactory& factory, Resources& gameResources, ProjectWindow& projectWindow, AssetType assetType)
	: AssetEditor(factory, gameResources, projectWindow.getProject(), assetType)
	, projectWindow(projectWindow)
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		if (!dllListenerAdded) {
			dll.addReloadListener(*this);
			dllListenerAdded = true;
		}
	});
}

GraphAssetEditor::~GraphAssetEditor()
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		if (dllListenerAdded) {
			dll.removeReloadListener(*this);
		}
	});
}

void GraphAssetEditor::onResourceLoaded()
{
}

void GraphAssetEditor::refreshAssets()
{
}

void GraphAssetEditor::save()
{
	if (isModified()) {
		const auto graph = graphEditor->getGraph();
		if (graph->getAssetId().isEmpty()) {
			throw Exception("Trying to save graph asset with no name", HalleyExceptions::Tools);
		}

		const auto strData = graph->toYAML();

		project.setAssetSaveNotification(false);
		project.writeAssetToDisk(assetPath, gsl::as_bytes(gsl::span<const char>(strData.c_str(), strData.length())));
		project.setAssetSaveNotification(true);

		graphEditor->setModified(false);
	}
}

bool GraphAssetEditor::isModified()
{
	return graphEditor && graphEditor->isModified();
}

void GraphAssetEditor::onProjectDLLStatusChange(ProjectDLL::Status status)
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

std::shared_ptr<const Resource> GraphAssetEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);
	bool modified = false;

	std::shared_ptr<BaseGraph> graph = makeGraph();
	if (!assetData.empty()) {
		auto config = YAMLConvert::parseConfig(assetData);
		graph->load(config.getRoot());
	} else {
		graph->makeDefault();
		modified = true;
	}
	graph->setAssetId(assetId);

	if (graphEditor) {
		graphEditor->setGraph(std::move(graph));
	} else {
		graphEditor = makeGraphEditor(std::move(graph));
		graphEditor->init();
		add(graphEditor, 1);
	}
	if (modified) {
		graphEditor->setModified(true);
	}

	return graph;
}
