#include "scripting_choose_node.h"

using namespace Halley;

ScriptingChooseNode::ScriptingChooseNode(UIFactory& factory, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, const Callback& callback)
	: ChooseAssetWindow(factory, callback, false, UISizerType::Grid, 2)
	, resources(resources)
	, scriptNodeTypes(scriptNodeTypes)
{
	setAssetIds(scriptNodeTypes->getTypes(false), scriptNodeTypes->getNames(false), "");
	setTitle(LocalisedString::fromHardcodedString("Add Scripting Node"));
}

std::shared_ptr<UISizer> ScriptingChooseNode::makeItemSizer(Sprite icon, std::shared_ptr<UILabel> label)
{
	return makeItemSizerBigIcon(std::move(icon), std::move(label));
}

Sprite ScriptingChooseNode::makeIcon(const String& id)
{
	const auto* type = scriptNodeTypes->tryGetNodeType(id);
	if (type) {
		const ScriptGraphNode dummy;
		return Sprite().setImage(resources, type->getIconName(dummy));
	} else {
		return Sprite();
	}
}
