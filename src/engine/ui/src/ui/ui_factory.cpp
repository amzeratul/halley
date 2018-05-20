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
#include "halley/support/logger.h"

using namespace Halley;

UIFactory::UIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet)
	: api(api)
	, resources(resources)
	, i18n(i18n)
	, styleSheet(styleSheet)
{
	factories["widget"] = [=] (const ConfigNode& node) { return makeBaseWidget(node); };
	factories["label"] = [=] (const ConfigNode& node) { return makeLabel(node); };
	factories["button"] = [=] (const ConfigNode& node) { return makeButton(node); };
	factories["textInput"] = [=] (const ConfigNode& node) { return makeTextInput(node); };
	factories["list"] = [=] (const ConfigNode& node) { return makeList(node); };
	factories["dropdown"] = [=] (const ConfigNode& node) { return makeDropdown(node); };
	factories["checkbox"] = [=] (const ConfigNode& node) { return makeCheckbox(node); };
	factories["image"] = [=] (const ConfigNode& node) { return makeImage(node); };
	factories["animation"] = [=] (const ConfigNode& node) { return makeAnimation(node); };
	factories["scrollBar"] = [=] (const ConfigNode& node) { return makeScrollBar(node); };
	factories["scrollPane"] = [=] (const ConfigNode& node) { return makeScrollPane(node); };
	factories["scrollBarPane"] = [=] (const ConfigNode& node) { return makeScrollBarPane(node); };
}

std::shared_ptr<UIWidget> UIFactory::makeUI(const String& configName)
{
	return makeUIFromNode(resources.get<ConfigFile>(configName)->getRoot());
}

std::shared_ptr<UIWidget> UIFactory::makeUIFromNode(const ConfigNode& node)
{
	return makeWidget(node["widget"]);
}

void UIFactory::setInputButtons(const String& key, UIInputButtons buttons)
{
	inputButtons[key] = buttons;
}

std::shared_ptr<UIWidget> UIFactory::makeWidget(const ConfigNode& node)
{
	auto widgetClass = node["class"].asString();
	auto iter = factories.find(widgetClass);
	if (iter == factories.end()) {
		throw Exception("Unknown widget class: " + widgetClass);
	}
	
	auto widget = iter->second(node);
	if (node.hasKey("size")) {
		widget->setMinSize(asVector2f(node["size"], {}));
	}
	return widget;
}

Maybe<UISizer> UIFactory::makeSizer(const ConfigNode& node)
{
	auto sizerNode = node["sizer"];
	if (sizerNode.getType() == ConfigNodeType::Undefined) {
		return {};
	} else {
		auto sizerType = fromString<UISizerType>(sizerNode["type"].asString("horizontal"));
		float gap = sizerNode["gap"].asFloat(1.0f);
		int nColumns = sizerNode["columns"].asInt(1);
		
		auto sizer = UISizer(sizerType, gap, nColumns);

		loadSizerChildren(sizer, node["children"]);

		return std::move(sizer);
	}
}

UISizer UIFactory::makeSizerOrDefault(const ConfigNode& node, UISizer&& defaultSizer)
{
	auto sizer = makeSizer(node);
	if (sizer) {
		return std::move(sizer.get());
	} else {
		return std::move(defaultSizer);
	}
}

