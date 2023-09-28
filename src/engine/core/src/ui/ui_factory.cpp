#include <utility>
#include "halley/file_formats/config_file.h"
#include "halley/api/halley_api.h"
#include "halley/ui/ui_factory.h"

#include "halley/ui/ui_definition.h"
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
#include "halley/ui/ui_validator.h"
#include "halley/graphics/material/material_definition.h"
#include "halley/ui/widgets/ui_framed_image.h"
#include "halley/ui/widgets/ui_hybrid_list.h"
#include "halley/ui/widgets/ui_spin_list.h"
#include "halley/ui/widgets/ui_option_list_morpher.h"
#include "halley/ui/widgets/ui_tree_list.h"
#include "halley/ui/behaviours/ui_reload_ui_behaviour.h"
#include "halley/ui/widgets/ui_debug_console.h"
#include "halley/ui/widgets/ui_render_surface.h"
#include "halley/ui/widgets/ui_spin_control2.h"

using namespace Halley;

UIFactoryWidgetProperties::Entry::Entry(String label, String name, String type, Vector<String> defaultValue)
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
	addFactory("button", [=] (const ConfigNode& node) { return makeButton(node); }, getButtonProperties());
	addFactory("textInput", [=] (const ConfigNode& node) { return makeTextInput(node); }, getTextInputProperties());
	addFactory("spinControl", [=] (const ConfigNode& node) { return makeSpinControl(node); }, getSpinControlProperties());
	addFactory("spinControl2", [=] (const ConfigNode& node) { return makeSpinControl2(node); }, getSpinControl2Properties());
	addFactory("list", [=] (const ConfigNode& node) { return makeList(node); }, getListProperties());
	addFactory("dropdown", [=] (const ConfigNode& node) { return makeDropdown(node); }, getDropdownProperties());
	addFactory("checkbox", [=] (const ConfigNode& node) { return makeCheckbox(node); }, getCheckboxProperties());
	addFactory("image", [=] (const ConfigNode& node) { return makeImage(node); }, getImageProperties());
	addFactory("multiImage", [=](const ConfigNode& node) { return makeMultiImage(node); }, getMultiImageProperties());
	addFactory("animation", [=] (const ConfigNode& node) { return makeAnimation(node); }, getAnimationProperties());
	addFactory("scrollBar", [=] (const ConfigNode& node) { return makeScrollBar(node); }, getScrollBarProperties());
	addFactory("scrollPane", [=] (const ConfigNode& node) { return makeScrollPane(node); }, getScrollPaneProperties());
	addFactory("scrollBarPane", [=] (const ConfigNode& node) { return makeScrollBarPane(node); }, getScrollBarPaneProperties());
	addFactory("slider", [=] (const ConfigNode& node) { return makeSlider(node); }, getSliderProperties());
	addFactory("horizontalDiv", [=] (const ConfigNode& node) { return makeHorizontalDiv(node); }, getHorizontalDivProperties());
	addFactory("verticalDiv", [=] (const ConfigNode& node) { return makeVerticalDiv(node); }, getVerticalDivProperties());
	addFactory("tabbedPane", [=] (const ConfigNode& node) { return makeTabbedPane(node); }, getTabbedPaneProperties());
	addFactory("pagedPane", [=] (const ConfigNode& node) { return makePagedPane(node); }, getPagedPaneProperties());
	addFactory("framedImage", [=] (const ConfigNode& node) { return makeFramedImage(node); }, getFramedImageProperties());
	addFactory("hybridList", [=] (const ConfigNode& node) { return makeHybridList(node); }, getHybridListProperties());
	addFactory("spinList", [=](const ConfigNode& node) { return makeSpinList(node); }, getSpinListProperties());
	addFactory("optionListMorpher", [=](const ConfigNode& node) { return makeOptionListMorpher(node); }, getOptionListMorpherProperties());
	addFactory("treeList", [=](const ConfigNode& node) { return makeTreeList(node); }, getTreeListProperties());
	addFactory("debugConsole", [=](const ConfigNode& node) { return makeDebugConsole(node); }, getDebugConsoleProperties());
	addFactory("renderSurface", [=](const ConfigNode& node) { return makeRenderSurface(node); }, getRenderSurfaceProperties());
}

UIFactory::~UIFactory()
{
}

void UIFactory::loadStyleSheetsFromResources()
{
	styleSheet = std::make_shared<UIStyleSheet>(resources, colourScheme);
}

void UIFactory::addFactory(const String& key, WidgetFactory factory, UIFactoryWidgetProperties props)
{
	if (props.iconName.isEmpty()) {
		props.iconName = "widget_icons/generic.png";
	}
	if (props.name.isEmpty()) {
		props.name = key;
	}

	factories[key] = std::move(factory);
	properties[key] = std::move(props);
}

bool UIFactory::hasFactoryFor(const String& key) const
{
	return factories.contains(key);
}

std::shared_ptr<UIWidget> UIFactory::makeWidgetFromFactory(const String& key, const ConfigNode& config)
{
	auto iter = factories.find(key);
	if (iter != factories.end()) {
		return iter->second(config);
	}
	if (fallbackFactory) {
		fallbackFactory->makeWidgetFromFactory(key, config);
	}
	return {};
}

void UIFactory::setFallbackFactory(UIFactory& factory)
{
	fallbackFactory = &factory;
}

void UIFactory::pushConditions(Vector<String> conds)
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

