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

	curTime += t;

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

void MeshEditor::updateCamera()
{
	const float distance = meshSize.length() * 1.5f;

	const auto yaw = Angle1f::fromRadians(static_cast<float>(curTime));
	const auto pitch = Angle1f::fromDegrees(30.0f);

	const Vector3f camPos = meshCentre + Vector3f(distance * yaw.sin(), 0, distance * -yaw.cos()) * pitch.cos() + Vector3f(0, distance * pitch.sin(), 0);
	const Vector3f lookPos = meshCentre;
	const auto rot = Quaternion::lookAt(lookPos - camPos, Vector3f(0, 1, 0));

	const auto cam = Camera()
		.setPosition(camPos)
		.setRotation(rot)
		.setCameraType(CameraType::Perspective)
		.setFieldOfView(Angle1f::fromDegrees(45.0f))
		.setClippingPlanes(distance * 0.002f, distance * 2.0f);

	scene3d->setCamera(cam);
}
