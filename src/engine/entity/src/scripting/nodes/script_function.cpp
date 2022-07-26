#include "script_function.h"
using namespace Halley;


Vector<IScriptNodeType::SettingType> ScriptFunctionCallExternal::getSettingTypes() const
{
	return {
		SettingType{ "function", "Halley::ResourceReference<Halley::ScriptGraph>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptFunctionCallExternal::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Call external function ");
	str.append(node.getSettings()["function"].asString(""), parameterColour);
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptFunctionCallExternal::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;

	size_t nOutput = 1;
	size_t nDataInput = 0;
	size_t nTargetInput = 0;
	size_t nDataOutput = 0;
	size_t nTargetOutput = 0;

	static thread_local std::vector<PinType> pins;
	pins.clear();
	pins.reserve(16);

	pins.emplace_back(ET::FlowPin, PD::Input);
	for (size_t i = 0; i < nOutput; ++i) {
		pins.emplace_back(ET::FlowPin, PD::Output);
	}
	for (size_t i = 0; i < nDataInput; ++i) {
		pins.emplace_back(ET::ReadDataPin, PD::Input);
	}
	for (size_t i = 0; i < nTargetInput; ++i) {
		pins.emplace_back(ET::TargetPin, PD::Input);
	}
	for (size_t i = 0; i < nDataOutput; ++i) {
		pins.emplace_back(ET::ReadDataPin, PD::Output);
	}
	for (size_t i = 0; i < nTargetOutput; ++i) {
		pins.emplace_back(ET::TargetPin, PD::Output);
	}

	return pins;
}

IScriptNodeType::Result ScriptFunctionCallExternal::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Call);
}





Vector<IScriptNodeType::SettingType> ScriptFunctionReturn::getSettingTypes() const
{
	return {
		SettingType{ "flowPins", "Halley::Range<int, 1, 4>", Vector<String>{"1"} },
		SettingType{ "dataPins", "Halley::Range<int, 0, 4>", Vector<String>{""} },
		SettingType{ "targetPins", "Halley::Range<int, 0, 4>", Vector<String>{""} },
	};
}

std::pair<String, Vector<ColourOverride>> ScriptFunctionReturn::getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const
{
	auto str = ColourStringBuilder(true);
	str.append("Return to caller");
	return str.moveResults();
}

gsl::span<const IScriptNodeType::PinType> ScriptFunctionReturn::getPinConfiguration(const ScriptGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = ScriptNodePinDirection;

	const size_t nInput = node.getSettings()["flowPins"].asInt(1);
	const size_t nDataInput = node.getSettings()["dataPins"].asInt(0);
	const size_t nTargetInput = node.getSettings()["targetPins"].asInt(0);

	if (nDataInput == 0 && nTargetInput == 0) {
		// Simple, common case
		using ET = ScriptNodeElementType;
		using PD = ScriptNodePinDirection;
		const static auto data = std::array<PinType, 4>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Input } };
		return gsl::span<const PinType>(data).subspan(0, nInput);
	} else {
		static thread_local std::vector<PinType> pins;
		pins.clear();
		pins.reserve(16);

		for (size_t i = 0; i < nInput; ++i) {
			pins.emplace_back(ET::FlowPin, PD::Input);
		}
		for (size_t i = 0; i < nDataInput; ++i) {
			pins.emplace_back(ET::ReadDataPin, PD::Input);
		}
		for (size_t i = 0; i < nTargetInput; ++i) {
			pins.emplace_back(ET::TargetPin, PD::Input);
		}
		return pins;
	}
}

IScriptNodeType::Result ScriptFunctionReturn::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	return Result(ScriptNodeExecutionState::Return);
}
