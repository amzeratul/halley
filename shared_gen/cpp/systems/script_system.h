// Halley codegen version 123
#pragma once

#include <halley.hpp>

#include "halley/entity/services/scripting_service.h"
#include "halley/entity/services/dev_service.h"

#include "components/scriptable_component.h"
#include "components/embedded_script_component.h"
#include "components/script_target_component.h"
#include "components/script_tag_target_component.h"
#include "messages/start_script_message.h"
#include "messages/terminate_script_message.h"
#include "messages/terminate_scripts_with_tag_message.h"
#include "messages/send_script_msg_message.h"
#include "system_messages/terminate_scripts_with_tag_system_message.h"

// Generated file; do not modify.
template <typename T>
class ScriptSystemBase : private Halley::System {
public:
	class ScriptableFamily : public Halley::FamilyBaseOf<ScriptableFamily> {
	public:
		ScriptableComponent& scriptable;
	
		using Type = Halley::FamilyType<ScriptableComponent>;
	
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
	
	protected:
		TargetFamily(const ScriptTargetComponent& scriptTarget)
			: scriptTarget(scriptTarget)
		{
		}
	};

	class TagTargetsFamily : public Halley::FamilyBaseOf<TagTargetsFamily> {
	public:
		const ScriptTagTargetComponent& scriptTagTarget;
	
		using Type = Halley::FamilyType<ScriptTagTargetComponent>;
	
	protected:
		TagTargetsFamily(const ScriptTagTargetComponent& scriptTagTarget)
			: scriptTagTarget(scriptTagTarget)
		{
		}
	};

	virtual void onMessageReceived(const StartScriptMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const TerminateScriptMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const TerminateScriptsWithTagMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const SendScriptMsgMessage& msg, ScriptableFamily& e) = 0;

	virtual void onMessageReceived(const TerminateScriptsWithTagSystemMessage& msg) = 0;

	ScriptSystemBase()
		: System({&scriptableFamily, &embeddedScriptFamily, &targetFamily, &tagTargetsFamily}, {StartScriptMessage::messageIndex, TerminateScriptMessage::messageIndex, TerminateScriptsWithTagMessage::messageIndex, SendScriptMsgMessage::messageIndex})
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
	void sendMessage(Halley::EntityId entityId, SendScriptMsgMessage msg) {
		sendMessageGeneric(entityId, std::move(msg));
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
	Halley::FamilyBinding<TagTargetsFamily> tagTargetsFamily{};

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
		initialiseFamilyBinding<T, TagTargetsFamily>(tagTargetsFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

	void processMessages() override final {
		doProcessMessages(scriptableFamily, std::array<int, 4>{ StartScriptMessage::messageIndex, TerminateScriptMessage::messageIndex, TerminateScriptsWithTagMessage::messageIndex, SendScriptMsgMessage::messageIndex });
	}

	void onMessagesReceived(int msgIndex, Halley::Message** msgs, size_t* idx, size_t n, Halley::FamilyBindingBase& family) override final {
		switch (msgIndex) {
		case StartScriptMessage::messageIndex: onMessagesReceived(reinterpret_cast<StartScriptMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case TerminateScriptMessage::messageIndex: onMessagesReceived(reinterpret_cast<TerminateScriptMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case TerminateScriptsWithTagMessage::messageIndex: onMessagesReceived(reinterpret_cast<TerminateScriptsWithTagMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
		case SendScriptMsgMessage::messageIndex: onMessagesReceived(reinterpret_cast<SendScriptMsgMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ScriptableFamily>&>(family)); break;
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
		}
	}
	bool canHandleSystemMessage(int msgIndex, const Halley::String& targetSystem) const override final {
		if (!targetSystem.isEmpty() && targetSystem != getName()) return false;
		switch (msgIndex) {
		case TerminateScriptsWithTagSystemMessage::messageIndex: return true;
		}
		return false;
	}
};

