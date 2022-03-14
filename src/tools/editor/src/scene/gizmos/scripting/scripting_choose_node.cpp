#include "scripting_choose_node.h"

using namespace Halley;

ScriptingChooseNode::ScriptingChooseNode(Vector2f minSize, UIFactory& factory, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, const Callback& callback)
	: ChooseAssetWindow(minSize, factory, callback, false)
	, resources(resources)
	, scriptNodeTypes(scriptNodeTypes)
{
	setAssetIds(scriptNodeTypes->getTypes(false), scriptNodeTypes->getNames(false), "");
	setTitle(LocalisedString::fromHardcodedString("Add Scripting Node"));
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
	const auto* type = scriptNodeTypes->tryGetNodeType(id);
	if (type) {
		const ScriptGraphNode dummy;
		auto sprite = Sprite().setImage(resources, type->getIconName(dummy)).setColour(ScriptRenderer::getNodeColour(*type));
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
		const auto* typeA = scriptNodeTypes->tryGetNodeType(a.first);
		const auto* typeB = scriptNodeTypes->tryGetNodeType(b.first);
		const auto classA = typeA ? typeA->getClassification() : ScriptNodeClassification::Unknown;
		const auto classB = typeB ? typeB->getClassification() : ScriptNodeClassification::Unknown;
		if (classA != classB) {
			return classA < classB;
		}
		return a.second < b.second;
	});
}

int ScriptingChooseNode::getNumColumns(Vector2f scrollPaneSize) const
{
	return 4;
}
