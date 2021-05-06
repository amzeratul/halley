#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptStart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "start"; }
		String getName() const override { return "Start"; }
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/start.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		gsl::span<const PinType> getPinConfiguration() const override;
		bool canAdd() const override { return false; }
		bool canDelete() const override { return false; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptRestart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "restart"; }
		String getName() const override { return "Restart"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/restart.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
	
	class ScriptStop final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stop"; }
		String getName() const override { return "Stop"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/stop.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
