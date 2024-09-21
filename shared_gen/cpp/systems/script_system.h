// Halley codegen version 136
#pragma once

#include <halley.hpp>

#include "halley/entity/services/scripting_service.h"
#include "halley/entity/services/dev_service.h"

#include "components/scriptable_component.h"
#include "components/embedded_script_component.h"
#include "components/script_target_component.h"
#include "messages/start_script_message.h"
#include "messages/terminate_script_message.h"
#include "messages/terminate_scripts_with_tag_message.h"
#include "messages/send_script_msg_message.h"
#include "messages/return_host_script_thread_message.h"
#include "messages/set_entity_variable_message.h"
#include "system_messages/terminate_scripts_with_tag_system_message.h"
#include "system_messages/start_host_script_thread_system_message.h"
#include "system_messages/cancel_host_script_thread_system_message.h"

// Generated file; do not modify.
template <typename T>
class ScriptSystemBase : private Halley::System {
public:
	class ScriptableFamily : public Halley::FamilyBaseOf<ScriptableFamily> {
	public:
		ScriptableComponent& scriptable;
	
		using Type = Halley::FamilyType<ScriptableComponent>;
	
		void prefetch() const {
			prefetchL2(&scriptable);
		}
	
	protected:
		ScriptableFamily(ScriptableComponent& scriptable)
			: scriptable(scriptable)
		{
		}
	};

	class EmbeddedScriptFamily : public Halley::FamilyBaseOf<EmbeddedScriptFamily> {
	public:
		EmbeddedScriptComponent& embeddedScript;
		ScriptableComponent& scriptable;
	
		using Type = Halley::FamilyType<EmbeddedScriptComponent, ScriptableComponent>;
	
		void prefetch() const {
			prefetchL2(&embeddedScript);
			prefetchL2(&scriptable);
		}
	
	protected:
		EmbeddedScriptFamily(EmbeddedScriptComponent& embeddedScript, ScriptableComponent& scriptable)
			: embeddedScript(embeddedScript)
			, scriptable(scriptable)
		{
		}
	};

	class TargetFamily : public Halley::FamilyBaseOf<TargetFamily> {
	public:
		const ScriptTargetComponent& scriptTarget;
	
		using Type = Halley::FamilyType<ScriptTargetComponent>;
	
		void prefetch() const {
			prefetchL2(&scriptTarget);
		}
	
	protected:
		TargetFamily(const ScriptTargetComponent& scriptTarget)
			: scriptTarget(scriptTarget)
		{
		}
	};

	virtual void onMessageReceived(const StartScriptMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const TerminateScriptMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const TerminateScriptsWithTagMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const SendScriptMsgMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const ReturnHostScriptThreadMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const SetEntityVariableMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const TerminateScriptsWithTagSystemMessage& msg) = 0;

	virtual void onMessageReceived(StartHostScriptThreadSystemMessage msg) = 0;

	virtual void onMessageReceived(CancelHostScriptThreadSystemMessage msg) = 0;

	ScriptSystemBase()
		: System({&scriptableFamily, &embeddedScriptFamily, &targetFamily}, {StartScriptMessage::messageIndex, TerminateScriptMessage::messageIndex, TerminateScriptsWithTagMessage::messageIndex, SendScriptMsgMessage::messageIndex, ReturnHostScriptThreadMessage::messageIndex, SetEntityVariableMessage::messageIndex})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	const Halley::HalleyAPI& getAPI() const {
		return doGetAPI();
	}
	Halley::World& getWorld() const {
		return doGetWorld();
	}
	Halley::Resources& getResources() const {
		return doGetResources();
	}
	Halley::SystemMessageBridge getMessageBridge() {
		return doGetMessageBridge();
	}
	Halley::TempMemoryPool& getTempMemoryPool() const {
		return doGetWorld().getUpdateMemoryPool();
	}
	void sendMessage(Halley::EntityId entityId, SendScriptMsgMessage msg) {
		sendMessageGeneric(entityId, std::move(msg));
	}
	void sendMessage(Halley::EntityId entityId, ReturnHostScriptThreadMessage msg) {
		sendMessageGeneric(entityId, std::move(msg));
	}
	void sendMessage(StartHostScriptThreadSystemMessage msg, std::function<void()> callback = {}) {
		Halley::String targetSystem = "";
		const size_t n = sendSystemMessageGeneric<decltype(msg), decltype(callback)>(std::move(msg), std::move(callback), targetSystem);
		if (n != 1) {
		    throw Halley::Exception("Sending non-multicast StartHostScriptThreadSystemMessage, but there are " + Halley::toString(n) + " systems receiving it (expecting exactly one).", Halley::HalleyExceptions::Entity);
		}
	}

	void sendMessage(const Halley::String& targetSystem, StartHostScriptThreadSystemMessage msg, std::function<void()> callback = {}) {
		const size_t n = sendSystemMessageGeneric<decltype(msg), decltype(callback)>(std::move(msg), std::move(callback), targetSystem);
		if (n != 1) {
		    throw Halley::Exception("Sending non-multicast StartHostScriptThreadSystemMessage, but there are " + Halley::toString(n) + " systems receiving it (expecting exactly one).", Halley::HalleyExceptions::Entity);
		}
	}

