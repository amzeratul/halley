// Halley codegen version 136
#pragma once

#include <halley.hpp>

#include "halley/entity/services/session_service.h"
#include "halley/entity/services/screen_service.h"

#include "components/network_component.h"

// Generated file; do not modify.
template <typename T>
class NetworkSendSystemBase : private Halley::System {
public:
	class NetworkFamily : public Halley::FamilyBaseOf<NetworkFamily> {
	public:
		NetworkComponent& network;
	
		using Type = Halley::FamilyType<NetworkComponent>;
	
		void prefetch() const {
			prefetchL2(&network);
		}
	
	protected:
		NetworkFamily(NetworkComponent& network)
			: network(network)
		{
		}
	};

	NetworkSendSystemBase()
		: System({&networkFamily}, {})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	Halley::World& getWorld() const {
		return doGetWorld();
	}
	Halley::Resources& getResources() const {
		return doGetResources();
	}
	Halley::TempMemoryPool& getTempMemoryPool() const {
		return doGetWorld().getUpdateMemoryPool();
	}

	SessionService& getSessionService() const {
		return *sessionService;
	}

	ScreenService& getScreenService() const {
		return *screenService;
	}
	Halley::FamilyBinding<NetworkFamily> networkFamily{};

private:
	friend Halley::System* halleyCreateNetworkSendSystem();

	SessionService* sessionService{ nullptr };
	ScreenService* screenService{ nullptr };
	void initBase() override final {
		sessionService = &doGetWorld().template getService<SessionService>(getName());
		screenService = &doGetWorld().template getService<ScreenService>(getName());
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, NetworkFamily>(networkFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

};

