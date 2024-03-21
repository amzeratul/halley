#include "script_transform.h"

#include "halley/entity/world.h"
#include "halley/navigation/world_position.h"
#include "halley/entity/components/transform_2d_component.h"
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
	if (auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 2))) {
		const auto pos = WorldPosition(readDataPin(environment, node, 3), transform->getGlobalPosition(), transform->getSubWorld());

		transform->setGlobalPosition(pos.pos);
		transform->setSubWorld(pos.subWorld);
	}

	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IGraphNodeType::PinType> ScriptSetHeight::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSetHeight::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Set the height of ");
	result.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	result.append(" to ");
	result.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	return result.moveResults();
}

IScriptNodeType::Result ScriptSetHeight::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	if (auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 2))) {
		const auto height = readDataPin(environment, node, 3).asFloat(0);
		transform->setGlobalHeight(height);
	}

	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IGraphNodeType::PinType> ScriptSetSubworld::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSetSubworld::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Set the subworld of ");
	result.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	result.append(" to ");
	result.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	return result.moveResults();
}

IScriptNodeType::Result ScriptSetSubworld::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	if (auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 2))) {
		const auto subWorld = readDataPin(environment, node, 3).asInt(0);
		transform->setSubWorld(subWorld);
	}

	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IScriptNodeType::PinType> ScriptGetPosition::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 6>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptGetPosition::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Get the position of ");
	result.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	result.append(" with offset ");
	result.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return result.moveResults();
}

String ScriptGetPosition::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 1 || elementIdx == 3) {
		return "Position of " + getConnectedNodeName(world, node, graph, 0) + " + " + getConnectedNodeName(world, node, graph, 2);
	} else if (elementIdx == 4) {
		return "Subworld of " + getConnectedNodeName(world, node, graph, 0);
	} else {
		return "Height of " + getConnectedNodeName(world, node, graph, 0);
	}
}

String ScriptGetPosition::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "World Position";
	} else if (elementIdx == 2) {
		return "Offset";
	} else if (elementIdx == 3) {
		return "Position Vector";
	} else if (elementIdx == 4) {
		return "SubWorld";
	} else if (elementIdx == 5) {
		return "Global Height";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

ConfigNode ScriptGetPosition::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto offset = readDataPin(environment, node, 2).asVector2f({});

	const auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 0));
	if (transform) {
		if (pinN == 1) {
			return (WorldPosition(transform->getGlobalPosition(), transform->getSubWorld()) + offset).toConfigNode();
		} else if (pinN == 3) {
			return ConfigNode(transform->getGlobalPosition() + offset);
		} else if (pinN == 4) {
			return ConfigNode(transform->getSubWorld());
		} else if (pinN == 5) {
			return ConfigNode(transform->getGlobalHeight());
		}
	}
	return ConfigNode();
}



gsl::span<const IScriptNodeType::PinType> ScriptGetRotation::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptGetRotation::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Get the rotation of ");
	result.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return result.moveResults();
}

String ScriptGetRotation::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
    return "Rotation of " + getConnectedNodeName(world, node, graph, 0);
}

String ScriptGetRotation::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Rotation (Degrees)";
	}
	if (elementIdx == 2) {
		return "Rotation (Radians)";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

ConfigNode ScriptGetRotation::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 0));
	if (transform) {
		if (pinN == 1) {
			return ConfigNode(transform->getGlobalRotation().toDegrees());
		}
		if (pinN == 2) {
			return ConfigNode(transform->getGlobalRotation().toRadians());
		}
	}
	return ConfigNode();
}


gsl::span<const IGraphNodeType::PinType> ScriptSetRotation::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSetRotation::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Set the rotation of ");
	result.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	result.append(" to ");
	result.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	return result.moveResults();
}

String ScriptSetRotation::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Target";
	} else if (elementIdx == 3) {
		return "Rotation (Degrees)";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

IScriptNodeType::Result ScriptSetRotation::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto* transform2D = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 2));
	if (transform2D) {
		transform2D->setGlobalRotation(Angle1f::fromDegrees(readDataPin(environment, node, 3).asFloat(0)));
	}

	return Result(ScriptNodeExecutionState::Done);
}


gsl::span<const IGraphNodeType::PinType> ScriptSetScale::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSetScale::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder result;
	result.append("Set the scale of ");
	result.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	result.append(" to ");
	result.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	return result.moveResults();
}

IScriptNodeType::Result ScriptSetScale::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	if (auto* transform = environment.tryGetComponent<Transform2DComponent>(readEntityId(environment, node, 2))) {
		const auto scale = readDataPin(environment, node, 3);

		if (scale.getType() == ConfigNodeType::Float || scale.getType() == ConfigNodeType::Int) {
			const auto s = scale.asFloat();
			transform->setGlobalScale(Vector2f(s, s));
		} else {
			transform->setGlobalScale(scale.asVector2f());
		}
	}

	return Result(ScriptNodeExecutionState::Done);
}