	void sendMessage(CancelHostScriptThreadSystemMessage msg, std::function<void()> callback = {}) {
		Halley::String targetSystem = "";
		const size_t n = sendSystemMessageGeneric<decltype(msg), decltype(callback)>(std::move(msg), std::move(callback), targetSystem);
		if (n != 1) {
		    throw Halley::Exception("Sending non-multicast CancelHostScriptThreadSystemMessage, but there are " + Halley::toString(n) + " systems receiving it (expecting exactly one).", Halley::HalleyExceptions::Entity);
		}
	}

	void sendMessage(const Halley::String& targetSystem, CancelHostScriptThreadSystemMessage msg, std::function<void()> callback = {}) {
		const size_t n = sendSystemMessageGeneric<decltype(msg), decltype(callback)>(std::move(msg), std::move(callback), targetSystem);
		if (n != 1) {
		    throw Halley::Exception("Sending non-multicast CancelHostScriptThreadSystemMessage, but there are " + Halley::toString(n) + " systems receiving it (expecting exactly one).", Halley::HalleyExceptions::Entity);
		}
	}


	ScriptingService& getScriptingService() const {
		return *scriptingService;
	}

	DevService& getDevService() const {
		return *devService;
	}
	Halley::FamilyBinding<ScriptableFamily> scriptableFamily{};
	Halley::FamilyBinding<EmbeddedScriptFamily> embeddedScriptFamily{};
	Halley::FamilyBinding<TargetFamily> targetFamily{};

private:
	friend Halley::System* halleyCreateScriptSystem();

	ScriptingService* scriptingService{ nullptr };
	DevService* devService{ nullptr };
	void initBase() override final {
		scriptingService = &doGetWorld().template getService<ScriptingService>(getName());
		devService = &doGetWorld().template getService<DevService>(getName());
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, ScriptableFamily>(scriptableFamily, static_cast<T*>(this));
		initialiseFamilyBinding<T, EmbeddedScriptFamily>(embeddedScriptFamily, static_cast<T*>(this));
		initialiseFamilyBinding<T, TargetFamily>(targetFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

	void processMessages() override final {
		doProcessMessages(scriptableFamily, std::array<int, 6>{ StartScriptMessage::messageIndex, TerminateScriptMessage::messageIndex, TerminateScriptsWithTagMessage::messageIndex, SendScriptMsgMessage::messageIndex, ReturnHostScriptThreadMessage::messageIndex, SetEntityVariableMessage::messageIndex });
	}

	void onMessagesReceived(int msgIndex, Halley::Message** msgs, size_t* idx, size_t n, Halley::FamilyBindingBase& family) override final {
		switch (msgIndex) {
		case StartScriptMessage::messageIndex: onMessagesReceived(reinterpret_cast<StartScriptMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case TerminateScriptMessage::messageIndex: onMessagesReceived(reinterpret_cast<TerminateScriptMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case TerminateScriptsWithTagMessage::messageIndex: onMessagesReceived(reinterpret_cast<TerminateScriptsWithTagMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case SendScriptMsgMessage::messageIndex: onMessagesReceived(reinterpret_cast<SendScriptMsgMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case ReturnHostScriptThreadMessage::messageIndex: onMessagesReceived(reinterpret_cast<ReturnHostScriptThreadMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case SetEntityVariableMessage::messageIndex: onMessagesReceived(reinterpret_cast<SetEntityVariableMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		}
	}

	template <typename M, typename F>
	void onMessagesReceived(M** msgs, size_t* idx, size_t n, F& family) {
		for (size_t i = 0; i < n; i++) static_cast<T*>(this)->onMessageReceived(*msgs[i], family[idx[i]]);
	}

	void onSystemMessageReceived(const Halley::SystemMessageContext& context) override final {
		switch (context.msgId) {
		case TerminateScriptsWithTagSystemMessage::messageIndex: {
		    auto& realMsg = reinterpret_cast<TerminateScriptsWithTagSystemMessage&>(*context.msg);
		    static_cast<T*>(this)->onMessageReceived(realMsg);
		    if (context.callback) {
		        context.callback(nullptr, {});
		    }
		    break;
		}
		case StartHostScriptThreadSystemMessage::messageIndex: {
		    auto& realMsg = reinterpret_cast<StartHostScriptThreadSystemMessage&>(*context.msg);
		    static_cast<T*>(this)->onMessageReceived(std::move(realMsg));
		    if (context.callback) {
		        context.callback(nullptr, {});
		    }
		    break;
		}
		case CancelHostScriptThreadSystemMessage::messageIndex: {
		    auto& realMsg = reinterpret_cast<CancelHostScriptThreadSystemMessage&>(*context.msg);
		    static_cast<T*>(this)->onMessageReceived(std::move(realMsg));
		    if (context.callback) {
		        context.callback(nullptr, {});
		    }
		    break;
		}
		}
	}
	bool canHandleSystemMessage(int msgIndex, const Halley::String& targetSystem) const override final {
		if (!targetSystem.isEmpty() && targetSystem != getName()) return false;
		switch (msgIndex) {
		case TerminateScriptsWithTagSystemMessage::messageIndex: return true;
		case StartHostScriptThreadSystemMessage::messageIndex: return true;
		case CancelHostScriptThreadSystemMessage::messageIndex: return true;
		}
		return false;
	}
};

