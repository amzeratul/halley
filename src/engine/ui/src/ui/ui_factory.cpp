#include <utility>
#include "halley/file_formats/config_file.h"
#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_button.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "halley/ui/widgets/ui_spin_control.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_image.h"
#include "halley/ui/widgets/ui_animation.h"
#include "halley/ui/widgets/ui_scrollbar.h"
#include "halley/ui/widgets/ui_scroll_pane.h"
#include "halley/ui/widgets/ui_scrollbar_pane.h"
#include "halley/ui/widgets/ui_checkbox.h"
#include "halley/ui/widgets/ui_slider.h"
#include "halley/ui/widgets/ui_paged_pane.h"
#include "halley/support/logger.h"
#include "ui_validator.h"
#include "halley/ui/widgets/ui_framed_image.h"
#include "halley/ui/widgets/ui_hybrid_list.h"
#include "widgets/ui_spin_list.h"
#include "widgets/ui_option_list_morpher.h"

using namespace Halley;

UIFactory::UIFactory(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet)
	: api(api)
	, resources(resources)
	, i18n(i18n)
	, styleSheet(std::move(styleSheet))
{
	if (api.platform && api.platform->hasKeyboard()) {
		keyboard = api.platform->getKeyboard();
	} else {
		keyboard = api.input->getKeyboard();
	}

	addFactory("widget", [=] (const ConfigNode& node) { return makeBaseWidget(node); });
	addFactory("label", [=] (const ConfigNode& node) { return makeLabel(node); });
	addFactory("button", [=] (const ConfigNode& node) { return makeButton(node); });
	addFactory("textInput", [=] (const ConfigNode& node) { return makeTextInput(node); });
	addFactory("spinControl", [=] (const ConfigNode& node) { return makeSpinControl(node); });
	addFactory("list", [=] (const ConfigNode& node) { return makeList(node); });
	addFactory("dropdown", [=] (const ConfigNode& node) { return makeDropdown(node); });
	addFactory("checkbox", [=] (const ConfigNode& node) { return makeCheckbox(node); });
	addFactory("image", [=] (const ConfigNode& node) { return makeImage(node); });
	addFactory("animation", [=] (const ConfigNode& node) { return makeAnimation(node); });
	addFactory("scrollBar", [=] (const ConfigNode& node) { return makeScrollBar(node); });
	addFactory("scrollPane", [=] (const ConfigNode& node) { return makeScrollPane(node); });
	addFactory("scrollBarPane", [=] (const ConfigNode& node) { return makeScrollBarPane(node); });
	addFactory("slider", [=] (const ConfigNode& node) { return makeSlider(node); });
	addFactory("horizontalDiv", [=] (const ConfigNode& node) { return makeHorizontalDiv(node); });
	addFactory("verticalDiv", [=] (const ConfigNode& node) { return makeVerticalDiv(node); });
	addFactory("tabbedPane", [=] (const ConfigNode& node) { return makeTabbedPane(node); });
	addFactory("pagedPane", [=] (const ConfigNode& node) { return makePagedPane(node); });
	addFactory("framedImage", [=] (const ConfigNode& node) { return makeFramedImage(node); });
	addFactory("hybridList", [=] (const ConfigNode& node) { return makeHybridList(node); });
	addFactory("spinList", [=](const ConfigNode& node) { return makeSpinList(node); });
	addFactory("optionListMorpher", [=](const ConfigNode& node) { return makeOptionListMorpher(node); });
}

void UIFactory::addFactory(const String& key, WidgetFactory factory)
{
	factories[key] = factory;
}

void UIFactory::pushConditions(std::vector<String> conds)
{
	conditionStack.push_back(conds.size());
	for (auto& c: conds) {
		conditions.emplace_back(std::move(c));
	}
}

void UIFactory::popConditions()
{
	if (conditionStack.empty()) {
		throw Exception("No conditions to pop!", HalleyExceptions::UI);
	}
	const size_t n = conditionStack.back();

	conditions.erase(conditions.begin() + (conditions.size() - n), conditions.end());
	conditionStack.pop_back();
}

