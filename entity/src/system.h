#pragma once

#include <vector>
#include <initializer_list>

#include "family_binding.h"
#include "family_mask.h"
#include "family_type.h"
#include "entity.h"

namespace Halley {
	class Message;
	class HalleyAPI;

	class System
	{
	public:
		System(std::initializer_list<FamilyBindingBase*> uninitializedFamilies);
		virtual ~System() {}

	protected:
		HalleyAPI& doGetAPI() const { return *api; }
		World& doGetWorld() const { return *world; }

		virtual void updateBase(Time) {}
		virtual void renderBase(Painter&) {}
		virtual void onMessagesReceived(int, Message*, size_t*, size_t) {}

		template <typename T, typename M, typename U, typename V>
		static void invokeIndividual(T* obj, M method, U& p, V& fam)
		{
			for (auto& e : fam) {
				(obj->*method)(p, e);
			}
		}

		template <typename T>
		void sendMessageGeneric(EntityId entityId, const T& msg)
		{
			doSendMessage(entityId, msg, sizeof(T), T::messageIndex);
		}

	private:
		friend class World;

		int nsTaken = 0;
		std::vector<FamilyBindingBase*> families;
		World* world;
		String name;
		HalleyAPI* api;

		void doUpdate(Time time);
		void doRender(Painter& painter);
		void onAddedToWorld(World& world);

		void doSendMessage(EntityId target, const Message& msg, size_t msgSize, int msgId);
	};

}

#define REGISTER_SYSTEM(sys) Halley::System* halleyCreate##sys##() { return new sys(); }
