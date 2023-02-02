#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptEntityAuthority final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "entityAuthority"; }
		String getName() const override { return "Entity Authority"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/entity_authority.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptHostAuthority final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "hostAuthority"; }
		String getName() const override { return "Host Authority"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/host_authority.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptIfEntityAuthority final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "ifEntityAuthority"; }
		String getName() const override { return "If Entity Authority"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/entity_authority.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptIfHostAuthority final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "ifHostAuthority"; }
		String getName() const override { return "If Host Authority"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/host_authority.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};


	class ScriptLockData : public ScriptStateData<ScriptLockData> {
	public:
		ScriptLockData() = default;
		ScriptLockData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		std::optional<int32_t> requestPending;
	};

	class ScriptLock final : public ScriptNodeTypeBase<ScriptLockData> {
	public:
		String getId() const override { return "lock"; }
		String getName() const override { return "Lock"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/lock.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		void doInitData(ScriptLockData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLockData& data) const override;
	};

	class ScriptLockAvailable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "lockAvailable"; }
		String getName() const override { return "Lock Available"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/lock_available.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};


	class ScriptLockAvailableGateData : public ScriptStateData<ScriptLockAvailableGateData> {
	public:
		ScriptLockAvailableGateData() = default;
		ScriptLockAvailableGateData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		std::optional<bool> flowing;
	};

	class ScriptLockAvailableGate final : public ScriptNodeTypeBase<ScriptLockAvailableGateData> {
	public:
		String getId() const override { return "lockAvailableGate"; }
		String getName() const override { return "Lock Available Gate"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/lock_available.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::State; }

		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		void doInitData(ScriptLockAvailableGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLockAvailableGateData& curData) const override;
	};
}
