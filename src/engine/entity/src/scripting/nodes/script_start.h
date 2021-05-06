#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptStart final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "start"; }
		String getName() const override { return "Start"; }
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/start.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }
		gsl::span<const PinType> getPinConfiguration() const override;
		bool canAdd() const override { return false; }
		bool canDelete() const override { return false; }
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
