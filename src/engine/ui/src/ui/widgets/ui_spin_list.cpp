#include "halley/ui/widgets/ui_spin_list.h"
#include "ui_style.h"
#include "widgets/ui_image.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"
#include "widgets/ui_label.h"
#include "halley/support/logger.h"

using namespace Halley;

UISpinList::UISpinList(String id, const UIStyle& style, std::vector<LocalisedString> os, int defaultOption)
	: UIWidget(std::move(id), {})
	, style(style)
	, curOption(defaultOption)
{
	sprite = style.getSprite("normal");

	setOptions(std::move(os));
}

void UISpinList::setSelectedOption(int option)
{
	int nextOption = clamp(option, 0, int(options.size()) - 1);
	if (curOption != nextOption) {
		curOption = nextOption;
		sendEvent(UIEvent(UIEventType::DropboxSelectionChanged, getId(), optionIds[curOption], curOption));

		if (getDataBindFormat() == UIDataBind::Format::String) {
			notifyDataBind(optionIds[curOption]);
		}
		else {
			notifyDataBind(curOption);
		}	
	}

	updateLabelPositions();
}

void UISpinList::setSelectedOption(const String& id)
{
	auto iter = std::find(optionIds.begin(), optionIds.end(), id);
	if (iter != optionIds.end()) {
		setSelectedOption(int(iter - optionIds.begin()));
	}
}

int UISpinList::getSelectedOption() const
{
	return curOption;
}

String UISpinList::getSelectedOptionId() const
{
	return curOption >= 0 && curOption < int(optionIds.size()) ? optionIds[curOption] : "";
}

LocalisedString UISpinList::getSelectedOptionText() const
{
	return options[curOption];
}

void UISpinList::setInputButtons(const UIInputButtons& buttons)
{
	inputButtons = buttons;
}

void UISpinList::setOptions(const std::vector<LocalisedString> os, int defaultOption)
{
	setOptions({}, os, defaultOption);
}

void UISpinList::updateLabelPositions() {
	if (!spinner || int(spinner->getChildren().size()) == 0) {
		return;
	}

	for (auto& child : spinner->getChildren()) {
		child->setMinSize(getSize());
	}

	auto currentLabel = spinner->getChildren().at(curOption);
	auto firstLabel = spinner->getChildren().at(0);
	auto offset = Vector2f(currentLabel->getSize().x * 0.5f - std::dynamic_pointer_cast<UILabel>(currentLabel)->getTextRenderer().getExtents().x * 0.5f, 0);
	spinner->setPosition(getPosition() - (currentLabel->getPosition() - firstLabel->getPosition()) + offset);

	for (auto& c : spinner->getChildren()) {
		auto label = std::dynamic_pointer_cast<UILabel>(c);
		const auto offsetFromStart = currentLabel->getPosition() - c->getPosition();
		std::dynamic_pointer_cast<UILabel>(c)->getTextRenderer().setClip(Rect4f(offsetFromStart, c->getSize().x, c->getSize().y));
	}

	spinner->layout();
}

void UISpinList::updateOptionLabels() {
	auto label = style.getTextRenderer("label").clone();
	float maxExtents = 0;
	for (auto& o : options) {
		maxExtents = std::max(maxExtents, label.clone().setText(o).getExtents().x);
	}

	auto minSize = Vector2f(maxExtents + 19, 14); // HACK
	setMinSize(std::max(getMinimumSize(), minSize));

	if (spinner) {
		spinner->destroy();
	}
	spinner = std::make_shared<UIWidget>("spinner_" + getId(), Vector2f(), UISizer(UISizerType::Horizontal, 0));

	auto i = 0;
	for (auto& o : options) {
		auto optionLabel = std::make_shared<UILabel>("spinner_option_" + optionIds[i], style.getTextRenderer("label"), o);
		spinner->add(optionLabel);
		++i;
	}

	addChild(spinner);
}

void UISpinList::setOptions(std::vector<String> oIds, const std::vector<LocalisedString>& os, int defaultOption)
{
	if (oIds.empty()) {
		oIds.resize(os.size());
		for (size_t i = 0; i < oIds.size(); ++i) {
			oIds[i] = toString(i);
		}
	}

	if (oIds.size() != os.size()) {
		throw Exception("Size mismatch between options and option ids", HalleyExceptions::UI);
	}

	options = os;
	optionIds = oIds;

	if (options.empty()) {
		options.emplace_back();
		optionIds.emplace_back();
	}
	curOption = clamp(curOption, 0, int(options.size() - 1));
	updateOptionLabels();

	if (defaultOption != -1) {
		setSelectedOption(defaultOption);
	}
}

void UISpinList::setOptions(const I18N& i18n, const String& i18nPrefix, const std::vector<String>& optionIds, int defaultOption)
{
	setOptions(optionIds, i18n.getVector(i18nPrefix, optionIds), defaultOption);
}

void UISpinList::onManualControlCycleValue(int delta)
{
	setSelectedOption(modulo(curOption + delta, int(options.size())));
}

void UISpinList::onManualControlActivate()
{
	getRoot()->setFocus(shared_from_this());
}

void UISpinList::draw(UIPainter& painter) const
{
	painter.draw(sprite);
}

void UISpinList::update(Time t, bool moved)
{
	bool optionsUpdated = false;
	for (auto& o : options) {
		if (o.checkForUpdates()) {
			optionsUpdated = true;
		}
	}
	if (optionsUpdated) {
		updateOptionLabels();
	}

	sprite = isEnabled() ? (isMouseOver() ? style.getSprite("hover") : style.getSprite("normal")) : style.getSprite("disabled");
	sprite.setPos(getPosition()).scaleTo(getSize());

	if (spinner && int(spinner->getChildren().size()) > 0) {
		updateLabelPositions();
	}
}

void UISpinList::readFromDataBind()
{
	auto data = getDataBind();
	if (data->getFormat() == UIDataBind::Format::String) {
		setSelectedOption(data->getStringData());
	}
	else {
		setSelectedOption(data->getIntData());
	}
}

void UISpinList::drawChildren(UIPainter& painter) const
{
	auto p = painter.withAdjustedLayer(1);
	UIWidget::drawChildren(p);
}
