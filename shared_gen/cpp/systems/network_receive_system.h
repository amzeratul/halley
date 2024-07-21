// Halley codegen version 131
#pragma once

#include <halley.hpp>

#include "halley/entity/services/session_service.h"
#include "halley/entity/services/dev_service.h"

#include "components/network_component.h"

// Generated file; do not modify.
template <typename T>
class NetworkReceiveSystemBase : private Halley::System {
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

	NetworkReceiveSystemBase()
		: System({&networkFamily}, {})
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
		return doGetWorld().getTempMemoryPool();
	}

	SessionService& getSessionService() const {
		return *sessionService;
	}

	DevService& getDevService() const {
		return *devService;
	}
	Halley::FamilyBinding<NetworkFamily> networkFamily{};

private:
	friend Halley::System* halleyCreateNetworkReceiveSystem();

	SessionService* sessionService{ nullptr };
	DevService* devService{ nullptr };
	void initBase() override final {
		sessionService = &doGetWorld().template getService<SessionService>(getName());
		devService = &doGetWorld().template getService<DevService>(getName());
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, NetworkFamily>(networkFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

};

