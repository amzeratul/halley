// Halley codegen version 131
#pragma once

#include <halley.hpp>

#include "halley/entity/services/screen_service.h"

#include "components/sprite_component.h"
#include "components/sprite_animation_component.h"
#include "halley/entity/components/transform_2d_component.h"
#include "components/sprite_animation_replicator_component.h"
#include "messages/play_animation_message.h"
#include "messages/play_animation_once_message.h"

// Generated file; do not modify.
template <typename T>
class SpriteAnimationSystemBase : private Halley::System {
public:
	class MainFamily : public Halley::FamilyBaseOf<MainFamily> {
	public:
		SpriteComponent& sprite;
		SpriteAnimationComponent& spriteAnimation;
		const Transform2DComponent& transform2D;
	
		using Type = Halley::FamilyType<SpriteComponent, SpriteAnimationComponent, Transform2DComponent>;
	
	protected:
		MainFamily(SpriteComponent& sprite, SpriteAnimationComponent& spriteAnimation, const Transform2DComponent& transform2D)
			: sprite(sprite)
			, spriteAnimation(spriteAnimation)
			, transform2D(transform2D)
		{
		}
	};

	class ReplicatorFamily : public Halley::FamilyBaseOf<ReplicatorFamily> {
	public:
		SpriteComponent& sprite;
		SpriteAnimationComponent& spriteAnimation;
		const SpriteAnimationReplicatorComponent& spriteAnimationReplicator;
	
		using Type = Halley::FamilyType<SpriteComponent, SpriteAnimationComponent, SpriteAnimationReplicatorComponent>;
	
	protected:
		ReplicatorFamily(SpriteComponent& sprite, SpriteAnimationComponent& spriteAnimation, const SpriteAnimationReplicatorComponent& spriteAnimationReplicator)
			: sprite(sprite)
			, spriteAnimation(spriteAnimation)
			, spriteAnimationReplicator(spriteAnimationReplicator)
		{
		}
	};

	virtual void onMessageReceived(const PlayAnimationMessage& msg, MainFamily& e) = 0;

	virtual void onMessageReceived(const PlayAnimationOnceMessage& msg, MainFamily& e) = 0;

	SpriteAnimationSystemBase()
		: System({&mainFamily, &replicatorFamily}, {PlayAnimationMessage::messageIndex, PlayAnimationOnceMessage::messageIndex})
	{
		static_assert(std::is_final_v<T>, "System must be final.");
	}
protected:
	Halley::World& getWorld() const {
		return doGetWorld();
	}
	Halley::TempMemoryPool& getTempMemoryPool() const {
		return doGetWorld().getTempMemoryPool();
	}

	ScreenService& getScreenService() const {
		return *screenService;
	}
	Halley::FamilyBinding<MainFamily> mainFamily{};
	Halley::FamilyBinding<ReplicatorFamily> replicatorFamily{};

private:
	friend Halley::System* halleyCreateSpriteAnimationSystem();

	ScreenService* screenService{ nullptr };
	void initBase() override final {
		screenService = &doGetWorld().template getService<ScreenService>(getName());
		invokeInit<T>(static_cast<T*>(this));
		initialiseFamilyBinding<T, MainFamily>(mainFamily, static_cast<T*>(this));
		initialiseFamilyBinding<T, ReplicatorFamily>(replicatorFamily, static_cast<T*>(this));
	}

	void updateBase(Halley::Time time) override final {
		static_cast<T*>(this)->update(time);
	}

	void processMessages() override final {
		doProcessMessages(mainFamily, std::array<int, 2>{ PlayAnimationMessage::messageIndex, PlayAnimationOnceMessage::messageIndex });
	}

	void onMessagesReceived(int msgIndex, Halley::Message** msgs, size_t* idx, size_t n, Halley::FamilyBindingBase& family) override final {
		switch (msgIndex) {
		case PlayAnimationMessage::messageIndex: onMessagesReceived(reinterpret_cast<PlayAnimationMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<MainFamily>&>(family)); break;
		case PlayAnimationOnceMessage::messageIndex: onMessagesReceived(reinterpret_cast<PlayAnimationOnceMessage**>(msgs), idx, n, reinterpret_cast<Halley::FamilyBinding<MainFamily>&>(family)); break;
		}
	}

	template <typename M, typename F>
	void onMessagesReceived(M** msgs, size_t* idx, size_t n, F& family) {
		for (size_t i = 0; i < n; i++) static_cast<T*>(this)->onMessageReceived(*msgs[i], family[idx[i]]);
	}

};

