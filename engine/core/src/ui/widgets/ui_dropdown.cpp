#include "halley/core/ui/widgets/ui_dropdown.h"
#include "ui/ui_style.h"

using namespace Halley;

UIDropdown::UIDropdown(String id, std::shared_ptr<UIStyle> style, const std::vector<String>& os, int defaultOption)
	: UIClickable(id, {})
	, style(style)
	, options(os)
	, curOption(defaultOption)
{
	sprite = style->getSprite("dropdown.normal");
	if (options.empty()) {
		options.push_back("");
	}
	curOption = clamp(curOption, 0, int(options.size() - 1));
	label = style->getTextRenderer("input.label").clone().setText(options[defaultOption]);

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

void UIDropdown::setSelectedOption(const String& option)
{
	auto result = std::find(options.begin(), options.end(), option);
	if (result != options.end()) {
		setSelectedOption(int(result - options.begin()));
	} else {
		setSelectedOption(0);
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
	painter.draw(sprite);
	painter.draw(label);
}

void UIDropdown::update(Time t, bool moved)
{
	if (isOpen) {
		auto focus = getRoot()->getCurrentFocus();
		if (!focus || (focus != this && !focus->isDescendentOf(*this))) {
			close();
		}
	}

	bool needUpdate = true;
	sprite = isEnabled() ? (isMouseOver() ? style->getSprite("dropdown.hover") : style->getSprite("dropdown.normal")) : style->getSprite("dropdown.disabled");

	if (needUpdate) {
		sprite.setPos(getPosition()).scaleTo(getSize());
		label.setAlignment(0.0f).setPosition(getPosition() + Vector2f(3, 0));

		if (dropdown) {
			dropdown->setPosition(getPosition() + Vector2f(0.0f, getSize().y));
		}
	}
}

void UIDropdown::onClicked(Vector2f mousePos)
{
	if (isOpen) {
		close();
	} else {
		open();
	}
}

void UIDropdown::doSetState(State state)
{
}

bool UIDropdown::isFocusLocked() const
{
	return isOpen || UIClickable::isFocusLocked();
}

void UIDropdown::open()
{
	isOpen = true;
		
	dropdown = std::make_shared<UIList>(getId() + "_list", style);
	dropdown->setMinSize(Vector2f(getSize().x, 1));
	int i = 0;
	for (auto& o: options) {
		dropdown->addTextItem("option_" + toString(i++), o);
	}
	addChild(dropdown);

	dropdown->getEventHandler().setHandle(UIEventType::ListSelectionChanged, [=] (const UIEvent& event)
	{
		
	});
}

void UIDropdown::close()
{
	isOpen = false;

	dropdown->destroy();
	dropdown.reset();
}

void UIDropdown::drawChildren(UIPainter& painter) const
{
	auto p = painter.withAdjustedLayer(1);
	UIWidget::drawChildren(p);
}
