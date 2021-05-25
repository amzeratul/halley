#include "script_variables.h"
using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptVariable::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::WriteDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::vector<IScriptNodeType::SettingType> ScriptVariable::getSettingTypes() const
{
	return { SettingType{ "variable", "Halley::String", std::vector<String>{""} } };
}

std::pair<String, std::vector<ColourOverride>> ScriptVariable::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Variable \"");
	str.append(node.getSettings()["variable"].asString(""), Colour4f(0.97f, 0.35f, 0.35f));
	str.append("\".");
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


gsl::span<const IScriptNodeType::PinType> ScriptLiteral::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::vector<IScriptNodeType::SettingType> ScriptLiteral::getSettingTypes() const
{
	return { SettingType{ "value", "Halley::String", std::vector<String>{"0"} } };
}

std::pair<String, std::vector<ColourOverride>> ScriptLiteral::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Literal with value ");
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



gsl::span<const IScriptNodeType::PinType> ScriptComparison::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::vector<IScriptNodeType::SettingType> ScriptComparison::getSettingTypes() const
{
	return { SettingType{ "operator", "Halley::String", std::vector<String>{"equals"} } };
}

std::pair<String, std::vector<ColourOverride>> ScriptComparison::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("True if \"");
	str.append(getConnectedNodeName(node, graph, 0), Colour4f(0.97f, 0.35f, 0.35f));
	str.append("\" ");
	str.append(node.getSettings()["operator"].asString("equals"), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" \"");
	str.append(getConnectedNodeName(node, graph, 1), Colour4f(0.97f, 0.35f, 0.35f));
	str.append("\".");
	return str.moveResults();
}

ConfigNode ScriptComparison::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	// TODO
	return ConfigNode(false);
}



gsl::span<const IScriptNodeType::PinType> ScriptSetVariable::getPinConfiguration() const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

std::pair<String, std::vector<ColourOverride>> ScriptSetVariable::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Copies value into variable.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptSetVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	writeDataPin(environment, node, 3, readDataPin(environment, node, 2));
	return Result(ScriptNodeExecutionState::Done);
}


