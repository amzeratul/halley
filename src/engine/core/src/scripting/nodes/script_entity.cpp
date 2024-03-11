#include "script_entity.h"
#include "halley/scripting/script_state_set.h"

#include <components/scriptable_component.h>

#include "halley/entity/world.h"
#include "halley/entity/components/transform_2d_component.h"
#include "halley/navigation/world_position.h"
using namespace Halley;

ConfigNode ScriptSpawnEntityData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNodeSerializer<EntityId>().serialize(entityId, context);
}

bool ScriptSpawnEntity::canKeepData() const
{
	return true;
}

bool ScriptSpawnEntity::hasDestructor(const ScriptGraphNode& node) const
{
	return node.getSettings()["autoDestroy"].asBool(false);
}

Vector<IScriptNodeType::SettingType> ScriptSpawnEntity::getSettingTypes() const
{
	return {
		SettingType{ "prefab", "Halley::ResourceReference<Halley::Prefab>", Vector<String>{""} },
		SettingType{ "asChild", "bool", Vector<String>{"false"} },
		SettingType{ "serializable", "bool", Vector<String>{"true"} },
		SettingType{ "autoDestroy", "bool", Vector<String>{"false"} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSpawnEntity::getPinConfiguration(const ScriptGraphNode& node) const
{
	const bool asChild = node.getSettings()["asChild"].asBool(false);

	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 6>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::TargetPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::TargetPin, PD::Input }
	};
	return gsl::span<const PinType>(data).subspan(0, asChild ? 6 : 5);
}

std::pair<String, Vector<ColourOverride>> ScriptSpawnEntity::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const bool asChild = node.getSettings()["asChild"].asBool(false);
	const bool serializable = node.getSettings()["serializable"].asBool(true);

	auto str = ColourStringBuilder(true);
	str.append("Spawn entity ");
	str.append(node.getSettings()["prefab"].asString(""), settingColour);
	str.append(" at position ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	str.append(" with rotation ");
	str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	if (asChild) {
		str.append(" relative to parent ");
		str.append(getConnectedNodeName(world, node, graph, 5), parameterColour);
	}
	if (serializable) {
		str.append(" and serialize it ");
	} else {
		str.append(" and don't serialize it ");
	}
	return str.moveResults();
}

String ScriptSpawnEntity::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Position";
	}
	if (elementIdx == 4) {
		return "Rotation (Radians)";
	}
	return ScriptNodeTypeBase<ScriptSpawnEntityData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptSpawnEntity::doInitData(ScriptSpawnEntityData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	data.entityId = ConfigNodeSerializer<EntityId>().deserialize(context, nodeData);
}

IScriptNodeType::Result ScriptSpawnEntity::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptSpawnEntityData& data) const
{
	data.entityId = EntityId();

	const auto& prefab = node.getSettings()["prefab"].asString("");
	const bool asChild = node.getSettings()["asChild"].asBool(false);
	const bool serializable = node.getSettings()["serializable"].asBool(true);
	const Vector3f position = readDataPin(environment, node, 2).asVector3f({});
	const Angle1f rotation = Angle1f(readDataPin(environment, node, 4).asFloat(0.0f));

	if (!prefab.isEmpty()) {
		EntityRef parent;
		if (asChild) {
			parent = environment.getWorld().getEntity(readEntityId(environment, node, 5));
		}
		auto factory = EntityFactory(environment.getWorld(), environment.getResources());
		auto entity = factory.createEntity(prefab, parent);

		if (auto* transform = entity.tryGetComponent<Transform2DComponent>()) {
			transform->setLocalPosition(position.xy());
			transform->setLocalHeight(position.z);
			transform->setLocalRotation(rotation);
		}

		entity.setSerializable(serializable);
		data.entityId = entity.getEntityId();
	}

	return Result(ScriptNodeExecutionState::Done);
}

EntityId ScriptSpawnEntity::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN, ScriptSpawnEntityData& curData) const
{
	return curData.entityId;
}

void ScriptSpawnEntity::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptSpawnEntityData& curData) const
{
	if (node.getSettings()["autoDestroy"].asBool(false)) {
		const auto e = environment.getWorld().tryGetEntity(curData.entityId);
		if (e.isValid() && e.isAlive()) {
			environment.getWorld().destroyEntity(curData.entityId);
		}
		curData.entityId = {};
	}
}


gsl::span<const IScriptNodeType::PinType> ScriptDestroyEntity::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptDestroyEntity::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Destroy entity ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptDestroyEntity::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.getWorld().destroyEntity(readEntityId(environment, node, 2));
	return Result(ScriptNodeExecutionState::Done);
}


