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
	uiEditor->onWidgetModified();
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
	throw Exception("Not implemented", 0);
}

void UIWidgetEditor::refresh()
{
	auto widgetBox = getWidget("widgetBox");
	auto fillBox = getWidget("fillBox");
	auto sizerBox = getWidget("sizerBox");
	auto classLabel = getWidgetAs<UILabel>("classLabel");

	if (curNode && entityFieldFactory) {
		if (curNode->hasKey("widget")) {
			auto& widgetNode = (*curNode)["widget"];
			const auto widgetClass = widgetNode["class"].asString();
			classLabel->setText(LocalisedString::fromUserString(widgetClass));
			widgetBox->setActive(true);

			populateWidgetBox(*widgetBox->getWidget("widgetContents"), widgetNode);
		} else {
			classLabel->setText(LocalisedString::fromUserString("sizer"));
			widgetBox->setActive(false);
		}

		fillBox->setActive(true);
		sizerBox->setActive(true);

		populateFillBox(*fillBox->getWidget("fillContents"), *curNode);
		populateSizerBox(*sizerBox->getWidget("sizerContents"), (*curNode)["sizer"]);
	} else {
		classLabel->setText(LocalisedString());
		widgetBox->setActive(false);
		fillBox->setActive(false);
		sizerBox->setActive(false);
	}
}

void UIWidgetEditor::populateWidgetBox(UIWidget& root, ConfigNode& node)
{
	root.clear();

	populateBox(root, node, uiEditor->getGameFactory().getPropertiesForWidget(node["class"].asString()).entries);
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
