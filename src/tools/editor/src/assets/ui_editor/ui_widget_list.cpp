#include "ui_widget_list.h"

#include "ui_editor.h"

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

	setHandle(UIEventType::TreeItemReparented, "widgetsList", [=] (const UIEvent& event)
	{
		const auto& src = event.getConfigData().asSequence();
		std::vector<MoveOperation> changes;
		changes.reserve(src.size());
		for (const auto& e: src) {
			changes.emplace_back(MoveOperation{ e["itemId"].asString(), e["parentId"].asString(), e["childIdx"].asInt() });
		}

		moveItems(changes);
	});
}

void UIWidgetList::setDefinition(std::shared_ptr<UIDefinition> def)
{
	definition = std::move(def);
	populateList();
}

void UIWidgetList::setUIEditor(UIEditor& editor)
{
	uiEditor = &editor;
}

UITreeList& UIWidgetList::getList()
{
	return *list;
}

void UIWidgetList::populateList()
{
	if (list && definition) {
		populateList(definition->getRoot(), "");
	}
}

void UIWidgetList::populateList(const ConfigNode& curNode, String parentId)
{
	const String id = curNode["uuid"].asString();

	String name;
	Sprite icon;
	bool canHaveChildren = true;

	if (curNode.hasKey("widget")) {
		const auto& widgetNode = curNode["widget"];

		const auto properties = uiEditor->getGameFactory().getPropertiesForWidget(widgetNode["class"].asString());
		canHaveChildren = properties.canHaveChildren;

		if (widgetNode.hasKey("id")) {
			name += widgetNode["id"].asString() + " ";
		}
		name += "[" + widgetNode["class"].asString() + "]";
	} else {
		name = "[sizer]";
	}

	list->addTreeItem(id, parentId, -1, LocalisedString::fromUserString(name), "label", icon, !canHaveChildren);

	if (curNode.hasKey("children")) {
		for (const auto& c: curNode["children"].asSequence()) {
			populateList(c, id);
		}
	}
}

void UIWidgetList::moveItems(gsl::span<const MoveOperation> changes)
{
	for (auto& c: changes) {
		const auto result = definition->findUUID(c.itemId);
		auto* widgetPtr = result.result;
		auto* oldParent = result.parent;
		auto* newParent = definition->findUUID(c.parentId).result;
		if (widgetPtr && oldParent && newParent) {
			auto widget = ConfigNode(std::move(*widgetPtr));
			auto& oldChildren = (*oldParent)["children"].asSequence();
			auto& newChildren = (*newParent)["children"].asSequence();
			std_ex::erase_if(oldChildren, [=] (const ConfigNode& n) { return &n == widgetPtr; });

			const int newPos = std::min(c.childIdx, static_cast<int>(newChildren.size()));
			newChildren.insert(newChildren.begin() + newPos, std::move(widget));
		}
	}

	uiEditor->onWidgetModified();
}