std::shared_ptr<UIWidget> UIFactory::makeUIWithHotReload(const String& configName, IUIReloadObserver* observer)
{
	auto uiDefinition = resources.get<UIDefinition>(configName);
	auto ui = makeUI(*uiDefinition);
	if (api.core->isDevMode()) {
		ui->addBehaviour(std::make_shared<UIReloadUIBehaviour>(*this, ResourceObserver(*uiDefinition), observer));
	}
	return ui;
}

std::shared_ptr<UIWidget> UIFactory::makeUI(const String& configName, Vector<String> conditions)
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
	return std::dynamic_pointer_cast<UIWidget>(makeWidget(definition.getRoot()));
}

void UIFactory::loadUI(UIWidget& target, const String& configName, IUIReloadObserver* observer)
{
	loadUI(target, *resources.get<UIDefinition>(configName), observer);
}

void UIFactory::loadUI(UIWidget& target, const UIDefinition& uiDefinition, IUIReloadObserver* observer)
{
	try {
		target.add(makeUI(uiDefinition), 1);
		target.onMakeUI();
	} catch (const std::exception& e) {
		Logger::logException(e);
	}

	if (api.core->isDevMode()) {
		target.addBehaviour(std::make_shared<UIReloadUIBehaviour>(*this, ResourceObserver(uiDefinition), observer));
	}
}

const UIFactoryWidgetProperties& UIFactory::getPropertiesForWidget(const String& widgetClass) const
{
	const auto iter = properties.find(widgetClass);
	if (iter != properties.end()) {
		return iter->second;
	}
	if (fallbackFactory) {
		return fallbackFactory->getPropertiesForWidget(widgetClass);
	}
	throw Exception("Unknown widget type: " + widgetClass, HalleyExceptions::Entity);
}

Vector<String> UIFactory::getWidgetClassList(bool mustAllowChildren) const
{
	Vector<String> result;
	result.reserve(properties.size());
	for (auto& p: properties) {
		if (!mustAllowChildren || p.second.canHaveChildren) {
			result.push_back(p.first);
		}
	}

	if (fallbackFactory) {
		for (const auto& id: fallbackFactory->getWidgetClassList(mustAllowChildren)) {
			if (!std_ex::contains(result, id)) {
				result.push_back(id);
			}
		}
	}

	return result;
}

const HashMap<String, UIInputButtons>& UIFactory::getInputButtons() const
{
	return inputButtons;
}

void UIFactory::setInputButtons(HashMap<String, UIInputButtons> buttons)
{
	inputButtons = std::move(buttons);
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

std::unique_ptr<UIFactory> UIFactory::clone() const
{
	return cloneWithResources(resources);
}

std::unique_ptr<UIFactory> UIFactory::cloneWithResources(Resources& newResources) const
{
	auto result = make(api, newResources, i18n, styleSheet, colourScheme);
	result->inputButtons = inputButtons;
	return result;
}

std::unique_ptr<UIFactory> UIFactory::make(const HalleyAPI& api, Resources& resources, const I18N& i18n, std::shared_ptr<UIStyleSheet> styleSheet, std::shared_ptr<const UIColourScheme> colourScheme) const
{
	return std::make_unique<UIFactory>(api, resources, i18n, styleSheet, colourScheme);
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

void UIFactory::setConstructionCallback(ConstructionCallback callback)
{
	constructionCallback = std::move(callback);
}

UISizerAlignFlags::Type UIFactory::parseSizerAlignFlags(const ConfigNode& node, UISizerAlignFlags::Type defaultValue)
{
	int fill = 0;
	const int horizontalMask = UISizerAlignFlags::FillHorizontal | UISizerAlignFlags::Left | UISizerAlignFlags::CentreHorizontal | UISizerAlignFlags::Right;
	const int verticalMask = UISizerAlignFlags::FillVertical | UISizerAlignFlags::Top | UISizerAlignFlags::CentreVertical | UISizerAlignFlags::Bottom;

	auto addFill = [&] (const String& fillName)
	{
		int val = 0;
		if (fillName == "fill") {
			val = UISizerFillFlags::Fill;
		} else if (fillName == "fillHorizontal") {
			val = UISizerFillFlags::FillHorizontal;
		} else if (fillName == "fillVertical") {
			val = UISizerFillFlags::FillVertical;
		} else if (fillName == "centre") {
			val = UISizerAlignFlags::Centre;
		} else if (fillName == "left") {
			val = UISizerAlignFlags::Left;
		} else if (fillName == "right") {
			val = UISizerAlignFlags::Right;
		} else if (fillName == "top") {
			val = UISizerAlignFlags::Top;
		} else if (fillName == "bottom") {
			val = UISizerAlignFlags::Bottom;
		} else if (fillName == "centreHorizontal") {
			val = UISizerAlignFlags::CentreHorizontal;
		} else if (fillName == "centreVertical") {
			val = UISizerAlignFlags::CentreVertical;
		}

		if ((val & horizontalMask) != 0 && (fill & horizontalMask) == 0) {
			fill |= val;
		}
		if ((val & verticalMask) != 0 && (fill & verticalMask) == 0) {
			fill |= val;
		}
	};

	if (node.getType() == ConfigNodeType::String) {
		addFill(node.asString());
	} else if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& fillNode: node.asSequence()) {
			addFill(fillNode.asString());
		}
	}

	if (fill == 0) {
		fill = defaultValue;
	}

	return static_cast<UISizerAlignFlags::Type>(fill);
}

