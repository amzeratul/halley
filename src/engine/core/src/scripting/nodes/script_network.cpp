#include "script_network.h"

#include "halley/support/logger.h"
using namespace Halley;

String ScriptEntityAuthority::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "has authority over " + getConnectedNodeName(node, graph, 0);
}

gsl::span<const IScriptNodeType::PinType> ScriptEntityAuthority::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::TargetPin, PD::Input }, PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptEntityAuthority::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Has authority over ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

ConfigNode ScriptEntityAuthority::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(environment.hasNetworkAuthorityOver(readEntityId(environment, node, 0)));
}




String ScriptHostAuthority::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	return "has host authority";
}

gsl::span<const IScriptNodeType::PinType> ScriptHostAuthority::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 1>{ PinType{ ET::ReadDataPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptHostAuthority::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "Has host authority", {} };
}

ConfigNode ScriptHostAuthority::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	return ConfigNode(environment.hasHostNetworkAuthority());
}




gsl::span<const IScriptNodeType::PinType> ScriptIfEntityAuthority::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 3>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptIfEntityAuthority::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append("If has authority over ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptIfEntityAuthority::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const auto entityId = readEntityId(environment, node, 1);
	const bool hasAuthority = environment.hasNetworkAuthorityOver(entityId);
	return Result(ScriptNodeExecutionState::Done, 0, hasAuthority ? 1 : 0);
}




gsl::span<const IScriptNodeType::PinType> ScriptIfHostAuthority::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::FlowPin, PD::Output } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptIfHostAuthority::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	return { "If has host authority", {} };
}

IScriptNodeType::Result ScriptIfHostAuthority::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	const bool hasAuthority = environment.hasHostNetworkAuthority();
	return Result(ScriptNodeExecutionState::Done, 0, hasAuthority ? 1 : 0);
}



ScriptLockData::ScriptLockData(const ConfigNode& node)
{
}

ConfigNode ScriptLockData::toConfigNode(const EntitySerializationContext& context)
{
	return {};
}

Vector<IGraphNodeType::SettingType> ScriptLock::getSettingTypes() const
{
	return {
		SettingType{ "acquireAuthority", "bool", Vector<String>{"false"} }
	};
}

bool ScriptLock::hasDestructor(const ScriptGraphNode& node) const
{
	return true;
}

void ScriptLock::doInitData(ScriptLockData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data.lock = {};
	data.requestPending = {};
}

void ScriptLock::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptLockData& data) const
{
	data.lock = {};
	data.requestPending = {};
}

String ScriptLock::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Player";
	} else if (elementIdx == 2) {
		return "Target";
	} else if (elementIdx == 3) {
		return "Lock Acquired";
	} else if (elementIdx == 4) {
		return "Lock Not Acquired";
	}
	return ScriptNodeTypeBase<ScriptLockData>::getPinDescription(node, elementType, elementIdx);
}

gsl::span<const IGraphNodeType::PinType> ScriptLock::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::FlowPin, PD::Output }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLock::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	str.append(" tries to acquire network lock");
	if (node.getSettings()["acquireAuthority"].asBool(false)) {
		str.append(" and authority", settingColour);
	}
	str.append(" over entity ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptLock::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLockData& data) const
{
	if (!data.requestPending.isValid()) {
		auto target = readEntityId(environment, node, 2);
		if (!target.isValid()) {
			return Result(ScriptNodeExecutionState::Done, 0, 1);
		}
		bool acquireAuthority = node.getSettings()["acquireAuthority"].asBool(false);
		data.requestPending = environment.getInterface<INetworkLockSystemInterface>().lockAcquire(readEntityId(environment, node, 1), target, acquireAuthority);
	}

	if (data.requestPending.hasValue()) {
		// Got response
		data.lock = data.requestPending.get();
		return Result(ScriptNodeExecutionState::Done, 0, data.lock ? 1 : 2);
	} else {
		// Waiting
		return Result(ScriptNodeExecutionState::Executing, time);
	}
}



String ScriptLockAvailable::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Lock for " + getConnectedNodeName(node, graph, 1) + " available to " + getConnectedNodeName(node, graph, 0);
	} else {
		return "Lock for " + getConnectedNodeName(node, graph, 1) + " available to all";
	}
}

