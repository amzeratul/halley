#include "widgets/ui_tree_list.h"
#include "widgets/ui_label.h"
using namespace Halley;


UITreeList::UITreeList(String id, UIStyle style)
	: UIList(std::move(id), std::move(style))
{
}

void UITreeList::addTreeItem(const String& id, const String& parentId, const LocalisedString& label)
{
	auto widget = std::make_shared<UILabel>(id + "_label", style.getTextRenderer("label"), label);

	if (style.hasTextRenderer("selectedLabel")) {
		widget->setSelectable(style.getTextRenderer("label"), style.getTextRenderer("selectedLabel"));
	}

	if (style.hasTextRenderer("disabledStyle")) {
		widget->setDisablable(style.getTextRenderer("label"), style.getTextRenderer("disabledStyle"));
	}

	int depth = 0;
	const auto parent = tryGetItem(parentId);
	if (parent) {
		depth = parent->getDepth() + 1;
	}
	
	auto item = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(getNumberOfItems()), style.getBorder("extraMouseBorder"));
	item->setDepth(depth);
	item->setParentItem(parentId);
	item->add(widget, 0, {}, UISizerFillFlags::Fill);

	const float indent = getStyle().getFloat("indentation");
	addItem(item, Vector4f(indent * float(depth), 0, 0, 0), UISizerAlignFlags::Left | UISizerFillFlags::FillVertical);
}

