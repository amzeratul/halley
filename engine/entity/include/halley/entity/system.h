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

	// lol MSVC workarounds
	struct HasInitTag;
	struct HasOnEntityAddedTag;
	struct HasOnEntityRemovedTag;

	// True if T::init() exists
	template <class, class = Halley::void_t<>> struct HasInitMember : std::false_type {};
	template <class T> struct HasInitMember<T, decltype(std::declval<T&>().init())> : std::true_type { };

	// True if T::onEntityAdded(F&) exists
	template <class, class, class = Halley::void_t<>> struct HasOnEntityAdded : std::false_type {};
	template <class T, class F> struct HasOnEntityAdded<T, F, decltype(std::declval<T>().onEntityAdded(std::declval<F&>()))> : std::true_type { };
	
	// True if T::onEntityRemoved(F&) exists
	template <class, class, class = Halley::void_t<>> struct HasOnEntityRemoved : std::false_type {};
	template <class T, class F> struct HasOnEntityRemoved<T, F, decltype(std::declval<T>().onEntityRemoved(std::declval<F&>()))> : std::true_type { };

	
	class System
	{
	public:
		System(std::initializer_list<FamilyBindingBase*> families, std::initializer_list<int> messageTypesReceived);
		virtual ~System() {}

		String getName() const { return name; }
		void setName(String n) { name = n; }
		long long getNanoSecondsTaken() const { return timer.lastElapsedNanoSeconds(); }
		long long getNanoSecondsTakenAvg() const { return timer.averageElapsedNanoSeconds(); }
		size_t getEntityCount() const;
		void tryInit();

	protected:
		HalleyAPI& doGetAPI() const { return *api; }
		World& doGetWorld() const { return *world; }

		virtual void initBase() {}
		virtual void updateBase(Time) {}
		virtual void renderBase(Painter&) {}
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

		template <typename T, typename std::enable_if<HasInitMember<T>::value, int>::type = 0>
		void invokeInit()
		{
			static_cast<T*>(this)->init();
		}

		template <typename T, typename std::enable_if<!HasInitMember<T>::value, int>::type = 0>
		void invokeInit()
		{}

		template <typename T, typename F, typename std::enable_if<HasOnEntityAdded<T, F>::value, int>::type = 0>
		void initialiseOnEntityAdded(FamilyBinding<F>& binding, T* system)
		{
			binding.setOnEntityAdded([system] (void* e) { system->onEntityAdded(*static_cast<F*>(e)); });
		}

		template <typename T, typename F, typename std::enable_if<!HasOnEntityAdded<T, F>::value, int>::type = 0>
		void initialiseOnEntityAdded(FamilyBinding<F>&, T*)
		{}

		template <typename T, typename F, typename std::enable_if<HasOnEntityRemoved<T, F>::value, int>::type = 0>
		void initialiseOnEntityRemoved(FamilyBinding<F>& binding, T* system)
		{
			binding.setOnEntityRemoved([system] (void* e) { system->onEntityRemoved(*static_cast<F*>(e)); });
		}

		template <typename T, typename F, typename std::enable_if<!HasOnEntityRemoved<T, F>::value, int>::type = 0>
		void initialiseOnEntityRemoved(FamilyBinding<F>&, T*)
		{}

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

		World* world = nullptr;
		HalleyAPI* api = nullptr;
		String name;
		int systemId = -1;
		bool initialised = false;

		StopwatchAveraging timer;

		void doUpdate(Time time);
		void doRender(Painter& painter);
		void onAddedToWorld(World& world, int id);

		void purgeMessages();
		void processMessages();
		void doSendMessage(EntityId target, std::unique_ptr<Message> msg, size_t msgSize, int msgId);
	};

}

#define REGISTER_SYSTEM(sys) Halley::System* halleyCreate##sys () { return new sys(); }
