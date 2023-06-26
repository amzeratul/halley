#include "script_node_variables.h"

#include "halley/maths/ops.h"
#include "halley/maths/tween.h"
#include "halley/support/logger.h"
using namespace Halley;

String ScriptVariable::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["variable"].asString("");
}

String ScriptVariable::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return node.getSettings()["scope"].asString("local") + ":" + node.getSettings()["variable"].asString("");
}

gsl::span<const IScriptNodeType::PinType> ScriptVariable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::WriteDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptVariable::getSettingTypes() const
{
	return {
		SettingType{ "scope", "Halley::ScriptVariableScope", Vector<String>{"local"} },
		SettingType{ "variable", "Halley::String", Vector<String>{""} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptVariable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Variable ");
	str.append(node.getSettings()["scope"].asString("local") + ":" + node.getSettings()["variable"].asString(""), settingColour);
	return str.moveResults();
}

ConfigNode ScriptVariable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto& vars = environment.getVariables(getScope(node));
	return ConfigNode(vars.getVariable(node.getSettings()["variable"].asString("")));
}

void ScriptVariable::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const
{
	auto& vars = environment.getVariables(getScope(node));
	vars.setVariable(node.getSettings()["variable"].asString(""), std::move(data));
}

ConfigNode ScriptVariable::doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node) const
{
	return doGetData(environment, node, 1);
}

ScriptVariableScope ScriptVariable::getScope(const ScriptGraphNode& node) const
{
	return fromString<ScriptVariableScope>(node.getSettings()["scope"].asString("local"));
}



String ScriptEntityVariable::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["variable"].asString("");
}

String ScriptEntityVariable::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "entity:" + node.getSettings()["variable"].asString("");
}

