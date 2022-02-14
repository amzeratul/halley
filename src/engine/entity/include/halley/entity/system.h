#pragma once

#include <halley/data_structures/vector.h>
#include <halley/concurrency/concurrent.h>
#include <initializer_list>

#include "family_binding.h"
#include "family_mask.h"
#include "family_type.h"
#include "entity.h"
#include "halley/utils/type_traits.h"
#include "system_message.h"
#include "halley/bytes/byte_serializer.h"

namespace Halley {
	class Message;
	class SystemMessage;
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

	// True if T::onEntityModified() exists
	template <class, class, class = Halley::void_t<>> struct HasOnEntitiesReloaded : std::false_type {};
	template <class T, class F> struct HasOnEntitiesReloaded<T, F, decltype(std::declval<T>().onEntitiesReloaded(std::declval<Span<F*>>()))> : std::true_type {};

	class SystemMessageBridge {
	public:
		SystemMessageBridge() = default;
		SystemMessageBridge(System& system);

		bool isValid() const;
		void sendMessageToEntity(EntityId target, int msgId, gsl::span<const gsl::byte> data);
		void sendMessageToSystem(const String& targetSystem, int messageType, gsl::span<const std::byte> data, SystemMessageCallback callback);

	private:
		System* system = nullptr;
	};
	
	class System
	{
		friend class SystemMessageBridge;
		
	public:
		System(Vector<FamilyBindingBase*> families, Vector<int> messageTypesReceived);
		virtual ~System() {}

		const String& getName() const { return name; }
		void setName(String n) { name = std::move(n); }
		size_t getEntityCount() const;
		bool tryInit();

		virtual bool canHandleSystemMessage(int messageId, const String& targetSystem) const { return false; }
		void receiveSystemMessage(const SystemMessageContext& context);
		void prepareSystemMessages();
		void processSystemMessages();
		size_t getSystemMessagesInInbox() const;

		void sendEntityMessageFromNetwork(EntityId target, int msgId, gsl::span<const std::byte> data);
		void sendSystemMessageFromNetwork(const String& targetSystem, int msgId, gsl::span<const std::byte> data, SystemMessageCallback callback);

	protected:
		const HalleyAPI& doGetAPI() const { return *api; }
		World& doGetWorld() const { return *world; }
		Resources& doGetResources() const { return *resources; }
		SystemMessageBridge doGetMessageBridge() { return SystemMessageBridge(*this); }

		virtual void initBase() {}
		virtual void deInit() {}
		virtual void updateBase(Time) {}
		virtual void renderBase(RenderContext&) {}

		virtual void processMessages();
		void doProcessMessages(FamilyBindingBase& family, gsl::span<const int> typesAccepted);
		virtual void onMessagesReceived(int, Message**, size_t*, size_t, FamilyBindingBase&) {}
		virtual void onSystemMessageReceived(int messageId, SystemMessage& msg, const SystemMessageCallback& callback) {}

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
		void sendMessageGeneric(EntityId entityId, T msg)
		{
			auto toSend = std::make_unique<T>();
			*toSend = std::move(msg);
			doSendMessage(entityId, std::move(toSend), T::messageIndex);
		}

		template <typename T, typename F>
		size_t sendSystemMessageGeneric(T msg, F returnLambda, const String& targetSystem)
		{
			SystemMessageContext context;

			context.msgId = T::messageIndex;
			context.msg = std::make_unique<T>(std::move(msg));
			context.callback = [=, returnLambda = std::move(returnLambda)] (std::byte* data, Bytes serializedData)
			{
				Expects((data != nullptr) ^ (!serializedData.empty())); // Exactly one must contain data
				
				if (returnLambda) {
					if constexpr (std::is_same_v<typename T::ReturnType, void>) {
						static_cast<void>(data);
						static_cast<void>(serializedData);
						returnLambda();
					} else {
						if (data) {
							returnLambda(std::move(*reinterpret_cast<typename T::ReturnType*>(data)));
						} else {
							auto options = SerializerOptions(SerializerOptions::maxVersion);
							returnLambda(Deserializer::fromBytes<typename T::ReturnType>(serializedData, std::move(options)));
						}
					}
				}
			};
			
			return doSendSystemMessage(std::move(context), targetSystem, T::messageDestination);
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
		void initializeOnEntityReloaded(FamilyBinding<F>& binding, T* system)
		{
			if constexpr (HasOnEntitiesReloaded<T, F>::value) {
				binding.setOnEntitiesReloaded([system](void* es, size_t count)
				{
					system->onEntitiesReloaded(Span<F*>(static_cast<F**>(es), count));
				});
			}
		}

		template <typename T, typename F>
		void initialiseFamilyBinding(FamilyBinding<F>& binding, T* system)
		{
			initialiseOnEntityAdded<T, F>(binding, system);
			initialiseOnEntityRemoved<T, F>(binding, system);
			initializeOnEntityReloaded<T, F>(binding, system);
		}

	private:
		friend class World;

		Vector<FamilyBindingBase*> families;
		Vector<int> messageTypesReceived;
		Vector<EntityId> messagesSentTo;
		Vector<std::pair<EntityId, MessageEntry>> outbox;
		Vector<const SystemMessageContext*> systemMessageInbox;
		Vector<const SystemMessageContext*> systemMessages;

		World* world = nullptr;
		const HalleyAPI* api = nullptr;
		Resources* resources = nullptr;
		String name;
		int systemId = -1;
		bool initialised = false;

		void doUpdate(Time time);
		void doRender(RenderContext& rc);
		void onAddedToWorld(World& world, int id);

		void purgeMessages();
		void doSendMessage(EntityId target, std::unique_ptr<Message> msg, int msgId);
		size_t doSendSystemMessage(SystemMessageContext context, const String& targetSystem, SystemMessageDestination destination);
		void dispatchMessages();
	};

}

#define REGISTER_SYSTEM(sys) Halley::System* halleyCreate##sys () { return new sys(); }
