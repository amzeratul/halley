#include "systems/render_system.h"

using namespace Halley;

class RenderSystem final : public RenderSystemBase<RenderSystem> {
public:
	void render(RenderContext& rc) const
	{
		static SpritePainter spritePainter;

		spritePainter.start(mainFamily.count());
		for (auto& e : mainFamily) {
			auto& sprite = e.sprite.sprite;
			sprite.setPos(e.position.position);
			spritePainter.add(sprite, 1, e.sprite.layer, sprite.getPosition().y);
		}

		rc.bind([&] (Painter& painter)
		{
			painter.clear(Colour());
			spritePainter.draw(1, painter);
			painter.flush();
		});
	}
};

REGISTER_SYSTEM(RenderSystem)
