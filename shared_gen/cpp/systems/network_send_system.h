// Halley codegen version 149
#pragma once

#include <halley.hpp>

#include "halley/entity/services/session_service.h"
#include "halley/entity/services/screen_service.h"
#include "halley/entity/services/dev_service.h"

#include "components/network_component.h"
#include "halley/entity/components/transform_2d_component.h"

// Generated file; do not modify.
template <typename T>
class NetworkSendSystemBase : private Halley::System {
public:
	class NetworkFamily : public Halley::FamilyBaseOf<NetworkFamily> {
	public:
		NetworkComponent& network;
		const Halley::MaybeRef<Transform2DComponent> transform2D{};
	
		using Type = Halley::FamilyType<NetworkComponent, Halley::MaybeRef<Transform2DComponent>>;
	
		void prefetch() const {
			prefetchL2(&network);
			prefetchL2(transform2D.tryGet());
		}
	
	protected:
		NetworkFamily(NetworkComponent& network, const Halley::MaybeRef<Transform2DComponent> transform2D)
			: network(network)
			, transform2D(transform2D)
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

	DevService& getDevService() const {
		return *devService;
	}
	Halley::FamilyBinding<NetworkFamily> networkFamily{};

private:
	friend Halley::System* halleyCreateNetworkSendSystem();

	SessionService* sessionService{ nullptr };
	ScreenService* screenService{ nullptr };
	DevService* devService{ nullptr };
	void preInitBase() override final {
		sessionService = &doGetWorld().template getService<SessionService>(getName());
		screenService = &doGetWorld().template getService<ScreenService>(getName());
		devService = &doGetWorld().template getService<DevService>(getName());
		invokePreInit<T>(static_cast<T*>(this));
	}
	void initBase() override final {
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, NetworkFamily>(networkFamily, static_cast<T*>(this), false);
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

};

