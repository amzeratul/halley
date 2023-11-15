#include "script_sprite.h"

#include "halley/graphics/sprite/animation_player.h"
#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include <components/sprite_component.h>

#include "halley/entity/world.h"
#include "components/sprite_animation_component.h"
#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptSpriteAnimation::getSettingTypes() const
{
	return {
		SettingType{ "sequence", "Halley::String", Vector<String>{"default"} },
		SettingType{ "loop", "bool", Vector<String>{"true"} },
		SettingType{ "wait", "bool", Vector<String>{"true"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSpriteAnimation::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteAnimation::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Play sequence ");
	str.append(node.getSettings()["sequence"].asString("default"), settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	if (node.getSettings()["loop"].asBool(true)) {
		str.append(" which loops ");
	}
	if (node.getSettings()["wait"].asBool(true)) {
		str.append(" and wait for it to finish ");
	}
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteAnimation::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto entity = environment.tryGetEntity(readEntityId(environment, node, 2));
	if (entity.isValid()) {
		auto* spriteAnimation = entity.tryGetComponent<SpriteAnimationComponent>();
		if (spriteAnimation) {
			if (spriteAnimation->player.getCurrentSequenceName() != node.getSettings()["sequence"].asString("")) {
				if (node.getSettings()["loop"].asBool(true)) {
					spriteAnimation->player.setSequence(node.getSettings()["sequence"].asString(""));
				}
				else {
					spriteAnimation->player.playOnce(node.getSettings()["sequence"].asString(""));
				}
			}

			if (node.getSettings()["wait"].asBool(true)) {
				if (spriteAnimation->player.isPlaying() && spriteAnimation->player.getCurrentSequenceName() == node.getSettings()["sequence"].asString("")) {
					return Result(ScriptNodeExecutionState::Executing);
				}
			}
		}
	}

	return Result(ScriptNodeExecutionState::Done);
}



Vector<IScriptNodeType::SettingType> ScriptSpriteDirection::getSettingTypes() const
{
	return { SettingType{ "direction", "Halley::String", Vector<String>{"right"} } };
}

gsl::span<const IScriptNodeType::PinType> ScriptSpriteDirection::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteDirection::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto dir = getConnectedNodeName(world, node, graph, 3);
	auto str = ColourStringBuilder(true);
	str.append("Set direction ");
	if (dir != "" && dir != "<empty>") {
		str.append(dir, parameterColour);
	} else {
		str.append(node.getSettings()["direction"].asString("right"), settingColour);
	}
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteDirection::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto dirPin = readDataPin(environment, node, 3);
	const auto dir = dirPin.getType() == ConfigNodeType::Undefined ? node.getSettings()["direction"].asString("right") : dirPin.asString();
	environment.setDirection(readEntityId(environment, node, 2), dir);
	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IScriptNodeType::PinType> ScriptSpriteAlpha::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteAlpha::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Set alpha of sprite ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	str.append(" to ");
	str.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteAlpha::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto* sprite = environment.tryGetComponent<SpriteComponent>(readEntityId(environment, node, 2));
	if (sprite) {
		const float value = readDataPin(environment, node, 3).asFloat(1.0f);
		sprite->sprite.getColour().a = value;
	}
	return Result(ScriptNodeExecutionState::Done);
}



Vector<IGraphNodeType::SettingType> ScriptSpriteActionPoint::getSettingTypes() const
{
	return {
		SettingType{ "actionPoint", "Halley::String", Vector<String>{""} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptSpriteActionPoint::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteActionPoint::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Get action point \"");
	str.append(node.getSettings()["actionPoint"].asString(""), parameterColour);
	str.append("\" for entity ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptSpriteActionPoint::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "World position";
	} else if (elementIdx == 2) {
		return "Offset";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

String ScriptSpriteActionPoint::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	const auto actionPoint = node.getSettings()["actionPoint"].asString("");

	if (elementIdx == 1) {
		return actionPoint + " of " + getConnectedNodeName(world, node, graph, 0) + " (world)";
	} else if (elementIdx == 2) {
		return actionPoint + " of " + getConnectedNodeName(world, node, graph, 0) + " (offset)";
	}

	return ScriptNodeTypeBase<void>::getShortDescription(world, node, graph, elementIdx);
}

ConfigNode ScriptSpriteActionPoint::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto actionPoint = node.getSettings()["actionPoint"].asString("");
	const auto entityId = readEntityId(environment, node, 0);

	const auto* spriteAnimation = environment.tryGetComponent<SpriteAnimationComponent>(entityId);
	if (!spriteAnimation) {
		return {};
	}

	const auto point = spriteAnimation->player.getCurrentActionPoint(actionPoint);

	if (pinN == 1) {
		const auto* transform = environment.tryGetComponent<Transform2DComponent>(entityId);
		if (!transform) {
			return {};
		}
		return (transform->getWorldPosition() + (point ? Vector2f(*point) : Vector2f())).toConfigNode();
	} else if (pinN == 2) {
		return point ? ConfigNode(Vector2f(*point)) : ConfigNode();
	}

	return {};
}

