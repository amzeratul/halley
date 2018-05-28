#include "halley/file_formats/config_file.h"
#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_button.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_image.h"
#include "halley/ui/widgets/ui_animation.h"
#include "halley/ui/widgets/ui_scrollbar.h"
#include "halley/ui/widgets/ui_scroll_pane.h"
#include "halley/ui/widgets/ui_scrollbar_pane.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_slider.h"
#include "halley/support/logger.h"
#include "ui_validator.h"

using namespace Halley;

UIFactory::UIFactory(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet)
	: api(api)
	, resources(resources)
	, i18n(i18n)
	, styleSheet(styleSheet)
{
	addFactory("widget", [=] (const ConfigNode& node) { return makeBaseWidget(node); });
	addFactory("label", [=] (const ConfigNode& node) { return makeLabel(node); });
	addFactory("button", [=] (const ConfigNode& node) { return makeButton(node); });
	addFactory("textInput", [=] (const ConfigNode& node) { return makeTextInput(node); });
	addFactory("list", [=] (const ConfigNode& node) { return makeList(node); });
	addFactory("dropdown", [=] (const ConfigNode& node) { return makeDropdown(node); });
	addFactory("checkbox", [=] (const ConfigNode& node) { return makeCheckbox(node); });
	addFactory("image", [=] (const ConfigNode& node) { return makeImage(node); });
	addFactory("animation", [=] (const ConfigNode& node) { return makeAnimation(node); });
	addFactory("scrollBar", [=] (const ConfigNode& node) { return makeScrollBar(node); });
	addFactory("scrollPane", [=] (const ConfigNode& node) { return makeScrollPane(node); });
	addFactory("scrollBarPane", [=] (const ConfigNode& node) { return makeScrollBarPane(node); });
	addFactory("slider", [=] (const ConfigNode& node) { return makeSlider(node); });
}

void UIFactory::addFactory(const String& key, WidgetFactory factory)
{
	factories[key] = factory;
}

void UIFactory::setConditions(std::vector<String> conds)
{
	conditions = std::move(conds);
}

void UIFactory::clearConditions()
{
	conditions.clear();
}

std::shared_ptr<UIWidget> UIFactory::makeUI(const String& configName)
{
	return makeUIFromNode(resources.get<ConfigFile>(configName)->getRoot());
}

std::shared_ptr<UIWidget> UIFactory::makeUIFromNode(const ConfigNode& node)
{
	return makeWidget(node);
}

void UIFactory::setInputButtons(const String& key, UIInputButtons buttons)
{
	inputButtons[key] = buttons;
}

UIStyle UIFactory::getStyle(const String& name) const
{
	return UIStyle(name, styleSheet);
}

std::shared_ptr<UIStyleSheet> UIFactory::getStyleSheet() const
{
	return styleSheet;
}

std::shared_ptr<UIWidget> UIFactory::makeWidget(const ConfigNode& entryNode)
{
	auto& widgetNode = entryNode["widget"];
	auto widgetClass = widgetNode["class"].asString();
	auto iter = factories.find(widgetClass);
	if (iter == factories.end()) {
		throw Exception("Unknown widget class: " + widgetClass);
	}
	
	auto widget = iter->second(entryNode);
	if (widgetNode.hasKey("size")) {
		widget->setMinSize(asVector2f(widgetNode["size"], {}));
	}
	return widget;
}

Maybe<UISizer> UIFactory::makeSizer(const ConfigNode& entryNode)
{
	const bool hasSizer = entryNode.hasKey("sizer");
	const bool hasChildren = entryNode.hasKey("children");
	if (!hasSizer && !hasChildren) {
		return {};
	}

	UISizer sizer;
	
	if (hasSizer) {
		auto& sizerNode = entryNode["sizer"];
		auto sizerType = fromString<UISizerType>(sizerNode["type"].asString("horizontal"));
		float gap = sizerNode["gap"].asFloat(1.0f);
		int nColumns = sizerNode["columns"].asInt(1);
		
		sizer = UISizer(sizerType, gap, nColumns);
	}

	loadSizerChildren(sizer, entryNode["children"]);

	return std::move(sizer);
}

UISizer UIFactory::makeSizerOrDefault(const ConfigNode& entryNode, UISizer&& defaultSizer)
{
	auto sizer = makeSizer(entryNode);
	if (sizer) {
		return std::move(sizer.get());
	} else {
		return std::move(defaultSizer);
	}
}

std::shared_ptr<UISizer> UIFactory::makeSizerPtr(const ConfigNode& entryNode)
{
	auto sizer = makeSizer(entryNode);
	if (sizer) {
		return std::make_shared<UISizer>(std::move(sizer.get()));
	} else {
		return {};
	}
}

