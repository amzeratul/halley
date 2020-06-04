#pragma once

#include <halley/data_structures/vector.h>
#include <halley/concurrency/concurrent.h>
#include <initializer_list>

#include "family_binding.h"
#include "family_mask.h"
#include "family_type.h"
#include "entity.h"
#include "halley/utils/type_traits.h"

namespace Halley {
	class Message;
	class HalleyAPI;

	template <typename T, std::size_t size = gsl::dynamic_extent> using Span = gsl::span<T, size>;

	// True if T::init() exists
	template <class, class = Halley::void_t<>> struct HasInitMember : std::false_type {};
	template <class T> struct HasInitMember<T, decltype(std::declval<T&>().init())> : std::true_type { };

	// True if T::onEntityAdded(F&) exists
	template <class, class, class = Halley::void_t<>> struct HasOnEntitiesAdded : std::false_type {};
	template <class T, class F> struct HasOnEntitiesAdded<T, F, decltype(std::declval<T>().onEntitiesAdded(std::declval<Span<F>>()))> : std::true_type { };
	
	// True if T::onEntityRemoved(F&) exists
	template <class, class, class = Halley::void_t<>> struct HasOnEntitiesRemoved : std::false_type {};
	template <class T, class F> struct HasOnEntitiesRemoved<T, F, decltype(std::declval<T>().onEntitiesRemoved(std::declval<Span<F>>()))> : std::true_type { };

	
	class System
	{
	public:
		System(Vector<FamilyBindingBase*> families, Vector<int> messageTypesReceived);
		virtual ~System() {}

		const String& getName() const { return name; }
		void setName(String n) { name = std::move(n); }
		size_t getEntityCount() const;
		bool tryInit();

		long long getNanoSecondsTaken() const { return timer.lastElapsedNanoSeconds(); }
		long long getNanoSecondsTakenAvg() const { return timer.averageElapsedNanoSeconds(); }
		void setCollectSamples(bool collect);

	protected:
		const HalleyAPI& doGetAPI() const { return *api; }
		World& doGetWorld() const { return *world; }
		Resources& doGetResources() const { return *resources; }

		virtual void initBase() {}
		virtual void deInit() {}
		virtual void updateBase(Time) {}
		virtual void renderBase(RenderContext&) {}
		virtual void onMessagesReceived(int, Message**, size_t*, size_t) {}

		template <typename F, typename V>
		static void invokeIndividual(F&& f, V& fam)
		{
			for (auto& e : fam) {
				f(e);
			}
		}

		template <typename F, typename V>
		static void invokeParallel(F&& f, V& fam)
		{
			Concurrent::foreach(std::begin(fam), std::end(fam), [&] (auto& e) {
				f(e);
			});
		}

		template <typename T>
		void sendMessageGeneric(EntityId entityId, const T& msg)
		{
			auto toSend = std::make_unique<T>();
			*toSend = msg;
			doSendMessage(entityId, std::move(toSend), sizeof(T), T::messageIndex);
		}

		template <typename T>
		void invokeInit(T* system)
		{
			if constexpr (HasInitMember<T>::value) {
				system->init();
			}
		}

		template <typename T, typename F>
		void initialiseOnEntityAdded(FamilyBinding<F>& binding, T* system)
		{
			if constexpr (HasOnEntitiesAdded<T, F>::value) {
				binding.setOnEntitiesAdded([system] (void* es, size_t count)
				{
					system->onEntitiesAdded(Span<F>(static_cast<F*>(es), count));
				});
			}
		}

		template <typename T, typename F>
		void initialiseOnEntityRemoved(FamilyBinding<F>& binding, T* system)
		{
			if constexpr (HasOnEntitiesRemoved<T, F>::value) {
				binding.setOnEntitiesRemoved([system] (void* es, size_t count)
				{
					system->onEntitiesRemoved(Span<F>(static_cast<F*>(es), count));
				});
			}
		}

		template <typename T, typename F>
		void initialiseFamilyBinding(FamilyBinding<F>& binding, T* system)
		{
			initialiseOnEntityAdded<T, F>(binding, system);
			initialiseOnEntityRemoved<T, F>(binding, system);
		}

	private:
		friend class World;

		Vector<FamilyBindingBase*> families;
		Vector<int> messageTypesReceived;
		Vector<EntityId> messagesSentTo;
		Vector<std::pair<EntityId, MessageEntry>> outbox;

		World* world = nullptr;
		const HalleyAPI* api = nullptr;
		Resources* resources = nullptr;
		String name;
		int systemId = -1;
		bool initialised = false;
		bool collectSamples = false;

		StopwatchAveraging timer;

		void doUpdate(Time time);
		void doRender(RenderContext& rc);
		void onAddedToWorld(World& world, int id);

		void purgeMessages();
		void processMessages();
		void doSendMessage(EntityId target, std::unique_ptr<Message> msg, size_t msgSize, int msgId);
		void dispatchMessages();
	};

}

#define REGISTER_SYSTEM(sys) Halley::System* halleyCreate##sys () { return new sys(); }