std::shared_ptr<UIWidget> UIFactory::makeUI(const String& configName)
{
	return makeUIFromNode(resources.get<ConfigFile>(configName)->getRoot());
}

std::shared_ptr<UIWidget> UIFactory::makeUI(const String& configName, std::vector<String> conditions)
{
	pushConditions(std::move(conditions));
	try {
		auto result = makeUI(configName);
		popConditions();
		return result;
	} catch (...) {
		popConditions();
		throw;
	}
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
		throw Exception("Unknown widget class: " + widgetClass, HalleyExceptions::UI);
	}
	
	auto widget = iter->second(entryNode);
	if (widgetNode.hasKey("size")) {
		widget->setMinSize(asVector2f(widgetNode["size"], {}));
	}
	if (widgetNode.hasKey("enabled")) {
		widget->setEnabled(widgetNode["enabled"].asBool(true));
	}
	if (widgetNode.hasKey("active")) {
		widget->setActive(widgetNode["active"].asBool(true));
	}
	if (widgetNode.hasKey("childLayerAdjustment")) {
		widget->setChildLayerAdjustment(widgetNode["childLayerAdjustment"].asInt());
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
		throw Exception("Unable to parse node as Vector2f.", HalleyExceptions::UI);
	}
}

LocalisedString UIFactory::parseLabel(const ConfigNode& node, const String& defaultOption, const String& key) {
	LocalisedString label;
	if (node.hasKey(key + "Key")) {
		label = i18n.get(node[key + "Key"].asString());
	} else if (node.hasKey(key)) {
		label = LocalisedString::fromUserString(node[key].asString(defaultOption));
	}
	return label;
}

std::vector<UIFactory::ParsedOption> UIFactory::parseOptions(const ConfigNode& node)
{
	std::vector<ParsedOption> result;
	if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& n: node.asSequence()) {
			auto id = n["id"].asString("");
			auto label = parseLabel(n, id);
			if (id.isEmpty()) {
				id = label.getString();
			}

			ParsedOption option;
			option.id = id;
			option.text = label;
			option.image = n["image"].asString("");
			option.spriteSheet = n["spriteSheet"].asString("");
			option.sprite = n["sprite"].asString("");
			option.inactiveImage = n["inactiveImage"].asString("");
			option.border = asVector4f(n["border"], Vector4f());
			option.active = n["active"].asBool(true);
			result.push_back(option);
		}
	}
	return result;
}

Vector4f UIFactory::asVector4f(const ConfigNode& node, Maybe<Vector4f> defaultValue)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		auto seq = node.asSequence();
		return Vector4f(seq.at(0).asFloat(), seq.at(1).asFloat(), seq.at(2).asFloat(), seq.at(3).asFloat());
	} else if (defaultValue) {
		return defaultValue.get();
	} else {
		throw Exception("Unable to parse node as Vector2f.", HalleyExceptions::UI);
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
	auto id = node["id"].asString("");
	auto style = UIStyle(node["style"].asString("label"), styleSheet);
	auto label = std::make_shared<UILabel>(id, style.getTextRenderer("label"), parseLabel(node));
	if (node.hasKey("maxWidth")) {
		label->setMaxWidth(node["maxWidth"].asFloat());
	}
	if (node.hasKey("maxHeight")) {
		label->setMaxHeight(node["maxHeight"].asFloat());
	}
	if (node.hasKey("wordWrapped")) {
		label->setWordWrapped(node["wordWrapped"].asBool());
	}
	if (node.hasKey("alignment")) {
		label->setAlignment(node["alignment"].asFloat());
	}
	if (node.hasKey("marquee")) {
		label->setMarquee(node["marquee"].asBool());
	}
	return label;
}

