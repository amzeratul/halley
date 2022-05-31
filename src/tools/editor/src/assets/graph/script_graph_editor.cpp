#include "script_graph_editor.h"
#include "halley/tools/project/project.h"
using namespace Halley;

ScriptGraphEditor::ScriptGraphEditor(UIFactory& factory, Resources& gameResources, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, gameResources, project, AssetType::ScriptGraph)
{
}

void ScriptGraphEditor::onMakeUI()
{
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

		const auto assetPath = Path("script_graph/" + scriptGraph->getAssetId() + ".yaml");
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
	
	return scriptGraph;
}

void ScriptGraphEditor::update(Time time, bool moved)
{
	AssetEditor::update(time, moved);
}
