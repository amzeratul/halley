#include "ui_widget_list.h"

using namespace Halley;

UIWidgetList::UIWidgetList(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 100), UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/ui_widget_list");
}

void UIWidgetList::onMakeUI()
{
	list = getWidgetAs<UITreeList>("widgetsList");
	populateList();
}

void UIWidgetList::setDefinition(std::shared_ptr<const UIDefinition> def)
{
	definition = std::move(def);
	populateList();
}

void UIWidgetList::populateList()
{
	if (list && definition) {
		populateList(definition->getRoot(), "");
	}
}

void UIWidgetList::populateList(const ConfigNode& curNode, String parentId)
{
	const String id = UUID::generate().toString();
	String name;

	if (curNode.hasKey("widget")) {
		const auto& widgetNode = curNode["widget"];
		name = widgetNode["class"].asString();
		if (widgetNode.hasKey("id")) {
			name += ":" + widgetNode["id"].asString();
		}
	} else {
		name = "sizer";
	}

	list->addTreeItem(id, parentId, -1, LocalisedString::fromUserString(name));

	if (curNode.hasKey("children")) {
		for (const auto& c: curNode["children"].asSequence()) {
			populateList(c, id);
		}
	}
}