gsl::span<const IGraphNodeType::PinType> ScriptEntityVariable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IGraphNodeType::SettingType> ScriptEntityVariable::getSettingTypes() const
{
	return {
		SettingType{ "variable", "Halley::String", Vector<String>{""} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptEntityVariable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Variable ");
	str.append("entity:" + node.getSettings()["variable"].asString(""), settingColour);
	str.append(" on entity ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptEntityVariable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto& vars = environment.getEntityVariables(readEntityId(environment, node, 0));
	return ConfigNode(vars.getVariable(node.getSettings()["variable"].asString("")));
}

ConfigNode ScriptEntityVariable::doGetDevConData(ScriptEnvironment& environment, const ScriptGraphNode& node) const
{
	return doGetData(environment, node, 1);
}


String ScriptLiteral::getLargeLabel(const ScriptGraphNode& node) const
{
	const auto& val = getConfigNode(node);
	const auto& str = val.asString("");
	if (val.getType() == ConfigNodeType::String) {
		return "\"" + str + "\"";
	} else if (val.getType() == ConfigNodeType::Undefined) {
		return "null";
	}
	return str;
}

String ScriptLiteral::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getLargeLabel(node);
}

gsl::span<const IScriptNodeType::PinType> ScriptLiteral::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptLiteral::getSettingTypes() const
{
	return { SettingType{ "value", "Halley::String", Vector<String>{"0"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptLiteral::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto data = getConfigNode(node);

	auto str = ColourStringBuilder(false);
	bool quoting = false;
	if (data.getType() == ConfigNodeType::Int) {
		str.append("Int ");
	} else if (data.getType() == ConfigNodeType::Float) {
		str.append("Float ");
	} else if (data.getType() == ConfigNodeType::Float2) {
		str.append("Vector2f ");
	} else if (data.getType() == ConfigNodeType::Bool) {
		str.append("Bool ");
	} else if (data.getType() != ConfigNodeType::Undefined) {
		str.append("String ");
		str.append("\"", settingColour);
		quoting = true;
	}
	
	str.append(data.asString(), settingColour);
	if (quoting) {
		str.append("\"", settingColour);
	}
	return str.moveResults();
}

ConfigNode ScriptLiteral::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return getConfigNode(node);
}

ConfigNode ScriptLiteral::getConfigNode(const ScriptGraphNode& node) const
{
	const auto& value = node.getSettings()["value"].asString("");
	if (value == "null") {
		return ConfigNode();
	}

	if (value.startsWith("(") && value.endsWith(")")) { // well this is hacky
		const auto string = value.mid(1, value.length() - 2);
		auto strings = string.split(",");
		for (auto& s: strings) {
			s.trimBoth();
		}
		if (strings.size() == 2 && strings[0].isNumber() && strings[1].isNumber()) {
			return ConfigNode(Vector2f(strings[0].toFloat(), strings[1].toFloat()));
		}
	}

	if (value == "true" || value == "false") {
		return ConfigNode(value == "true");
	}

	if (value.isNumber()) {
		if (value.isInteger()) {
			return ConfigNode(value.toInteger());
		} else {
			return ConfigNode(value.toFloat());
		}
	}
	return ConfigNode(value);
}



String ScriptColourLiteral::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["value"].asString("#FFFFFF");
}

String ScriptColourLiteral::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getLargeLabel(node);
}

gsl::span<const IScriptNodeType::PinType> ScriptColourLiteral::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptColourLiteral::getSettingTypes() const
{
	return { SettingType{ "value", "Halley::Colour4f", Vector<String>{"#FFFFFF"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptColourLiteral::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(false);
	str.append("Colour");
	return str.moveResults();
}

ConfigNode ScriptColourLiteral::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(node.getSettings()["value"].asString("#FFFFFF"));
}



String ScriptComparison::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["operator"].asString("==");
}

String ScriptComparison::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(world, node, graph, 0);
	auto b = getConnectedNodeName(world, node, graph, 1);
	auto op = getLargeLabel(node);
	return addParentheses(std::move(a)) + " " + op + " " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptComparison::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptComparison::getSettingTypes() const
{
	return { SettingType{ "operator", "Halley::MathRelOp", Vector<String>{"=="} } };
}

std::pair<String, Vector<ColourOverride>> ScriptComparison::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("True if ");
	str.append(addParentheses(getConnectedNodeName(world, node, graph, 0)), parameterColour);
	str.append(" ");
	str.append(node.getSettings()["operator"].asString("=="), settingColour);
	str.append(" ");
	str.append(addParentheses(getConnectedNodeName(world, node, graph, 1)), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptComparison::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto a = readDataPin(environment, node, 0);
	const auto b = readDataPin(environment, node, 1);
	const auto typeA = a.getType();
	const auto typeB = b.getType();
	const auto op = fromString<MathRelOp>(node.getSettings()["operator"].asString("=="));

	bool result = false;
	if (typeA == ConfigNodeType::EntityId || typeB == ConfigNodeType::EntityId) {
		result = MathOps::compare(op, a.asInt64(0), b.asInt64(0));
	} else if (typeA == ConfigNodeType::String || typeB == ConfigNodeType::String) {
		result = MathOps::compare(op, a.asString(""), b.asString(""));
	} else if (typeA == ConfigNodeType::Float || typeB == ConfigNodeType::Float) {
		result = MathOps::compare(op, a.asFloat(0), b.asFloat(0));
	} else if (typeA == ConfigNodeType::Int64 || typeB == ConfigNodeType::Int64) {
		result = MathOps::compare(op, a.asInt64(0), b.asInt64(0));
	} else if (typeA == ConfigNodeType::Int || typeB == ConfigNodeType::Int) {
		result = MathOps::compare(op, a.asInt(0), b.asInt(0));
	} else {
		Logger::logError("ScriptComparison node can't compare types " + toString(typeA) + " and " + toString(typeB));
	}

	return ConfigNode(result);
}



String ScriptArithmetic::getLargeLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["operator"].asString("+");
}

String ScriptArithmetic::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(world, node, graph, 0);
	auto b = getConnectedNodeName(world, node, graph, 1);
	auto op = getLargeLabel(node);

	if (op[0] >= 'a' && op[0] <= 'z') {
		// If starts with a lower case letter, assume it's in function form
		return op + "(" + a + ", " + b + ")";
	} else if (a == "<empty>" && op == "-") {
		// Prefix form
		return op + addParentheses(std::move(b));
	} else {
		// Otherwise, assume infix form
		return addParentheses(std::move(a)) + " " + op + " " + addParentheses(std::move(b));
	}
}

gsl::span<const IScriptNodeType::PinType> ScriptArithmetic::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptArithmetic::getSettingTypes() const
{
	return { SettingType{ "operator", "Halley::MathOp", Vector<String>{"+"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptArithmetic::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns ");
	str.append(getShortDescription(world, node, graph, 0), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptArithmetic::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pin_n) const
{
	const auto a = readDataPin(environment, node, 0);
	const auto b = readDataPin(environment, node, 1);
	const auto op = fromString<MathOp>(node.getSettings()["operator"].asString("+"));

	const auto type = ConfigNode::getPromotedType(std::array<ConfigNodeType, 2>{ a.getType(), b.getType() }, true);

	if (type == ConfigNodeType::String) {
		if (op == MathOp::Add) {
			return ConfigNode(a.asString("") + b.asString(""));
		} else {
			Logger::logError("Attempting to perform illegal op " + toString(op) + " with string types.");
			return ConfigNode();
		}
	} else if (type == ConfigNodeType::Float) {
		return ConfigNode(MathOps::apply(op, a.asFloat(0), b.asFloat(0)));
	} else if (type == ConfigNodeType::Int64) {
		return ConfigNode(MathOps::apply(op, a.asInt64(0), b.asInt64(0)));
	} else if (type == ConfigNodeType::Int) {
		return ConfigNode(MathOps::apply(op, a.asInt(0), b.asInt(0)));
	} else if (type == ConfigNodeType::Float2) {
		return ConfigNode(MathOps::apply(op, a.asVector2f({}), b.asVector2f({})));
	} else if (type == ConfigNodeType::Int2) {
		return ConfigNode(MathOps::apply(op, a.asVector2i({}), b.asVector2i({})));
	} else {
		Logger::logError("ScriptComparison node can't perform arithmetic with types " + toString(a.getType()) + " and " + toString(b.getType()));
		return ConfigNode();
	}
}



String ScriptValueOr::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(world, node, graph, 0);
	auto b = getConnectedNodeName(world, node, graph, 1);
	return addParentheses(std::move(a)) + " ?? " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptValueOr::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptValueOr::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(" if defined, otherwise ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptValueOr::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto a = readDataPin(environment, node, 0);
	if (a.getType() != ConfigNodeType::Undefined) {
		return a;
	} else {
		return readDataPin(environment, node, 1);
	}
}



String ScriptConditionalOperator::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto a = getConnectedNodeName(world, node, graph, 0);
	auto b = getConnectedNodeName(world, node, graph, 1);
	auto c = getConnectedNodeName(world, node, graph, 2);
	return addParentheses(std::move(a)) + " ? " + addParentheses(std::move(b)) + " : " + addParentheses(std::move(c));
}

gsl::span<const IGraphNodeType::PinType> ScriptConditionalOperator::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptConditionalOperator::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("If ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(", ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	str.append(", otherwise ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptConditionalOperator::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto a = readDataPin(environment, node, 0);
	if (a.asBool(false)) {
		return readDataPin(environment, node, 1);
	} else {
		return readDataPin(environment, node, 2);
	}
}


Vector<IScriptNodeType::SettingType> ScriptLerp::getSettingTypes() const
{
	return {
		SettingType{ "from", "float", Vector<String>{"0"} },
		SettingType{ "to", "float", Vector<String>{"1"} },
		SettingType{ "curve", "Halley::TweenCurve", Vector<String>{"linear"} }
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptLerp::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLerp::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Returns lerp(");
	str.append(toString(node.getSettings()["from"].asFloat(0)), settingColour);
	str.append(", ");
	str.append(toString(node.getSettings()["to"].asFloat(1)), settingColour);
	str.append(", ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(", ");
	str.append(node.getSettings()["curve"].asString("linear"), settingColour);
	str.append(")");
	return str.moveResults();
}

String ScriptLerp::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId element_idx) const
{
	const auto from = node.getSettings()["from"].asFloat(0);
	const auto to = node.getSettings()["to"].asFloat(1);
	const auto curve = node.getSettings()["curve"].asString("linear");
	return "lerp(" + toString(from) + ", " + toString(to) + ", " + getConnectedNodeName(world, node, graph, 0) + ", " + curve + ")";
}

ConfigNode ScriptLerp::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pin_n) const
{
	const auto from = node.getSettings()["from"].asFloat(0);
	const auto to = node.getSettings()["to"].asFloat(1);
	const auto curve = node.getSettings()["curve"].asEnum(TweenCurve::Linear);
	const auto t = readDataPin(environment, node, 0).asFloat(0);
	return ConfigNode(lerp(from, to, Tween<float>::applyCurve(t, curve)));
}



gsl::span<const IScriptNodeType::PinType> ScriptAdvanceTo::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 9>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::WriteDataPin, PD::Output }
	};
	return data;
}

String ScriptAdvanceTo::getPinDescription(const ScriptGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow output.";
	} else if (elementIdx == 2) {
		return "Flow output if target is reached.";
	} else if (elementIdx == 3) {
		return "Flow output if target is not reached.";
	} else if (elementIdx == 4) {
		return "Flow output if variable was modified.";
	} else if (elementIdx == 5) {
		return "Flow output if variable was not modified.";
	} else if (elementIdx == 6) {
		return "Target value";
	} else if (elementIdx == 7) {
		return "Increment value";
	} else if (elementIdx == 8) {
		return "Variable being modified";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, element, elementIdx);
}

std::pair<String, Vector<ColourOverride>> ScriptAdvanceTo::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Advance ");
	str.append(getConnectedNodeName(world, node, graph, 8), parameterColour);
	str.append(" towards ");
	str.append(getConnectedNodeName(world, node, graph, 6), parameterColour);
	str.append(" by ");
	str.append(getConnectedNodeName(world, node, graph, 7), parameterColour);
	return str.moveResults();
}

namespace {
	template <typename T>
	std::tuple<T, bool, bool> advanceWithFlags(T a, T b, T inc)
	{
		const bool modified = !(a == b);
		const T c = advance<T>(a, b, inc);
		const bool reached = c == b;
		return { c, modified, reached };
	}
}

IScriptNodeType::Result ScriptAdvanceTo::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto target = readDataPin(environment, node, 6);
	const auto amount = readDataPin(environment, node, 7);
	const auto var = readDataPin(environment, node, 8);

	const auto type = ConfigNode::getPromotedType(std::array<ConfigNodeType, 3>{ var.getType(), target.getType(), amount.getType() }, true);
	bool modified = false;
	bool reached = false;
	
	if (type == ConfigNodeType::Float) {
		float v;
		std::tie(v, modified, reached) = advanceWithFlags(var.asFloat(0), target.asFloat(0), amount.asFloat(0));
		//Logger::logDev("AdvanceTo(" + toString(node.getId()) + "): " + toString(var.asFloat(0)) + " -> " + toString(v) + " | modified=" + toString(modified) + ", reached=" + toString(reached));
		writeDataPin(environment, node, 8, ConfigNode(v));
	} else if (type == ConfigNodeType::Int) {
		int v;
		std::tie(v, modified, reached) = advanceWithFlags(var.asInt(0), target.asInt(0), amount.asInt(0));
		writeDataPin(environment, node, 8, ConfigNode(v));
	} else if (type == ConfigNodeType::Int64) {
		int64_t v;
		std::tie(v, modified, reached) = advanceWithFlags(var.asInt64(0), target.asInt64(0), amount.asInt64(0));
		writeDataPin(environment, node, 8, ConfigNode(v));
	} else {
		Logger::logError("Cannot advance variable of type " + toString(type));
	}

	constexpr uint8_t normalOutPinMask = 1 << 0;
	constexpr uint8_t reachedOutPinMask = 1 << 1;
	constexpr uint8_t notReachedOutPinMask = 1 << 2;
	constexpr uint8_t modifiedOutPinMask = 1 << 3;
	constexpr uint8_t notModifiedOutPinMask = 1 << 4;
	return Result(ScriptNodeExecutionState::Done, 0, (normalOutPinMask) | (reached ? reachedOutPinMask : notReachedOutPinMask) | (modified ? modifiedOutPinMask : notModifiedOutPinMask));
}



