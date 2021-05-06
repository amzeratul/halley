#include "scripting/script_node_type.h"

#include "nodes/script_branching.h"
#include "nodes/script_flow_control.h"
#include "nodes/script_play_animation.h"
#include "nodes/script_wait.h"
#include "nodes/script_wait_for.h"
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
	auto getName = [](ScriptNodeElementType type) -> const char*
	{
		switch (type) {
		case ScriptNodeElementType::FlowPin:
			return "Flow";
		case ScriptNodeElementType::DataPin:
			return "Data";
		case ScriptNodeElementType::TargetPin:
			return "Target";
		}
		return "?";
	};

	auto getIO = [](ScriptNodePinDirection direction) -> const char*
	{
		switch (direction) {
		case ScriptNodePinDirection::Input:
			return " Input";
		case ScriptNodePinDirection::Output:
			return " Output";
		}
		return nullptr;
	};

	const auto& config = getPinConfiguration();
	size_t typeIdx = 0;
	size_t typeTotal = 0;
	for (size_t i = 0; i < config.size(); ++i) {
		if (i == elementIdx) {
			typeIdx = typeTotal;
		}
		if (config[i] == elementType) {
			++typeTotal;
		}
	}

	ColourStringBuilder builder;
	builder.append(getName(elementType.type));
	builder.append(getIO(elementType.direction));
	if (typeTotal > 1) {
		builder.append(" " + toString(static_cast<int>(typeIdx)));
	}
	return builder.moveResults();
}

IScriptNodeType::PinType IScriptNodeType::getPin(size_t n) const
{
	const auto& pins = getPinConfiguration();
	if (n < pins.size()) {
		return pins[n];
	}
	return PinType{ ScriptNodeElementType::Undefined, ScriptNodePinDirection::Input };
}

ConfigNode IScriptNodeType::readDataPin(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto& pins = node.getPins();
	if (pinN >= pins.size()) {
		return ConfigNode();
	}

	const auto& pin = pins[pinN];
	if (!pin.dstNode) {
		return ConfigNode();
	}

	const auto& nodes = environment.getCurrentGraph()->getNodes();
	const auto& dstNode = nodes[pin.dstNode.value()];
	return dstNode.getNodeType().getData(environment, dstNode, pin.dstPin);
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
	addScriptNode(std::make_unique<ScriptWaitFor>());
	addScriptNode(std::make_unique<ScriptPlayAnimation>());
	addScriptNode(std::make_unique<ScriptBranch>());
	addScriptNode(std::make_unique<ScriptFork>());
	addScriptNode(std::make_unique<ScriptMergeOne>());
	addScriptNode(std::make_unique<ScriptMergeAll>());
}