std::shared_ptr<UIWidget> UIFactory::makeButton(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("button"), styleSheet);
	auto label = parseLabel(node);

	auto sizer = makeSizerOrDefault(entryNode, UISizer());
	if (!label.getString().isEmpty()) {
		const auto& renderer = style.getTextRenderer("label");
		auto uiLabel = std::make_shared<UILabel>(id + "_label", renderer, label);
		if (style.hasTextRenderer("hoveredLabel")) {
			uiLabel->setHoverable(style.getTextRenderer("label"), style.getTextRenderer("hoveredLabel"));
		}
		if (style.hasTextRenderer("selectedLabel"))
		{
			uiLabel->setSelectable(style.getTextRenderer("label"), style.getTextRenderer("selectedLabel"));
		}
		sizer.add(uiLabel, 1, style.getBorder("labelBorder"), UISizerAlignFlags::Centre);
	}

	auto result = std::make_shared<UIButton>(id, style, std::move(sizer));

	if (node.hasKey("mouseBorder")) {
		result->setMouseExtraBorder(asVector4f(node["mouseBorder"], Vector4f()));
	}

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeTextInput(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("input"), styleSheet);
	auto label = parseLabel(node);
	auto ghostText = parseLabel(node, "", "ghost");

	auto result = std::make_shared<UITextInput>(keyboard, id, style, "", label);;
	if (!ghostText.getString().isEmpty()) {
		result->setGhostText(ghostText);
	}

	auto validatorName = node["validator"].asString("");
	if (!validatorName.isEmpty()) {
		if (validatorName == "numeric") {
			result->setValidator(std::make_shared<UINumericValidator>(true));
		} else if (validatorName == "numericPositive") {
		result->setValidator(std::make_shared<UINumericValidator>(false));
		} else {
			throw Exception("Unknown text input validator: " + validatorName, HalleyExceptions::UI);
		}
	}

	if (node.hasKey("maxLength")) {
		result->setMaxLength(node["maxLength"].asInt());
	}

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeSpinControl(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("spinControl"), styleSheet);

	auto result = std::make_shared<UISpinControl>(keyboard, id, style, 0.0f);

	if (node.hasKey("minValue")) {
		result->setMinimumValue(node["minValue"].asFloat());
	}
	if (node.hasKey("maxValue")) {
		result->setMaximumValue(node["maxValue"].asFloat());
	}
	if (node.hasKey("increment")) {
		result->setIncrement(node["increment"].asFloat());
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
	auto options = parseOptions(node["options"]);

	auto widget = std::make_shared<UIList>(id, style, orientation, nColumns);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	for (auto& o: options) {
		if (!o.image.isEmpty() || !o.sprite.isEmpty()) {
			Sprite normalSprite;
			
			if (!o.image.isEmpty()) {
				normalSprite.setImage(resources, o.image);
			} else {
				normalSprite.setSprite(resources, o.spriteSheet, o.sprite);
			}
			
			auto image = std::make_shared<UIImage>(normalSprite);

			if (!o.inactiveImage.isEmpty()) {
				Sprite inactiveSprite = Sprite().setImage(resources, o.inactiveImage);
				image->setSelectable(inactiveSprite, normalSprite);
				image->setSprite(inactiveSprite);
			}

			widget->addItem(o.id, image, 1, o.border, UISizerAlignFlags::Centre);
		} else {
			widget->addTextItem(o.id, o.text);
		}
		widget->setItemActive(o.id, o.active);
	}

	widget->setDrag(node["canDrag"].asBool(false));
	widget->setUniformSizedItems(node["uniformSizedItems"].asBool(false));

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
	auto options = parseOptions(node["options"]);

	std::vector<String> optionIds;
	std::vector<LocalisedString> optionLabels;
	for (auto& o: options) {
		optionIds.push_back(o.id);
		optionLabels.push_back(o.text);
	}

	auto widget = std::make_shared<UIDropdown>(id, style, scrollStyle, listStyle);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	widget->setOptions(optionIds, optionLabels);
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
	auto id = node["id"].asString("");
	auto materialName = node["material"].asString("");
	auto col = node["colour"].asString("#FFFFFF");
	auto flip = node["flip"].asBool(false);
	auto pivot = asMaybeVector2f(node["pivot"]);
	auto rotation = Angle1f::fromDegrees(node["rotation"].asFloat(0.0f));

	auto sprite = Sprite();
	
	if (node.hasKey("image")) {
		auto imageName = node["image"].asString();
		sprite.setImage(resources, imageName, materialName);
	} else if (node.hasKey("sprite")) {
		auto spriteName = node["sprite"].asString();
		auto spriteSheetName = node["spriteSheet"].asString();
		sprite.setSprite(resources, spriteSheetName, spriteName, materialName);
	}

	sprite.setColour(Colour4f::fromString(col)).setFlip(flip).setRotation(rotation);
	
	if (pivot) {
		sprite.setPivot(pivot.get());
	}
	Vector4f innerBorder = asVector4f(node["innerBorder"], Vector4f());

	auto image = std::make_shared<UIImage>(id, sprite, makeSizer(entryNode), innerBorder);
	if (node.hasKey("layerAdjustment")) {
		image->setLayerAdjustment(node["layerAdjustment"].asInt());
	}
	return image;
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
	auto id = node["id"].asString("");
	auto clipSize = asVector2f(node["clipSize"], Vector2f());
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);

	return std::make_shared<UIScrollPane>(id, clipSize, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical);
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
	auto id = node["id"].asString("");
	auto clipSize = asVector2f(node["clipSize"], Vector2f());
	auto style = UIStyle(node["style"].asString("scrollbar"), styleSheet);
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);
	auto alwaysShow = !node["autoHide"].asBool(false);

	return std::make_shared<UIScrollBarPane>(id, clipSize, style, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical, alwaysShow);
}

