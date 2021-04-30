#include "scripting/script_node_type.h"

#include "nodes/script_start.h"
#include "nodes/script_play_animation.h"
#include "nodes/script_wait.h"
using namespace Halley;

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getDescription(const ScriptGraphNode& node,	const World& world, ScriptNodeElementType elementType, uint8_t elementIdx) const
{
	switch (elementType) {
	case ScriptNodeElementType::Input:
	case ScriptNodeElementType::Output:
		return getIOPinDescription(node, elementType, elementIdx);
	case ScriptNodeElementType::Target:
		return getTargetPinDescription(node, world, elementIdx);
	case ScriptNodeElementType::Node:
		return getNodeDescription(node, world);
	}
	
	return { "?", {} };
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getNodeDescription(const ScriptGraphNode& node,	const World& world) const
{
	return { getName(), {} };
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getIOPinDescription(const ScriptGraphNode& node, ScriptNodeElementType elementType, uint8_t elementIdx) const
{
	if (elementType == ScriptNodeElementType::Input) {
		if (getNumInputPins() <= 1) {
			return { "Input", {} };
		} else {
			return { "Input " + toString(static_cast<int>(elementIdx)), {} };
		}
	} else {
		if (getNumOutputPins() <= 1) {
			return { "Output", {} };
		} else {
			return { "Output " + toString(static_cast<int>(elementIdx)), {} };
		}
	}
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getTargetPinDescription(const ScriptGraphNode& node, const World& world, uint8_t elementIdx) const
{
	if (getNumTargetPins() <= 1) {
		return { "Target", {} };
	} else {
		return { "Target " + toString(static_cast<int>(elementIdx)), {} };
	}
}

ScriptNodeTypeCollection::ScriptNodeTypeCollection()
{
	addBasicScriptNodes();
}

void ScriptNodeTypeCollection::addScriptNode(std::unique_ptr<IScriptNodeType> nodeType)
{
	auto name = nodeType->getId();
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

std::vector<String> ScriptNodeTypeCollection::getTypes(bool includeNonAddable) const
{
	std::vector<String> result;
	result.reserve(nodeTypes.size());
	for (const auto& [id, v]: nodeTypes) {
		if (v->canAdd() || includeNonAddable) {
			result.push_back(id);
		}
	}
	return result;
}

std::vector<String> ScriptNodeTypeCollection::getNames(bool includeNonAddable) const
{
	std::vector<String> result;
	result.reserve(nodeTypes.size());
	for (const auto& [id, v]: nodeTypes) {
		if (v->canAdd() || includeNonAddable) {
			result.push_back(v->getName());
		}
	}
	return result;
}

void ScriptNodeTypeCollection::addBasicScriptNodes()
{
	addScriptNode(std::make_unique<ScriptStart>());
	addScriptNode(std::make_unique<ScriptWait>());
	addScriptNode(std::make_unique<ScriptPlayAnimation>());
}
