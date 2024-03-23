#include "scripting_choose_node.h"

using namespace Halley;

ScriptingChooseNode::ScriptingChooseNode(Vector2f minSize, UIFactory& factory, Resources& resources, std::shared_ptr<GraphNodeTypeCollection> nodeTypes, const Callback& callback)
	: ChooseAssetWindow(minSize, factory, callback, {})
	, resources(resources)
	, nodeTypes(nodeTypes)
{
	setAssetIds(nodeTypes->getTypes(false), nodeTypes->getNames(false), "");
	setTitle(LocalisedString::fromHardcodedString("Add Node"));
}

std::shared_ptr<UISizer> ScriptingChooseNode::makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch)
{
	if (hasSearch) {
		return ChooseAssetWindow::makeItemSizer(std::move(icon), std::move(label), hasSearch);
	} else {
		return makeItemSizerBigIcon(std::move(icon), std::move(label));
	}
}

std::shared_ptr<UIImage> ScriptingChooseNode::makeIcon(const String& id, bool hasSearch)
{
	const auto* type = nodeTypes->tryGetGraphNodeType(id);
	if (type) {
		const ScriptGraphNode dummy;
		auto sprite = Sprite().setImage(resources, type->getIconName(dummy)).setColour(type->getColour());
		if (hasSearch) {
			sprite.setScale(0.5f);
		}
		return std::make_shared<UIImage>(sprite);
	} else {
		return {};
	}
}

void ScriptingChooseNode::sortItems(Vector<std::pair<String, String>>& items)
{
	std::sort(items.begin(), items.end(), [&] (const auto& a, const auto& b)
	{
		const auto* typeA = nodeTypes->tryGetGraphNodeType(a.first);
		const auto* typeB = nodeTypes->tryGetGraphNodeType(b.first);
		const auto classA = typeA ? typeA->getSortOrder() : 0;
		const auto classB = typeB ? typeB->getSortOrder() : 0;
		if (classA != classB) {
			return classA < classB;
		}
		return a.second < b.second;
	});
}

int ScriptingChooseNode::getNumColumns(Vector2f scrollPaneSize) const
{
	return static_cast<int>(std::ceil(scrollPaneSize.x / 180.0f));
}
