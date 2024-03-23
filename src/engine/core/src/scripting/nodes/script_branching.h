#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptBranch final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "branch"; }
		String getName() const override { return "If"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/branch.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, uint8_t elementIdx) const override;
	};

	class ScriptFork final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "fork"; }
		String getName() const override { return "Fork"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/fork.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptMergeAny final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "mergeAny"; }
		String getName() const override { return "Merge Any"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/merge_one.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptMergeAll final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "mergeAll"; }
		String getName() const override { return "Merge All"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/merge_all.png"; }
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
