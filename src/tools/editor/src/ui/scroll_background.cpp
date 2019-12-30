#include "scroll_background.h"
using namespace Halley;

ScrollBackground::ScrollBackground(String id, Resources& res, UISizer sizer)
	: UIWidget(std::move(id), {}, std::move(sizer))
{
	bg = Sprite()
		.setImage(res, "checkered.png")
		.setColour(Colour4f::fromString("#111111"));
}

void ScrollBackground::update(Time t, bool moved)
{
	if (moved) {
		bg
			.setPos(getPosition())
			.setSize(getSize())
			.setTexRect(Rect4f(Vector2f(), getSize() / Vector2f(16, 16)));
	}
}

void ScrollBackground::draw(UIPainter& painter) const
{
	painter.draw(bg);
}
