#include "script_sprite.h"

#include "halley/graphics/sprite/animation_player.h"
#ifndef DONT_INCLUDE_HALLEY_HPP
#define DONT_INCLUDE_HALLEY_HPP
#endif
#include <components/sprite_component.h>

#include "halley/entity/world.h"
#include "components/colour_component.h"
#include "components/sprite_animation_component.h"
#include "halley/entity/components/transform_2d_component.h"
#include "halley/maths/colour_gradient.h"

using namespace Halley;

ConfigNode ScriptSpriteAnimationData::toConfigNode(const EntitySerializationContext& context)
{
	return {};
}

Vector<IScriptNodeType::SettingType> ScriptSpriteAnimation::getSettingTypes() const
{
	return {
		SettingType{ "sequence", "Halley::String", Vector<String>{"default"} },
		SettingType{ "loop", "bool", Vector<String>{"true"} },
		SettingType{ "wait", "bool", Vector<String>{"true"} },
		SettingType{ "reverse", "bool", Vector<String>{"false"} },
		SettingType{ "sync", "bool", Vector<String>{"false"} },
		SettingType{ "speed", "float", Vector<String>{"1"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSpriteAnimation::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } });
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteAnimation::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Play sequence ");
	str.append(node.getSettings()["sequence"].asString("default"), settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	if (node.getSettings()["reverse"].asBool(false)) {
		str.append(" in reverse ");
	}
	if (node.getSettings()["loop"].asBool(true)) {
		str.append(" which loops ");
	}
	if (node.getSettings()["wait"].asBool(true)) {
		str.append(" and wait for it to finish ");
	}
	if (node.getSettings()["sync"].asBool(false)) {
		str.append(". Animation will be synchronised over network");
	}
	return str.moveResults();
}

void ScriptSpriteAnimation::doInitData(ScriptSpriteAnimationData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.playId = {};
}

IScriptNodeType::Result ScriptSpriteAnimation::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptSpriteAnimationData& data) const
{
	const auto entityId = readEntityId(environment, node, 2);

	if (auto* spriteAnimation = environment.tryGetComponent<SpriteAnimationComponent>(entityId)) {
		const auto& sequence = node.getSettings()["sequence"].asString("");
		const bool loop = node.getSettings()["loop"].asBool(true);
		const auto reverse = node.getSettings()["reverse"].asBool(false);
		const float speed = node.getSettings()["speed"].asFloat(1.0f);
		spriteAnimation->player.setPlaybackSpeed(speed);

		if (!data.playId) {
			if (loop) {
				data.playId = spriteAnimation->player.setSequence(sequence);
			} else {
				data.playId = spriteAnimation->player.playOnce(sequence, {}, reverse);
			}
			if (node.getSettings()["sync"].asBool(false)) {
				auto entity = environment.tryGetEntity(entityId);
				if (environment.getWorld().isEntityNetworkOwner(entity)) {
					environment.postAnimationEvent(sequence, reverse, !loop, entity.getEntityId());
				}
			}
		}

		if (node.getSettings()["wait"].asBool(true)) {
			if (spriteAnimation->player.getCurrentPlayId() == data.playId) {
				return Result(ScriptNodeExecutionState::Executing);
			}
		}
	}

	return Result(ScriptNodeExecutionState::Done);
}



gsl::span<const IGraphNodeType::PinType> ScriptSpriteAnimationState::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output }
	});
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteAnimationState::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Get animation info for ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptSpriteAnimationState::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Target";
	} else if (elementIdx == 1) {
		return "Sequence";
	} else if (elementIdx == 2) {
		return "Direction";
	} else if (elementIdx == 3) {
		return "Frame";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

String ScriptSpriteAnimationState::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto target = getConnectedNodeName(node, graph, 0);
	if (elementIdx == 1) {
		return target + ".sequence";
	} else if (elementIdx == 2) {
		return target + ".direction";
	} else if (elementIdx == 3) {
		return target + ".frame";
	}
	return ScriptNodeTypeBase<void>::getShortDescription(node, graph, elementIdx);
}

ConfigNode ScriptSpriteAnimationState::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	if (const auto* spriteAnimation = environment.tryGetComponent<SpriteAnimationComponent>(readEntityId(environment, node, 0))) {
		const auto& player = spriteAnimation->player;

		if (pinN == 1) {
			return ConfigNode(player.getCurrentSequenceName());
		} else if (pinN == 2) {
			return ConfigNode(player.getCurrentDirectionName());
		} else if (pinN == 3) {
			return ConfigNode(player.getCurrentSequenceFrame());
		}
	}

	return {};
}


Vector<IScriptNodeType::SettingType> ScriptSpriteDirection::getSettingTypes() const
{
	return { SettingType{ "direction", "Halley::String", Vector<String>{"right"} } };
}

gsl::span<const IScriptNodeType::PinType> ScriptSpriteDirection::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } });
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteDirection::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	const auto dir = getConnectedNodeName(node, graph, 3);
	auto str = ColourStringBuilder(true);
	str.append("Set direction ");
	if (dir != "" && dir != "<empty>") {
		str.append(dir, parameterColour);
	} else {
		str.append(node.getSettings()["direction"].asString("right"), settingColour);
	}
	str.append(" on entity ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteDirection::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto dirPin = readDataPin(environment, node, 3);
	const auto dir = dirPin.getType() == ConfigNodeType::Undefined ? node.getSettings()["direction"].asString("right") : dirPin.asString();
	environment.setDirection(readEntityId(environment, node, 2), dir);
	return Result(ScriptNodeExecutionState::Done);
}


