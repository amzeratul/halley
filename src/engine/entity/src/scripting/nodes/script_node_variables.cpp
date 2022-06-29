#include "script_node_variables.h"

#include "halley/maths/ops.h"
#include "halley/support/logger.h"
using namespace Halley;

String ScriptVariable::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["variable"].asString("");
}

String ScriptVariable::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	return node.getSettings()["scope"].asString("script") + ":" + node.getSettings()["variable"].asString("");
}

gsl::span<const IScriptNodeType::PinType> ScriptVariable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::WriteDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptVariable::getSettingTypes() const
{
	return {
		SettingType{ "scope", "Halley::ScriptVariableScope", Vector<String>{"script"} },
		SettingType{ "variable", "Halley::String", Vector<String>{""} }
	};
}

std::pair<String, Vector<ColourOverride>> ScriptVariable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Variable ");
	str.append(node.getSettings()["scope"].asString("script") + ":" + node.getSettings()["variable"].asString(""), parameterColour);
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

ScriptVariableScope ScriptVariable::getScope(const ScriptGraphNode& node) const
{
	return fromString<ScriptVariableScope>(node.getSettings()["scope"].asString("script"));
}


String ScriptLiteral::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["value"].asString("0");
}

String ScriptLiteral::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	return getLabel(node);
}

gsl::span<const IScriptNodeType::PinType> ScriptLiteral::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
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

	auto str = ColourStringBuilder(true);
	bool quoting = false;
	if (data.getType() == ConfigNodeType::Int) {
		str.append("Int ");
	} else if (data.getType() == ConfigNodeType::Float) {
		str.append("Float ");
	} else {
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
	const auto& value = node.getSettings()["value"].asString("0");
	if (value.isNumber()) {
		if (value.isInteger()) {
			return ConfigNode(value.toInteger());
		} else {
			return ConfigNode(value.toFloat());
		}
	}
	return ConfigNode(value);
}



String ScriptComparison::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["operator"].asString("==");
}

String ScriptComparison::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	auto a = getConnectedNodeName(world, node, graph, 0);
	auto b = getConnectedNodeName(world, node, graph, 1);
	auto op = getLabel(node);
	return addParentheses(std::move(a)) + " " + op + " " + addParentheses(std::move(b));
}

gsl::span<const IScriptNodeType::PinType> ScriptComparison::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
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



String ScriptArithmetic::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["operator"].asString("+");
}

String ScriptArithmetic::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	auto a = getConnectedNodeName(world, node, graph, 0);
	auto b = getConnectedNodeName(world, node, graph, 1);
	auto op = getLabel(node);

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
	using PD = ScriptNodePinDirection;
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
	const auto typeA = a.getType();
	const auto typeB = b.getType();
	const auto op = fromString<MathOp>(node.getSettings()["operator"].asString("+"));

	if (typeA == ConfigNodeType::String || typeB == ConfigNodeType::String) {
		if (op == MathOp::Add) {
			return ConfigNode(a.asString("") + b.asString(""));
		} else {
			Logger::logError("Attempting to perform illegal op " + toString(op) + " with string types.");
			return ConfigNode();
		}
	} else if (typeA == ConfigNodeType::Float || typeB == ConfigNodeType::Float) {
		return ConfigNode(MathOps::apply(op, a.asFloat(0), b.asFloat(0)));
	} else if (typeA == ConfigNodeType::Int64 || typeB == ConfigNodeType::Int64) {
		return ConfigNode(MathOps::apply(op, a.asInt64(0), b.asInt64(0)));
	} else if (typeA == ConfigNodeType::Int || typeB == ConfigNodeType::Int) {
		return ConfigNode(MathOps::apply(op, a.asInt(0), b.asInt(0)));
	} else {
		Logger::logError("ScriptComparison node can't perform arithmetic with types " + toString(typeA) + " and " + toString(typeB));
		return ConfigNode();
	}
}


