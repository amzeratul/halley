#include "../gen/cpp/systems/render_system.h"

using namespace Halley;

void RenderSystem::render(Halley::Painter& painter) const
{
	Camera& cam = painter.getCurrentCamera();
	Rect4i viewPort = painter.getViewPort();

	static SpritePainter spritePainter;

	Vector2f size = Vector2f(viewPort.getSize()) / cam.getZoom();
	assert(cam.getAngle().getRadians() == 0); // Camera rotation not accounted by following line
	Rect4f worldView(cam.getPosition() - size * 0.5f, size);

	spritePainter.start(mainFamily.count());
	for (auto& e : mainFamily) {
		auto& sprite = e.sprite->sprite;
		if (sprite.isInView(worldView)) {
			spritePainter.add(sprite, e.sprite->layer, sprite.getPosition().y - worldView.getY());
		}
	}

	spritePainter.draw(painter);
}
