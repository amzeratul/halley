#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptVariable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "variable"; }
		String getName() const override { return "Variable"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/variable.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const override;
		ConfigNode doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node) const override;

	private:
		ScriptVariableScope getScope(const ScriptGraphNode& node) const;
	};

	class ScriptEntityVariable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "entityVariable"; }
		String getName() const override { return "Entity Variable"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/variable.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
		ConfigNode doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const override;
	};
	
	class ScriptLiteral final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "literal"; }
		String getName() const override { return "Literal"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/literal.png"; }
		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;

	private:
		ConfigNode getConfigNode(const ScriptGraphNode& node) const;
	};

	
	class ScriptVariableTable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "variableTable"; }
		String getName() const override { return "Variable Table"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/variable_table.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		Vector<SettingType> getSettingTypes() const override;
		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptECSVariable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "ecsVariable"; }
		String getName() const override { return "ECS Variable"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/ecs_variable.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }

		Vector<SettingType> getSettingTypes() const override;
		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
		void doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const override;
	};


	class ScriptColourLiteral final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "colourLiteral"; }
		String getName() const override { return "Colour Literal"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/colour_literal.png"; }
		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Variable; }
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptComparison final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "comparison"; }
		String getName() const override { return "Comparison"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/comparison.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }
		
		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptArithmetic final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "arithmetic"; }
		String getName() const override { return "Arithmetic"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/arithmetic.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getLargeLabel(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptValueOr final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "valueOr"; }
		String getName() const override { return "Value Or"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/value_or.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptConditionalOperator final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "conditionalOperator"; }
		String getName() const override { return "Conditional Operator"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/value_or.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptLerp final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "lerp"; }
		String getName() const override { return "Lerp"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/lerp.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
	
	class ScriptAdvanceTo final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "advanceTo"; }
		String getName() const override { return "Advance Variable To"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/advanceTo.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
	
	class ScriptSetVariable final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "setVariable"; }
		String getName() const override { return "Variable Set"; }
		String getLabel(const ScriptGraphNode& node) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/set_variable.png"; }
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;

		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptHoldVariableData : public ScriptStateData<ScriptHoldVariableData> {
	public:
		ConfigNode prevValue;
		bool started = false;

		ScriptHoldVariableData() = default;
		ScriptHoldVariableData(const ConfigNode& node);
		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptHoldVariable final : public ScriptNodeTypeBase<ScriptHoldVariableData> {
	public:
		String getId() const override { return "holdVariable"; }
		String getName() const override { return "Variable Hold"; }
		String getLabel(const ScriptGraphNode& node) const override;
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/set_variable.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		bool hasDestructor(const ScriptGraphNode& node) const override { return true; }
		Vector<SettingType> getSettingTypes() const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		void doInitData(ScriptHoldVariableData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptHoldVariableData& curData) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptHoldVariableData& curData) const override;
	};

	class ScriptEntityIdToData final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "convEntityIdToData"; }
		String getName() const override { return "Conv EntityId->Data"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convEntityIdToData.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptDataToEntityId final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "convDataToEntityId"; }
		String getName() const override { return "Conv Data->EntityId"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convDataToEntityId.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }
		
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;

		EntityId doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const override;
	};

	class ScriptToVector final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "toVector"; }
		String getName() const override { return "To Vector2"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/toVector.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }
		
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptFromVector final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "fromVector"; }
		String getName() const override { return "From Vector2"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/fromVector.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }
		
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptInsertValueIntoMap final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "insertValueIntoMap"; }
		String getName() const override { return "Insert Value->Map"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convDataToEntityId.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptGetValueFromMap final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "getValueFromMap"; }
		String getName() const override { return "Get Value<-Map"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convEntityIdToData.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptPackMap final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "packMap"; }
		String getName() const override { return "Pack Map"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/map_pack.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptUnpackMap final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "unpackMap"; }
		String getName() const override { return "Unpack Map"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/map_unpack.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptInsertValueIntoSequence final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "insertValueIntoSequence"; }
		String getName() const override { return "Insert Value->Sequence"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convDataToEntityId.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};

	class ScriptHasSequenceValue final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "hasSequenceValue"; }
		String getName() const override { return "Has Sequence Value"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/convEntityIdToData.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }

		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const override;
	};
}