gsl::span<const IScriptNodeType::PinType> ScriptAdvanceTo::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 6>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptAdvanceTo::getPinDescription(const ScriptGraphNode& node, PinType element, ScriptPinId elementIdx) const
{
	if (elementIdx == 1) {
		return { "Flow output if target not reached.", {} };
	} else if (elementIdx == 2) {
		return { "Flow output if target reached.", {} };
	} else if (elementIdx == 3) {
		return { "Target value", {} };
	} else if (elementIdx == 4) {
		return { "Increment value", {} };
	} else if (elementIdx == 5) {
		return { "Variable being modified", {} };
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, element, elementIdx);
}

std::pair<String, Vector<ColourOverride>> ScriptAdvanceTo::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Advance ");
	str.append(getConnectedNodeName(world, node, graph, 5), parameterColour);
	str.append(" towards ");
	str.append(getConnectedNodeName(world, node, graph, 3), parameterColour);
	str.append(" by ");
	str.append(getConnectedNodeName(world, node, graph, 4), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptAdvanceTo::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto target = readDataPin(environment, node, 3);
	const auto amount = readDataPin(environment, node, 4);
	const auto val = readDataPin(environment, node, 5);

	auto type = val.getType();
	if (type == ConfigNodeType::Undefined) {
		type = amount.getType();
	}

	bool reached = false;

	if (type == ConfigNodeType::Float) {
		auto [v, done] = advanceWithFlag(val.asFloat(0), target.asFloat(0), amount.asFloat(0));
		reached = done;
		writeDataPin(environment, node, 5, ConfigNode(v));
	} else if (type == ConfigNodeType::Int) {
		auto [v, done] = advanceWithFlag(val.asInt(0), target.asInt(0), amount.asInt(0));
		reached = done;
		writeDataPin(environment, node, 5, ConfigNode(v));
	} else if (type == ConfigNodeType::Int64) {
		auto [v, done] = advanceWithFlag(val.asInt64(0), target.asInt64(0), amount.asInt64(0));
		reached = done;
		writeDataPin(environment, node, 5, ConfigNode(v));
	} else {
		Logger::logError("Cannot advance variable of type " + toString(type));
	}

	return Result(ScriptNodeExecutionState::Done, 0, reached ? 2 : 1);
}


String ScriptSetVariable::getLabel(const ScriptGraphNode& node) const
{
	if (!node.getPin(2).hasConnection()) {
		return toString(node.getSettings()["defaultValue"].asInt(0));
	}
	return "";
}

gsl::span<const IScriptNodeType::PinType> ScriptSetVariable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

Vector<IScriptNodeType::SettingType> ScriptSetVariable::getSettingTypes() const
{
	return { SettingType{ "defaultValue", "int", Vector<String>{"0"} } };
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
	auto data = node.getPin(2).hasConnection() ? readDataPin(environment, node, 2) : ConfigNode(node.getSettings()["defaultValue"].asInt(0));
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
	using PD = ScriptNodePinDirection;
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
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

String ScriptEntityIdToData::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	return getConnectedNodeName(world, node, graph, 0);
}

std::pair<String, Vector<ColourOverride>> ScriptEntityIdToData::getNodeDescription(const ScriptGraphNode& node, const World* world,	const ScriptGraph& graph) const
{
	return { "Convert EntityId to data.", {} };
}

ConfigNode ScriptEntityIdToData::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(readEntityId(environment, node, 0).value);
}



gsl::span<const IScriptNodeType::PinType> ScriptDataToEntityId::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::TargetPin, PD::Output } };
	return data;
}

String ScriptDataToEntityId::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, ScriptPinId elementIdx) const
{
	return getConnectedNodeName(world, node, graph, 0);
}

std::pair<String, Vector<ColourOverride>> ScriptDataToEntityId::getNodeDescription(const ScriptGraphNode& node, const World* world,	const ScriptGraph& graph) const
{
	return { "Convert data to EntityId.", {} };
}

EntityId ScriptDataToEntityId::doGetEntityId(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptPinId pinN) const
{
	const auto data = readDataPin(environment, node, 0);
	EntityId result;
	result.value = data.asInt64(-1);
	return result;
}