ConfigNode UIFactory::makeSizerAlignFlagsNode(UISizerAlignFlags::Type align)
{
	String horizontal;
	String vertical;

	if (align & UISizerAlignFlags::FillHorizontal) {
		horizontal = "fillHorizontal";
	} else if (align & UISizerAlignFlags::CentreHorizontal) {
		horizontal = "centreHorizontal";
	} else if (align & UISizerAlignFlags::Left) {
		horizontal = "left";
	} else if (align & UISizerAlignFlags::Right) {
		horizontal = "right";
	}

	if (align & UISizerAlignFlags::FillVertical) {
		vertical = "fillVertical";
	} else if (align & UISizerAlignFlags::CentreVertical) {
		vertical = "centreVertical";
	} else if (align & UISizerAlignFlags::Top) {
		vertical = "top";
	} else if (align & UISizerAlignFlags::Bottom) {
		vertical = "bottom";
	}

	ConfigNode::SequenceType result;
	result.push_back(ConfigNode(horizontal));
	result.push_back(ConfigNode(vertical));
	return result;
}

UISizerAlignFlags::Type UIFactory::normalizeDirection(UISizerAlignFlags::Type align, bool removeFill)
{
	int value = static_cast<int>(align);
	const int horizontalMask = UISizerAlignFlags::FillHorizontal | UISizerAlignFlags::Left | UISizerAlignFlags::CentreHorizontal | UISizerAlignFlags::Right;
	const int verticalMask = UISizerAlignFlags::FillVertical | UISizerAlignFlags::Top | UISizerAlignFlags::CentreVertical | UISizerAlignFlags::Bottom;

	if (removeFill) {
		value &= ~UISizerAlignFlags::Fill;
	}
	
	if ((value & horizontalMask) == 0) {
		value |= UISizerAlignFlags::CentreHorizontal;
	}
	if ((value & verticalMask) == 0) {
		value |= UISizerAlignFlags::CentreVertical;
	}

	return UISizerAlignFlags::Type(value);
}

