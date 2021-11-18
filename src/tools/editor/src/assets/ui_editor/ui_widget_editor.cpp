#include "ui_widget_editor.h"

#include "ui_editor.h"
#include "src/scene/entity_editor.h"

using namespace Halley;

UIWidgetEditor::UIWidgetEditor(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(300, 100), UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/ui_widget_editor");
}

void UIWidgetEditor::setSelectedWidget(const String& id, ConfigNode* node)
{
	curId = id;
	curNode = node;
	refresh();
}

void UIWidgetEditor::setGameResources(Resources& resources)
{
	entityFieldFactory = std::make_shared<EntityEditorFactory>(factory);
	entityFieldFactory->setGameResources(resources);
	entityFieldFactory->setCallbacks(*this);
	refresh();
}

void UIWidgetEditor::setUIEditor(UIEditor& editor)
{
	uiEditor = &editor;
}

void UIWidgetEditor::onEntityUpdated()
{
	uiEditor->onWidgetModified(curId);
}

void UIWidgetEditor::reloadEntity()
{
}

void UIWidgetEditor::setTool(const String& tool, const String& componentName, const String& fieldName)
{
}

void UIWidgetEditor::setDefaultName(const String& name, const String& prevName)
{
}

ISceneEditorWindow& UIWidgetEditor::getSceneEditorWindow() const
{
	ISceneEditorWindow* badPtr = nullptr;
	return *badPtr;
}

void UIWidgetEditor::refresh()
{
	auto widgetBox = getWidget("widgetBox");
	auto genericWidgetBox = getWidget("genericWidgetBox");
	auto fillBox = getWidget("fillBox");
	auto sizerBox = getWidget("sizerBox");

	if (curNode && entityFieldFactory) {
		bool hasSizer = true;
		if (curNode->hasKey("widget")) {
			auto& widgetNode = (*curNode)["widget"];
			const auto widgetClass = widgetNode["class"].asString();

			if (widgetClass != "widget") {
				widgetBox->setActive(true);
				const auto properties = uiEditor->getGameFactory().getPropertiesForWidget(widgetNode["class"].asString());
				populateWidgetBox(*widgetBox->getWidget("widgetContents"), widgetNode, properties);
				hasSizer = properties.canHaveChildren;
			} else {
				widgetBox->setActive(false);
			}

			genericWidgetBox->setActive(true);
			populateGenericWidgetBox(*genericWidgetBox->getWidget("genericWidgetContents"), widgetNode);
		} else {
			widgetBox->setActive(false);
			genericWidgetBox->setActive(false);
		}

		fillBox->setActive(true);
		populateFillBox(*fillBox->getWidget("fillContents"), *curNode);

		sizerBox->setActive(hasSizer);
		if (hasSizer) {
			populateSizerBox(*sizerBox->getWidget("sizerContents"), (*curNode)["sizer"]);
		}
	} else {
		widgetBox->setActive(false);
		genericWidgetBox->setActive(false);
		fillBox->setActive(false);
		sizerBox->setActive(false);
	}
}

void UIWidgetEditor::populateWidgetBox(UIWidget& root, ConfigNode& node, const UIFactoryWidgetProperties& properties)
{
	root.clear();

	auto icon = getWidgetAs<UIImage>("classIcon");
	auto label = getWidgetAs<UILabel>("classLabel");
	label->setText(LocalisedString::fromUserString(properties.name));
	icon->setSprite(Sprite().setImage(uiEditor->getGameFactory().getResources(), properties.iconName));

	populateBox(root, node, properties.entries);
}

void UIWidgetEditor::populateGenericWidgetBox(UIWidget& root, ConfigNode& node)
{
	root.clear();

	populateBox(root, node, uiEditor->getGameFactory().getGlobalWidgetProperties().entries);
}

void UIWidgetEditor::populateFillBox(UIWidget& root, ConfigNode& node)
{
	root.clear();

	using Entry = UIFactoryWidgetProperties::Entry;
	std::array<Entry, 3> entries = {
		Entry{ "Fill", "fill", "Halley::String", std::vector<String>{"fill"} },
		Entry{ "Proportion", "proportion", "int", std::vector<String>{"0"} },
		Entry{ "Border", "border", "Halley::Vector4f", std::vector<String>{"0", "0", "0", "0"}}
	};
	populateBox(root, node, entries);
}

void UIWidgetEditor::populateSizerBox(UIWidget& root, ConfigNode& node)
{
	node.ensureType(ConfigNodeType::Map);
	root.clear();

	using Entry = UIFactoryWidgetProperties::Entry;
	std::array<Entry, 4> entries = {
		Entry{ "Type", "type", "Halley::String", std::vector<String>{"horizontal"} },
		Entry{ "Gap", "gap", "float", std::vector<String>{"1"} },
		Entry{ "Columns", "columns", "int", std::vector<String>{"1"}},
		Entry{ "Column Proportions", "columnProportions", "std::vector<int>", std::vector<String>{}}
	};
	populateBox(root, node, entries);
}

void UIWidgetEditor::populateBox(UIWidget& root, ConfigNode& node, gsl::span<const UIFactoryWidgetProperties::Entry> entries)
{
	for (const auto& e: entries) {
		const auto params = ComponentFieldParameters("", ComponentDataRetriever(node, e.name, e.label), e.defaultValue);
		auto field = entityFieldFactory->makeField(e.type, params, ComponentEditorLabelCreation::Always);
		root.add(field);
	}
}
