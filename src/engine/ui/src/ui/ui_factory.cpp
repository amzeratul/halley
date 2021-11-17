#include <utility>
#include "halley/file_formats/config_file.h"
#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"

#include "ui_definition.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_label.h"
#include "halley/ui/widgets/ui_button.h"
#include "halley/ui/widgets/ui_textinput.h"
#include "halley/ui/widgets/ui_spin_control.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/widgets/ui_image.h"
#include "halley/ui/widgets/ui_multi_image.h"
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
#include "widgets/ui_tree_list.h"
#include "halley/ui/behaviours/ui_reload_ui_behaviour.h"
#include "widgets/ui_debug_console.h"
#include "widgets/ui_spin_control2.h"

using namespace Halley;

UIFactoryWidgetProperties::Entry::Entry(String label, String name, String type, std::vector<String> defaultValue)
	: label(std::move(label))
	, name(std::move(name))
	, type(std::move(type))
	, defaultValue(std::move(defaultValue))
{
}

UIFactoryWidgetProperties::Entry::Entry(String label, String name, String type, String defaultValue)
	: label(std::move(label))
	, name(std::move(name))
	, type(std::move(type))
{
	this->defaultValue.emplace_back(std::move(defaultValue));
}

UIFactory::UIFactory(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet, std::shared_ptr<const UIColourScheme> colourScheme)
	: api(api)
	, resources(resources)
	, i18n(i18n)
	, colourScheme(std::move(colourScheme))
	, styleSheet(std::move(styleSheet))
{
	if (!this->colourScheme) {
		loadDefaultColourScheme();
	}
	
	addFactory("widget", [=] (const ConfigNode& node) { return makeBaseWidget(node); }, getBaseWidgetProperties());
	addFactory("label", [=] (const ConfigNode& node) { return makeLabel(node); }, getLabelProperties());
	addFactory("button", [=] (const ConfigNode& node) { return makeButton(node); });
	addFactory("textInput", [=] (const ConfigNode& node) { return makeTextInput(node); });
	addFactory("spinControl", [=] (const ConfigNode& node) { return makeSpinControl(node); });
	addFactory("spinControl2", [=] (const ConfigNode& node) { return makeSpinControl2(node); });
	addFactory("list", [=] (const ConfigNode& node) { return makeList(node); });
	addFactory("dropdown", [=] (const ConfigNode& node) { return makeDropdown(node); });
	addFactory("checkbox", [=] (const ConfigNode& node) { return makeCheckbox(node); });
	addFactory("image", [=] (const ConfigNode& node) { return makeImage(node); });
	addFactory("multiImage", [=](const ConfigNode& node) { return makeMultiImage(node); });
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
	addFactory("treeList", [=](const ConfigNode& node) { return makeTreeList(node); });
	addFactory("debugConsole", [=](const ConfigNode& node) { return makeDebugConsole(node); });
}

UIFactory::~UIFactory()
{
}

