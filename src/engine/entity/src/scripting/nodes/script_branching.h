#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptIf final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "if"; }
		String getName() const override { return "Branch If"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/branch.png"; }
		gsl::span<const PinType> getPinConfiguration() const override;
		std::pair<String, std::vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World& world) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::FlowControl; }
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
		std::pair<String, std::vector<ColourOverride>> getPinDescription(const ScriptGraphNode& node, PinType elementType, uint8_t elementIdx) const override;
	};
}