std::shared_ptr<UIWidget> UIFactory::makeSlider(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("slider"), styleSheet);
	auto minValue = node["minValue"].asFloat(0);
	auto maxValue = node["maxValue"].asFloat(1);
	auto value = node["value"].asFloat(0.5f);

	auto slider = std::make_shared<UISlider>(id, style, minValue, maxValue, value);
	if (node.hasKey("granularity")) {
		slider->setGranularity(node["granularity"].asFloat());
	}

	return slider;
}

std::shared_ptr<UIWidget> UIFactory::makeHorizontalDiv(const ConfigNode& entryNode)
{
	const auto& widgetNode = entryNode["widget"];
	auto id = widgetNode["id"].asString("");
	auto style = getStyle(widgetNode["style"].asString("horizontalDiv"));
	return std::make_shared<UIImage>(id, style.getSprite("image"));
}

std::shared_ptr<UIWidget> UIFactory::makeVerticalDiv(const ConfigNode& entryNode)
{
	const auto& widgetNode = entryNode["widget"];
	auto id = widgetNode["id"].asString("");
	auto style = getStyle(widgetNode["style"].asString("verticalDiv"));
	return std::make_shared<UIImage>(id, style.getSprite("image"));
}

std::shared_ptr<UIWidget> UIFactory::makeTabbedPane(const ConfigNode& entryNode)
{
	const auto& widgetNode = entryNode["widget"];
	auto id = widgetNode["id"].asString();
	auto tabs = std::make_shared<UIList>(id, getStyle("tabs"), UISizerType::Horizontal, 1);
	applyInputButtons(*tabs, widgetNode["inputButtons"].asString("tabs"));

	std::vector<const ConfigNode*> tabNodes;
	if (widgetNode.hasKey("tabs")) {
		for (auto& tabNode: widgetNode["tabs"].asSequence()) {
			if (tabNode.hasKey("if")) {
				if (!resolveConditions(tabNode["if"])) {
					continue;
				}
			}
			auto label = parseLabel(tabNode);
			tabs->addTextItem(id + "_tab_" + toString(tabNodes.size()), label);
			tabNodes.push_back(&tabNode);
		}
	}

	auto pane = std::make_shared<UIPagedPane>(id + "_pagedPane", int(tabNodes.size()), Vector2f());
	for (int i = 0; i < int(tabNodes.size()); ++i) {
		auto& tabNode = *tabNodes[i];
		pane->getPage(i)->add(makeSizerPtr(tabNode), 1);
	}

	tabs->setHandle(UIEventType::ListSelectionChanged, [pane] (const UIEvent& event)
	{
		pane->setPage(event.getIntData());
		event.getCurWidget().sendEvent(UIEvent(UIEventType::TabChanged, event.getSourceId(), event.getIntData()));
	});

	auto result = std::make_shared<UIWidget>(id + "_container", Vector2f(), UISizer(UISizerType::Vertical));
	result->add(tabs);
	result->add(pane, 1);
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makePagedPane(const ConfigNode& entryNode)
{
	const auto& widgetNode = entryNode["widget"];

	std::vector<const ConfigNode*> pageNodes;
	if (widgetNode.hasKey("pages")) {
		for (auto& pageNode: widgetNode["pages"].asSequence()) {
			if (pageNode.hasKey("if")) {
				if (!resolveConditions(pageNode["if"])) {
					continue;
				}
			}
			pageNodes.push_back(&pageNode);
		}
	}

	auto pane = std::make_shared<UIPagedPane>(widgetNode["id"].asString(), int(pageNodes.size()));
	for (int i = 0; i < int(pageNodes.size()); ++i) {
		auto& pageNode = *pageNodes[i];
		const auto element = pageNode.hasKey("widget") ? std::shared_ptr<IUIElement>(makeWidget(pageNode)) : makeSizerPtr(pageNode);
		pane->getPage(i)->add(element, 1);
	}

	return pane;
}

std::shared_ptr<UIWidget> UIFactory::makeFramedImage(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];

	const auto id = node["id"].asString("");
	const auto scrollPos = asVector2f(node["scrollPos"], Vector2f());
	const auto scrollSpeed = asVector2f(node["scrollSpeed"], Vector2f());
	const auto style = getStyle(node["style"].asString());
	
	auto image = std::make_shared<UIFramedImage>(id, style, makeSizer(entryNode));
	image->setScrolling(scrollSpeed, scrollPos);
	return image;
}

