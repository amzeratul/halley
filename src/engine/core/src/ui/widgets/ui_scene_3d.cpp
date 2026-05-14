#include "halley/ui/widgets/ui_scene_3d.h"

#include "halley/api/halley_api.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_surface.h"
#include "halley/graphics/render_target/render_target_texture.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/graphics/mesh/mesh.h"

using namespace Halley;

UIScene3D::UIScene3D(String id, const HalleyAPI& api, Resources& resources, Colour4f bgCol)
	: UIWidget(std::move(id))
	, resources(resources)
	, bgCol(bgCol)
{
	renderSurface = std::make_unique<RenderSurface>(*api.video, resources);
}

void UIScene3D::update(Time t, bool moved)
{
	for (auto& renderer: renderers) {
		renderer->update(t);
	}
}

bool UIScene3D::hasRender() const
{
	return true;
}

void UIScene3D::draw(UIPainter& painter) const
{
	auto sharedThis = shared_from_this();
	painter.draw([sharedThis](Painter& painter)
	{
		std::dynamic_pointer_cast<const UIScene3D>(sharedThis)->drawOnPainter(painter);
	});
}

void UIScene3D::drawOnPainter(Painter& painter) const
{
	if (renderSurface->isReady()) {
		Sprite sprite = (material ? renderSurface->getSurfaceSprite(material) : renderSurface->getSurfaceSprite()).clone(false);
		sprite
			.setPosition(getPosition())
			.setScale(1.0f / renderScale)
			.draw(painter);
	}
}

void UIScene3D::render(RenderContext& rc) const
{
	renderSurface->setSize(Vector2i(getSize() * renderScale));

	if (renderSurface->isReady()) {
		rc.with(camera).with(renderSurface->getRenderTarget()).bind([&](Painter& painter) {
			painter.clear(bgCol);
			for (auto& r : renderers) {
				r->render(painter);
			}
		});
	}
}

std::shared_ptr<const Mesh> UIScene3D::loadMesh(const String& meshId)
{
	clearRenderers();
	if (auto mesh = resources.tryGet<Mesh>(meshId)) {
	    auto meshRenderer = std::make_unique<MeshRenderer>(mesh);
		addRenderer(std::move(meshRenderer));

		const auto [centre, size] = mesh->getCentreAndSize();
		setOrbitCamera(centre, 0, 45, size.length() * 1.5f);

		return mesh;
	} else {
		Logger::logError("Mesh not found: " + meshId);
		return {};
	}
}

void UIScene3D::addRenderer(std::unique_ptr<MeshRenderer> renderer)
{
	renderers += std::move(renderer);
}

void UIScene3D::clearRenderers()
{
	renderers.clear();
}

void UIScene3D::setCamera(Camera camera)
{
	this->camera = std::move(camera);
}

Camera& UIScene3D::getCamera()
{
	return camera;
}

const Camera& UIScene3D::getCamera() const
{
	return camera;
}

void UIScene3D::setOrbitCamera(Vector3f centre, float yawDegrees, float pitchDegrees, float distance, float fov)
{
	const auto yaw = Angle1f::fromDegrees(yawDegrees);
	const auto pitch = Angle1f::fromDegrees(pitchDegrees);

	const auto rot = Quaternion(Vector3f(0, 1, 0), -yaw) * Quaternion(Vector3f(1, 0, 0), pitch);
	const Vector3f camDir = rot * Vector3f(0, 0, -1);
	const Vector3f lookPos = centre;
	const Vector3f camPos = lookPos + distance * camDir;

	const auto cam = Camera()
		.setPosition(camPos)
		.setRotation(rot)
		.setCameraType(CameraType::Perspective)
		.setFieldOfView(Angle1f::fromDegrees(fov))
		.setClippingPlanes(distance * 0.002f, distance * 2.0f);

	setCamera(cam);
}

void UIScene3D::setBGColour(Colour4f colour)
{
	bgCol = colour;
}

Colour4f UIScene3D::getBGColour() const
{
	return bgCol;
}

void UIScene3D::setMaterial(const String& material)
{
    this->material = resources.get<MaterialDefinition>(material)->createMaterial();
}

Sprite UIScene3D::getSurfaceSprite() const
{
	if (material) {
		return renderSurface->getSurfaceSprite(material);
	}
	return renderSurface->getSurfaceSprite();
}

void UIScene3D::setRenderScale(float scale)
{
	renderScale = scale;
}

float UIScene3D::getRenderScale() const
{
	return renderScale;
}

std::shared_ptr<Halley::Material> UIScene3D::getMaterial()
{
	return material;
}
