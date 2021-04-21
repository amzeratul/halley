#include "scripting/script_node_type.h"

#include "nodes/script_play_animation.h"
#include "nodes/script_wait.h"
using namespace Halley;

ScriptNodeTypeCollection::ScriptNodeTypeCollection()
{
	addBasicScriptNodes();
}

void ScriptNodeTypeCollection::addScriptNode(std::unique_ptr<IScriptNodeType> nodeType)
{
	auto name = nodeType->getName();
	nodeTypes[std::move(name)] = std::move(nodeType);
}

const IScriptNodeType* ScriptNodeTypeCollection::tryGetNodeType(const String& typeId) const
{
	const auto iter = nodeTypes.find(typeId);
	if (iter != nodeTypes.end()) {
		return iter->second.get();
	}
	return nullptr;
}

void ScriptNodeTypeCollection::addBasicScriptNodes()
{
	addScriptNode(std::make_unique<ScriptWait>());
	addScriptNode(std::make_unique<ScriptPlayAnimation>());
}
