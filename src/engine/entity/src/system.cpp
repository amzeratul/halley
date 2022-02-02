#include "system.h"
#include <halley/data_structures/flat_map.h>
#include "halley/support/debug.h"
#include "halley/support/logger.h"
#include "halley/support/profiler.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

SystemMessageBridge::SystemMessageBridge(System& system)
	: system(&system)
{
}

bool SystemMessageBridge::isValid() const
{
	return system != nullptr;
}

void SystemMessageBridge::sendMessageToEntity(EntityId target, int msgId, gsl::span<const gsl::byte> data)
{
	system->sendRawMessage(target, msgId, data);
}

System::System(Vector<FamilyBindingBase*> uninitializedFamilies, Vector<int> messageTypesReceived)
	: families(std::move(uninitializedFamilies))
	, messageTypesReceived(std::move(messageTypesReceived))
{
}

size_t System::getEntityCount() const
{
	size_t n = 0;
	for (auto f : families) {
		n += f->count();
	}
	return n;
}

bool System::tryInit()
{
	if (!initialised) {
		initBase();
		initialised = true;
		return true;
	}
	return false;
}

void System::onAddedToWorld(World& w, int id) {
	world = &w;
	systemId = id;
	for (auto f : families) {
		f->bindFamily(*f, w);
	}
}

void System::purgeMessages()
{
	if (!messagesSentTo.empty()) {
		for (auto& target: messagesSentTo) {
			// Purge all messages of this age
			Entity* entity = world->tryGetRawEntity(target);

			if (entity) {
				std_ex::erase_if(entity->inbox, [&] (const MessageEntry& e) { return e.age == systemId; });
			}
		}
		messagesSentTo.clear();
	}
}

void System::processMessages()
{
	// This whole method is probably very inefficient.
	struct MessageBox
	{
		Vector<Message*> msg;
		Vector<size_t> elemIdx;
	};
	FlatMap<int, MessageBox> inboxes;

	if (!families.empty()) {
		auto& fam = *families[0];
		size_t sz = fam.count();
		for (size_t i = 0; i < sz; i++) {
			FamilyBase* elem = reinterpret_cast<FamilyBase*>(fam.getElement(i));
			Entity* entity = world->tryGetRawEntity(elem->entityId);
			if (entity) {
				for (const auto& msg: entity->inbox) {
					if (std::find(messageTypesReceived.begin(), messageTypesReceived.end(), msg.type) != messageTypesReceived.end()) {
						auto& inbox = inboxes[msg.type];
						inbox.msg.emplace_back(msg.msg.get());
						inbox.elemIdx.emplace_back(i);
					}
				}
			}
		}
		for (auto& iter : inboxes) {
			int id = iter.first;
			auto& inbox = iter.second;
			onMessagesReceived(id, inbox.msg.data(), inbox.elemIdx.data(), inbox.msg.size());
		}
	}
}

void System::doSendMessage(EntityId entityId, std::unique_ptr<Message> msg, int id)
{
	if (world->isEntityNetworkRemote(entityId)) {
		world->sendNetworkMessage(entityId, id, std::move(msg));
	} else {
		outbox.emplace_back(std::make_pair(entityId, MessageEntry(std::move(msg), id, systemId)));
	}
}

void System::dispatchMessages()
{
	if (!outbox.empty()) {
		for (auto& o: outbox) {
			Entity* entity = world->tryGetRawEntity(o.first);
			if (entity) {
				entity->inbox.emplace_back(std::move(o.second));
				messagesSentTo.push_back(o.first);
			}
		}
		outbox.clear();
	}
}

size_t System::doSendSystemMessage(SystemMessageContext context, const String& targetSystem)
{
	return world->sendSystemMessage(std::move(context), targetSystem);
}

void System::receiveSystemMessage(const SystemMessageContext& context)
{
	systemMessageInbox.push_back(&context);
}

void System::prepareSystemMessages()
{
	systemMessages = std::move(systemMessageInbox);
	systemMessageInbox.clear();
}

void System::processSystemMessages()
{
	for (auto& message: systemMessages) {
		onSystemMessageReceived(message->msgId, *message->msg, message->callback);
	}
	systemMessages.clear();
}

size_t System::getSystemMessagesInInbox() const
{
	return systemMessageInbox.size();
}

void System::sendRawMessage(EntityId target, int msgId, gsl::span<const std::byte> data)
{
	doSendMessage(target, world->deserializeMessage(msgId, data), msgId);
}

void System::doUpdate(Time time) {
	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
	ProfilerEvent event(ProfilerEventType::WorldSystemUpdate, name);

	purgeMessages();
	if (!messageTypesReceived.empty()) {
		processMessages();
	}
	
	updateBase(time);
	dispatchMessages();
	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
}

void System::doRender(RenderContext& rc) {
	if (!initialised) {
		throw Exception("System " + name + " is being rendered before being initialised. Make sure a World::step() happens before World::render().", HalleyExceptions::Entity);
	}
	
	ProfilerEvent event(ProfilerEventType::WorldSystemRender, name);
	renderBase(rc);

	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
}
