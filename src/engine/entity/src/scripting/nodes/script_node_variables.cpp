#include "script_node_variables.h"

#include "halley/maths/ops.h"
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
	str.append(node.getSettings()["scope"].asString("local") + ":" + node.getSettings()["variable"].asString(""), parameterColour);
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
	const auto& vars = environment.getVariables(getScope(node));
	return ConfigNode(vars.getVariable(node.getSettings()["variable"].asString("")));
}

ScriptVariableScope ScriptVariable::getScope(const ScriptGraphNode& node) const
{
	return fromString<ScriptVariableScope>(node.getSettings()["scope"].asString("local"));
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
	} else if (data.getType() != ConfigNodeType::Undefined) {
		str.append("String ");
		str.append("\"", parameterColour);
		quoting = true;
	}
	
	str.append(data.asString(), parameterColour);
	if (quoting) {
		str.append("\"", parameterColour);
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
	if (value.isNumber()) {
		if (value.isInteger()) {
			return ConfigNode(value.toInteger());
		} else {
			return ConfigNode(value.toFloat());
		}
	}
	return ConfigNode(value);
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
	str.append(node.getSettings()["operator"].asString("=="), parameterColour);
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
	if (typeA == ConfigNodeType::String || typeB == ConfigNodeType::String) {
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



Vector<IScriptNodeType::SettingType> ScriptLerp::getSettingTypes() const
{
	return {
		SettingType{ "from", "float", Vector<String>{"0"} },
		SettingType{ "to", "float", Vector<String>{"1"} },
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
	str.append(toString(node.getSettings()["from"].asFloat(0)), parameterColour);
	str.append(", ");
	str.append(toString(node.getSettings()["to"].asFloat(1)), parameterColour);
	str.append(", ");
	str.append(getConnectedNodeName(world, node, graph, 0), parameterColour);
	str.append(")");
	return str.moveResults();
}

String ScriptLerp::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId element_idx) const
{
	const auto from = node.getSettings()["from"].asFloat(0);
	const auto to = node.getSettings()["to"].asFloat(1);
	return "lerp(" + toString(from) + ", " + toString(to) + ", " + getConnectedNodeName(world, node, graph, 0) + ")";
}

ConfigNode ScriptLerp::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pin_n) const
{
	const auto from = node.getSettings()["from"].asFloat(0);
	const auto to = node.getSettings()["to"].asFloat(1);
	const auto t = readDataPin(environment, node, 0).asFloat(0);
	return ConfigNode(lerp(from, to, t));
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

std::pair<String, Vector<ColourOverride>> ScriptAdvanceTo::getPinDescription(const ScriptGraphNode& node, PinType element, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return { "Flow output.", {} };
	} else if (elementIdx == 2) {
		return { "Flow output if target is reached.", {} };
	} else if (elementIdx == 3) {
		return { "Flow output if target is not reached.", {} };
	} else if (elementIdx == 4) {
		return { "Flow output if variable was modified.", {} };
	} else if (elementIdx == 5) {
		return { "Flow output if variable was not modified.", {} };
	} else if (elementIdx == 6) {
		return { "Target value", {} };
	} else if (elementIdx == 7) {
		return { "Increment value", {} };
	} else if (elementIdx == 8) {
		return { "Variable being modified", {} };
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
	auto label = getLabel(node);

	auto str = ColourStringBuilder(true);
	str.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	str.append(" := ");
	str.append(label.isEmpty() ? getConnectedNodeName(world, node, graph, 2) : label, parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptSetVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	auto data = node.getPin(2).hasConnection() ? readDataPin(environment, node, 2) : ConfigNode(node.getSettings()["defaultValue"].asFloat(0));
	writeDataPin(environment, node, 3, std::move(data));
	return Result(ScriptNodeExecutionState::Done);
}




ScriptHoldVariableData::ScriptHoldVariableData(const ConfigNode& node)
{
	prevValue = ConfigNode(node["prevValue"]);
}

ConfigNode ScriptHoldVariableData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["prevValue"] = ConfigNode(prevValue);
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
	};
}

std::pair<String, Vector<ColourOverride>> ScriptHoldVariable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	const auto label1 = !node.getPin(2).hasConnection() ? toString(node.getSettings()["defaultValue"].asInt(0)) : "";
	const auto label2 = !node.getPin(3).hasConnection() ? toString(node.getSettings()["defaultPrevValue"].asInt(0)) : "";

	auto str = ColourStringBuilder(true);
	str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	str.append(" := ");
	str.append(label1.isEmpty() ? getConnectedNodeName(world, node, graph, 2) : label1, parameterColour);
	str.append(", then ");
	str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	str.append(" := ");
	str.append(label2.isEmpty() ? getConnectedNodeName(world, node, graph, 3) : label2, parameterColour);
	return str.moveResults();
}

void ScriptHoldVariable::doInitData(ScriptHoldVariableData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
}

IScriptNodeType::Result ScriptHoldVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node,ScriptHoldVariableData& curData) const
{
	auto prevData = node.getPin(3).hasConnection() ? readDataPin(environment, node, 3) : ConfigNode(node.getSettings()["defaultPrevValue"].asInt(0));
	curData.prevValue = std::move(prevData);

	auto data = node.getPin(2).hasConnection() ? readDataPin(environment, node, 2) : ConfigNode(node.getSettings()["defaultValue"].asInt(0));
	writeDataPin(environment, node, 4, std::move(data));
	return Result(ScriptNodeExecutionState::Done);
}

void ScriptHoldVariable::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptHoldVariableData& curData) const
{
	writeDataPin(environment, node, 4, std::move(curData.prevValue));
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
	return { "Convert EntityId to data.", {} };
}

ConfigNode ScriptEntityIdToData::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(readRawEntityId(environment, node, 0).value);
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
	result.value = data.asInt64(-1);
	return result;
}