String ScriptSetVariable::getLabel(const ScriptGraphNode& node) const
{
	if (!node.getPin(2).hasConnection()) {
		return toString(node.getSettings()["defaultValue"].asFloat(0));
	}
	return "";
}

gsl::span<const IScriptNodeType::PinType> ScriptSetVariable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptSetVariable::getSettingTypes() const
{
	return { SettingType{ "defaultValue", "float", Vector<String>{"0"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptSetVariable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	str.append(" := ");
	str.append(node.getPin(2).hasConnection() ? getConnectedNodeName(world, node, graph, 2) : getLabel(node), node.getPin(2).hasConnection() ? parameterColour : settingColour);
	return str.moveResults();
}

String ScriptSetVariable::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 3) {
		return node.getPin(2).hasConnection() ? getConnectedNodeName(world, node, graph, 2) : getLabel(node);
	}
	return "Set Variable";
}

IScriptNodeType::Result ScriptSetVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto data = node.getPin(2).hasConnection() ? readDataPin(environment, node, 2) : ConfigNode(node.getSettings()["defaultValue"].asFloat(0));
	writeDataPin(environment, node, 3, std::move(data));
	return Result(ScriptNodeExecutionState::Done);
}




ScriptHoldVariableData::ScriptHoldVariableData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		prevValue = ConfigNode(node["prevValue"]);
		started = node["started"].asBool(false);
	} else {
		prevValue = ConfigNode();
		started = false;
	}
}

