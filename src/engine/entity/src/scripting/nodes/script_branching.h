#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptBranch final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "branch"; }
		String getName() const override { return "Branch If"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/branch.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
		std::pair<String, std::vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const override;
	};

	class ScriptFork final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "fork"; }
		String getName() const override { return "Fork"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/fork.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptMergeAny final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "mergeAny"; }
		String getName() const override { return "Merge Any"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/merge_one.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptMergeAll final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "mergeAll"; }
		String getName() const override { return "Merge All"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/merge_all.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