Vector<IGraphNodeType::SettingType> ScriptFindChildByName::getSettingTypes() const
{
	return {
		SettingType{ "childName", "Halley::String", Vector<String>{""} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptFindChildByName::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::TargetPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptFindChildByName::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Try to find first level child with name ");

	if (node.getPin(0).hasConnection()) {
		str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	}
	else {
		str.append(toString(node.getSettings()["childName"].asString("")), settingColour);
	}

	str.append(" on ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);

	return str.moveResults();
}

EntityId ScriptFindChildByName::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	const auto entityRef = environment.getWorld().tryGetEntity(readEntityId(environment, node, 1));
	if (entityRef.isValid()) {
		auto childName = node.getSettings()["childName"].asString("");
		if (node.getPin(0).hasConnection()) {
			childName = readDataPin(environment, node, 0).asString("");
		}
		if (childName.isEmpty()) {
			return {};
		}
		const auto childRef = entityRef.getChildWithName(childName);
		if (childRef.isValid()) {
			return childRef.getEntityId();
		}
	}

	return {};
}



Vector<IGraphNodeType::SettingType> ScriptGetParent::getSettingTypes() const
{
	return { };
}

gsl::span<const IScriptNodeType::PinType> ScriptGetParent::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::TargetPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptGetParent::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Get parent of entity ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return str.moveResults();
}

EntityId ScriptGetParent::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	const auto entityRef = environment.getWorld().tryGetEntity(readEntityId(environment, node, 0));
	if (entityRef.getParent().isValid()) {
		return entityRef.getParent().getEntityId();
	}
	return {};
}


Vector<IGraphNodeType::SettingType> ScriptEntityReference::getSettingTypes() const
{
	return {
		SettingType{ "key", "Halley::String", Vector<String>{""} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptEntityReference::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::TargetPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptEntityReference::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Entity reference set in ScriptableComponent with key ");
	if (node.getPin(2).hasConnection()) {
		str.append(getConnectedNodeName(world, node, graph, 2), settingColour);
	} else {
		str.append(node.getSettings()["key"].asString(""), settingColour);
	}
	return str.moveResults();
}

String ScriptEntityReference::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		if (node.getPin(2).hasConnection()) {
			return getConnectedNodeName(world, node, graph, 2);
		} else {
			return node.getSettings()["key"].asString("");
		}
	} else {
		if (node.getPin(2).hasConnection()) {
			return "pos(" + getConnectedNodeName(world, node, graph, 2) + ")";
		} else {
			return "pos(" + node.getSettings()["key"].asString("") + ")";
		}
	}
}

String ScriptEntityReference::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["key"].asString("");
}

String ScriptEntityReference::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Position";
	}
	if (elementIdx == 2) {
		return "Key";
	}
	return "Entity";
}

EntityId ScriptEntityReference::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	auto key = node.getSettings()["key"].asString("");
	if (node.getPin(2).hasConnection()) {
		key = readDataPin(environment, node, 2).asString();
	}
	if (key.isEmpty()) {
		return {};
	}

	const auto entityRef = environment.tryGetEntity({});
	const auto& references = entityRef.getComponent<ScriptableComponent>().entityReferences;
	const auto find = references.find(key);
	if (find == references.end()) {
		return {};
	}

	return find->second;
}

ConfigNode ScriptEntityReference::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto entityId = doGetEntityId(environment, node, 0);
	if (!entityId.isValid()) {
		return {};
	}
	if (auto* transform = environment.tryGetComponent<Transform2DComponent>(entityId)) {
		return transform->getWorldPosition().toConfigNode();
	} else {
		return {};
	}
}



Vector<IGraphNodeType::SettingType> ScriptEntityParameter::getSettingTypes() const
{
	return {
		SettingType{ "key", "Halley::String", Vector<String>{""} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptEntityParameter::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::TargetPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptEntityParameter::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Entity parameter set in ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	str.append("'s ScriptableComponent with key ");
	str.append(node.getSettings()["key"].asString(""), settingColour);
	return str.moveResults();
}

String ScriptEntityParameter::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Param \"" + node.getSettings()["key"].asString("") + "\"";
}

String ScriptEntityParameter::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["key"].asString("");
}

ConfigNode ScriptEntityParameter::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto key = node.getSettings()["key"].asString("");
	if (key.isEmpty()) {
		return {};
	}

	const auto entityRef = environment.tryGetEntity(readEntityId(environment, node, 1));
	if (!entityRef.isValid()) {
		Logger::logError("Trying to access ScriptEntityParameter, but EntityRef is not valid! " + toString(readEntityId(environment, node, 1)));
		return {};
	}
    if (!entityRef.hasComponent<ScriptableComponent>()) {
		Logger::logError("Entity with name " + entityRef.getName() + " does not have ScriptableComponent attached to it!");
		return {};
	}
	const auto& params = entityRef.getComponent<ScriptableComponent>().entityParams;
	const auto find = params.find(key);
	if (find == params.end()) {
		return {};
	}

	return ConfigNode(find->second);
}