ConfigNode ScriptHoldVariableData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["prevValue"] = ConfigNode(prevValue);
	result["started"] = started;
	return result;
}

String ScriptHoldVariable::getLabel(const ScriptGraphNode& node) const
{
	if (!node.getPin(2).hasConnection()) {
		if (!node.getPin(3).hasConnection()) {
			return toString(node.getSettings()["defaultValue"].asInt(0)) + " -> " + toString(node.getSettings()["defaultPrevValue"].asInt(0));
		} else {
			return toString(node.getSettings()["defaultValue"].asInt(0));
		}
	}
	return "";
}

gsl::span<const IScriptNodeType::PinType> ScriptHoldVariable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptHoldVariable::getSettingTypes() const
{
	return {
		SettingType{ "defaultValue", "int", Vector<String>{"0"} },
		SettingType{ "defaultPrevValue", "int", Vector<String>{"0"} },
		SettingType{ "continuous", "bool", Vector<String>{"false"} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptHoldVariable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto label1 = !node.getPin(2).hasConnection() ? toString(node.getSettings()["defaultValue"].asInt(0)) : "";
	const auto label2 = !node.getPin(3).hasConnection() ? toString(node.getSettings()["defaultPrevValue"].asInt(0)) : "";

	auto str = ColourStringBuilder(true);
	str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	str.append(" := ");
	str.append(label1.isEmpty() ? getConnectedNodeName(world, node, graph, 2) : label1, label1.isEmpty() ? parameterColour : settingColour);
	str.append(", then ");
	str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	str.append(" := ");
	str.append(label2.isEmpty() ? getConnectedNodeName(world, node, graph, 3) : label2, label2.isEmpty() ? parameterColour : settingColour);
	if (node.getSettings()["continuous"].asBool(false)) {
		str.append(" (continuously)", settingColour);
	}
	return str.moveResults();
}

void ScriptHoldVariable::doInitData(ScriptHoldVariableData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptHoldVariableData(nodeData);
	data.started = false;
}

IScriptNodeType::Result ScriptHoldVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node,ScriptHoldVariableData& curData) const
{
	const bool continuous = node.getSettings()["continuous"].asBool(false);
	const bool first = !curData.started;
	curData.started = true;

	if (first) {
		auto prevData = node.getPin(3).hasConnection() ? readDataPin(environment, node, 3) : ConfigNode(node.getSettings()["defaultPrevValue"].asInt(0));
		curData.prevValue = std::move(prevData);
	}

	auto data = node.getPin(2).hasConnection() ? readDataPin(environment, node, 2) : ConfigNode(node.getSettings()["defaultValue"].asInt(0));
	writeDataPin(environment, node, 4, std::move(data));

	if (continuous) {
		return Result(first ? ScriptNodeExecutionState::Fork : ScriptNodeExecutionState::Executing, first ? 0 : time);
	} else {
		return Result(ScriptNodeExecutionState::Done);
	}
}

void ScriptHoldVariable::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptHoldVariableData& curData) const
{
	writeDataPin(environment, node, 4, std::move(curData.prevValue));
}



