// Halley codegen version 123
#pragma once

#include <halley.hpp>

#include "halley/entity/services/session_service.h"

#include "components/network_component.h"
#include "system_messages/network_entity_lock_system_message.h"

// Generated file; do not modify.
template <typename T>
class NetworkLockSystemBase : private Halley::System {
public:
	class NetworkFamily : public Halley::FamilyBaseOf<NetworkFamily> {
	public:
		NetworkComponent& network;
	
		using Type = Halley::FamilyType<NetworkComponent>;
	
	protected:
		NetworkFamily(NetworkComponent& network)
			: network(network)
		{
		}
	};

	virtual bool onMessageReceived(NetworkEntityLockSystemMessage msg) = 0;

	NetworkLockSystemBase()
		: System({&networkFamily}, {})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	Halley::World& getWorld() const {
		return doGetWorld();
	}
	void sendMessage(NetworkEntityLockSystemMessage msg, std::function<void(bool)> callback = {}) {
		Halley::String targetSystem = "";
		const size_t n = sendSystemMessageGeneric<decltype(msg), decltype(callback)>(std::move(msg), std::move(callback), targetSystem);
		if (n != 1) {
		    throw Halley::Exception("Sending non-multicast NetworkEntityLockSystemMessage, but there are " + Halley::toString(n) + " systems receiving it (expecting exactly one).", Halley::HalleyExceptions::Entity);
		}
	}

	void sendMessage(const Halley::String& targetSystem, NetworkEntityLockSystemMessage msg, std::function<void(bool)> callback = {}) {
		const size_t n = sendSystemMessageGeneric<decltype(msg), decltype(callback)>(std::move(msg), std::move(callback), targetSystem);
		if (n != 1) {
		    throw Halley::Exception("Sending non-multicast NetworkEntityLockSystemMessage, but there are " + Halley::toString(n) + " systems receiving it (expecting exactly one).", Halley::HalleyExceptions::Entity);
		}
	}


	SessionService& getSessionService() const {
		return *sessionService;
	}
	Halley::FamilyBinding<NetworkFamily> networkFamily{};

private:
	friend Halley::System* halleyCreateNetworkLockSystem();

	SessionService* sessionService{ nullptr };
	void initBase() override final {
		sessionService = &doGetWorld().template getService<SessionService>(getName());
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, NetworkFamily>(networkFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

	void onSystemMessageReceived(const Halley::SystemMessageContext& context) override final {
		switch (context.msgId) {
		case NetworkEntityLockSystemMessage::messageIndex: {
		    auto& realMsg = reinterpret_cast<NetworkEntityLockSystemMessage&>(*context.msg);
		    auto result = static_cast<T*>(this)->onMessageReceived(std::move(realMsg));
		    if (context.callback) {
		        if (context.remote) {
		            context.callback(nullptr, Halley::Serializer::toBytes(result, Halley::SerializerOptions(Halley::SerializerOptions::maxVersion)));
		        } else {
		            context.callback(reinterpret_cast<std::byte*>(&result), {});
		        }
		    }
		    break;
		}
		}
	}
	bool canHandleSystemMessage(int msgIndex, const Halley::String& targetSystem) const override final {
		if (!targetSystem.isEmpty() && targetSystem != getName()) return false;
		switch (msgIndex) {
		case NetworkEntityLockSystemMessage::messageIndex: return true;
		}
		return false;
	}
};

