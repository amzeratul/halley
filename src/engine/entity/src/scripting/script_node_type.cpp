#include "scripting/script_node_type.h"

#include "nodes/script_start.h"
#include "nodes/script_play_animation.h"
#include "nodes/script_restart.h"
#include "nodes/script_stop.h"
#include "nodes/script_wait.h"
using namespace Halley;

std::vector<IScriptNodeType::SettingType> IScriptNodeType::getSettingTypes() const
{
	return {};
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getDescription(const ScriptGraphNode& node,	const World& world, PinType elementType, uint8_t elementIdx) const
{
	switch (elementType.type) {
	case ScriptNodeElementType::DataPin:
	case ScriptNodeElementType::FlowPin:
	case ScriptNodeElementType::TargetPin:
		return getPinDescription(node, elementType, elementIdx);
	case ScriptNodeElementType::Node:
		return getNodeDescription(node, world);
	}
	
	return { "?", {} };
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getNodeDescription(const ScriptGraphNode& node,	const World& world) const
{
	return { getName(), {} };
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const
{
	if (elementType.type == ScriptNodeElementType::FlowPin && elementType.direction == ScriptNodePinDirection::Input) {
		return { "Flow Input " + toString(static_cast<int>(elementIdx)), {} };
	} else if (elementType.type == ScriptNodeElementType::FlowPin && elementType.direction == ScriptNodePinDirection::Output) {
		return { "Flow Output " + toString(static_cast<int>(elementIdx)), {} };
	} else if (elementType.type == ScriptNodeElementType::DataPin && elementType.direction == ScriptNodePinDirection::Input) {
		return { "Data Input " + toString(static_cast<int>(elementIdx)), {} };
	} else if (elementType.type == ScriptNodeElementType::DataPin && elementType.direction == ScriptNodePinDirection::Output) {
		return { "Data Output " + toString(static_cast<int>(elementIdx)), {} };
	} else if (elementType.type == ScriptNodeElementType::TargetPin) {
		return { "Target " + toString(static_cast<int>(elementIdx)), {} };
	}
	return { "?", {} };
}

IScriptNodeType::PinType IScriptNodeType::getPin(size_t n) const
{
	const auto& pins = getPinConfiguration();
	if (n < pins.size()) {
		return pins[n];
	}
	return PinType{ ScriptNodeElementType::Undefined, ScriptNodePinDirection::Input };
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
	addScriptNode(std::make_unique<ScriptRestart>());
	addScriptNode(std::make_unique<ScriptStop>());
	addScriptNode(std::make_unique<ScriptWait>());
	addScriptNode(std::make_unique<ScriptPlayAnimation>());
}
