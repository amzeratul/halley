#include "mesh_editor.h"

#include "../../../tools/src/assets/importers/mesh_importer.h"
#include "halley/tools/project/project.h"

using namespace Halley;

MeshEditor::MeshEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, resources, project, type)
	, project(project)
	, projectWindow(projectWindow)
{
}

void MeshEditor::refreshAssets()
{
}

bool MeshEditor::isReadyToLoad() const
{
	return project.areAssetsLoaded();
}

void MeshEditor::update(Time t, bool moved)
{
}

std::shared_ptr<const Resource> MeshEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	std::shared_ptr<Mesh> mesh;

	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);
	if (!assetData.empty()) {
		mesh = std::shared_ptr(MeshImporter::parse(assetPath, assetData, *this));
	}
	mesh->setAssetId(assetId);

	return mesh;
}

void MeshEditor::onTabbedIn()
{
}

Bytes MeshEditor::readAdditionalFile(const Path& filePath)
{
	return Path::readFile(filePath);
}
