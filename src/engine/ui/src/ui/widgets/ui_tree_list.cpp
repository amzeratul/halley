#include <utility>


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

	const auto treeControls = std::make_shared<UITreeListControls>(id, style.getSubStyle("controls").getSprite("element"), style.getSubStyle("controls"));
	listItem->add(treeControls, 0, {}, UISizerFillFlags::Fill);

	auto labelWidget = std::make_shared<UILabel>(id + "_label", style.getTextRenderer("label"), label);
	if (style.hasTextRenderer("selectedLabel")) {
		labelWidget->setSelectable(style.getTextRenderer("label"), style.getTextRenderer("selectedLabel"));
	}
	if (style.hasTextRenderer("disabledStyle")) {
		labelWidget->setDisablable(style.getTextRenderer("label"), style.getTextRenderer("disabledStyle"));
	}
	listItem->add(labelWidget, 0, style.getBorder("labelBorder"), UISizerFillFlags::Fill);

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
	const auto res = root.tryFindId(id);
	if (res) {
		return *res;
	}
	return root;
}

UITreeListControls::UITreeListControls(String id, Sprite elementSprite, UIStyle style)
	: UIWidget(std::move(id), Vector2f(), UISizer(UISizerType::Horizontal, 0))
	, style(std::move(style))
	, elementSprite(std::move(elementSprite))
{
}

float UITreeListControls::updateGuides(const std::vector<int>& itemsLeftPerDepth)
{
	auto getSprite = [&] (size_t depth) -> Sprite
	{
		const auto left = itemsLeftPerDepth[depth];
		const bool deepest = depth == itemsLeftPerDepth.size() - 1;
		if (deepest) {
			if (left == 1) {
				return style.getSprite("guide_l");
			} else {
				return style.getSprite("guide_t");
			}
		} else {
			if (left == 1) {
				return Sprite().setSize(Vector2f(22, 22));
			} else {
				return style.getSprite("guide_i");
			}
		}
	};
	
	if (waitingConstruction || itemsLeftPerDepth.size() != guides.size()) {
		clear();
		guides.clear();

		for (size_t i = 0; i < itemsLeftPerDepth.size(); ++i) {
			guides.push_back(std::make_shared<UIImage>(getSprite(i)));
			add(guides.back(), 0, Vector4f(0, -1, 0, 0));
		}

		elementImage = std::make_shared<UIImage>(elementSprite);
		add(elementImage, 0, Vector4f(), UISizerAlignFlags::Centre);

		totalIndent = getLayoutMinimumSize(false).x;
		waitingConstruction = false;
	}

	return totalIndent;
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
		const auto res = c.tryFindId(id);
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
	std::vector<int> itemsLeftPerDepth;
	doUpdateTree(itemsLeftPerDepth);
}

void UITreeListItem::doUpdateTree(std::vector<int>& itemsLeftPerDepth)
{
	if (listItem && treeControls) {
		const float totalIndent = treeControls->updateGuides(itemsLeftPerDepth);
		listItem->setClickableInnerBorder(Vector4f(totalIndent, 0, 0, 0));
	}

	itemsLeftPerDepth.push_back(int(children.size()));

	for (auto& c: children) {
		c.doUpdateTree(itemsLeftPerDepth);
		itemsLeftPerDepth.back()--;
	}

	itemsLeftPerDepth.pop_back();
}
