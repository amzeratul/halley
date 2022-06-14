#include "script_variables.h"
using namespace Halley;

String ScriptVariable::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["variable"].asString("");
}

String ScriptVariable::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph) const
{
	return getLabel(node);
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
	return { SettingType{ "variable", "Halley::String", Vector<String>{""} } };
}

std::pair<String, Vector<ColourOverride>> ScriptVariable::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Variable ");
	str.append(node.getSettings()["variable"].asString(""), Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

ConfigNode ScriptVariable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return environment.getVariable(node.getSettings()["variable"].asString(""));
}

void ScriptVariable::doSetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ConfigNode data) const
{
	environment.setVariable(node.getSettings()["variable"].asString(""), std::move(data));
}


String ScriptLiteral::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["value"].asString("0");
}

String ScriptLiteral::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph) const
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
	auto str = ColourStringBuilder(true);
	str.append("Literal ");
	str.append(node.getSettings()["value"].asString("0"), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(".");
	return str.moveResults();
}

ConfigNode ScriptLiteral::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto& value = node.getSettings()["value"].asString("0");
	if (value.isNumber()) {
		if (value.isInteger()) {
			return ConfigNode(value.toInteger());
		} else {
			return ConfigNode(value.toFloat());
		}
	} else {
		if (value.asciiLower() == "true") {
			return ConfigNode(true);
		}
	}
	// All other cases
	return ConfigNode(false);
}


String ScriptComparison::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["operator"].asString("equals");
}

String ScriptComparison::getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph) const
{
	auto a = getConnectedNodeName(world, node, graph, 0);
	auto b = getConnectedNodeName(world, node, graph, 1);
	auto op = getLabel(node);
	return addParentheses(std::move(a)) + " " + std::move(op) + " " + addParentheses(std::move(b));
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
	return { SettingType{ "operator", "Halley::String", Vector<String>{"equals"} } };
}

std::pair<String, Vector<ColourOverride>> ScriptComparison::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("True if ");
	str.append(addParentheses(getConnectedNodeName(world, node, graph, 0)), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" ");
	str.append(node.getSettings()["operator"].asString("equals"), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" ");
	str.append(addParentheses(getConnectedNodeName(world, node, graph, 1)), Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

ConfigNode ScriptComparison::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto a = readDataPin(environment, node, 0);
	const auto b = readDataPin(environment, node, 1);
	// TODO
	return ConfigNode(false);
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
	str.append(getConnectedNodeName(world, node, graph, 3), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" := ");
	str.append(label.isEmpty() ? getConnectedNodeName(world, node, graph, 2) : label, Colour4f(0.97f, 0.35f, 0.35f));
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
	str.append(getConnectedNodeName(world, node, graph, 3), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" := ");
	str.append(label1.isEmpty() ? getConnectedNodeName(world, node, graph, 2) : label1, Colour4f(0.97f, 0.35f, 0.35f));
	str.append(", then ");
	str.append(getConnectedNodeName(world, node, graph, 3), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" := ");
	str.append(label2.isEmpty() ? getConnectedNodeName(world, node, graph, 3) : label2, Colour4f(0.97f, 0.35f, 0.35f));
	return str.moveResults();
}

void ScriptHoldVariable::doInitData(ScriptHoldVariableData& data, const ScriptGraphNode& node, const ConfigNode& nodeData) const
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


