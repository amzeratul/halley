#include "halley/ui/widgets/ui_scene_3d.h"

#include "halley/api/halley_api.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_surface.h"
#include "halley/graphics/render_target/render_target_texture.h"

using namespace Halley;

UIScene3D::UIScene3D(String id, const HalleyAPI& api, Resources& resources)
	: UIWidget(std::move(id))
	, resources(resources)
{
	renderSurface = std::make_unique<RenderSurface>(*api.video, resources);
	bgCol = Colour4f(0.1f, 0.1f, 0.1f);
}

void UIScene3D::update(Time t, bool moved)
{
	curTime += t;

	{
		const float c = std::cos(float(curTime));
		const float s = std::sin(float(curTime));
		const float r = 600;
		const Vector3f p = Vector3f(r * s, r * -c, 100.0f);
		const Vector3f t = Vector3f(0, 0, 50);
		const auto q = Quaternion::lookAt(t - p, Vector3f(0, 0, 1));

		Camera cam;
		cam.setPosition(p).setRotation(q).setCameraType(CameraType::Perspective).setFieldOfView(Angle1f::fromDegrees(75.0f));
		setCamera(cam);
	}

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
		auto sprite = renderSurface->getSurfaceSprite().clone()
			.setPosition(getPosition());

		sprite.draw(painter);
	}
}

void UIScene3D::render(RenderContext& rc) const
{
	renderSurface->setSize(Vector2i(getSize()));

	if (renderSurface->isReady()) {
		rc.with(camera).with(renderSurface->getRenderTarget()).bind([&](Painter& painter) {
			painter.clear(bgCol);
			for (auto& r : renderers) {
				r->render(painter);
			}
		});
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

void UIScene3D::setBGColour(Colour4f colour)
{
	bgCol = colour;
}

Colour4f UIScene3D::getBGColour() const
{
	return bgCol;
}
