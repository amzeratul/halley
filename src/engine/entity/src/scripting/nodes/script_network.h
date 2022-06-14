#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptEntityAuthority final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "entityAuthority"; }
		String getName() const override { return "Entity Authority"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/entity_authority.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptHostAuthority final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "hostAuthority"; }
		String getName() const override { return "Host Authority"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/host_authority.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph) const override;
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
}
