#include "scripting_choose_node.h"

using namespace Halley;

ScriptingChooseNode::ScriptingChooseNode(UIFactory& factory, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, const Callback& callback)
	: ChooseAssetWindow(factory, callback, false)
{
	setAssetIds(scriptNodeTypes->getTypes(false), scriptNodeTypes->getNames(false), "");
	setTitle(LocalisedString::fromHardcodedString("Add Scripting Node"));
}
