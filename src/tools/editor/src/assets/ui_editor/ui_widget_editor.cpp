#include "ui_widget_editor.h"

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

void UIWidgetEditor::refresh()
{
	auto widgetBox = getWidget("widgetBox");
	auto fillBox = getWidget("fillBox");
	auto sizerBox = getWidget("sizerBox");
	auto classLabel = getWidgetAs<UILabel>("classLabel");

	if (curNode) {
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
	} else {
		classLabel->setText(LocalisedString());
		widgetBox->setActive(false);
		fillBox->setActive(false);
		sizerBox->setActive(false);
	}
}

void UIWidgetEditor::populateWidgetBox(UIWidget& root, ConfigNode& widgetNode)
{
	root.clear();

	// TODO
}
