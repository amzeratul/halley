#include "halley/core/ui/widgets/ui_dropdown.h"
#include "ui/ui_style.h"

using namespace Halley;

UIDropdown::UIDropdown(String id, std::shared_ptr<UIStyle> style, const std::vector<String>& os, int defaultOption)
	: UIWidget(id, {})
	, style(style)
	, options(os)
	, curOption(defaultOption)
{
	sprite = style->dropdownNormal;
	if (options.empty()) {
		options.push_back("");
	}
	curOption = clamp(curOption, 0, int(options.size() - 1));
	label = style->inputLabel.clone().setText(options[defaultOption]);

	float maxExtents = 0;
	for (auto& o: options) {
		maxExtents = std::max(maxExtents, label.clone().setText(o).getExtents().x);
	}

	setMinSize(Vector2f(maxExtents + 14, 14));
}

bool UIDropdown::isFocusable() const
{
	return true;
}

void UIDropdown::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	painter.draw(label);
}

void UIDropdown::update(Time t, bool moved)
{
	bool needUpdate = true;
	sprite = isMouseOver() ? style->dropdownHover : style->dropdownNormal;

	if (needUpdate) {
		sprite.setPos(getPosition()).scaleTo(getSize());
		label.setAlignment(0.0f).setPosition(getPosition() + Vector2f(3, 0));
	}
}
