#include "script_lua.h"

#include "halley/lua/lua_reference.h"

using namespace Halley;

ScriptLuaExpressionData::ScriptLuaExpressionData(const ConfigNode& node)
{
	results = node["results"].asVector<ConfigNode>({});
}

ConfigNode ScriptLuaExpressionData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["results"] = results;
	return result;
}

Vector<IGraphNodeType::SettingType> ScriptLuaExpression::getSettingTypes() const
{
	return {
		SettingType{ "args", "Halley::Vector<Halley::String>", Vector<String>{""} },
		SettingType{ "outputs", "Halley::Range<int, 1, 8>", {"1"}},
		SettingType{ "code", "Halley::LuaExpression", {""}},
	};
}

gsl::span<const IGraphNodeType::PinType> ScriptLuaExpression::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	
	const auto& settings = node.getSettings();
	const size_t argsN = settings["args"].asVector<String>({}).size();
	const size_t outputs = settings["outputs"].asInt(1);
	const size_t flowN = nFlowPins();

	static thread_local std::vector<PinType> pins;
	pins.clear();
	pins.reserve(argsN + outputs + flowN);

	for (size_t i = 0; i < flowN; ++i) {
		pins.emplace_back(ET::FlowPin, i == 0 ? PD::Input : PD::Output);
	}

	for (size_t i = 0; i < argsN; ++i) {
		pins.emplace_back(ET::ReadDataPin, PD::Input);
	}
	for (size_t i = 0; i < outputs; ++i) {
		pins.emplace_back(ET::ReadDataPin, PD::Output);
	}
	
	return pins;
}

std::pair<String, Vector<ColourOverride>> ScriptLuaExpression::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Return ");
	str.append(node.getSettings()["code"].asString(""), parameterColour);
	return str.moveResults();
}

String ScriptLuaExpression::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return node.getSettings()["code"].asString("") + " (" + toString(elementIdx) + ")";
}

String ScriptLuaExpression::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	const size_t flowN = nFlowPins();
	if (elementIdx < flowN) {
		return ScriptNodeTypeBase<ScriptLuaExpressionData>::getPinDescription(node, elementType, elementIdx);
	} else {
		const auto& settings = node.getSettings();
		const auto& args = settings["args"].asVector<String>({});

		const auto idx = elementIdx - flowN;
		if (idx < args.size()) {
			return args[idx];
		} else {
			return "Output " + toString(idx - args.size());
		}
	}
}

void ScriptLuaExpression::doInitData(ScriptLuaExpressionData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	data.results = {};
}

ConfigNode ScriptLuaExpression::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLuaExpressionData& data) const
{
	evaluate(environment, node, data);

	const auto args = node.getSettings()["args"].asVector<String>({});
	const size_t argsN = args.size();
	return ConfigNode(data.results[pinN - argsN]);
}

size_t ScriptLuaExpression::nFlowPins() const
{
	return 0;
}

void ScriptLuaExpression::evaluate(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptLuaExpressionData& data) const
{
	auto& state = environment.getInterface<ILuaInterface>().getLuaState();
	auto stackOps = LuaStackOps(state);

	const auto args = node.getSettings()["args"].asVector<String>({});
	const size_t argsN = args.size();
	const size_t outputs = node.getSettings()["outputs"].asInt(1);

	if (!data.expr) {
		auto code = node.getSettings()["code"].asString("");
		if (!code.startsWith("return") && !code.contains('\n')) {
			code = "return " + code;
		}
		if (argsN > 0) {
			String exprStr = "local " + String::concatList(args, ", ") + " = ...\n" + code;
			data.expr = LuaExpression(std::move(exprStr));
		} else {
			data.expr = LuaExpression(std::move(code));
		}
	}

	const int firstInputPin = static_cast<int>(nFlowPins());

	LuaFunctionCaller::startCall(state);
	data.expr->get(state).pushToLuaStack();
	for (size_t i = 0; i < argsN; ++i) {
		stackOps.push(readDataPin(environment, node, i + firstInputPin));
	}
	LuaFunctionCaller::call(state, static_cast<int>(argsN), static_cast<int>(outputs));

	data.results.resize(outputs);
	for (size_t i = 0; i < outputs; ++i) {
		data.results[outputs - i - 1] = stackOps.popConfigNode();
	}
	LuaFunctionCaller::endCall(state);
}


std::pair<String, Vector<ColourOverride>> ScriptLuaStatement::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Execute ");
	str.append(node.getSettings()["code"].asString(""), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptLuaStatement::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptLuaExpressionData& data) const
{
	const auto args = node.getSettings()["args"].asVector<String>({});
	const size_t argsN = args.size();

	const auto idx = pinN - argsN;
	return idx < data.results.size() ? ConfigNode(data.results[idx]) : ConfigNode();
}

IScriptNodeType::Result ScriptLuaStatement::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLuaExpressionData& data) const
{
	evaluate(environment, node, data);

	return Result(ScriptNodeExecutionState::Done);
}

size_t ScriptLuaStatement::nFlowPins() const
{
	return 2;
}
