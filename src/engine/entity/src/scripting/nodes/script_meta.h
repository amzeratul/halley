#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptComment final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "comment"; }
		String getName() const override { return "Comment"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/comment.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Comment; }

		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
	};
}
