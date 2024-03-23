#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptSetPosition final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setPosition"; }
		String getName() const override { return "Set Position"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/position.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSetHeight final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setHeight"; }
		String getName() const override { return "Set Height"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/height.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSetSubworld final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setSubworld"; }
		String getName() const override { return "Set Subworld"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/subworld.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptGetPosition final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "getPosition"; }
		String getName() const override { return "Get Position"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/position.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptGetRotation final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "getRotation"; }
		String getName() const override { return "Get Rotation"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/rotation.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptSetRotation final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setRotation"; }
		String getName() const override { return "Set Rotation"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/rotation.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSetScale final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setScale"; }
		String getName() const override { return "Set Scale"; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/scale.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
