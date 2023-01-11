#include "script_transform.h"

#include "world.h"
#include "halley/navigation/world_position.h"
#include "components/transform_2d_component.h"
using namespace Halley;


gsl::span<const IScriptNodeType::PinType> ScriptSetPosition::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSetPosition::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Set the position of ");
	result.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	result.append(" to ");
	result.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	return result.moveResults();
}

IScriptNodeType::Result ScriptSetPosition::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 2));
	const auto pos = WorldPosition(readDataPin(environment, node, 3), transform->getGlobalPosition(), transform->getSubWorld());

	transform->setGlobalPosition(pos.pos);
	transform->setSubWorld(pos.subWorld);

	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IScriptNodeType::PinType> ScriptGetPosition::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptGetPosition::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Get the position of ");
	result.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return result.moveResults();
}

String ScriptGetPosition::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId element_idx) const
{
	return "Position of " + getConnectedNodeName(world, node, graph, 0);
}

ConfigNode ScriptGetPosition::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 0));
	if (transform) {
		return WorldPosition(transform->getGlobalPosition(), transform->getSubWorld()).toConfigNode();
	}
	return ConfigNode();
}
