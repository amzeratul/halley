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
		System(std::initializer_list<FamilyBindingBase*> families, std::initializer_list<int> messageTypesReceived);
		virtual ~System() {}

		String getName() const { return name; }
		void setName(String n) { name = n; }
		int getNanoSecondsTaken() const { return nsTaken; }
		int getNanoSecondsTakenAvg() const { return nsTakenAvg; }

	protected:
		HalleyAPI& doGetAPI() const { return *api; }
		World& doGetWorld() const { return *world; }

		virtual void updateBase(Time) {}
		virtual void renderBase(Painter&) {}
		virtual void onMessagesReceived(int, Message**, size_t*, size_t) {}

		template <typename T, typename M, typename U, typename V>
		static void invokeIndividual(T* obj, M method, U& p, V& fam)
		{
			for (auto& e : fam) {
				(obj->*method)(p, e);
			}
		}

		template <typename T, typename M, typename U, typename V>
		static void invokeParallel(T* obj, M method, U& p, V& fam)
		{
			// TODO, fallback to invokeIndividual for now
			invokeIndividual<T, M, U, V>(obj, method, p, fam);
		}

		template <typename T>
		void sendMessageGeneric(EntityId entityId, const T& msg)
		{
			auto toSend = std::make_unique<T>();
			*toSend = msg;
			doSendMessage(entityId, std::move(toSend), sizeof(T), T::messageIndex);
		}

	private:
		friend class World;

		std::vector<FamilyBindingBase*> families;
		std::vector<int> messageTypesReceived;
		std::vector<EntityId> messagesSentTo;

		World* world;
		HalleyAPI* api;
		String name;
		int systemId = -1;

		int nsTaken = 0;
		int nsTakenAvg = 0;
		int nsTakenAvgAccum = 0;
		int nsTakenAvgSamples = 0;

		void doUpdate(Time time);
		void doRender(Painter& painter);
		void onAddedToWorld(World& world, int id);

		void purgeMessages();
		void processMessages();
		void doSendMessage(EntityId target, std::unique_ptr<Message> msg, size_t msgSize, int msgId);

		void onTimingInformation(int ns);
	};

}

#define REGISTER_SYSTEM(sys) Halley::System* halleyCreate##sys##() { return new sys(); }
