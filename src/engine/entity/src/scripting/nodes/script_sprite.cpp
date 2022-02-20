#include "script_sprite.h"

#include "halley/core/graphics/sprite/animation_player.h"
#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include "world.h"
#include "components/sprite_animation_component.h"

using namespace Halley;

Vector<IScriptNodeType::SettingType> ScriptSpriteAnimation::getSettingTypes() const
{
	return { SettingType{ "sequence", "Halley::String", Vector<String>{"default"} } };
}

gsl::span<const IScriptNodeType::PinType> ScriptSpriteAnimation::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteAnimation::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Play sequence ");
	str.append(node.getSettings()["sequence"].asString("default"), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteAnimation::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto entity = environment.tryGetEntity(readEntityId(environment, node, 2));
	if (entity.isValid()) {
		auto* spriteAnimation = entity.tryGetComponent<SpriteAnimationComponent>();
		if (spriteAnimation) {
			spriteAnimation->player.setSequence(node.getSettings()["sequence"].asString(""));
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
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteDirection::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Set direction ");
	str.append(node.getSettings()["direction"].asString("right"), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteDirection::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.setDirection(readEntityId(environment, node, 2), node.getSettings()["direction"].asString("right"));
	return Result(ScriptNodeExecutionState::Done);
}