Vector<IGraphNodeType::SettingType> ScriptEntityTargetReference::getSettingTypes() const
{
	return {
		SettingType{ "scriptTargetId", "Halley::ScriptTargetId", Vector<String>{""} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptEntityTargetReference::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptEntityTargetReference::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Entity with ScriptTarget reference ");
	str.append(node.getSettings()["scriptTargetId"].asString(""), settingColour);
	return str.moveResults();
}

String ScriptEntityTargetReference::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return node.getSettings()["scriptTargetId"].asString("");
	} else {
		return "pos(" + node.getSettings()["scriptTargetId"].asString("") + ")";
	}
}

String ScriptEntityTargetReference::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["scriptTargetId"].asString("");
}

EntityId ScriptEntityTargetReference::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	return environment.getScriptTarget(node.getSettings()["scriptTargetId"].asString(""));
}

ConfigNode ScriptEntityTargetReference::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto entityId = doGetEntityId(environment, node, 0);
	if (!entityId.isValid()) {
		return {};
	}
	if (auto* transform = environment.tryGetComponent<Transform2DComponent>(entityId)) {
		return transform->getWorldPosition().toConfigNode();
	} else {
		return {};
	}
}



Vector<IGraphNodeType::SettingType> ScriptHasTags::getSettingTypes() const
{
	return {
		SettingType{ "tags", "Halley::Vector<Halley::String>", Vector<String>{""} }
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptHasTags::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptHasTags::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Checks if ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(" has tags ");
	str.append("[" + String::concatList(node.getSettings()["tags"].asVector<String>({}), ", ") + "]", settingColour);
	return str.moveResults();
}

String ScriptHasTags::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getConnectedNodeName(world, node, graph, 0) + " has tags " + "[" + String::concatList(node.getSettings()["tags"].asVector<String>({}), ", ") + "]";
}

ConfigNode ScriptHasTags::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto* scriptable = environment.tryGetComponent<ScriptableComponent>(readEntityId(environment, node, 0));
	if (!scriptable) {
		return ConfigNode(false);
	}

	for (const auto& tag: node.getSettings()["tags"].asSequence()) {
		if (!std_ex::contains(scriptable->tags, tag.asString())) {
			return ConfigNode(false);
		}
	}

	return ConfigNode(true);
}


ConfigNode ScriptToggleEntityEnabledData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["entityId"] = ConfigNodeSerializer<EntityId>().serialize(entityId, context);
	result["previousState"] = previousState;
	return result;
}

String ScriptToggleEntityEnabled::getIconName(const ScriptGraphNode& node) const
{
	if (node.getSettings().getType() == ConfigNodeType::Map && node.getSettings()["enabled"].asBool(true)) {
		return "script_icons/toggle_enabled_on.png";
	}
	return "script_icons/toggle_enabled_off.png";
}

Vector<IGraphNodeType::SettingType> ScriptToggleEntityEnabled::getSettingTypes() const
{
	return {
		SettingType{ "enabled", "bool", Vector<String>{"true"} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptToggleEntityEnabled::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptToggleEntityEnabled::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Toggle entity enabled to ");
	str.append(node.getSettings()["enabled"].asString("true"), parameterColour);
	str.append(" and return to previous after");
	return str.moveResults();
}

void ScriptToggleEntityEnabled::doInitData(ScriptToggleEntityEnabledData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	if (nodeData.getType() != ConfigNodeType::Map) {
		return;
	}

	data.entityId = ConfigNodeSerializer<EntityId>().deserialize(context, nodeData["entityId"]);
	data.previousState = nodeData["previousState"].asBool(false);
}

IScriptNodeType::Result ScriptToggleEntityEnabled::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptToggleEntityEnabledData& data) const
{
	const auto entityId = readEntityId(environment, node, 2);
	if (!entityId.isValid()) {
		Logger::logError("Entity with id " + toString(entityId) + " does not exist and can't be toggled!");
		return Result(ScriptNodeExecutionState::Done);
	}
	auto entityRef = environment.getWorld().getEntity(entityId);
	data.entityId = entityRef.getEntityId();
	data.previousState = entityRef.isEnabled();
	entityRef.setEnabled(node.getSettings()["enabled"].asBool(true));
	return Result(ScriptNodeExecutionState::Done);
}

void ScriptToggleEntityEnabled::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptToggleEntityEnabledData& data) const
{
	auto entityRef = environment.getWorld().getEntity(data.entityId);
	entityRef.setEnabled(data.previousState);
}