Vector<IGraphNodeType::SettingType> ScriptEntityIdToData::getSettingTypes() const
{
	return {
		SettingType{ "readRaw", "bool", Vector<String>{"true"} },
	};
}

gsl::span<const IScriptNodeType::PinType> ScriptEntityIdToData::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptEntityIdToData::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getConnectedNodeName(world, node, graph, 0);
}

std::pair<String, Vector<ColourOverride>> ScriptEntityIdToData::getNodeDescription(const ScriptGraphNode& node, const World* world,	const ScriptGraph& graph) const
{
	if (node.getSettings()["readRaw"].asBool(true)) {
		return { "Convert EntityId to data. Using readRaw function.", {} };
	}
	return { "Convert EntityId to data. Using read function which returns entity to which the script is attached to, if input is invalid.", {} };
}

ConfigNode ScriptEntityIdToData::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	if (node.getSettings()["readRaw"].asBool(true)) {
		return ConfigNode(EntityIdHolder{ readRawEntityId(environment, node, 0).value });
	}
	return ConfigNode(EntityIdHolder{ readEntityId(environment, node, 0).value });
}



gsl::span<const IScriptNodeType::PinType> ScriptDataToEntityId::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::TargetPin, PD::Output } };
	return data;
}

String ScriptDataToEntityId::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return getConnectedNodeName(world, node, graph, 0);
}

