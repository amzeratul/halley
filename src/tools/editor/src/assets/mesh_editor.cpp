#include "mesh_editor.h"

#include "../../../tools/src/assets/importers/mesh_importer.h"
#include "halley/tools/project/project.h"
#include "src/ui/project_window.h"

using namespace Halley;

MeshEditor::MeshEditor(UIFactory& factory, Resources& resources, AssetType type, Project& project, ProjectWindow& projectWindow)
	: AssetEditor(factory, resources, project, type)
	, project(project)
	, projectWindow(projectWindow)
{
	setInteractWithMouse(true);
}

void MeshEditor::refreshAssets()
{
	AssetEditor::refreshAssets();
}

bool MeshEditor::isReadyToLoad() const
{
	return project.areAssetsLoaded();
}

void MeshEditor::update(Time t, bool moved)
{
	AssetEditor::update(t, moved);

	if (auto delta = inertialDrag.update(t, 60.0f, 5.0f)) {
		onMouseDelta(*delta);
	}

	if (scene3d) {
		updateCamera();
	}
}

std::shared_ptr<const Resource> MeshEditor::loadResource(const Path& assetPath, const String& assetId, AssetType assetType)
{
	if (!scene3d) {
		scene3d = std::make_shared<UIScene3D>("scene3d", projectWindow.getAPI(), project.getGameResources());
		add(scene3d, 1);
	}

	std::shared_ptr<Mesh> mesh;

	const auto assetData = Path::readFile(project.getAssetsSrcPath() / assetPath);
	if (!assetData.empty()) {
		mesh = std::shared_ptr(MeshImporter::parse(assetPath, assetData, *this));
	}
	mesh->setAssetId(assetId);
	mesh->loadDependencies(project.getGameResources());
	std::tie(meshCentre, meshSize) = mesh->getCentreAndSize();

	scene3d->clearRenderers();
	auto renderer = std::make_unique<MeshRenderer>(mesh);
	scene3d->addRenderer(std::move(renderer));

	updateCamera();

	return mesh;
}

void MeshEditor::onTabbedIn()
{
}

Bytes MeshEditor::readAdditionalFile(const Path& filePath)
{
	return Path::readFile(project.getAssetsSrcPath() / filePath);
}

void MeshEditor::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	inertialDrag.stop();
	dragging = true;
}

void MeshEditor::releaseMouse(Vector2f mousePos, int button)
{
	dragging = false;
}

void MeshEditor::onMouseOver(Vector2f mousePos)
{
	if (dragging) {
		inertialDrag.appendAbsolute(mousePos);
	}
}

void MeshEditor::updateCamera()
{
	scene3d->setOrbitCamera(meshCentre, yawAndPitch.x, yawAndPitch.y, meshSize.length() * 1.5f, 45);
}

void MeshEditor::onMouseDelta(Vector2f delta)
{
	yawAndPitch.x = modulo(yawAndPitch.x - delta.x, 360.0f);
	yawAndPitch.y = clamp(yawAndPitch.y + delta.y, -90.0f, 90.0f);
}
