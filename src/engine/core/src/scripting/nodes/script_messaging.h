#pragma once
#include "halley/scripting/script_environment.h"

namespace Halley {
	class ScriptSendMessage final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "sendMessage"; }
		String getName() const override { return "Send Message"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/send_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptSendGenericMessage final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "sendGenericMessage"; }
		String getName() const override { return "Send Generic Message"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/send_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};

	class ScriptReceiveMessageData final : public ScriptStateData<ScriptReceiveMessageData> {
	public:
		ConfigNode curArgs;
		bool hasMessageActive = false;

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptReceiveMessage final : public ScriptNodeTypeBase<ScriptReceiveMessageData> {
	public:
		String getId() const override { return "receiveMessage"; }
		String getName() const override { return "Receive Message"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/receive_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Terminator; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;
		String getLabel(const ScriptGraphNode& node) const override;
		bool hasDestructor(const ScriptGraphNode& node) const override;
		bool showDestructor() const override;
		std::optional<Vector2f> getNodeSize(const BaseGraphNode& node, float curZoom) const override;

		void doInitData(ScriptReceiveMessageData& data, const ScriptGraphNode& node, const EntitySerializationContext& context,	const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptReceiveMessageData& data) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptReceiveMessageData& data) const override;
		void doDestructor(ScriptEnvironment& environment, const ScriptGraphNode& node, ScriptReceiveMessageData& data) const override;

		bool canReceiveMessage(const ScriptGraphNode& node, const String& messageId, bool requiresSpawningScript) const;
		bool tryReceiveMessage(const ScriptGraphNode& node, ScriptReceiveMessageData& data, ScriptMessage& msg) const;
		std::pair<String, int> getMessageIdAndParams(const ScriptGraphNode& node) const;
	};


	class ScriptSendSystemMessageData final : public ScriptStateData<ScriptSendSystemMessageData> {
	public:
		bool waitingForResult = false;
		ConfigNode result;

		ScriptSendSystemMessageData() = default;
		ScriptSendSystemMessageData(const ConfigNode& node);

		ConfigNode toConfigNode(const EntitySerializationContext& context) override;
	};

	class ScriptSendSystemMessage final : public ScriptNodeTypeBase<ScriptSendSystemMessageData> {
	public:
		String getId() const override { return "sendSystemMessage"; }
		String getName() const override { return "Send System Msg"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/send_system_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		String getPinDescription(const ScriptGraphNode& node, PinType elementType, GraphPinId elementIdx) const override;
		String getShortDescription(const World* world, const ScriptGraphNode& node, const ScriptGraph& graph, GraphPinId elementIdx) const override;

		void doInitData(ScriptSendSystemMessageData& data, const ScriptGraphNode& node, const EntitySerializationContext& context, const ConfigNode& nodeData) const override;
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node, ScriptSendSystemMessageData& curData) const override;
		ConfigNode doGetData(ScriptEnvironment& environment, const ScriptGraphNode& node, size_t pinN, ScriptSendSystemMessageData& curData) const override;

	private:
		template <typename T> std::function<void(std::byte*, Bytes)> makeCallback(ScriptEnvironment& environment, const ScriptGraphNode& node) const;
	};

	class ScriptSendEntityMessage final : public ScriptNodeTypeBase<void> {
	public:
		String getId() const override { return "sendEntityMessage"; }
		String getName() const override { return "Send Entity Msg"; }
		String getIconName(const ScriptGraphNode& node) const override { return "script_icons/send_entity_message.png"; }
		ScriptNodeClassification getClassification() const override { return ScriptNodeClassification::Action; }

		Vector<SettingType> getSettingTypes() const override;
		gsl::span<const PinType> getPinConfiguration(const ScriptGraphNode& node) const override;
		std::pair<String, Vector<ColourOverride>> getNodeDescription(const ScriptGraphNode& node, const World* world, const ScriptGraph& graph) const override;
		
		Result doUpdate(ScriptEnvironment& environment, Time time, const ScriptGraphNode& node) const override;
	};
}