gsl::span<const IGraphNodeType::PinType> ScriptLockAvailable::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 4>{
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::TargetPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Output },
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLockAvailable::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Is lock for ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	str.append(" available or acquired by ");
	str.append(getConnectedNodeName(node, graph, 0), parameterColour);
	return str.moveResults();
}

String ScriptLockAvailable::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 2) {
		return "Available to entity";
	} else if (elementIdx == 3) {
		return "Available to all";
	}
	return ScriptNodeTypeBase<void>::getPinDescription(node, elementType, elementIdx);
}

ConfigNode ScriptLockAvailable::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN) const
{
	const auto player = readEntityId(environment, node, 0);
	const auto toLock = readEntityId(environment, node, 1);

	const auto& lockInterface = environment.getInterface<INetworkLockSystemInterface>();
	if (pinN == 2) {
		return ConfigNode(lockInterface.isLockedByOrAvailableTo(player, toLock));
	} else if (pinN == 3) {
		return ConfigNode(lockInterface.getLockStatus(toLock) == INetworkLockSystemInterface::LockStatus::Unlocked);
	}

	return {};
}




ScriptLockAvailableGateData::ScriptLockAvailableGateData(const ConfigNode& node)
{
	if (node.hasKey("flowing")) {
		flowing = node["flowing"].asBool();
	}
}

ConfigNode ScriptLockAvailableGateData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	if (flowing) {
		result["flowing"] = *flowing;
	}
	return result;
}

String ScriptLockAvailableGate::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Player";
	} else if (elementIdx == 2) {
		return "Target";
	} else if (elementIdx == 3) {
		return "Flow if available";
	} else if (elementIdx == 4) {
		return "Flow if not available";
	}
	return ScriptNodeTypeBase<ScriptLockAvailableGateData>::getPinDescription(node, elementType, elementIdx);
}

void ScriptLockAvailableGate::doInitData(ScriptLockAvailableGateData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const
{
	data.flowing.reset();
}

gsl::span<const IGraphNodeType::PinType> ScriptLockAvailableGate::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 5>{ PinType{ ET::FlowPin, PD::Input }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::TargetPin, PD::Input }, PinType{ ET::FlowPin, PD::Output, true }, PinType{ ET::FlowPin, PD::Output, true } };
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptLockAvailableGate::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Flow based on lock for ");
	str.append(getConnectedNodeName(node, graph, 2), parameterColour);
	str.append(" being available or acquired by ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptLockAvailableGate::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptLockAvailableGateData& data) const
{
	const auto shouldFlow = environment.getInterface<INetworkLockSystemInterface>().isLockedByOrAvailableTo(readEntityId(environment, node, 1), readEntityId(environment, node, 2));

	if (shouldFlow != data.flowing) {
		data.flowing = shouldFlow;
		if (shouldFlow) {
			return Result(ScriptNodeExecutionState::Fork, 0, 1, 2);
		} else {
			return Result(ScriptNodeExecutionState::Fork, 0, 2, 1);
		}
	} else {
		return Result(ScriptNodeExecutionState::Executing, time);
	}
}



ScriptTransferToHostData::ScriptTransferToHostData(const ConfigNode& node)
{
	if (node.getType() == ConfigNodeType::Map) {
		waiting = node["waiting"].asBool();
		params = node["params"];
		returnedValue = node["hasReturnedValue"].asBool(true) ? std::optional<ConfigNode>(node["returnedValue"]) : std::optional<ConfigNode>();
	} else {
		waiting = false;
		returnedValue = std::nullopt;
	}
}

ConfigNode ScriptTransferToHostData::toConfigNode(const EntitySerializationContext& context)
{
	ConfigNode::MapType result;
	result["waiting"] = waiting;
	result["params"] = params;
	result["returnedValue"] = returnedValue;
	result["hasReturnedValue"] = returnedValue.has_value();
	return result;
}


gsl::span<const IGraphNodeType::PinType> ScriptTransferToHost::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 6>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::FlowPin, PD::Output, false, false, true, true },
		PinType{ ET::FlowPin, PD::Output },
		PinType{ ET::ReadDataPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Output, false, false, true, true },
		PinType{ ET::ReadDataPin, PD::Output }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptTransferToHost::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Transfer flow control to host, passing ");
	str.append(getConnectedNodeName(node, graph, 3), parameterColour);
	return str.moveResults();
}

