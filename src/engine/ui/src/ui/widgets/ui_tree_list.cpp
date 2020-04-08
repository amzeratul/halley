#include "widgets/ui_tree_list.h"

#include "widgets/ui_image.h"
#include "widgets/ui_label.h"
using namespace Halley;


UITreeList::UITreeList(String id, UIStyle style)
	: UIList(std::move(id), std::move(style))
{
}

void UITreeList::addTreeItem(const String& id, const String& parentId, const LocalisedString& label)
{
	auto listItem = std::make_shared<UIListItem>(id, *this, style.getSubStyle("item"), int(getNumberOfItems()), style.getBorder("extraMouseBorder"));

	auto treeControls = std::make_shared<UITreeListControls>(id, style.getSprite("element"));
	listItem->add(treeControls, 0, {}, UISizerFillFlags::Fill);

	auto widget = std::make_shared<UILabel>(id + "_label", style.getTextRenderer("label"), label);
	if (style.hasTextRenderer("selectedLabel")) {
		widget->setSelectable(style.getTextRenderer("label"), style.getTextRenderer("selectedLabel"));
	}
	if (style.hasTextRenderer("disabledStyle")) {
		widget->setDisablable(style.getTextRenderer("label"), style.getTextRenderer("disabledStyle"));
	}
	listItem->add(widget, 0, {}, UISizerFillFlags::Fill);

	auto treeItem = UITreeListItem(id, listItem, treeControls);
	
	auto& parentItem = getItemOrRoot(parentId);
	parentItem.addChild(std::move(treeItem));

	addItem(listItem, Vector4f(), UISizerAlignFlags::Left | UISizerFillFlags::FillVertical);
}

void UITreeList::update(Time t, bool moved)
{
	root.updateTree();
}

UITreeListItem& UITreeList::getItemOrRoot(const String& id)
{
	auto res = root.tryFindId(id);
	if (res) {
		return *res;
	}
	return root;
}

UITreeListControls::UITreeListControls(String id, Sprite element)
	: UIWidget(std::move(id), Vector2f(), UISizer(UISizerType::Horizontal, 0))
{
	spacer = std::make_shared<UIWidget>();
	elementImage = std::make_shared<UIImage>(element);
	add(spacer);
	add(elementImage);
}

void UITreeListControls::setDepth(int depth)
{
	spacer->setMinSize(Vector2f(15.0f * depth, 0));
}

UITreeListItem::UITreeListItem() = default;

UITreeListItem::UITreeListItem(String id, std::shared_ptr<UIListItem> listItem, std::shared_ptr<UITreeListControls> treeControls)
	: id(std::move(id))
	, listItem(std::move(listItem))
	, treeControls(std::move(treeControls))
{}

UITreeListItem* UITreeListItem::tryFindId(const String& id)
{
	if (id == this->id) {
		return this;
	}

	for (auto& c: children) {
		auto res = c.tryFindId(id);
		if (res) {
			return res;
		}
	}

	return nullptr;
}

void UITreeListItem::addChild(UITreeListItem item)
{
	children.emplace_back(std::move(item));
}

void UITreeListItem::updateTree()
{
	doUpdateTree(0);
}

void UITreeListItem::doUpdateTree(int depth)
{
	if (listItem && treeControls) {
		const float totalIndent = 15.0f * depth;
		listItem->setClickableInnerBorder(Vector4f(totalIndent, 0, 0, 0));
		treeControls->setDepth(depth);
	}
	
	for (auto& c: children) {
		c.doUpdateTree(depth + 1);
	}
}
