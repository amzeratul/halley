#include "../gen/cpp/systems/render_system.h"

void RenderSystem::render(Halley::Painter& painter, MainFamily& e) const
{
	e.sprite->sprite.draw(painter);
}
