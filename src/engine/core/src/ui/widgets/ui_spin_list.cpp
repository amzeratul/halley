#include "halley/ui/widgets/ui_spin_list.h"

#include "halley/input/input_keyboard.h"
#include "halley/ui/ui_style.h"
#include "halley/ui/widgets/ui_image.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/ui/widgets/ui_label.h"

using namespace Halley;

UISpinList::UISpinList(String id, const UIStyle& style, Vector<LocalisedString> os, int defaultOption)
	: UIWidget(std::move(id), {}, UISizer())
	, curOption(defaultOption)
{
	styles.emplace_back(style);

	leftArrow = std::make_shared<UISpinListArrow>(*this, "spinlist_arrow_left_" + getId(), styles[0], true);
	add(leftArrow, 0, {}, UISizerFillFlags::Left);

	label = std::make_shared<UILabel>("spinlist_label_" + getId(), style, LocalisedString::fromUserString(""));
	add(label, 1, {}, UISizerFillFlags::Centre);

	rightArrow = std::make_shared<UISpinListArrow>(*this, "spinlist_arrow_right_" + getId(), styles[0], false);
	add(rightArrow, 0, {}, UISizerFillFlags::Right);

	setOptions(std::move(os));
}

void UISpinList::setSelectedOptionSilent(int option)
{
	int nextOption = option;
	if (option >= static_cast<int>(options.size())) {
		nextOption = 0;
	}
	if (option < 0) {
		nextOption = static_cast<int>(options.size()) - 1;
	}

	if (curOption != nextOption) {
		curOption = nextOption;
		const auto spinSound = styles[0].getString("spinSound");
		if (!spinSound.isEmpty()) {
			playSound(spinSound);
		}

		label->setText(options[curOption]);
		if (getDataBindFormat() == UIDataBind::Format::String) {
			notifyDataBind(optionIds[curOption]);
		}
		else {
			notifyDataBind(curOption);
		}
	}
}

void UISpinList::setSelectedOption(int option)
{
	int nextOption = option;
	if (option >= static_cast<int>(options.size())) {
		nextOption = 0;
	}
	if (option < 0) {
		nextOption = static_cast<int>(options.size()) - 1;
	}
	if (curOption != nextOption) {
		curOption = nextOption;
		sendEvent(UIEvent(UIEventType::DropdownSelectionChanged, getId(), optionIds[curOption], curOption));

		const auto spinSound = styles[0].getString("spinSound");
		if (!spinSound.isEmpty()) {
			playSound(spinSound);
		}

		label->setText(options[curOption]);
		if (getDataBindFormat() == UIDataBind::Format::String) {
			notifyDataBind(optionIds[curOption]);
		}
		else {
			notifyDataBind(curOption);
		}	
	}
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

void UISpinList::setOptions(const Vector<LocalisedString> os, int defaultOption)
{
	setOptions({}, os, defaultOption);
}

void UISpinList::setOptions(Vector<String> oIds, const Vector<LocalisedString>& os, int defaultOption)
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
	label->setText(options[curOption]);

	if (defaultOption != -1) {
		setSelectedOption(defaultOption);
	}
}

void UISpinList::setOptions(const I18N& i18n, const String& i18nPrefix, const Vector<String>& optionIds, int defaultOption)
{
	setOptions(optionIds, i18n.getVector(i18nPrefix, optionIds), defaultOption);
}

void UISpinList::setMinMax(int min, int max)
{
	Vector<LocalisedString> os;
	os.reserve(max - min);
	for (int i = min; i <= max; ++i) {
		os.push_back(LocalisedString::fromUserString(String(toString(i))));
	}

	setOptions(std::move(os));
}

void UISpinList::onManualControlCycleValue(int delta)
{
	setSelectedOption(modulo(curOption + delta, int(options.size())));
}

bool UISpinList::canReceiveFocus() const
{
	return false;
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
		label->setText(options[curOption]);
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

void UISpinList::arrowPressed(bool left)
{
	setSelectedOption(left ? curOption - 1 : curOption + 1);
}


UISpinListArrow::UISpinListArrow(UISpinList& parent, String id, const UIStyle& style, bool left)
    : UIImage(id, left ? style.getSprite("normalLeft") : style.getSprite("normalRight"), UISizer())
    , parent(parent)
    , left(left)
{
	setInteractWithMouse(true);
	styles.emplace_back(style);
}

void UISpinListArrow::update(Time t, bool moved)
{
	const auto& style = styles.at(0);
	auto target = getRect().getTopLeft();
	const auto hoverAnimationLength = style.getFloat("hoverAnimationLength", 0.0f);
	if (isMouseOver() && hoverAnimationLength != 0.0f) {
		if (!wasHovering) {
			wasHovering = true;
			hoverTime = 0.0f;
		}
		hoverTime += static_cast<float>(t);

		const auto offset = abs(sin(hoverTime / hoverAnimationLength)) * style.getFloat("hoverAnimationDistance", 1.0f);
		target = getRect().getTopLeft() + Vector2f((left ? -offset : offset) , 0.0f);
	}

	if (!isMouseOver()) {
		wasHovering = false;
	}

	const auto animationLength = style.getFloat("animationLength", 0.0f);
	if (animationLength != 0.0f && time < animationLength) {
		time += static_cast<float>(t);
		hoverTime = 0.0f;
		const auto step = clamp(time / animationLength, 0.0f, 1.0f);
		const auto offset = 1.0f - pow((step * 2.0f - 1.0f), 2.0f);
		target = getRect().getTopLeft() + Vector2f((left ? -offset : offset) * style.getFloat("animationDistance", 1.0f), 0.0f);
	}


	const String dir = left ? "Left" : "Right";
	auto sprite = isEnabled() ? (isMouseOver() ? style.getSprite("hover" + dir) : style.getSprite("normal" + dir)) : style.getSprite("disabled" + dir);
	sprite.setPosition(target);
	setSprite(sprite);
}

void UISpinListArrow::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	parent.arrowPressed(left);
	time = 0.0f;
}