void UIFactory::addFactory(const String& key, WidgetFactory factory, UIFactoryWidgetProperties props)
{
	const auto& baseProps = getGlobalWidgetProperties();
	props.entries.insert(props.entries.begin(), baseProps.entries.begin(), baseProps.entries.end());

	factories[key] = std::move(factory);
	properties[key] = std::move(props);
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
	return makeUI(*resources.get<UIDefinition>(configName));
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

std::shared_ptr<UIWidget> UIFactory::makeUI(const UIDefinition& definition)
{
	return makeWidget(definition.getRoot());
}

void UIFactory::loadUI(UIWidget& target, const String& configName)
{
	loadUI(target, *resources.get<UIDefinition>(configName));
}

void UIFactory::loadUI(UIWidget& target, const UIDefinition& uiDefinition)
{
	try {
		target.add(makeUI(uiDefinition), 1);
		target.onMakeUI();
	} catch (const std::exception& e) {
		Logger::logException(e);
	}
	
	target.addBehaviour(std::make_shared<UIReloadUIBehaviour>(*this, ResourceObserver(uiDefinition)));
}

const UIFactoryWidgetProperties& UIFactory::getPropertiesForWidget(const String& widgetClass) const
{
	const auto iter = properties.find(widgetClass);
	if (iter == properties.end()) {
		throw Exception("Unknown widget type: " + widgetClass, HalleyExceptions::Entity);
	}
	return iter->second;
}

std::vector<String> UIFactory::getWidgetClassList() const
{
	std::vector<String> result;
	result.reserve(properties.size());
	for (auto& p: properties) {
		result.push_back(p.first);
	}
	return result;
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

Resources& UIFactory::getResources() const
{
	return resources;
}

std::unique_ptr<UIFactory> UIFactory::withResources(Resources& newResources) const
{
	auto result = std::make_unique<UIFactory>(api, newResources, i18n, styleSheet, colourScheme);
	result->inputButtons = inputButtons;
	return result;
}

const I18N& UIFactory::getI18N() const
{
	return i18n;
}

void UIFactory::setStyleSheet(std::shared_ptr<UIStyleSheet> styleSheet)
{
	this->styleSheet = std::move(styleSheet);
}

std::shared_ptr<const UIColourScheme> UIFactory::getColourScheme() const
{
	return colourScheme;
}

void UIFactory::update()
{
	styleSheet->updateIfNeeded();
}

Sprite UIFactory::makeAssetTypeIcon(AssetType type) const
{
	return Sprite();
}

std::shared_ptr<UIWidget> UIFactory::makeWidget(const ConfigNode& entryNode)
{
	styleSheet->updateIfNeeded();
	
	auto& widgetNode = entryNode["widget"];
	auto widgetClass = widgetNode["class"].asString();
	auto iter = factories.find(widgetClass);
	if (iter == factories.end()) {
		throw Exception("Unknown widget class: " + widgetClass, HalleyExceptions::UI);
	}
	
	auto widget = iter->second(entryNode);
	if (widgetNode.hasKey("size")) {
		widget->setMinSize(widgetNode["size"].asVector2f({}));
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
	if (widgetNode.hasKey("tooltip")) {
		widget->setToolTip(LocalisedString::fromUserString(widgetNode["tooltip"].asString()));
	}
	if (widgetNode.hasKey("tooltipKey")) {
		widget->setToolTip(i18n.get(widgetNode["tooltipKey"].asString()));
	}
	return widget;
}

UIFactoryWidgetProperties UIFactory::getGlobalWidgetProperties() const
{
	UIFactoryWidgetProperties result;
	result.entries.emplace_back("Id", "id", "Halley::String", "");
	result.entries.emplace_back("Active", "active", "bool", "true");
	result.entries.emplace_back("Enabled", "enabled", "bool", "true");
	result.entries.emplace_back("Minimum Size", "size", "Halley::Vector2f");
	result.entries.emplace_back("Child Layer Adjustment", "childLayerAdjustment", "int", "0");
	result.entries.emplace_back("Tooltip", "tooltip", "Halley::String");
	result.entries.emplace_back("Tooltip (Key)", "tooltipKey", "Halley::String");
	return result;
}

std::optional<UISizer> UIFactory::makeSizer(const ConfigNode& entryNode)
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

		if (sizerNode["columnProportions"].getType() == ConfigNodeType::Sequence) {
			auto& seq = sizerNode["columnProportions"].asSequence();
			std::vector<float> proportions;
			proportions.reserve(seq.size());
			for (auto& e: seq) {
				proportions.push_back(e.asFloat());
			}
			sizer.setColumnProportions(proportions);
		}
	}

	loadSizerChildren(sizer, entryNode["children"]);

	return sizer;
}

UISizer UIFactory::makeSizerOrDefault(const ConfigNode& entryNode, UISizer&& defaultSizer)
{
	auto sizer = makeSizer(entryNode);
	if (sizer) {
		return std::move(sizer.value());
	} else {
		return std::move(defaultSizer);
	}
}

std::shared_ptr<UISizer> UIFactory::makeSizerPtr(const ConfigNode& entryNode)
{
	auto sizer = makeSizer(entryNode);
	if (sizer) {
		return std::make_shared<UISizer>(std::move(sizer.value()));
	} else {
		return {};
	}
}

void UIFactory::loadSizerChildren(UISizer& sizer, const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node.asSequence()) {
			float proportion = childNode["proportion"].asFloat(0);
			Vector4f border = childNode["border"].asVector4f(Vector4f());
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
		for (const auto& n: node.asSequence()) {
			auto id = n["id"].asString("");
			auto label = parseLabel(n, id);
			if (id.isEmpty()) {
				id = label.getString();
			}

			ParsedOption option;
			option.id = id;
			option.text = label;
			option.image = n["image"].asString("");
			option.inactiveImage = n["inactiveImage"].asString("");
			option.spriteSheet = n["spriteSheet"].asString("");
			option.sprite = n["sprite"].asString("");
			option.border = n["border"].asVector4f(Vector4f());
			option.active = n["active"].asBool(true);
			option.tooltip = parseLabel(n, "", "tooltip");
			option.iconColour = n["iconColour"].asString("");
			result.push_back(option);
		}
	}
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeBaseWidget(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
	auto minSize = node["minSize"].asVector2f(Vector2f(0, 0));
	auto innerBorder = node["innerBorder"].asVector4f(Vector4f(0, 0, 0, 0));
	return std::make_shared<UIWidget>(id, minSize, makeSizer(entryNode), innerBorder);
}

UIFactoryWidgetProperties UIFactory::getBaseWidgetProperties() const
{
	UIFactoryWidgetProperties result;
	result.entries.emplace_back("Inner Border", "innerBorder", "Halley::Vector4f", std::vector<String>{"", "", "", ""});
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeLabel(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
	auto style = UIStyle(node["style"].asString("label"), styleSheet);
	auto label = std::make_shared<UILabel>(id, std::move(style), parseLabel(node));
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
	if (node.hasKey("colour")) {
		label->setColour(getColour(node["colour"].asString()));
	}
	if (node.hasKey("fontSize")) {
		label->getTextRenderer().setSize(node["fontSize"].asFloat());
	}
	return label;
}

UIFactoryWidgetProperties UIFactory::getLabelProperties() const
{
	UIFactoryWidgetProperties result;
	result.canHaveChildren = false;
	result.entries.emplace_back("Style", "style", "Halley::String", "label");
	result.entries.emplace_back("Max Width", "maxWidth", "float", "");
	result.entries.emplace_back("Max Height", "maxHeight", "float", "");
	result.entries.emplace_back("Alignment", "alignment", "float", "");
	result.entries.emplace_back("Font Size", "fontSize", "float", "");
	result.entries.emplace_back("Marquee", "marquee", "bool", "");
	result.entries.emplace_back("Word Wrap", "wordWrapped", "bool", "");
	result.entries.emplace_back("Colour", "colour", "Halley::String", "");
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeButton(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("button"), styleSheet);
	auto label = parseLabel(node);

	auto sizer = makeSizerOrDefault(entryNode, UISizer());
	auto result = std::make_shared<UIButton>(id, style, std::move(sizer));

	if (!label.getString().isEmpty()) {
		result->setLabel(LocalisedString::fromUserString(label.getString()));
	}
	
	if (node.hasKey("icon")) {
		result->setIcon(Sprite().setImage(getResources(), node["icon"].asString()));
	}

	if (node.hasKey("mouseBorder")) {
		result->setMouseExtraBorder(node["mouseBorder"].asVector4f(Vector4f()));
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

	auto result = std::make_shared<UITextInput>(id, style, "", label);;
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
	result->setReadOnly(node["readOnly"].asBool(false));
	result->setHistoryEnabled(node["history"].asBool(false));
	result->setClearOnSubmit(node["clearOnSubmit"].asBool(false));

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeSpinControl(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("spinControl"), styleSheet);

	auto result = std::make_shared<UISpinControl>(id, style, 0.0f);

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

std::shared_ptr<UIWidget> UIFactory::makeSpinControl2(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("spinControl"), styleSheet);

	auto result = std::make_shared<UISpinControl2>(id, style, 0.0f, node["allowFloat"].asBool(false));

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

std::shared_ptr<UIWidget> UIFactory::makeDropdown(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("dropdown"), styleSheet);
	auto label = parseLabel(node);
	auto options = parseOptions(node["options"]);

	std::vector<UIDropdown::Entry> entries;
	entries.reserve(options.size());
	for (auto& o: options) {
		Sprite icon;
		if (!o.image.isEmpty()) {
			icon = Sprite().setImage(getResources(), o.image).setColour(getColour(o.iconColour.isEmpty() ? "#FFFFFF" : o.iconColour));
		}
		entries.emplace_back(o.id, o.text, icon);
	}

	auto widget = std::make_shared<UIDropdown>(id, style);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	widget->setOptions(entries);
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
	auto pivot = node.hasKey("pivot") ? node["pivot"].asVector2f() : std::optional<Vector2f>{};
	auto rotation = Angle1f::fromDegrees(node["rotation"].asFloat(0.0f));

	auto sprite = Sprite();
	
	if (node.hasKey("image")) {
		auto imageName = node["image"].asString();
		if (colourScheme) {
			sprite = colourScheme->getSprite(resources, imageName, materialName);
		} else {
			sprite = Sprite().setImage(resources, imageName, materialName);
		}
	} else if (node.hasKey("sprite")) {
		auto spriteName = node["sprite"].asString();
		auto spriteSheetName = node["spriteSheet"].asString();
		sprite.setSprite(resources, spriteSheetName, spriteName, materialName);
	}

	sprite.setColour(getColour(col)).setFlip(flip).setRotation(rotation);
	
	if (pivot) {
		sprite.setPivot(pivot.value());
	}
	Vector4f innerBorder = node["innerBorder"].asVector4f(Vector4f());

	auto image = std::make_shared<UIImage>(id, sprite, makeSizer(entryNode), innerBorder);
	if (node.hasKey("layerAdjustment")) {
		image->setLayerAdjustment(node["layerAdjustment"].asInt());
	}
	return image;
}

std::shared_ptr<UIWidget> UIFactory::makeMultiImage(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	const auto id = node["id"].asString("");
	const auto size = node["size"].asVector2f(Vector2f());
	const auto materialName = node["material"].asString("");

	std::vector<Sprite> sprites = {};
	std::vector<Vector2f> offsets = {};
	
	if (node.hasKey("images")) {
		const auto& images = node["images"].asSequence();

		for(const auto& imageName : images) {
			Sprite sprite = Sprite();
			sprite.setImage(resources, imageName.asString(), materialName);
		}
	}

	if(node.hasKey("offsets")) {
		offsets = node["offsets"].asVector<Vector2f>();
	}

	return std::make_shared<UIMultiImage>(id, size, sprites, offsets);
}

std::shared_ptr<UIWidget> UIFactory::makeAnimation(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto size = node["size"].asVector2f(Vector2f());
	auto animationOffset = node["offset"].asVector2f(Vector2f());
	auto animationName = node["animation"].asString("");
	auto sequence = node["sequence"].asString("default");
	auto direction = node["direction"].asString("default");

	auto animation = AnimationPlayer(animationName.isEmpty() ? std::shared_ptr<const Animation>() : resources.get<Animation>(animationName), sequence, direction);

	return std::make_shared<UIAnimation>(id, size, animationOffset, animation);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollPane(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
	auto clipSize = node["clipSize"].asVector2f(Vector2f());
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);
	auto mouseWheelEnabled = node["mouseWheelEnabled"].asBool(true);

	auto result = std::make_shared<UIScrollPane>(id, clipSize, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical);
	result->setScrollWheelEnabled(mouseWheelEnabled);
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeScrollBar(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
	auto style = UIStyle(node["style"].asString("scrollbar"), styleSheet);
	auto scrollDirection = fromString<UIScrollDirection>(node["scrollDirection"].asString("vertical"));
	auto alwaysShow = !node["autoHide"].asBool(false);

	return std::make_shared<UIScrollBar>(id, scrollDirection, style, alwaysShow);
}

std::shared_ptr<UIWidget> UIFactory::makeScrollBarPane(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
	auto clipSize = node["clipSize"].asVector2f(Vector2f());
	auto style = UIStyle(node["style"].asString("scrollbar"), styleSheet);
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);
	auto alwaysShow = !node["autoHide"].asBool(false);
	auto mouseWheelEnabled = node["mouseWheelEnabled"].asBool(true);

	auto result = std::make_shared<UIScrollBarPane>(id, clipSize, style, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical, alwaysShow);
	result->getPane()->setScrollWheelEnabled(mouseWheelEnabled);
	return result;
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
	const auto& style = getStyle(widgetNode["style"].asString("horizontalDiv"));
	return std::make_shared<UIImage>(std::move(id), style.getSprite("image"));
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
			tabs->addTextItem(tabNode["id"].asString(id + "_tab_" + toString(tabNodes.size())), label);
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
	const auto scrollPos = node["scrollPos"].asVector2f(Vector2f());
	const auto scrollSpeed = node["scrollSpeed"].asVector2f(Vector2f());
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
	auto label = parseLabel(node);
	auto options = parseOptions(node["options"]);

	std::vector<String> optionIds;
	std::vector<LocalisedString> optionLabels;
	for (auto& o : options) {
		optionIds.push_back(o.id);
		optionLabels.push_back(o.text);
	}

	auto widget = std::make_shared<UIOptionListMorpher>(id, dropdownStyle, spinlistStyle);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	widget->setOptions(optionIds, optionLabels);
	return widget;
}
std::shared_ptr<UIWidget> UIFactory::makeList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto style = UIStyle(node["style"].asString("list"), styleSheet);
	auto label = parseLabel(node);

	auto orientation = fromString<UISizerType>(node["type"].asString("vertical"));
	int nColumns = node["columns"].asInt(1);

	auto widget = std::make_shared<UIList>(node["id"].asString(), style, orientation, nColumns);
	applyListProperties(*widget, node, "list");

	return widget;
}

std::shared_ptr<UIWidget> UIFactory::makeTreeList(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("treeList"), styleSheet);
	auto label = parseLabel(node);

	auto widget = std::make_shared<UITreeList>(id, style);
	applyListProperties(*widget, node, "treeList");

	return widget;
}

void UIFactory::applyListProperties(UIList& list, const ConfigNode& node, const String& inputConfigName)
{
	applyInputButtons(list, node["inputButtons"].asString(inputConfigName));

	auto options = parseOptions(node["options"]);
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

			list.addImage(o.id, image, 1, o.border, UISizerAlignFlags::Centre);
		} else {
			list.addTextItem(o.id, o.text);
		}

		if (!o.tooltip.getString().isEmpty()) {
			list.getItem(o.id)->setToolTip(o.tooltip);
		}

		list.setItemActive(o.id, o.active);
	}

	list.setDragEnabled(node["canDrag"].asBool(false));
	list.setUniformSizedItems(node["uniformSizedItems"].asBool(false));
	list.setSingleClickAccept(node["singleClickAccept"].asBool(true));
	list.setMultiSelect(node["multiSelect"].asBool(false));
}

std::shared_ptr<UIWidget> UIFactory::makeDebugConsole(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();

	auto widget = std::make_shared<UIDebugConsole>(id, *this, std::make_shared<UIDebugConsoleController>());

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

Colour4f UIFactory::getColour(const String& key) const
{
	if (key.startsWith("#")) {
		return Colour4f::fromString(key);
	} else if (key.startsWith("$") && colourScheme) {
		return colourScheme->getColour(key.mid(1));
	} else {
		return Colour4f();
	}
}

void UIFactory::loadDefaultColourScheme()
{
	const String defaultColourScheme = "colour_schemes/default";
	if (resources.exists<ConfigFile>(defaultColourScheme)) {
		colourScheme = std::make_shared<UIColourScheme>(resources.get<ConfigFile>(defaultColourScheme)->getRoot(), resources);
	}
}
