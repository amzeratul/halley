#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptSetPosition final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setPosition"; }
		String getName() const override { return "Set Position"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/position.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSetHeight final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setHeight"; }
		String getName() const override { return "Set Height"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/height.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSetSubworld final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setSubworld"; }
		String getName() const override { return "Set Subworld"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/subworld.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptGetPosition final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "getPosition"; }
		String getName() const override { return "Get Position"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/position.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptGetRotation final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "getRotation"; }
		String getName() const override { return "Get Rotation"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/rotation.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptSetScale final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setScale"; }
		String getName() const override { return "Set Scale"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/scale.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
