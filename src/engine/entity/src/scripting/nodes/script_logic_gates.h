#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptLogicGateAnd final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "logicGateAnd"; }
		String getName() const override { return "Logic Gate AND"; }
		String getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/logic_gate_and.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptLogicGateOr final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "logicGateOr"; }
		String getName() const override { return "Logic Gate OR"; }
		String getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/logic_gate_or.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptLogicGateXor final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "logicGateXor"; }
		String getName() const override { return "Logic Gate XOR"; }
		String getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/logic_gate_xor.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptLogicGateNot final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "logicGateNot"; }
		String getName() const override { return "Logic Gate NOT"; }
		String getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/logic_gate_not.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
}
