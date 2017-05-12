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

String UIDropdown::getSelectedOptionText() const
{
	return options[curOption];
}

void UIDropdown::draw(UIPainter& painter) const
{
	int offset = isOpen ? 1 : 0;

	if (isOpen) {
		painter.draw(dropdownSprite, offset);
	}

	painter.draw(sprite, offset);
	painter.draw(label, offset);

	if (isOpen) {
		for (auto& label: optionsLabels) {
			painter.draw(label, offset);
		}
	}
}

void UIDropdown::update(Time t, bool moved)
{
	bool needUpdate = true;
	sprite = isMouseOver() ? style->dropdownHover : style->dropdownNormal;

	if (needUpdate) {
		sprite.setPos(getPosition()).scaleTo(getSize());
		label.setAlignment(0.0f).setPosition(getPosition() + Vector2f(3, 0));
		
		for (size_t i = 0; i < optionsLabels.size(); ++i) {
			optionsLabels[i].setPosition(getPosition() + Vector2f(3.0f, float((i + 1) * 14)));
		}
		dropdownSprite.setPos(getPosition());
	}

	if (!isOpen) {
		optionsLabels.clear();
	}
}

void UIDropdown::onClicked(Vector2f mousePos)
{
	if (isOpen) {
		isOpen = false;
		Vector2f relPos = mousePos - getPosition();
		float entryH = 14.0f;
		int idx = clamp(int(std::floor(relPos.y / entryH)) - 1, 0, int(options.size() - 1));
		setSelectedOption(idx);
	} else {
		isOpen = true;
		optionsLabels.clear();
		optionsExtent = Vector2f(14, 14);
		for (auto& o: options) {
			optionsLabels.push_back(style->inputLabel.clone().setText(o));
			auto ext = optionsLabels.back().getExtents();
			optionsExtent.x = std::max(optionsExtent.x, ext.x + 14);
			optionsExtent.y += 14;
		}
		dropdownSprite = style->dropdownNormal;
		dropdownSprite.setPos(getPosition()).scaleTo(optionsExtent);
	}
}

void UIDropdown::doSetState(State state)
{
}

bool UIDropdown::isFocusLocked() const
{
	return isOpen || UIClickable::isFocusLocked();
}

void UIDropdown::onFocusLost()
{
	isOpen = false;
}

Rect4f UIDropdown::getMouseRect() const
{
	if (isOpen) {
		return Rect4f(getPosition(), getPosition() + optionsExtent);
	} else {
		return UIWidget::getMouseRect();
	}
}
