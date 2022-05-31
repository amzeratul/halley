#include "infini_canvas.h"
using namespace Halley;

InfiniCanvas::InfiniCanvas(String id, UIStyle style, UISizer sizer, std::shared_ptr<InputKeyboard> keyboard)
	: BaseCanvas(std::move(id), std::move(style), std::move(sizer), std::move(keyboard))
{
}

void InfiniCanvas::setScrollPosition(Vector2f pos)
{
	scrollPos = pos;
}

Vector2f InfiniCanvas::getScrollPosition() const
{
	return scrollPos;
}

Vector2f InfiniCanvas::getBasePosition() const
{
	return getPosition();
}