gsl::span<const IGraphNodeType::PinType> ScriptSpriteGetDirection::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output }
	});
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteGetDirection::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Get direction of ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	return str.moveResults();
}

String ScriptSpriteGetDirection::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Direction of " + getConnectedNodeName(node, graph, 2);
}

ConfigNode ScriptSpriteGetDirection::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	if (auto* spriteAnimation = environment.tryGetComponent<SpriteAnimationComponent>(readEntityId(environment, node, 0))) {
		return ConfigNode(spriteAnimation->player.getCurrentDirectionName());
	}
	return {};
}


gsl::span<const IScriptNodeType::PinType> ScriptSpriteAlpha::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input } });
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteAlpha::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Set alpha of sprite ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	str.append(" to ");
	str.append(getConnectedNodeName(node, graph, 3), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSpriteAlpha::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto entityId = readRawEntityId(environment, node, 2);
	const float value = readDataPin(environment, node, 3).asFloat(1.0f);
	auto e = environment.tryGetEntity(entityId);

	if (!e.isValid()) {
		Logger::logError("Can't set sprite alpha for entityId " + toString(entityId) + " - entity not found");
	} else if (auto* colour = e.tryGetComponent<ColourComponent>(true)) {
		colour->colour.a = value;
	} else if (auto* sprite = e.tryGetComponent<SpriteComponent>(true)) {
		sprite->sprite.getColour().a = value;
	} else {
		Logger::logError("Can't set sprite alpha for entity \"" + e.getName() + "\" - no Colour or Sprite component");
	}

	return Result(ScriptNodeExecutionState::Done);
}



Vector<IGraphNodeType::SettingType> ScriptSpriteActionPoint::getSettingTypes() const
{
	return {
		SettingType{ "actionPoint", "Halley::String", Vector<String>{""} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptSpriteActionPoint::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input }
	});
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteActionPoint::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Get action point \"");
	str.append(node.getSettings()["actionPoint"].asString(""), settingColour);
	str.append("\" for entity ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptSpriteActionPoint::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "World position";
	} else if (elementIdx == 2) {
		return "Local position";
	} else if (elementIdx == 3) {
		return "Has point";
	} else if (elementIdx == 4) {
		return "Offset";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

String ScriptSpriteActionPoint::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	const auto actionPoint = node.getSettings()["actionPoint"].asString("");

	if (elementIdx == 1) {
		return actionPoint + " of " + getConnectedNodeName(node, graph, 0) + " (world)";
	} else if (elementIdx == 2) {
		return actionPoint + " of " + getConnectedNodeName(node, graph, 0) + " (local)";
	} else if (elementIdx == 2) {
		return getConnectedNodeName(node, graph, 0) + " has " + actionPoint;
	}

	return ScriptNodeTypeBase<void>::getShortDescription(node, graph, elementIdx);
}

ConfigNode ScriptSpriteActionPoint::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto actionPoint = node.getSettings()["actionPoint"].asString("");
	const auto entityId = readEntityId(environment, node, 0);
	const auto offset = readDataPin(environment, node, 4).asVector2f(Vector2f());

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
		return (transform->getWorldPosition() + ((point ? Vector2f(*point) : Vector2f()) + offset)).toConfigNode();
	} else if (pinN == 2) {
		return point ? ConfigNode(Vector2f(*point) + offset) : ConfigNode();
	} else if (pinN == 3) {
		return ConfigNode(point.has_value());
	}

	return {};
}



Vector<IGraphNodeType::SettingType> ScriptColourGradient::getSettingTypes() const
{
	return {
		SettingType{ "gradient", "Halley::ColourGradient", Vector<String>{""} },
		SettingType{ "bias", "Halley::Colour4f", Vector<String>{"#00000000"} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptColourGradient::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output }
	});
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptColourGradient::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Sample colour at ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptColourGradient::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Position along gradient (0..1)";
	} else if (elementIdx == 1) {
		return "Colour";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

String ScriptColourGradient::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Colour from Gradient";
}

ConfigNode ScriptColourGradient::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto samplePos = clamp(readDataPin(environment, node, 0).asFloat(), 0.0f, 1.0f);
	const auto gradient = ColourGradient(node.getSettings()["gradient"]);
	const auto bias = Colour4f(node.getSettings()["bias"]);
	return (gradient.evaluatePrecomputed(samplePos) + bias).toConfigNode();
}



Vector<IGraphNodeType::SettingType> ScriptSpriteCenter::getSettingTypes() const
{
	return { };
}

gsl::span<const IGraphNodeType::PinType> ScriptSpriteCenter::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::to_array({
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output }
	});
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSpriteCenter::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Return sprite rect center position");
	return str.moveResults();
}

String ScriptSpriteCenter::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "EntityId";
	} else if (elementIdx == 1) {
		return "Position";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

String ScriptSpriteCenter::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Sprite Center";
}

ConfigNode ScriptSpriteCenter::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto entity = environment.tryGetEntity(readEntityId(environment, node, 0));
	if (!entity.isValid()) {
		return {};
	}
	const auto* spriteComponent = entity.tryGetComponent<SpriteComponent>();
	if (spriteComponent == nullptr) {
		return {};
	}
	return ConfigNode(spriteComponent->sprite.getAABB().getCenter());
}

