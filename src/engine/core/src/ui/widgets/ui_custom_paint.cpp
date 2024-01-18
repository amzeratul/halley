#include "halley/ui/widgets/ui_custom_paint.h"

using namespace Halley;

UICustomPaint::UICustomPaint(String id, std::optional<UISizer> sizer, DrawCallback callback)
	: UIWidget(id, {}, std::move(sizer))
	, callback(std::move(callback))
{
}

void UICustomPaint::setCallback(DrawCallback callback)
{
	this->callback = std::move(callback);
}

void UICustomPaint::draw(UIPainter& painter) const
{
	if (callback) {
		callback(painter, getRect());
	}
}
