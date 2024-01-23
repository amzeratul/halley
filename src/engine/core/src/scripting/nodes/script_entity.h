#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptSpawnEntityData final : public ScriptStateData<ScriptSpawnEntityData> {
	public:
		EntityId entityId;
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptSpawnEntity final : public ScriptNodeTypeBase<ScriptSpawnEntityData> {
	public:
		String getId() const override { return "spawnEntity"; }
		String getName() const override { return "Spawn Entity"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/spawn_entity.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		bool canKeepData() const override;
		bool hasDestructor(const ScriptGraphNode& node) const override;

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptSpawnEntityData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptSpawnEntityData& data) const override;
		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN, ScriptSpawnEntityData& curData) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptSpawnEntityData& curData) const override;
	};

	class ScriptDestroyEntity final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "destroyEntity"; }
		String getName() const override { return "Destroy Entity"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/destroy_entity.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptFindChildByName final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "findChildByName"; }
		String getName() const override { return "Find E. Child"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/find_child.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;
	};

	class ScriptGetParent final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "getParent"; }
		String getName() const override { return "Get Parent"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/get_parent.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;
	};

	class ScriptEntityReference final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "entityReference"; }
		String getName() const override { return "Entity Reference"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/entity_ref.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getLargeLabel(const ScriptGraphNode& node) const override;

		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptEntityTargetReference final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "entityTargetRef"; }
		String getName() const override { return "Script Target"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/entity_ref.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getLargeLabel(const ScriptGraphNode& node) const override;

		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptHasTags : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "hasTags"; }
		String getName() const override { return "Has Tags"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/tag.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptToggleEntityEnabledData final : public ScriptStateData<ScriptToggleEntityEnabledData> {
	public:
		EntityId entityId;
		bool previousState = false;
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptToggleEntityEnabled final : public ScriptNodeTypeBase<ScriptToggleEntityEnabledData> {
	public:
		String getId() const override { return "toggleEntityEnabled"; }
		String getName() const override { return "Toggle Enabled"; }
		String getIconName(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		bool hasDestructor(const ScriptGraphNode& node) const override { return true; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		void doInitData(ScriptToggleEntityEnabledData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptToggleEntityEnabledData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptToggleEntityEnabledData& curData) const override;
	};
}
