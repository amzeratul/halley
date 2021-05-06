#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptPlayAnimation final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "playAnimation"; }
		String getName() const override { return "Play Animation"; }
		std::vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/play_animation.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
