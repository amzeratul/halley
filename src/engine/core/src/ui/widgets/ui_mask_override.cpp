#include "halley/ui/widgets/ui_mask_override.h"

using namespace Halley;

UIMaskOverride::UIMaskOverride(String id, Vector2f minSize, std::optional<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(std::move(id), minSize, std::move(sizer), innerBorder)
{}

void UIMaskOverride::setMaskOverride(std::optional<int> mask)
{
	this->mask = mask;
}

void UIMaskOverride::drawChildren(UIPainter& painter) const
{
	if (mask) {
		auto p2 = painter.withMask(*mask);
		UIWidget::drawChildren(p2);
	} else {
		UIWidget::drawChildren(painter);
	}
}