std::shared_ptr<UIWidget> UIFactory::makeHybridList(const ConfigNode& node)
{
	auto& widgetNode = node["widget"];
	auto style = getStyle(node["style"].asString("hybridList"));
	auto list = std::make_shared<UIHybridList>(widgetNode["id"].asString(), style);
	if (widgetNode.hasKey("options")) {
		for (auto& optionsNode: widgetNode["options"].asSequence()) {
			if (optionsNode.hasKey("if")) {
				if (!resolveConditions(optionsNode["if"])) {
					continue;
				}
			}

			if (optionsNode["divider"].asBool(false)) {
				list->addDivider(optionsNode["id"].asString(""));
			} else {
				const auto id = optionsNode["id"].asString();
				const auto label = parseLabel(optionsNode);
				list->addTextItem(id, label);
			}
		}
	}
	applyInputButtons(*list, "list");
	return list;
}

std::shared_ptr<UIWidget> UIFactory::makeSpinList(const ConfigNode& entryNode) {
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("spinlist"), styleSheet);
	auto label = parseLabel(node);
	auto options = parseOptions(node["options"]);

	std::vector<String> optionIds;
	std::vector<LocalisedString> optionLabels;
	for (auto& o : options) {
		optionIds.push_back(o.id);
		optionLabels.push_back(o.text);
	}

	auto widget = std::make_shared<UISpinList>(id, style);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	widget->setOptions(optionIds, optionLabels);
	return widget;
}

std::shared_ptr<UIWidget> UIFactory::makeOptionListMorpher(const ConfigNode& entryNode) {
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto dropdownStyle = UIStyle(node["dropdownStyle"].asString("dropdown"), styleSheet);
	auto spinlistStyle = UIStyle(node["spinlistStyle"].asString("spinlist"), styleSheet);
	auto scrollStyle = UIStyle(node["ScrollBarStyle"].asString("scrollbar"), styleSheet);
	auto listStyle = UIStyle(node["listStyle"].asString("list"), styleSheet);
	auto label = parseLabel(node);
	auto options = parseOptions(node["options"]);

	std::vector<String> optionIds;
	std::vector<LocalisedString> optionLabels;
	for (auto& o : options) {
		optionIds.push_back(o.id);
		optionLabels.push_back(o.text);
	}

	auto widget = std::make_shared<UIOptionListMorpher>(id, dropdownStyle, spinlistStyle, scrollStyle, listStyle);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	widget->setOptions(optionIds, optionLabels);
	return widget;
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
			ok &= resolveCondition(c.asString());
		}
		return ok;
	} else {
		return resolveCondition(node.asString());
	}
}
