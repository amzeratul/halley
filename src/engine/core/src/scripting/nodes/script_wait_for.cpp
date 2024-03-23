#include "script_wait_for.h"
using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptWaitFor::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptWaitFor::getNodeDescription(const BaseGraphNode& node, const World* world, const BaseGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Wait until ");
	str.append(addParentheses(getConnectedNodeName(world, node, graph, 1)), settingColour);
	str.append(" is true");
	return str.moveResults();
}

IScriptNodeType::Result ScriptWaitFor::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool done = readDataPin(environment, node, 1).asBool(false);
	return Result(done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing);
}