std::pair<String, Vector<ColourOverride>> ScriptDataToEntityId::getNodeDescription(const ScriptGraphNode& node, const World* world,	const ScriptGraph& graph) const
{
	return { "Convert data to EntityId.", {} };
}

EntityId ScriptDataToEntityId::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, GraphPinId pinN) const
{
	const auto data = readDataPin(environment, node, 0);
	EntityId result;
	result.value = data.asEntityId({}).value;
	return result;
}


gsl::span<const IGraphNodeType::PinType> ScriptToVector::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptToVector::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "(" + getConnectedNodeName(world, node, graph, 0) + ", " + getConnectedNodeName(world, node, graph, 1) + ")";
}

std::pair<String, Vector<ColourOverride>> ScriptToVector::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Return (");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(", ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	str.append(")");
	return str.moveResults();
}

String ScriptToVector::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "x";
	} else if (elementIdx == 1) {
		return "y";
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptToVector::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto x = readDataPin(environment, node, 0).asFloat(0);
	const auto y = readDataPin(environment, node, 1).asFloat(0);
	return ConfigNode(Vector2f(x, y));
}



gsl::span<const IGraphNodeType::PinType> ScriptFromVector::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptFromVector::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return getConnectedNodeName(world, node, graph, 0) + ".x";
	} else if (elementIdx == 2) {
		return getConnectedNodeName(world, node, graph, 0) + ".y";
	}
	return "";
}

std::pair<String, Vector<ColourOverride>> ScriptFromVector::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Split vector ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptFromVector::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "x";
	} else if (elementIdx == 2) {
		return "y";
	} else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptFromVector::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	if (pinN == 1) {
		return ConfigNode(readDataPin(environment, node, 0).asVector2f({}).x);
	} else if (pinN == 2) {
		return ConfigNode(readDataPin(environment, node, 0).asVector2f({}).y);
	} else {
		return {};
	}
}



gsl::span<const IGraphNodeType::PinType> ScriptInsertValueIntoMap::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptInsertValueIntoMap::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	auto startMap = getConnectedNodeName(world, node, graph, 0);
	if (startMap.startsWith("{") && startMap.endsWith("}")) {
		startMap = startMap.mid(1, startMap.size() - 2).trimBoth();
	}
	if (startMap == "<empty>") {
		startMap = {};
	}
	return "{ " + (startMap.isEmpty() ? "" : startMap + ", ") + getConnectedNodeName(world, node, graph, 2) + " := " + getConnectedNodeName(world, node, graph, 1) + " }";
}

std::pair<String, Vector<ColourOverride>> ScriptInsertValueIntoMap::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Insert into map ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(" key ");
	str.append(getConnectedNodeName(world, node, graph, 2), parameterColour);
	str.append(" with value ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	return str.moveResults();
}

String ScriptInsertValueIntoMap::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Map (can be empty)";
	}
	else if (elementIdx == 1) {
		return "Value";
	}
	else if (elementIdx == 2) {
		return "Key";
	}
	else if (elementIdx == 3) {
		return "Map";
	}
	else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptInsertValueIntoMap::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto map = readDataPin(environment, node, 0);
	if (map.getType() != ConfigNodeType::Map) {
		map = ConfigNode::MapType();
	}

	const auto value = readDataPin(environment, node, 1);
	const auto key = readDataPin(environment, node, 2).asString("");
	if (key.isEmpty() || value.getType() == ConfigNodeType::Undefined) {
		return map;
	}
	map[key] = value;

    return map;
}



gsl::span<const IGraphNodeType::PinType> ScriptGetValueFromMap::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptGetValueFromMap::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "Get value with key " +
		getConnectedNodeName(world, node, graph, 1) +
		" from map " +
		getConnectedNodeName(world, node, graph, 0);
}

std::pair<String, Vector<ColourOverride>> ScriptGetValueFromMap::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Get value with key ");
	str.append(getConnectedNodeName(world, node, graph, 1), parameterColour);
	str.append(" from map ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptGetValueFromMap::getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 0) {
		return "Map";
	}
	else if (elementIdx == 1) {
		return "Key";
	}
	else if (elementIdx == 2) {
		return "Value";
	}
	else {
		return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
	}
}

ConfigNode ScriptGetValueFromMap::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	auto map = readDataPin(environment, node, 0);
	if (map.getType() != ConfigNodeType::Map) {
		return {};
	}

	const auto key = readDataPin(environment, node, 1).asString("");
	if (key.isEmpty()) {
		return {};
	}

	return ConfigNode(map[key]);
}