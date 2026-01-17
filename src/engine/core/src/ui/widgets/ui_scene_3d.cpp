#include "halley/ui/widgets/ui_scene_3d.h"

using namespace Halley;

UIScene3D::UIScene3D(String id, Resources& resources)
	: UIWidget(std::move(id))
	, resources(resources)
{
	bg = Sprite().setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
}

void UIScene3D::update(Time t, bool moved)
{
	bg
		.setPos(getPosition())
		.scaleTo(getSize());
}

bool UIScene3D::hasRender() const
{
	return true;
}

void UIScene3D::draw(UIPainter& painter) const
{
	painter.draw(bg);
}

void UIScene3D::render(RenderContext& rc) const
{
	// TODO
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
	bg.setColour(colour);
}

Colour4f UIScene3D::getBGColour() const
{
	return bg.getColour();
}