std::shared_ptr<UISizer> UIFactory::makeSizerPtr(const ConfigNode& node)
{
	auto sizerNode = node["sizer"];
	if (sizerNode.getType() == ConfigNodeType::Undefined) {
		return {};
	} else {
		auto sizerType = fromString<UISizerType>(sizerNode["type"].asString("horizontal"));
		float gap = sizerNode["gap"].asFloat(1.0f);
		int nColumns = sizerNode["columns"].asInt(1);
		
		auto sizer = std::make_shared<UISizer>(sizerType, gap, nColumns);

		loadSizerChildren(*sizer, node["children"]);

		return sizer;
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
				sizer.add(makeWidget(childNode["widget"]), proportion, border, fill);
			} else if (childNode.hasKey("sizer")) {
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

std::shared_ptr<UIWidget> UIFactory::makeBaseWidget(const ConfigNode& node)
{
	auto id = node["id"].asString("");
	auto minSize = asVector2f(node["minSize"], Vector2f(0, 0));
	auto innerBorder = asVector4f(node["innerBorder"], Vector4f(0, 0, 0, 0));
	return std::make_shared<UIWidget>(id, minSize, makeSizer(node), innerBorder);
}

std::shared_ptr<UIWidget> UIFactory::makeLabel(const ConfigNode& node)
{
	auto style = node["style"].asString("label");
	return std::make_shared<UILabel>(styleSheet->getTextRenderer(style), parseLabel(node));
}

std::shared_ptr<UIWidget> UIFactory::makeButton(const ConfigNode& node)
{
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("button"), styleSheet);
	auto label = parseLabel(node);

	auto sizer = makeSizer(node);
	if (!sizer) {
		sizer = UISizer();
	}
	if (!label.getString().isEmpty()) {
		sizer->add(std::make_shared<UILabel>(style.getTextRenderer("label"), label), 1, Vector4f(), UISizerAlignFlags::Centre);
	}

	return std::make_shared<UIButton>(id, style, std::move(sizer));
}

std::shared_ptr<UIWidget> UIFactory::makeTextInput(const ConfigNode& node)
{
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("input"), styleSheet);
	auto label = parseLabel(node);

	return std::make_shared<UITextInput>(api.input->getKeyboard(), id, style, "", label);
}

std::shared_ptr<UIWidget> UIFactory::makeList(const ConfigNode& node)
{
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("list"), styleSheet);
	auto label = parseLabel(node);

	auto orientation = fromString<UISizerType>(node["type"].asString("vertical"));
	int nColumns = node["columns"].asInt(1);

	auto widget = std::make_shared<UIList>(id, style, orientation, nColumns);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	return widget;
}

std::shared_ptr<UIWidget> UIFactory::makeDropdown(const ConfigNode& node)
{
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

std::shared_ptr<UIWidget> UIFactory::makeCheckbox(const ConfigNode& node)
{
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("checkbox"), styleSheet);
	auto checked = node["checked"].asBool(false);

	return std::make_shared<UICheckbox>(id, style, checked);
}

std::shared_ptr<UIWidget> UIFactory::makeImage(const ConfigNode& node)
{
	auto imageName = node["image"].asString();
	auto materialName = node["material"].asString("");
	auto sprite = Sprite().setImage(resources, imageName, materialName);
	Vector4f innerBorder = asVector4f(node["innerBorder"], Vector4f());

	return std::make_shared<UIImage>(sprite, makeSizer(node), innerBorder);
}

std::shared_ptr<UIWidget> UIFactory::makeAnimation(const ConfigNode& node)
{
	auto id = node["id"].asString();
	auto size = asVector2f(node["size"], Vector2f());
	auto animationOffset = asVector2f(node["offset"], Vector2f());
	auto animationName = node["animation"].asString();
	auto sequence = node["sequence"].asString("default");
	auto direction = node["direction"].asString("default");

	auto animation = AnimationPlayer(resources.get<Animation>(animationName), sequence, direction);

	return std::make_shared<UIAnimation>(id, size, animationOffset, animation);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollPane(const ConfigNode& node)
{
	auto clipSize = asVector2f(node["clipSize"], Vector2f());
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);

	return std::make_shared<UIScrollPane>(clipSize, makeSizerOrDefault(node, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollBar(const ConfigNode& node)
{
	auto style = UIStyle(node["style"].asString("scrollbar"), styleSheet);
	auto scrollDirection = fromString<UIScrollDirection>(node["scrollDirection"].asString("vertical"));
	auto alwaysShow = !node["autoHide"].asBool(false);

	return std::make_shared<UIScrollBar>(scrollDirection, style, alwaysShow);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollBarPane(const ConfigNode& node)
{
	auto clipSize = asVector2f(node["clipSize"], Vector2f());
	auto style = UIStyle(node["style"].asString("scrollbar"), styleSheet);
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);
	auto alwaysShow = !node["autoHide"].asBool(false);

	return std::make_shared<UIScrollBarPane>(clipSize, style, makeSizerOrDefault(node, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical, alwaysShow);
}