void UIFactory::loadSizerChildren(UISizer& sizer, const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node.asSequence()) {
			float proportion = childNode["proportion"].asFloat(0);
			Vector4f border = asVector4f(childNode["border"], Vector4f());
			int fill = 0;

			auto addFill = [&] (const String& fillName)
			{
				if (fillName == "fill") {
					fill |= UISizerFillFlags::Fill;
				} else if (fillName == "fillHorizontal") {
					fill |= UISizerFillFlags::FillHorizontal;
				} else if (fillName == "fillVertical") {
					fill |= UISizerFillFlags::FillVertical;
				} else if (fillName == "centre") {
					fill |= UISizerAlignFlags::Centre;
				} else if (fillName == "left") {
					fill |= UISizerAlignFlags::Left;
				} else if (fillName == "right") {
					fill |= UISizerAlignFlags::Right;
				} else if (fillName == "top") {
					fill |= UISizerAlignFlags::Top;
				} else if (fillName == "bottom") {
					fill |= UISizerAlignFlags::Bottom;
				} else if (fillName == "centreHorizontal") {
					fill |= UISizerAlignFlags::CentreHorizontal;
				} else if (fillName == "centreVertical") {
					fill |= UISizerAlignFlags::CentreVertical;
				}
			};

			if (childNode["fill"].getType() == ConfigNodeType::String) {
				addFill(childNode["fill"].asString());
			} else if (childNode["fill"].getType() == ConfigNodeType::Sequence) {
				for (auto& fillNode: childNode["fill"].asSequence()) {
					addFill(fillNode.asString());
				}
			} else {
				fill = UISizerFillFlags::Fill;
			}

			if (childNode.hasKey("widget")) {
				sizer.add(makeWidget(childNode), proportion, border, fill);
			} else if (childNode.hasKey("sizer") || childNode.hasKey("children")) {
				sizer.add(makeSizerPtr(childNode), proportion, border, fill);
			} else if (childNode.hasKey("spacer")) {
				sizer.addSpacer(proportion);
			} else if (childNode.hasKey("stretchSpacer")) {
				sizer.addStretchSpacer(proportion);
			}
		}
	}
}

void UIFactory::applyInputButtons(UIWidget& widget, const String& key)
{
	if (!key.isEmpty()) {
		auto iter = inputButtons.find(key);
		if (iter == inputButtons.end()) {
			Logger::logWarning("Input buttons binding not found: \"" + key + "\".");
		} else {
			widget.setInputButtons(iter->second);
		}
	}
}

Maybe<Vector2f> UIFactory::asMaybeVector2f(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		auto seq = node.asSequence();
		return Vector2f(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else {
		return {};
	}
}

Vector2f UIFactory::asVector2f(const ConfigNode& node, Maybe<Vector2f> defaultValue)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		auto seq = node.asSequence();
		return Vector2f(seq.at(0).asFloat(), seq.at(1).asFloat());
	} else if (defaultValue) {
		return defaultValue.get();
	} else {
		throw Exception("Unable to parse node as Vector2f.");
	}
}

LocalisedString UIFactory::parseLabel(const ConfigNode& node) {
	LocalisedString label;
	if (node.hasKey("textKey")) {
		label = i18n.get(node["textKey"].asString());
	} else if (node.hasKey("text")) {
		label = LocalisedString::fromUserString(node["text"].asString());
	}
	return label;
}

Vector4f UIFactory::asVector4f(const ConfigNode& node, Maybe<Vector4f> defaultValue)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		auto seq = node.asSequence();
		return Vector4f(seq.at(0).asFloat(), seq.at(1).asFloat(), seq.at(2).asFloat(), seq.at(3).asFloat());
	} else if (defaultValue) {
		return defaultValue.get();
	} else {
		throw Exception("Unable to parse node as Vector2f.");
	}
}

std::shared_ptr<UIWidget> UIFactory::makeBaseWidget(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
	auto minSize = asVector2f(node["minSize"], Vector2f(0, 0));
	auto innerBorder = asVector4f(node["innerBorder"], Vector4f(0, 0, 0, 0));
	return std::make_shared<UIWidget>(id, minSize, makeSizer(entryNode), innerBorder);
}

std::shared_ptr<UIWidget> UIFactory::makeLabel(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto style = UIStyle(node["style"].asString("label"), styleSheet);
	return std::make_shared<UILabel>(style.getTextRenderer("label"), parseLabel(node));
}

std::shared_ptr<UIWidget> UIFactory::makeButton(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("button"), styleSheet);
	auto label = parseLabel(node);

	auto sizer = makeSizerOrDefault(entryNode, UISizer());
	if (!label.getString().isEmpty()) {
		sizer.add(std::make_shared<UILabel>(style.getTextRenderer("label"), label), 1, Vector4f(), UISizerAlignFlags::Centre);
	}

	return std::make_shared<UIButton>(id, style, std::move(sizer));
}

