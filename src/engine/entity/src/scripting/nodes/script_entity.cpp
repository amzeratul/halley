#include "script_entity.h"

#include "world.h"
#include "components/transform_2d_component.h"
using namespace Halley;

ConfigNode ScriptSpawnEntityData::toConfigNode(const EntitySerializationContext& context)
{
	return ConfigNodeSerializer<EntityId>().serialize(entityId, context);
}

bool ScriptSpawnEntity::canKeepData() const
{
	return true;
}

Vector<IScriptNodeType::SettingType> ScriptSpawnEntity::getSettingTypes() const
{
	return {
		SettingType{ "prefab", "Halley::ResourceReference<Halley::Prefab>", Vector<String>{""} },
		SettingType{ "asChild", "bool", Vector<String>{"0"} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptSpawnEntity::getPinConfiguration(const ScriptGraphNode& node) const
{
	const bool asChild = node.getSettings()["asChild"].asBool(false);

	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::TargetPin, PD::Output }, PinType{ ET::TargetPin, PD::Input } };
	return gsl::span<const PinType>(data).subspan(0, asChild ? 5 : 4);
}

std::pair<String, Vector<ColourOverride>> ScriptSpawnEntity::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const bool asChild = node.getSettings()["asChild"].asBool(false);

	auto str = ColourStringBuilder(true);
	str.append("Spawn entity ");
	str.append(node.getSettings()["prefab"].asString(""), parameterColour);
	str.append(" at position ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	if (asChild) {
		str.append(" relative to parent ");
		str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	}
	return str.moveResults();
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
	const Vector2f position = readDataPin(environment, node, 2).asVector2f(Vector2f());

	if (!prefab.isEmpty()) {
		EntityRef parent;
		if (asChild) {
			parent = environment.getWorld().getEntity(readEntityId(environment, node, 4));
		}
		auto factory = EntityFactory(environment.getWorld(), environment.getResources());
		auto entity = factory.createEntity(prefab, parent);

		if (auto* transform = entity.tryGetComponent<Transform2DComponent>()) {
			transform->setLocalPosition(position);
		}

		data.entityId = entity.getEntityId();
	}

	return Result(ScriptNodeExecutionState::Done);
}

EntityId ScriptSpawnEntity::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN, ScriptSpawnEntityData& curData) const
{
	return curData.entityId;
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
		str.append(toString(node.getSettings()["childName"].asString("")), parameterColour);
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