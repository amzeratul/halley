#include "scripting/script_node_type.h"

#include <cassert>


#include "world.h"
#include "nodes/script_branching.h"
#include "nodes/script_execution_control.h"
#include "nodes/script_logic_gates.h"
#include "nodes/script_audio.h"
#include "nodes/script_entity.h"
#include "nodes/script_flow_gate.h"
#include "nodes/script_input.h"
#include "nodes/script_loop.h"
#include "nodes/script_messaging.h"
#include "nodes/script_network.h"
#include "nodes/script_sprite.h"
#include "nodes/script_node_variables.h"
#include "nodes/script_transform.h"
#include "nodes/script_ui.h"
#include "nodes/script_wait.h"
#include "nodes/script_wait_for.h"
using namespace Halley;

String IScriptNodeType::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	return getName();
}

String IScriptNodeType::getLargeLabel(const ScriptGraphNode& node) const
{
	return "";
}

String IScriptNodeType::getLabel(const ScriptGraphNode& node) const
{
	return "";
}

Vector<IScriptNodeType::SettingType> IScriptNodeType::getSettingTypes() const
{
	return {};
}

std::pair<String, Vector<ColourOverride>> IScriptNodeType::getDescription(const ScriptGraphNode& node, const World* world, PinType elementType, uint8_t elementIdx, const ScriptGraph& graph) const
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

std::pair<String, Vector<ColourOverride>> IScriptNodeType::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	return { getName(), {} };
}

std::pair<String, Vector<ColourOverride>> IScriptNodeType::getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const
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

String IScriptNodeType::getIconName(const ScriptGraphNode& node) const
{
	return "";
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
	return environment.readInputDataPin(node, static_cast<ScriptPinId>(pinN));
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
	dstNode.getNodeType().setData(environment, dstNode, dst.dstPin, std::move(data), environment.getNodeData(dst.dstNode.value()));
}

String IScriptNodeType::getConnectedNodeName(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, size_t pinN) const
{
	const auto& pin = node.getPin(pinN);
	if (pin.connections.empty()) {
		if (node.getNodeType().getPin(node, pinN).type == ScriptNodeElementType::TargetPin) {
			return "<current entity>";
		} else {
			return "<empty>";
		}
	}
	assert(pin.connections.size() == 1);

	if (pin.connections[0].dstNode) {
		const auto& otherNode = graph.getNodes().at(pin.connections[0].dstNode.value());
		return otherNode.getNodeType().getShortDescription(world, otherNode, graph, pin.connections[0].dstPin);
	} else if (pin.connections[0].entityIdx && world) {
		const auto target = world->tryGetEntity(graph.getEntityId(pin.connections[0].entityIdx));
		if (target.isValid()) {
			return target.getName();
		}
	}
	
	return "<unknown>";
}

EntityId IScriptNodeType::readEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t idx) const
{
	return environment.readInputEntityId(node, static_cast<ScriptPinId>(idx));
}

EntityId IScriptNodeType::readRawEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t idx) const
{
	return environment.readInputEntityIdRaw(node, static_cast<ScriptPinId>(idx));
}

std::array<IScriptNodeType::OutputNode, 8> IScriptNodeType::getOutputNodes(const ScriptGraphNode& node, uint8_t outputActiveMask) const
{
	std::array<OutputNode, 8> result;
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
						result[nOutputsFound++] = OutputNode{ conn.dstNode, static_cast<ScriptPinId>(i) };
					}
				}
			}

			++curOutputPin;
		}
	}

	return result;
}

