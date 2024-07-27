// Halley codegen version 131
#pragma once

#include <halley.hpp>

#include "halley/entity/services/screen_service.h"
#include "halley/entity/services/dev_service.h"
#include "halley/entity/services/debug_draw_service.h"

#include "components/particles_component.h"
#include "halley/entity/components/transform_2d_component.h"
#include "messages/stop_particles_message.h"

// Generated file; do not modify.
template <typename T>
class ParticleSystemBase : private Halley::System {
public:
	class ParticleFamily : public Halley::FamilyBaseOf<ParticleFamily> {
	public:
		ParticlesComponent& particles;
		const Transform2DComponent& transform2D;
	
		using Type = Halley::FamilyType<ParticlesComponent, Transform2DComponent>;
	
	protected:
		ParticleFamily(ParticlesComponent& particles, const Transform2DComponent& transform2D)
			: particles(particles)
			, transform2D(transform2D)
		{
		}
	};

	virtual void onMessageReceived(const StopParticlesMessage& msg, ParticleFamily& e) = 0;

	ParticleSystemBase()
		: System({&particleFamily}, {StopParticlesMessage::messageIndex})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	Halley::World& getWorld() const {
		return doGetWorld();
	}
	Halley::TempMemoryPool& getTempMemoryPool() const {
		return doGetWorld().getUpdateMemoryPool();
	}

	DevService& getDevService() const {
		return *devService;
	}

	DebugDrawService& getDebugDrawService() const {
		return *debugDrawService;
	}

	ScreenService& getScreenService() const {
		return *screenService;
	}
	Halley::FamilyBinding<ParticleFamily> particleFamily{};

private:
	friend Halley::System* halleyCreateParticleSystem();

	DevService* devService{ nullptr };
	DebugDrawService* debugDrawService{ nullptr };
	ScreenService* screenService{ nullptr };
	void initBase() override final {
		devService = &doGetWorld().template getService<DevService>(getName());
		debugDrawService = &doGetWorld().template getService<DebugDrawService>(getName());
		screenService = &doGetWorld().template getService<ScreenService>(getName());
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, ParticleFamily>(particleFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

	void processMessages() override final {
		doProcessMessages(particleFamily, std::array<int, 1>{ StopParticlesMessage::messageIndex });
	}

	void onMessagesReceived(int msgIndex, Halley::Message** msgs, size_t* idx, size_t n, Halley::FamilyBindingBase& family) override final {
		switch (msgIndex) {
		case StopParticlesMessage::messageIndex: onMessagesReceived(reinterpret_cast<StopParticlesMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<ParticleFamily>&>(family)); break;
		}
	}

	template <typename M, typename F>
	void onMessagesReceived(M** msgs, size_t* idx, size_t n, F& family) {
		for (size_t i = 0; i < n; i++) static_cast<T*>(this)->onMessageReceived(*msgs[i], family[idx[i]]);
	}

};

