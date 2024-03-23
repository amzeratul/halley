#pragma once
#include "halley/lua/lua_reference.h"
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptLuaExpressionData : public ScriptStateData<ScriptLuaExpressionData> {
	public:
		ScriptLuaExpressionData() = default;
		ScriptLuaExpressionData(const ConfigNode& node);

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;

		std::optional<LuaExpression> expr;
		Vector<ConfigNode> results;
	};

	class ScriptLuaExpression : public ScriptNodeTypeBase<ScriptLuaExpressionData> {
	public:
		String getId() const override { return "luaExpression"; }
		String getName() const override { return "Lua Expression"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Expression; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/lua.png"; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const BaseGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;

		void doInitData(ScriptLuaExpressionData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLuaExpressionData& data) const override;

	protected:
		virtual size_t nFlowPins() const;
		void evaluate(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptLuaExpressionData& data) const;
	};

	class ScriptLuaStatement final : public ScriptLuaExpression {
	public:
		String getId() const override { return "luaStatement"; }
		String getName() const override { return "Lua Statement"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }
		String getIconName(const BaseGraphNode& node) const override { return "script_icons/lua.png"; }

		std::pair<String, Vector<ColourOverride>> getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const override;

		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLuaExpressionData& curData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLuaExpressionData& data) const override;

	protected:
		size_t nFlowPins() const override;
	};
}
