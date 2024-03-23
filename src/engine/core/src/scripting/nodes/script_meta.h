#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptComment final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "comment"; }
		String getName() const override { return "Comment"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/comment.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Comment; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		String getLargeLabel(const BaseGraphNode& node) const override;
	};

	class ScriptDebugDisplay final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "debugDisplay"; }
		String getName() const override { return "Debug Display"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/debug_display.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::DebugDisplay; }

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
	};

	class ScriptLog final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "log"; }
		String getName() const override { return "Log"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/comment.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		String getLargeLabel(const BaseGraphNode& node) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
