#include "script_wait_for.h"
using namespace Halley;

gsl::span<const IScriptNodeType::PinType> ScriptWaitFor::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptWaitFor::getNodeDescription(const ScriptGraphNode& node, const World& world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Wait until ");
	str.append(addParentheses(getConnectedNodeName(world, node, graph, 1)), Colour4f(0.97f, 0.35f, 0.35f));
	str.append(" is true");
	return str.moveResults();
}

IScriptNodeType::Result ScriptWaitFor::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool done = readDataPin(environment, node, 1).asBool(false);
	return Result(done ? ScriptNodeExecutionState::Done : ScriptNodeExecutionState::Executing);
}
