#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptStart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "start"; }
		String getName() const override { return "Start"; }
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/start.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		bool canAdd() const override { return false; }
		bool canDelete() const override { return false; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptRestart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "restart"; }
		String getName() const override { return "Restart"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/restart.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
	
	class ScriptStop final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stop"; }
		String getName() const override { return "Stop"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/stop.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
	
	class ScriptSpinwait final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "spinwait"; }
		String getName() const override { return "Spinwait"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/spinwait.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptStartScript final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "startScript"; }
		String getName() const override { return "Start Script"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/start.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptStopScript final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stopScript"; }
		String getName() const override { return "Stop Script"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/stop.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptStopTag final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "stopTag"; }
		String getName() const override { return "Stop Tag"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/stop_tag.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