std::shared_ptr<IUIElement> UIFactory::makeWidget(const ConfigNode& entryNode)
{
	std::shared_ptr<IUIElement> element;

	if (entryNode.hasKey("widget")) {
		styleSheet->updateIfNeeded();
		
		auto& widgetNode = entryNode["widget"];
		auto widgetClass = widgetNode["class"].asString();
		auto iter = factories.find(widgetClass);
		if (iter == factories.end()) {
			if (fallbackFactory) {
				iter = fallbackFactory->factories.find(widgetClass);
			}
		}
		if (iter == factories.end()) {
			throw Exception("Unknown widget class: " + widgetClass, HalleyExceptions::UI);
		}
		
		auto widget = iter->second(entryNode);
		if (widgetNode.hasKey("size")) {
			const auto size = widgetNode["size"].asVector2f({});
			if (size != Vector2f()) {
				widget->setMinSize(size);
			}
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

		element = widget;
	} else {
		auto sizer = makeSizer(entryNode);
		if (sizer) {
			element = std::make_shared<UISizer>(std::move(sizer.value()));
		}
	}

	if (element && constructionCallback) {
		constructionCallback(element, entryNode["uuid"].asString(""));
	}

	return element;
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
			sizer.setColumnProportions(sizerNode["columnProportions"].asVector<float>());
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

void UIFactory::loadSizerChildren(UISizer& sizer, const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Sequence) {
		for (auto& childNode: node.asSequence()) {
			float proportion = childNode["proportion"].asFloat(0);
			Vector4f border = childNode["border"].asVector4f(Vector4f());

			if (childNode.hasKey("widget") || childNode.hasKey("sizer") || childNode.hasKey("children")) {
				const auto fill = parseSizerAlignFlags(childNode["fill"]);
				sizer.add(makeWidget(childNode), proportion, border, fill);
			} else if (childNode.hasKey("spacer") || childNode.hasKey("stretchSpacer")) {
				auto& spacerNode = childNode[childNode.hasKey("spacer") ? "spacer" : "stretchSpacer"];
				const auto size = spacerNode["size"].asFloat(0);
				const auto size2d = Vector2f(sizer.getType() == UISizerType::Horizontal ? size : 0.0f, sizer.getType() == UISizerType::Vertical ? size : 0.0f);
				auto element = std::make_shared<UISizerSpacer>(size2d);
				
				if (constructionCallback) {
					constructionCallback(element, childNode["uuid"].asString(""));
				}

				sizer.add(std::move(element), proportion);
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

UIFactory::ParsedOption::ParsedOption(const ConfigNode& n)
{
	id = n["id"].asString("");
	text = n["text"].asString("");
	textKey = n["textKey"].asString("");
	tooltip = n["tooltip"].asString("");
	tooltipKey = n["tooltipKey"].asString("");

	if (id.isEmpty()) {
		id = text;
	}
	
	image = n["image"].asString("");
	imageColour = n.hasKey("iconColour") ? n["iconColour"].asString() : n["imageColour"].asString("");
	inactiveImage = n["inactiveImage"].asString("");
	spriteSheet = n["spriteSheet"].asString("");
	sprite = n["sprite"].asString("");
	border = n["border"].asVector4f(Vector4f());
	active = n["active"].asBool(true);
}

void UIFactory::ParsedOption::generateDisplay(const I18N& i18n)
{
	displayText = textKey.isEmpty() ? LocalisedString::fromUserString(text) : i18n.get(textKey);
	displayTooltip = tooltipKey.isEmpty() ? LocalisedString::fromUserString(tooltip) : i18n.get(tooltipKey);
}

Vector<UIFactory::ParsedOption> UIFactory::parseOptions(const ConfigNode& node)
{
	Vector<ParsedOption> result = node.asVector<ParsedOption>();
	for (auto& n: result) {
		n.generateDisplay(i18n);
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
	result.entries.emplace_back("Inner Border", "innerBorder", "Halley::Vector4f", Vector<String>{"", "", "", ""});
	result.name = "Widget";
	result.iconName = "widget_icons/widget.png";
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
		if (node["marquee"].getType() == ConfigNodeType::Bool) {
			label->setMarquee(node["marquee"].asBool() ? std::optional<float>(10.0f) : std::nullopt);
		} else {
			label->setMarquee(node["marquee"].asFloat());
		}
	}
	if (node.hasKey("colour")) {
		label->setColour(getColour(node["colour"].asString()));
	}
	if (node.hasKey("fontSize")) {
		label->setFontSize(node["fontSize"].asFloat());
	}
	return label;
}

UIFactoryWidgetProperties UIFactory::getLabelProperties() const
{
	UIFactoryWidgetProperties result;
	result.canHaveChildren = false;
	result.entries.emplace_back("Text", "text", "Halley::String", "");
	result.entries.emplace_back("Text (Loc Key)", "textKey", "Halley::String", "");
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<label>", "label");
	result.entries.emplace_back("Max Width", "maxWidth", "std::optional<float>", "");
	result.entries.emplace_back("Max Height", "maxHeight", "std::optional<float>", "");
	result.entries.emplace_back("Alignment", "alignment", "std::optional<float>", "");
	result.entries.emplace_back("Font Size", "fontSize", "std::optional<float>", "");
	result.entries.emplace_back("Marquee", "marquee", "std::optional<float>", "");
	result.entries.emplace_back("Word Wrap", "wordWrapped", "bool", "");
	result.entries.emplace_back("Colour", "colour", "std::optional<Halley::Colour4f>", "");

	result.name = "Label";
	result.iconName = "widget_icons/label.png";

	return result;
}

UIFactoryWidgetProperties UIFactory::getSpinControlProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Spin Control";
	result.canHaveChildren = false;
	result.iconName = "widget_icons/spinControl.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getMultiImageProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Multi-Image";
	result.iconName = "widget_icons/multiImage.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getSliderProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Slider";
	result.canHaveChildren = false;
	result.iconName = "widget_icons/slider.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getFramedImageProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Framed Image";
	result.iconName = "widget_icons/framedImage.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getHybridListProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Hybrid List";
	result.canHaveChildren = false;
	result.iconName = "widget_icons/hybridList.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getSpinListProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Spin List";
	result.canHaveChildren = false;
	//result.iconName = "widget_icons/spinList.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getOptionListMorpherProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Option List Morpher";
	result.canHaveChildren = false;
	//result.iconName = "widget_icons/optionListMorpher.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getDebugConsoleProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Debug Console";
	result.canHaveChildren = false;
	result.iconName = "widget_icons/debugConsole.png";
	return result;
}

UIFactoryWidgetProperties UIFactory::getTreeListProperties() const
{
	UIFactoryWidgetProperties result = getBaseListProperties();
	result.name = "Tree List";
	result.iconName = "widget_icons/treeList.png";
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeButton(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
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

UIFactoryWidgetProperties UIFactory::getButtonProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Button";
	result.iconName = "widget_icons/button.png";
	result.canHaveChildren = true;
	result.entries.emplace_back("Text", "text", "Halley::String", "");
	result.entries.emplace_back("Text (Loc Key)", "textKey", "Halley::String", "");
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<button>", "button");
	result.entries.emplace_back("Icon", "icon", "Halley::ResourceReference<Halley::SpriteResource>", "");
	result.entries.emplace_back("Mouse Border", "mouseBorder", "Halley::Vector4f", Vector<String>{"0", "0", "0", "0"});
	return result;
}

UIFactoryWidgetProperties UIFactory::getTextInputProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Text Input";
	result.iconName = "widget_icons/textInput.png";
	result.canHaveChildren = false;

	result.entries.emplace_back("Max Length", "maxLength", "std::optional<int>", "");
	result.entries.emplace_back("History", "history", "bool", "false");
	result.entries.emplace_back("Read Only", "readOnly", "bool", "false");
	result.entries.emplace_back("Multi Line", "multiLine", "bool", "false");
	result.entries.emplace_back("Clear on Submit", "clearOnSubmit", "bool", "false");
	result.entries.emplace_back("Ghost Text", "ghost", "Halley::String", "");
	result.entries.emplace_back("Show Ghost when Focused", "showGhostWhenFocused", "bool", "false");
	result.entries.emplace_back("Select All on Click", "selectAllOnClick", "bool", "false");
	result.entries.emplace_back("Auto Size", "autoSize", "std::optional<Halley::Range<float>>", "");
	result.entries.emplace_back("Auto Size Horizontally", "autoSizeHorizontal", "bool", "true");

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeTextInput(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("input"), styleSheet);
	auto label = parseLabel(node);
	auto ghostText = parseLabel(node, "", "ghost");

	auto result = std::make_shared<UITextInput>(id, style, "", label);
	if (!ghostText.getString().isEmpty()) {
		result->setGhostText(ghostText);
	}

	auto validatorName = node["validator"].asString("");
	if (!validatorName.isEmpty()) {
		if (validatorName == "numeric") {
			result->setValidator(std::make_shared<UINumericValidator>(true, true));
		} else if (validatorName == "numericPositive") {
			result->setValidator(std::make_shared<UINumericValidator>(false, true));
		} else if (validatorName == "integer") {
			result->setValidator(std::make_shared<UINumericValidator>(true, false));
		} else if (validatorName == "integerPositive") {
			result->setValidator(std::make_shared<UINumericValidator>(false, false));
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
	result->setShowGhostWhenFocused(node["showGhostWhenFocused"].asBool(false));
	result->setMultiLine(node["multiLine"].asBool(false));
	result->setSelectAllOnClick(node["selectAllOnClick"].asBool(false));

	if (node.hasKey("autoSize")) {
		result->setAutoSize(node["autoSize"].asFloatRange(), node["autoSizeHorizontal"].asBool(true));
	}

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

UIFactoryWidgetProperties UIFactory::getSpinControl2Properties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Spin Control 2";
	result.iconName = "widget_icons/spinControl.png";
	result.canHaveChildren = false;

	result.entries.emplace_back("Allow Float", "allowFloat", "bool", "false");
	result.entries.emplace_back("Min Value", "minValue", "std::optional<float>", "");
	result.entries.emplace_back("Max Value", "maxValue", "std::optional<float>", "");
	result.entries.emplace_back("Increment", "increment", "std::optional<float>", "");
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<spinControl>", "label");

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

UIFactoryWidgetProperties UIFactory::getDropdownProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Dropdown";
	result.iconName = "widget_icons/dropdown.png";
	result.canHaveChildren = false;

	result.entries.emplace_back("Style", "style", "Halley::UIStyle<dropdown>", "dropdown");
	result.entries.emplace_back("Options", "options", "Halley::Vector<Halley::UIFactory::ParsedOption>", "");
	result.entries.emplace_back("Input Buttons", "inputButtons", "Halley::String", "list");
	result.entries.emplace_back("Notify on Hover", "notifyOnHover", "bool", "");

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeDropdown(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("dropdown"), styleSheet);
	auto label = parseLabel(node);
	auto options = parseOptions(node["options"]);

	Vector<UIDropdown::Entry> entries;
	entries.reserve(options.size());
	for (auto& o: options) {
		Sprite icon;
		if (!o.image.isEmpty()) {
			icon = Sprite().setImage(getResources(), o.image).setColour(getColour(o.imageColour.isEmpty() ? "#FFFFFF" : o.imageColour));
		}
		entries.emplace_back(o.id, o.displayText, icon);
	}

	auto widget = std::make_shared<UIDropdown>(id, style);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	widget->setOptions(entries);

	widget->setNotifyOnHover(node["notifyOnHover"].asBool(false));

	return widget;
}

UIFactoryWidgetProperties UIFactory::getCheckboxProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Checkbox";
	result.iconName = "widget_icons/checkbox.png";
	result.canHaveChildren = false;

	result.entries.emplace_back("Style", "style", "Halley::UIStyle<checkbox>", "checkbox");
	result.entries.emplace_back("Checked", "checked", "bool", "false");
	result.entries.emplace_back("Label", "label", "Halley::String", "");
	result.entries.emplace_back("Label (Loc Key)", "labelKey", "Halley::String", "");

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeCheckbox(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("checkbox"), styleSheet);
	const auto checked = node["checked"].asBool(false);
	auto label = parseLabel(node, "", "label");

	return std::make_shared<UICheckbox>(std::move(id), std::move(style), checked, std::move(label));
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
		const auto imageName = node["image"].asString();
		if (imageName.isEmpty()) {
			sprite = {};
		} else if (colourScheme) {
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

	if (node.hasKey("interactWithMouse")) {
		image->setInteractWithMouse(node["interactWithMouse"].asBool(false));
	}

	return image;
}

UIFactoryWidgetProperties UIFactory::getImageProperties() const
{
	UIFactoryWidgetProperties result;
	result.entries.emplace_back("Image", "image", "Halley::ResourceReference<Halley::SpriteResource>", "");
	result.entries.emplace_back("Material", "material", "Halley::ResourceReference<Halley::MaterialDefinition>", MaterialDefinition::defaultMaterial);
	result.entries.emplace_back("Colour", "colour", "Halley::UIColour", "#FFFFFF");
	result.entries.emplace_back("Flip", "flip", "bool", "false");
	result.entries.emplace_back("Pivot", "pivot", "std::optional<Halley::Vector2f>", "");
	result.entries.emplace_back("Rotation", "rotation", "Halley::Angle1f", "0");
	result.entries.emplace_back("Layer Adjustment", "layerAdjustment", "std::optional<int>", "");
	result.entries.emplace_back("Interact with Mouse", "interactWithMouse", "bool", "false");
	result.entries.emplace_back("Inner Border", "innerBorder", "Halley::Vector4f", "");

	result.name = "Image";
	result.iconName = "widget_icons/image.png";
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeMultiImage(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	const auto id = node["id"].asString("");
	const auto size = node["size"].asVector2f(Vector2f());
	const auto materialName = node["material"].asString("");

	Vector<Sprite> sprites = {};
	Vector<Vector2f> offsets = {};
	
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

UIFactoryWidgetProperties UIFactory::getAnimationProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Animation";
	result.iconName = "widget_icons/animation.png";
	result.canHaveChildren = true;

	result.entries.emplace_back("Animation", "animation", "Halley::ResourceReference<Halley::Animation>", "");
	result.entries.emplace_back("Sequence", "sequence", "Halley::String", "default");
	result.entries.emplace_back("Direction", "direction", "Halley::String", "default");
	result.entries.emplace_back("Offset", "offset", "std::optional<Halley::Vector2f>", "");
	return result;
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

	return std::make_shared<UIAnimation>(id, size, makeSizer(entryNode), animationOffset, animation);
}

UIFactoryWidgetProperties UIFactory::getScrollPaneProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Scroll Pane";
	//result.iconName = "widget_icons/scrollPane.png";

	result.entries.emplace_back("Clip Size", "clipSize", "Halley::Vector2f", "");
	result.entries.emplace_back("Scroll Horizontal", "scrollHorizontal", "bool", "false");
	result.entries.emplace_back("Scroll Vertical", "scrollVertical", "bool", "true");
	result.entries.emplace_back("Scroll Speed", "scrollSpeed", "float", "50");
	result.entries.emplace_back("Smooth Go To", "smoothGoTo", "bool", "false");
	result.entries.emplace_back("Mouse Wheel", "mouseWheelEnabled", "bool", "true");

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeScrollPane(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString("");
	auto clipSize = node["clipSize"].asVector2f(Vector2f());
	auto scrollHorizontal = node["scrollHorizontal"].asBool(false);
	auto scrollVertical = node["scrollVertical"].asBool(true);
	auto mouseWheelEnabled = node["mouseWheelEnabled"].asBool(true);
	auto scrollSpeed = node["scrollSpeed"].asFloat(50.0f);
	auto smoothGoTo = node["smoothGoTo"].asBool(false);

	auto result = std::make_shared<UIScrollPane>(id, clipSize, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical, scrollSpeed, smoothGoTo);
	result->setScrollWheelEnabled(mouseWheelEnabled);
	return result;
}

UIFactoryWidgetProperties UIFactory::getScrollBarProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Scrollbar";
	result.iconName = "widget_icons/scrollBar.png";
	result.canHaveChildren = false;

	result.entries.emplace_back("Scroll Direction", "scrollDirection", "Halley::UIScrollDirection", "vertical");
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<scrollbar>", "scrollbar");
	result.entries.emplace_back("Auto Hide", "autoHide", "bool", "false");
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

UIFactoryWidgetProperties UIFactory::getScrollBarPaneProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Scrollbar Pane";
	//result.iconName = "widget_icons/dropdown.png";

	result.entries.emplace_back("Clip Size", "clipSize", "Halley::Vector2f", "");
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<scrollbar>", "scrollbar");
	result.entries.emplace_back("Scroll Horizontal", "scrollHorizontal", "bool", "false");
	result.entries.emplace_back("Scroll Vertical", "scrollVertical", "bool", "true");
	result.entries.emplace_back("Scroll Speed", "scrollSpeed", "float", "50");
	result.entries.emplace_back("Smooth Go To", "smoothGoTo", "bool", "false");
	result.entries.emplace_back("Auto Hide", "autoHide", "bool", "false");
	result.entries.emplace_back("Mouse Wheel", "mouseWheelEnabled", "bool", "true");

	return result;
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
	auto scrollSpeed = node["scrollSpeed"].asFloat(50.0f);
	auto smoothGoTo = node["smoothGoTo"].asBool(false);

	auto result = std::make_shared<UIScrollBarPane>(id, clipSize, style, makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)), scrollHorizontal, scrollVertical, alwaysShow, Vector2f(), scrollSpeed, smoothGoTo);
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
	bool hasSpinControl = node["hasSpinControl"].asBool(false);

	auto slider = std::make_shared<UISlider>(id, style, minValue, maxValue, value, hasSpinControl);
	if (node.hasKey("granularity")) {
		slider->setGranularity(node["granularity"].asFloat());
	}
	slider->setMouseWheelSpeed(node["mouseWheelSpeed"].asFloat(1.0f));
	slider->setShowLabel(node["showLabel"].asBool(true));

	return slider;
}

UIFactoryWidgetProperties UIFactory::getHorizontalDivProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Horizontal Divider";
	result.iconName = "widget_icons/horizontalDiv.png";
	result.canHaveChildren = false;
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<horizontalDiv>", "horizontalDiv");
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeHorizontalDiv(const ConfigNode& entryNode)
{
	return makeDivider(entryNode, UISizerType::Horizontal);
}

UIFactoryWidgetProperties UIFactory::getVerticalDivProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Vertical Divider";
	result.iconName = "widget_icons/verticalDiv.png";
	result.canHaveChildren = false;
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<verticalDiv>", "verticalDiv");
	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeVerticalDiv(const ConfigNode& entryNode)
{
	return makeDivider(entryNode, UISizerType::Vertical);
}

std::shared_ptr<UIWidget> UIFactory::makeDivider(const ConfigNode& entryNode, UISizerType type)
{
	const auto& widgetNode = entryNode["widget"];
	auto id = widgetNode["id"].asString("");
	const auto& style = getStyle(widgetNode["style"].asString(type == UISizerType::Horizontal ? "horizontalDiv" : "verticalDiv"));

	if (style.hasSprite("midImage")) {
		const auto gap = style.getFloat("gap", 0);

		const int align = type == UISizerType::Horizontal
			? (UISizerAlignFlags::CentreVertical | UISizerFillFlags::FillHorizontal)
			: (UISizerAlignFlags::CentreHorizontal | UISizerFillFlags::FillVertical);

		auto result = std::make_shared<UIWidget>(std::move(id), Vector2f(), UISizer(type, gap));
		result->add(std::make_shared<UIImage>("", style.getSprite("image")), 1, {}, align);
		result->add(std::make_shared<UIImage>("", style.getSprite("midImage")), 0, {}, UISizerAlignFlags::Centre);
		result->add(std::make_shared<UIImage>("", style.getSprite("image")), 1, {}, align);
		return result;
	} else {
		return std::make_shared<UIImage>(std::move(id), style.getSprite("image"));
	}
}

std::shared_ptr<UIWidget> UIFactory::makeTabbedPane(const ConfigNode& entryNode)
{
	const auto& widgetNode = entryNode["widget"];
	auto id = widgetNode["id"].asString();
	auto style = widgetNode["style"].asString("tabs");
	auto tabs = std::make_shared<UIList>(id, getStyle(style), UISizerType::Horizontal, 1);
	applyInputButtons(*tabs, widgetNode["inputButtons"].asString("tabs"));

	Vector<const ConfigNode*> tabNodes;
	auto loadChildren = [&] (const ConfigNode& root)
	{
		for (auto& tabNode: root.asSequence()) {
			if (tabNode.hasKey("if")) {
				if (!resolveConditions(tabNode["if"])) {
					continue;
				}
			}
			auto label = parseLabel(tabNode);
			tabs->addTextItem(tabNode["id"].asString(id + "_tab_" + toString(tabNodes.size())), label);
			tabNodes.push_back(&tabNode);
		}
	};

	if (widgetNode.hasKey("tabs")) {
		loadChildren(widgetNode["tabs"]);
	} else if (entryNode.hasKey("children")) {
		loadChildren(entryNode["children"]);
	}

	auto pane = std::make_shared<UIPagedPane>(id + "_pagedPane", int(tabNodes.size()), Vector2f());
	for (int i = 0; i < int(tabNodes.size()); ++i) {
		auto& tabNode = *tabNodes[i];
		pane->getPage(i)->add(makeWidget(tabNode), 1);
	}

	tabs->setHandle(UIEventType::ListSelectionChanged, [pane, tabs] (const UIEvent& event)
	{
		const auto& item = tabs->getItem(event.getStringData());
		pane->setPage(item->getAbsoluteIndex());
		event.getCurWidget().sendEvent(UIEvent(UIEventType::TabChanged, event.getSourceId(), event.getIntData()));
	});

	auto result = std::make_shared<UIWidget>(id + "_container", Vector2f(), UISizer(UISizerType::Vertical));
	result->add(tabs);
	result->add(pane, 1);
	return result;
}

UIFactoryWidgetProperties UIFactory::getTabbedPaneProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Tabbed Pane";
	result.iconName = "widget_icons/tabbedPane.png";
	result.childName = "Tab";

	result.entries.emplace_back("Style", "style", "Halley::UIStyle<list>", "tabs");

	result.childEntries.emplace_back("Id", "id", "Halley::String", "");
	result.childEntries.emplace_back("Text", "text", "Halley::String", "");
	result.childEntries.emplace_back("Text (Key)", "textKey", "Halley::String", "");
	result.childEntries.emplace_back("If", "if", "Vector<Halley::String>", Vector<String>());

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makePagedPane(const ConfigNode& entryNode)
{
	const auto& widgetNode = entryNode["widget"];

	Vector<const ConfigNode*> pageNodes;
	auto loadChildren = [&] (const ConfigNode& root)
	{
		for (auto& pageNode: root.asSequence()) {
			if (pageNode.hasKey("if")) {
				if (!resolveConditions(pageNode["if"])) {
					continue;
				}
			}
			pageNodes.push_back(&pageNode);
		}
	};

	if (widgetNode.hasKey("pages")) {
		loadChildren(widgetNode["pages"]);
	} else if (entryNode.hasKey("children")) {
		loadChildren(entryNode["children"]);
	}

	auto pane = std::make_shared<UIPagedPane>(widgetNode["id"].asString(), int(pageNodes.size()));
	for (int i = 0; i < int(pageNodes.size()); ++i) {
		pane->getPage(i)->add(makeWidget(*pageNodes[i]), 1);
	}

	return pane;
}

UIFactoryWidgetProperties UIFactory::getPagedPaneProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Paged Pane";
	result.iconName = "widget_icons/pagedPane.png";
	result.childName = "Page";

	result.childEntries.emplace_back("Id", "id", "Halley::String", "");
	result.childEntries.emplace_back("Text", "text", "Halley::String", "");
	result.childEntries.emplace_back("Text (Key)", "textKey", "Halley::String", "");
	result.childEntries.emplace_back("If", "if", "Vector<Halley::String>", Vector<String>());

	return result;
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

UIFactoryWidgetProperties UIFactory::getListProperties() const
{
	UIFactoryWidgetProperties result = getBaseListProperties();

	result.entries.emplace_back("Columns", "columns", "int", "1");
	result.entries.emplace_back("Type", "type", "Halley::UISizerType", "vertical");
	result.entries.emplace_back("Text", "text", "Halley::String", "");
	result.entries.emplace_back("Style", "style", "Halley::UIStyle<list>", "list");

	return result;
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
	auto style = getStyle(widgetNode["style"].asString("hybridList"));
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

	Vector<String> optionIds;
	Vector<LocalisedString> optionLabels;
	for (auto& o : options) {
		optionIds.push_back(o.id);
		optionLabels.push_back(o.displayText);
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

	Vector<String> optionIds;
	Vector<LocalisedString> optionLabels;
	for (auto& o : options) {
		optionIds.push_back(o.id);
		optionLabels.push_back(o.displayText);
	}

	auto widget = std::make_shared<UIOptionListMorpher>(id, dropdownStyle, spinlistStyle);
	applyInputButtons(*widget, node["inputButtons"].asString("list"));
	widget->setOptions(optionIds, optionLabels);
	return widget;
}

std::shared_ptr<UIWidget> UIFactory::makeTreeList(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	auto style = UIStyle(node["style"].asString("treeList"), styleSheet);
	auto label = parseLabel(node);

	auto widget = std::make_shared<UITreeList>(id, style);
	widget->setSingleRoot(node["singleRoot"].asBool(false));
	applyListProperties(*widget, node, "treeList");

	return widget;
}

void UIFactory::applyListProperties(UIList& list, const ConfigNode& node, const String& inputConfigName)
{
	applyInputButtons(list, node["inputButtons"].asString(inputConfigName));

	const auto options = parseOptions(node["options"]);
	for (const auto& o: options) {
		if (!o.image.isEmpty() || !o.sprite.isEmpty()) {
			Sprite normalSprite;
			
			if (!o.image.isEmpty()) {
				normalSprite.setImage(resources, o.image);
			} else {
				normalSprite.setSprite(resources, o.spriteSheet, o.sprite);
			}

			if (!o.imageColour.isEmpty()) {
				normalSprite.setColour(Colour4f(o.imageColour));
			}
			auto image = std::make_shared<UIImage>(normalSprite);

			if (!o.inactiveImage.isEmpty()) {
				Sprite inactiveSprite = Sprite().setImage(resources, o.inactiveImage);
				image->setSelectable(inactiveSprite, normalSprite);
				image->setSprite(inactiveSprite);
			}

			list.addImage(o.id, image, 1, o.border, UISizerAlignFlags::Centre);
		} else {
			list.addTextItem(o.id, o.displayText);
		}

		if (!o.displayTooltip.getString().isEmpty()) {
			list.getItem(o.id)->setToolTip(o.displayTooltip);
		}

		list.setItemActive(o.id, o.active);
	}

	list.setDragEnabled(node["canDrag"].asBool(false));
	list.setUniformSizedItems(node["uniformSizedItems"].asBool(false));
	list.setSingleClickAccept(node["singleClickAccept"].asBool(true));
	list.setMultiSelect(node["multiSelect"].asBool(false));
	list.setAcceptKeyboardInput(node["acceptKeyboardInput"].asBool(true));
	list.setFocusable(node["focusable"].asBool(true));
	list.setRequiresSelection(node["requiresSelection"].asBool(true));
	list.setShowSelection(node["showSelection"].asBool(true));
}

UIFactoryWidgetProperties UIFactory::getBaseListProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "List";
	result.iconName = "widget_icons/list.png";
	result.canHaveChildren = false;

	result.entries.emplace_back("Can Drag", "canDrag", "bool", "false");
	result.entries.emplace_back("Multi-select", "multiSelect", "bool", "false");
	result.entries.emplace_back("Single Click Accept", "singleClickAccept", "bool", "true");
	result.entries.emplace_back("Uniform Sized Items", "uniformSizedItems", "bool", "false");
	result.entries.emplace_back("Accept Keyboard Input", "acceptKeyboardInput", "bool", "true");
	result.entries.emplace_back("Focusable", "focusable", "bool", "true");
	result.entries.emplace_back("Requires Selection", "requiresSelection", "bool", "true");
	result.entries.emplace_back("Show Selection", "showSelection", "bool", "true");
	result.entries.emplace_back("Input Buttons", "inputButtons", "Halley::String", "list");

	result.entries.emplace_back("Options", "options", "Halley::Vector<Halley::UIFactory::ParsedOption>", "");

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeDebugConsole(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	
	auto widget = std::make_shared<UIDebugConsole>(id, *this, std::make_shared<UIDebugConsoleController>(getResources(), api));

	return widget;
}

UIFactoryWidgetProperties UIFactory::getRenderSurfaceProperties() const
{
	UIFactoryWidgetProperties result;
	result.name = "Render Surface";
	result.iconName = "widget_icons/render_surface.png";

	result.entries.emplace_back("Material", "material", "Halley::ResourceReference<Halley::MaterialDefinition>", MaterialDefinition::defaultMaterial);
	result.entries.emplace_back("Use Filtering", "useFilter", "bool", "false");
	result.entries.emplace_back("Colour", "colour", "Halley::UIColour", "#FFFFFF");
	result.entries.emplace_back("Scale", "scale", "Halley::Vector2f", Vector<String>{ "1", "1" });

	return result;
}

std::shared_ptr<UIWidget> UIFactory::makeRenderSurface(const ConfigNode& entryNode)
{
	const auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	const auto material = node["material"].asString("Halley/Sprite");
	const auto useFilter = node["useFilter"].asBool(false);

	RenderSurfaceOptions options;
	options.useFiltering = useFilter;
	options.canBeUpdatedOnCPU = false;
	options.createDepthStencil = false;
	options.mipMap = false;
	options.powerOfTwo = false;

	const auto colour = Colour4f::fromString(node["colour"].asString("#FFFFFF"));
	const auto scale = node["scale"].asVector2f(Vector2f(1, 1));
	
	auto widget = std::make_shared<UIRenderSurface>(std::move(id), Vector2f(), makeSizer(entryNode), api, resources, material, options);
	widget->setColour(colour);
	widget->setScale(scale);

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
	const String defaultColourScheme = "colour_schemes/flat_halley_dark";
	if (resources.exists<ConfigFile>(defaultColourScheme)) {
		colourScheme = std::make_shared<UIColourScheme>(resources.get<ConfigFile>(defaultColourScheme)->getRoot(), resources);
	}
}

void UIFactory::setColourScheme(const String& assetId)
{
	if (resources.exists<ConfigFile>(assetId)) {
		colourScheme = std::make_shared<UIColourScheme>(resources.get<ConfigFile>(assetId)->getRoot(), resources);
	}
}
