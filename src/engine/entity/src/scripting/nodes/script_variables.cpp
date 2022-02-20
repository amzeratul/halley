#include "script_variables.h"
using namespace Halley;

String ScriptVariable::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["variable"].asString("");
}

String ScriptVariable::getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const
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

std::pair<String, Vector<ColourOverride>> ScriptVariable::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
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

String ScriptLiteral::getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const
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

std::pair<String, Vector<ColourOverride>> ScriptLiteral::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
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


String ScriptComparison::getLabel(const ScriptGraphNode& node) const
{
	return node.getSettings()["operator"].asString("equals");
}

String ScriptComparison::getShortDescription(const World& world, const ScriptGraphNode& node, const ScriptGraph& graph) const
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

std::pair<String, Vector<ColourOverride>> ScriptComparison::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
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
	// TODO
	return ConfigNode(false);
}



gsl::span<const IScriptNodeType::PinType> ScriptSetVariable::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::WriteDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptSetVariable::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Copies value into variable.");
	return str.moveResults();
}

IScriptNodeType::Result ScriptSetVariable::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	writeDataPin(environment, node, 3, readDataPin(environment, node, 2));
	return Result(ScriptNodeExecutionState::Done);
}


