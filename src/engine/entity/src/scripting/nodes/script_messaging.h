#pragma once
#include "scripting/script_environment.h"

namespace Halley {
	class ScriptSendMessage final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "sendMessage"; }
		String getName() const override { return "Message Send"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/send_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptReceiveMessage final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "receiveMessage"; }
		String getName() const override { return "Message Receive"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/receive_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSendSystemMessage final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "sendSystemMessage"; }
		String getName() const override { return "System Msg. Send"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/send_system_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
