#include "halley/core/ui/widgets/ui_dropdown.h"
#include "ui/ui_style.h"

using namespace Halley;

UIDropdown::UIDropdown(String id, std::shared_ptr<UIStyle> style, const std::vector<String>& os, int defaultOption)
	: UIClickable(id, {})
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

void UIDropdown::setSelectedOption(int option)
{
	int nextOption = clamp(option, 0, int(options.size()));
	if (curOption != nextOption) {
		curOption = nextOption;
		label.setText(options[curOption]);
		sendEvent(UIEvent(UIEventType::DropboxSelectionChanged, getId(), options[curOption]));
	}
}

int UIDropdown::getSelectedOption() const
{
	return curOption;
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

void UIDropdown::onClicked()
{
	setSelectedOption((curOption + 1) % options.size());
}

void UIDropdown::doSetState(State state)
{
}

bool UIDropdown::isFocusLocked() const
{
	return isOpen || UIClickable::isFocusLocked();
}
