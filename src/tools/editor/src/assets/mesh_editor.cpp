#include "mesh_editor.h"

using namespace Halley;

MeshEditor::MeshEditor(UIFactory& factory, Resources& gameResources, AssetType type, Project& project, MetadataEditor& metadataEditor)
    : AssetEditor(factory, gameResources, project, type)
    , metadataEditor(metadataEditor)
        
{
    setupWindow();
}

void MeshEditor::reload()
{
	loadAssetData();
}

void MeshEditor::refreshAssets()
{
	loadAssetData();
}

void MeshEditor::update(Time t, bool moved)
{
}

std::shared_ptr<const Resource> MeshEditor::loadResource(const String& assetId)
{
	std::shared_ptr<const Resource> resource;

	if (assetType != AssetType::Mesh) {
		return {};
	}

	resource = gameResources.get<Mesh>(assetId);

    return resource;
}

void MeshEditor::setupWindow()
{
    add(factory.makeUI("halley/mesh_editor"), 1);
	meshDisplay = getWidgetAs<MeshEditorDisplay>("display");
}

void MeshEditor::loadAssetData()
{
	meshDisplay->setMesh(std::dynamic_pointer_cast<const Mesh>(resource));
}

MeshEditorDisplay::MeshEditorDisplay(String id, Resources& resources, const HalleyAPI& api)
    : UIWidget(std::move(id))
    , resources(resources)
    , api(api)
{
	surface = std::make_shared<RenderSurface>(*api.video, resources);
}

void MeshEditorDisplay::setMetadataEditor(MetadataEditor& metadataEditor)
{
	this->metadataEditor = &metadataEditor;
}

void MeshEditorDisplay::setMesh(std::shared_ptr<const Mesh> mesh)
{
	meshRenderer = std::make_shared<MeshRenderer>();
    meshRenderer->setMesh(std::move(mesh));
}

void MeshEditorDisplay::update(Time t, bool moved)
{
	curTime += t;
	if (meshRenderer) {
		meshRenderer->update(t);
	}
}

void MeshEditorDisplay::draw(UIPainter& p) const
{
	p.draw([=](Painter& painter) {
		Sprite canvas;
		if (surface->isReady()) {
			canvas = surface->getSurfaceSprite();
		}
		else {
			canvas.setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
		}
		canvas.setPos(getPosition() + Vector2f(1, 1)).setSize(Vector2f(getSize()));
		canvas.draw(painter);
	});
}

void MeshEditorDisplay::render(RenderContext& rc) const
{
	UIWidget::render(rc);

	const float c = std::cos(float(curTime));
	const float s = std::sin(float(curTime));
	const float r = 0.5f;
	const Vector3f p = Vector3f(r * s, 0.5f, r * -c);
	const Vector3f t = Vector3f(0, 0.5f, 0);
	const auto q = Quaternion::lookAt(t - p, Vector3f(0, 1, 0));

	Camera cam;
	cam.setCameraType(CameraType::Perspective).setFieldOfView(Angle1f::fromDegrees(90.0f));
	cam.setPosition(p);
	cam.setRotation(q);

	const auto viewPort = rc.getCamera().getViewPort().value_or(rc.getDefaultRenderTarget().getViewPort()).getSize();
	if (getRect().getWidth() > 0 && getRect().getHeight() > 0) {
		surface->setSize(Vector2i(static_cast<int>(getRect().getWidth()), static_cast<int>(getRect().getHeight())));
	}

	rc.with(surface->getRenderTarget()).with(cam).bind([&](Painter& painter)
	{
		painter.clear(Colour4f(0, 0, 0, 0));
		if (meshRenderer) {
			meshRenderer->render(painter);
		}
	});
}