std::shared_ptr<UIWidget> UIFactory::makeTextInput(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("input"), styleSheet);
	auto label = parseLabel(node);

	auto result = std::make_shared<UITextInput>(api.input->getKeyboard(), id, style, "", label);;

	auto validatorName = node["validator"].asString("");
	if (!validatorName.isEmpty()) {
		if (validatorName == "numeric") {
			result->setValidator(std::make_shared<UINumericValidator>(true));
		} else if (validatorName == "numericPositive") {
		result->setValidator(std::make_shared<UINumericValidator>(false));
		} else {
			throw Exception("Unknown text input validator: " + validatorName);
		}
	}

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("list"), styleSheet);
	auto label = parseLabel(node);

	auto orientation = fromString<UISizerType>(node["type"].asString("vertical"));
	int nColumns = node["columns"].asInt(1);

	auto widget = std::make_shared<UIList>(id, style, orientation, nColumns);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	return widget;
}

std::shared_ptr<UIWidget> UIFactory::makeDropdown(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("dropdown"), styleSheet);
	auto scrollStyle = UIStyle(node["ScrollBarStyle"].asString("scrollbar"), styleSheet);
	auto listStyle = UIStyle(node["listStyle"].asString("list"), styleSheet);
	auto label = parseLabel(node);

	// TODO?
	std::vector<LocalisedString> options;
	int defaultOption = 0;

	auto widget = std::make_shared<UIDropdown>(id, style, scrollStyle, listStyle, options, defaultOption);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	return widget;
}

std::shared_ptr<UIWidget> UIFactory::makeCheckbox(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("checkbox"), styleSheet);
	auto checked = node["checked"].asBool(false);

	return std::make_shared<UICheckbox>(id, style, checked);
}

std::shared_ptr<UIWidget> UIFactory::makeImage(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto imageName = node["image"].asString();
	auto materialName = node["material"].asString("");
	auto sprite = Sprite().setImage(resources, imageName, materialName);
	Vector4f innerBorder = asVector4f(node["innerBorder"], Vector4f());

	return std::make_shared<UIImage>(sprite, makeSizer(entryNode), innerBorder);
}

std::shared_ptr<UIWidget> UIFactory::makeAnimation(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto size = asVector2f(node["size"], Vector2f());
	auto animationOffset = asVector2f(node["offset"], Vector2f());
	auto animationName = node["animation"].asString();
	auto sequence = node["sequence"].asString("default");
	auto direction = node["direction"].asString("default");

	auto animation = AnimationPlayer(resources.get<Animation>(animationName), sequence, direction);

	return std::make_shared<UIAnimation>(id, size, animationOffset, animation);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollPane(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto clipSize = asVector2f(node["clipSize"], Vector2f());
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);

	return std::make_shared<UIScrollPane>(clipSize, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollBar(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto style = UIStyle(node["style"].asString("scrollbar"), styleSheet);
	auto scrollDirection = fromString<UIScrollDirection>(node["scrollDirection"].asString("vertical"));
	auto alwaysShow = !node["autoHide"].asBool(false);

	return std::make_shared<UIScrollBar>(scrollDirection, style, alwaysShow);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollBarPane(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto clipSize = asVector2f(node["clipSize"], Vector2f());
	auto style = UIStyle(node["style"].asString("scrollbar"), styleSheet);
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);
	auto alwaysShow = !node["autoHide"].asBool(false);

	return std::make_shared<UIScrollBarPane>(clipSize, style, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical, alwaysShow);
}

std::shared_ptr<UIWidget> UIFactory::makeSlider(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("slider"), styleSheet);
	auto minValue = node["minValue"].asFloat(0);
	auto maxValue = node["maxValue"].asFloat(1);
	auto value = node["value"].asFloat(0.5f);

	return std::make_shared<UISlider>(id, style, minValue, maxValue, value);
}

bool UIFactory::hasCondition(const String& condition) const
{
	return std::find(conditions.begin(), conditions.end(), condition) != conditions.end();
}

bool UIFactory::resolveConditions(const ConfigNode& node) const
{
	auto resolveCondition = [&] (const String& cond) -> bool
	{
		if (cond.startsWith("!")) {
			return !hasCondition(cond.mid(1));
		} else {
			return hasCondition(cond);
		}
	};

	if (node.getType() == ConfigNodeType::Sequence) {
		bool ok = true;
		for (auto& c: node.asSequence()) {
			ok &= hasCondition(c.asString());
		}
		return ok;
	} else {
		return resolveCondition(node.asString());
	}
}