String ScriptTransferToHost::getPinDescription(const BaseGraphNode& node, PinType elementType, GraphPinId elementIdx) const
{
	if (elementIdx == 1) {
		return "Flow on Host";
	} else if (elementIdx == 2) {
		return "Flow after Host returns";
	} else if (elementIdx == 3) {
		return "Parameters";
	} else if (elementIdx == 4) {
		return "Host-side parameters";
	} else if (elementIdx == 5) {
		return "Returned value";
	}
	return ScriptNodeTypeBase<ScriptTransferToHostData>::getPinDescription(node, elementType, elementIdx);
}

String ScriptTransferToHost::getShortDescription(const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const
{
	if (elementIdx == 4) {
		return "clientParams";
	} else if (elementIdx == 5) {
		return "hostResult";
	}
	return "";
}

void ScriptTransferToHost::doInitData(ScriptTransferToHostData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const
{
	data = ScriptTransferToHostData(nodeData);
}

IScriptNodeType::Result ScriptTransferToHost::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptTransferToHostData& curData) const
{
	if (!curData.waiting) {
		curData.waiting = true;
		curData.returnedValue = std::nullopt;
		environment.startHostThread(node.getId(), readDataPin(environment, node, 3));
	} else if (curData.returnedValue) {
		curData.waiting = false;
		return Result(ScriptNodeExecutionState::Done, 0, 2);
	}

	return Result(ScriptNodeExecutionState::Executing, time);
}

void ScriptTransferToHost::doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptTransferToHostData& curData) const
{
	if (curData.waiting) {
		environment.cancelHostThread(node.getId());
		curData = {};
	}
}

ConfigNode ScriptTransferToHost::doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptTransferToHostData& curData) const
{
	if (pinN == 4) {
		return ConfigNode(curData.params);
	} else if (pinN == 5) {
		return curData.returnedValue ? ConfigNode(*curData.returnedValue) : ConfigNode();
	}

	return {};
}

void ScriptTransferToHost::notifyReturn(const ScriptGraphNode& node, ScriptTransferToHostData& curData, ConfigNode params) const
{
	if (curData.waiting) {
		curData.returnedValue = std::move(params);
	}
}

void ScriptTransferToHost::setParameters(const ScriptGraphNode& node, ScriptTransferToHostData& curData, ConfigNode params) const
{
	curData.params = std::move(params);
}


gsl::span<const IGraphNodeType::PinType> ScriptTransferToClient::getPinConfiguration(const BaseGraphNode& node) const
{
	using ET = ScriptNodeElementType;
	using PD = GraphNodePinDirection;
	const static auto data = std::array<PinType, 2>{
		PinType{ ET::FlowPin, PD::Input },
		PinType{ ET::ReadDataPin, PD::Input }
	};
	return data;
}

std::pair<String, Vector<ColourOverride>> ScriptTransferToClient::getNodeDescription(const BaseGraphNode& node, const BaseGraph& graph) const
{
	ColourStringBuilder str;
	str.append("Transfer flow control back to client, passing ");
	str.append(getConnectedNodeName(node, graph, 1), parameterColour);
	return str.moveResults();
}

IScriptNodeType::Result ScriptTransferToClient::doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const
{
	environment.returnHostThread(readDataPin(environment, node, 1));
	return Result(ScriptNodeExecutionState::Done);
}