ScriptPinId IScriptNodeType::getNthOutputPinIdx(const ScriptGraphNode& node, size_t n) const
{
	const auto& pinConfig = getPinConfiguration(node);
	size_t curOutputPin = 0;
	for (size_t i = 0; i < pinConfig.size(); ++i) {
		if (pinConfig[i].type == ScriptNodeElementType::FlowPin && pinConfig[i].direction == ScriptNodePinDirection::Output) {
			if (curOutputPin == n) {
				return static_cast<ScriptPinId>(i);
			}
			++curOutputPin;
		}
	}
	return 0xFF;
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

Vector<String> ScriptNodeTypeCollection::getTypes(bool includeNonAddable) const
{
	Vector<String> result;
	result.reserve(nodeTypes.size());
	for (const auto& [id, v]: nodeTypes) {
		if (v->canAdd() || includeNonAddable) {
			result.push_back(id);
		}
	}
	return result;
}

Vector<String> ScriptNodeTypeCollection::getNames(bool includeNonAddable) const
{
	Vector<String> result;
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
	addScriptNode(std::make_unique<ScriptDestructor>());
	addScriptNode(std::make_unique<ScriptRestart>());
	addScriptNode(std::make_unique<ScriptStop>());
	addScriptNode(std::make_unique<ScriptSpinwait>());
	addScriptNode(std::make_unique<ScriptStartScript>());
	addScriptNode(std::make_unique<ScriptStartScriptName>());
	addScriptNode(std::make_unique<ScriptStopScript>());
	addScriptNode(std::make_unique<ScriptStopTag>());
	addScriptNode(std::make_unique<ScriptWait>());
	addScriptNode(std::make_unique<ScriptWaitFor>());
	addScriptNode(std::make_unique<ScriptSpriteAnimation>());
	addScriptNode(std::make_unique<ScriptSpriteDirection>());
	addScriptNode(std::make_unique<ScriptSpriteAlpha>());
	addScriptNode(std::make_unique<ScriptBranch>());
	addScriptNode(std::make_unique<ScriptMergeAll>());
	addScriptNode(std::make_unique<ScriptLogicGateAnd>());
	addScriptNode(std::make_unique<ScriptLogicGateOr>());
	addScriptNode(std::make_unique<ScriptLogicGateXor>());
	addScriptNode(std::make_unique<ScriptLogicGateNot>());
	addScriptNode(std::make_unique<ScriptAudioEvent>());
	addScriptNode(std::make_unique<ScriptVariable>());
	addScriptNode(std::make_unique<ScriptLiteral>());
	addScriptNode(std::make_unique<ScriptComparison>());
	addScriptNode(std::make_unique<ScriptArithmetic>());
	addScriptNode(std::make_unique<ScriptLerp>());
	addScriptNode(std::make_unique<ScriptAdvanceTo>());
	addScriptNode(std::make_unique<ScriptSetVariable>());
	addScriptNode(std::make_unique<ScriptHoldVariable>());
	addScriptNode(std::make_unique<ScriptInputButton>());
	addScriptNode(std::make_unique<ScriptForLoop>());
	addScriptNode(std::make_unique<ScriptWhileLoop>());
	addScriptNode(std::make_unique<ScriptLerpLoop>());
	addScriptNode(std::make_unique<ScriptEveryFrame>());
	addScriptNode(std::make_unique<ScriptEveryTime>());
	addScriptNode(std::make_unique<ScriptFlowGate>());
	addScriptNode(std::make_unique<ScriptEntityAuthority>());
	addScriptNode(std::make_unique<ScriptHostAuthority>());
	addScriptNode(std::make_unique<ScriptIfEntityAuthority>());
	addScriptNode(std::make_unique<ScriptIfHostAuthority>());
	addScriptNode(std::make_unique<ScriptSendMessage>());
	addScriptNode(std::make_unique<ScriptReceiveMessage>());
	addScriptNode(std::make_unique<ScriptSendSystemMessage>());
	addScriptNode(std::make_unique<ScriptSendEntityMessage>());
	addScriptNode(std::make_unique<ScriptEntityIdToData>());
	addScriptNode(std::make_unique<ScriptDataToEntityId>());
	addScriptNode(std::make_unique<ScriptUIModal>());
	addScriptNode(std::make_unique<ScriptUIInWorld>());
	addScriptNode(std::make_unique<ScriptSetPosition>());
	addScriptNode(std::make_unique<ScriptGetPosition>());
	addScriptNode(std::make_unique<ScriptSpawnEntity>());
	addScriptNode(std::make_unique<ScriptDestroyEntity>());
}
