#include "script_sprite.h"

#include "halley/graphics/sprite/animation_player.h"
#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include <components/sprite_component.h>

#include "halley/entity/world.h"
#include "components/sprite_animation_component.h"

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
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteDirection::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Set direction ");
	str.append(node.getSettings()["direction"].asString("right"), settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteDirection::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.setDirection(readEntityId(environment, node, 2), node.getSettings()["direction"].asString("right"));
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
