#include "scripting/script_node_type.h"


#include "world.h"
#include "nodes/script_branching.h"
#include "nodes/script_flow_control.h"
#include "nodes/script_logic_gates.h"
#include "nodes/script_music.h"
#include "nodes/script_sprite.h"
#include "nodes/script_variables.h"
#include "nodes/script_wait.h"
#include "nodes/script_wait_for.h"
using namespace Halley;

String IScriptNodeType::getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const
{
	return getName();
}

String IScriptNodeType::getLabel(const ScriptGraphNode& node) const
{
	return "";
}

std::vector<IScriptNodeType::SettingType> IScriptNodeType::getSettingTypes() const
{
	return {};
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getDescription(const ScriptGraphNode& node,	const World& world, PinType elementType, uint8_t elementIdx, const ScriptGraph& graph) const
{
	switch (elementType.type) {
	case ScriptNodeElementType::ReadDataPin:
	case ScriptNodeElementType::WriteDataPin:
	case ScriptNodeElementType::FlowPin:
	case ScriptNodeElementType::TargetPin:
		return getPinDescription(node, elementType, elementIdx);
	case ScriptNodeElementType::Node:
		return getNodeDescription(node, world, graph);
	}
	
	return { "?", {} };
}

std::pair<String, std::vector<ColourOverride>> IScriptNodeType::getNodeDescription(const ScriptGraphNode& node,	const World& world, const ScriptGraph& graph) const
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
		case ScriptNodeElementType::ReadDataPin:
			return "Read Data";
		case ScriptNodeElementType::WriteDataPin:
			return "Write Data";
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

	const auto& config = getPinConfiguration(node);
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

IScriptNodeType::PinType IScriptNodeType::getPin(const ScriptGraphNode& node, size_t n) const
{
	const auto& pins = getPinConfiguration(node);
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
	if (pin.connections.empty() || !pin.connections[0].dstNode) {
		return ConfigNode();
	}
	assert(pin.connections.size() == 1);

	const auto& dst = pin.connections[0];
	const auto& nodes = environment.getCurrentGraph()->getNodes();
	const auto& dstNode = nodes[dst.dstNode.value()];
	return dstNode.getNodeType().getData(environment, dstNode, dst.dstPin);
}

void IScriptNodeType::writeDataPin(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const
{
	const auto& pins = node.getPins();
	if (pinN >= pins.size()) {
		return;
	}

	const auto& pin = pins[pinN];
	if (pin.connections.empty() || !pin.connections[0].dstNode) {
		return;
	}
	assert(pin.connections.size() == 1);

	const auto& dst = pin.connections[0];
	const auto& nodes = environment.getCurrentGraph()->getNodes();
	const auto& dstNode = nodes[dst.dstNode.value()];
	dstNode.getNodeType().setData(environment, dstNode, dst.dstPin, std::move(data));
}

String IScriptNodeType::getConnectedNodeName(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph, size_t pinN) const
{
	const auto& pin = node.getPin(pinN);
	if (pin.connections.empty()) {
		return "<empty>";
	}
	assert(pin.connections.size() == 1);

	if (pin.connections[0].dstNode) {
		const auto& otherNode = graph.getNodes().at(pin.connections[0].dstNode.value());
		return otherNode.getNodeType().getShortDescription(world, otherNode, graph);
	} else if (pin.connections[0].entity.isValid()) {
		const auto target = world.tryGetEntity(pin.connections[0].entity);
		if (target.isValid()) {
			return target.getName();
		}
	}
	
	return "<unknown>";
}

EntityId IScriptNodeType::readEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t idx) const
{
	if (idx < node.getPins().size()) {
		const auto& pin = node.getPins()[idx];
		if (!pin.connections.empty()) {
			const auto& conn = pin.connections[0];
			if (conn.entity.isValid()) {
				return conn.entity;
			} else if (conn.dstNode) {
				const auto& nodes = environment.getCurrentGraph()->getNodes();
				const auto& dstNode = nodes.at(conn.dstNode.value());
				return dstNode.getNodeType().getEntityId(environment, dstNode, conn.dstPin);
			}
		}
	}
	return EntityId();
}

std::array<OptionalLite<uint32_t>, 8> IScriptNodeType::getOutputNodes(const ScriptGraphNode& node, uint32_t outputActiveMask) const
{
	std::array<OptionalLite<uint32_t>, 8> result;
	result.fill({});
	
	const auto& pinConfig = getPinConfiguration(node);

	size_t curOutputPin = 0;
	size_t nOutputsFound = 0;
	for (size_t i = 0; i < pinConfig.size(); ++i) {
		if (pinConfig[i].type == ScriptNodeElementType::FlowPin && pinConfig[i].direction == ScriptNodePinDirection::Output) {
			const bool outputActive = (outputActiveMask & (1 << curOutputPin)) != 0;
			if (outputActive) {
				const auto& output = node.getPin(i);
				for (auto& conn: output.connections) {
					if (conn.dstNode) {
						result[nOutputsFound++] = conn.dstNode;
					}
				}
			}

			++curOutputPin;
		}
	}

	return result;
}

String IScriptNodeType::addParentheses(String str)
{
	if (str.contains(' ')) {
		return "(" + std::move(str) + ")";
	}
	return str;
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
	addScriptNode(std::make_unique<ScriptSpriteAnimation>());
	addScriptNode(std::make_unique<ScriptSpriteDirection>());
	addScriptNode(std::make_unique<ScriptBranch>());
	//addScriptNode(std::make_unique<ScriptFork>());
	//addScriptNode(std::make_unique<ScriptMergeAny>());
	addScriptNode(std::make_unique<ScriptMergeAll>());
	addScriptNode(std::make_unique<ScriptLogicGateAnd>());
	addScriptNode(std::make_unique<ScriptLogicGateOr>());
	addScriptNode(std::make_unique<ScriptLogicGateXor>());
	addScriptNode(std::make_unique<ScriptLogicGateNot>());
	addScriptNode(std::make_unique<ScriptPlayMusic>());
	addScriptNode(std::make_unique<ScriptStopMusic>());
	addScriptNode(std::make_unique<ScriptVariable>());
	addScriptNode(std::make_unique<ScriptLiteral>());
	addScriptNode(std::make_unique<ScriptComparison>());
	addScriptNode(std::make_unique<ScriptSetVariable>());
}
