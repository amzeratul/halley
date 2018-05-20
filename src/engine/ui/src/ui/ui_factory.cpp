#include "halley/file_formats/config_file.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_button.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "halley/core/api/halley_api.h"

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
}

std::shared_ptr<UIWidget> UIFactory::makeUI(const ConfigNode& node)
{
	return makeWidget(node["widget"]);
}

std::shared_ptr<UIWidget> UIFactory::makeWidget(const ConfigNode& node)
{
	auto widgetClass = node["class"].asString();
	auto iter = factories.find(widgetClass);
	if (iter == factories.end()) {
		throw Exception("Unknown widget class: " + widgetClass);
	} else {
		return iter->second(node);
	}
}

Maybe<UISizer> UIFactory::makeSizer(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Undefined) {
		return {};
	} else {
		auto sizerType = fromString<UISizerType>(node["type"].asString("horizontal"));
		float gap = node["gap"].asFloat(1.0f);
		int nColumns = node["columns"].asInt(1);
		
		auto sizer = UISizer(sizerType, gap, nColumns);

		loadSizerChildren(sizer, node);

		return std::move(sizer);
	}
}

std::shared_ptr<UISizer> UIFactory::makeSizerPtr(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Undefined) {
		return {};
	} else {
		auto sizerType = fromString<UISizerType>(node["type"].asString("horizontal"));
		float gap = node["gap"].asFloat(1.0f);
		int nColumns = node["columns"].asInt(1);
		
		auto sizer = std::make_shared<UISizer>(sizerType, gap, nColumns);

		loadSizerChildren(*sizer, node);

		return sizer;
	}
}

void UIFactory::loadSizerChildren(UISizer& sizer, const ConfigNode& node)
{
	if (node.hasKey("children")) {
		for (auto& childNode: node["children"].asSequence()) {
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
				sizer.add(makeSizerPtr(childNode["sizer"]), proportion, border, fill);
			} else if (childNode.hasKey("spacer")) {
				sizer.addSpacer(proportion);
			} else if (childNode.hasKey("stretchSpacer")) {
				sizer.addStretchSpacer(proportion);
			}
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
	return std::make_shared<UIWidget>(id, minSize, makeSizer(node["sizer"]), innerBorder);
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

	auto sizer = makeSizer(node["sizer"]);
	if (!sizer) {
		sizer = UISizer();
	}
	if (!label.getString().isEmpty()) {
		sizer->add(std::make_shared<UILabel>(style.getTextRenderer("label"), label), 1, Vector4f(), UISizerAlignFlags::Centre);
	}

	auto button = std::make_shared<UIButton>(id, style, std::move(sizer));
	if (node.hasKey("size")) {
		auto minSize = asVector2f(node["size"], {});
		button->setMinSize(minSize);
	}
	return button;
}

std::shared_ptr<UIWidget> UIFactory::makeTextInput(const ConfigNode& node)
{
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("input"), styleSheet);
	auto label = parseLabel(node);

	auto input = std::make_shared<UITextInput>(api.input->getKeyboard(), id, style, "", label);
	if (node.hasKey("size")) {
		auto minSize = asVector2f(node["size"], {});
		input->setMinSize(minSize);
	}
	return input;
}
