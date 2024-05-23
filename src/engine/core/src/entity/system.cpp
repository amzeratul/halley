#include "halley/entity/system.h"
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

void SystemMessageBridge::sendMessageToEntity(EntityId target, int msgId, gsl::span<const gsl::byte> data, uint8_t fromPeerId)
{
	system->sendEntityMessage(target, msgId, data, fromPeerId);
}

void SystemMessageBridge::sendMessageToEntity(EntityId target, const String& messageName, const ConfigNode& messageData)
{
	system->sendEntityMessageConfig(target, messageName, messageData);
}

void SystemMessageBridge::sendMessageToSystem(const String& targetSystem, int messageType, gsl::span<const std::byte> data, SystemMessageCallback callback, uint8_t fromPeerId)
{
	system->sendSystemMessage(targetSystem, messageType, data, std::move(callback), fromPeerId);
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
		HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
		initBase();
		HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
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
}

void System::doProcessMessages(FamilyBindingBase& family, gsl::span<const int> typesAccepted)
{
	// This whole method is probably very inefficient.
	struct MessageBox
	{
		Vector<Message*> msg;
		Vector<size_t> elemIdx;
	};
	HashMap<int, MessageBox> inboxes;

	const size_t sz = family.count();
	for (size_t i = 0; i < sz; i++) {
		const FamilyBase* elem = static_cast<FamilyBase*>(family.getElement(i));
		const Entity* entity = world->tryGetRawEntity(elem->entityId);
		if (entity) {
			for (const auto& msg: entity->inbox) {
				if (std::find(typesAccepted.begin(), typesAccepted.end(), msg.type) != typesAccepted.end()) {
					auto& inbox = inboxes[msg.type];
					inbox.msg.emplace_back(msg.msg.get());
					inbox.elemIdx.emplace_back(i);
				}
			}
		}
	}
	for (auto& iter: inboxes) {
		const int msgId = iter.first;
		auto& inbox = iter.second;
		onMessagesReceived(msgId, inbox.msg.data(), inbox.elemIdx.data(), inbox.msg.size(), family);
	}
}

void System::doSendMessage(EntityId entityId, std::unique_ptr<Message> msg, int id)
{
	const auto e = world->tryGetEntity(entityId);
	if (!e.isValid()) {
		return;
	}

	if (world->isEntityNetworkRemote(e)) {
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

size_t System::doSendSystemMessage(SystemMessageContext context, const String& targetSystem, SystemMessageDestination destination)
{
	return world->sendSystemMessage(std::move(context), targetSystem, destination);
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
	ProfilerEvent event(ProfilerEventType::WorldSystemMessages, name);

	for (auto& message: systemMessages) {
		onSystemMessageReceived(*message);
	}
	systemMessages.clear();
}

size_t System::getSystemMessagesInInbox() const
{
	return systemMessageInbox.size();
}

void System::sendEntityMessage(EntityId target, int msgId, gsl::span<const std::byte> data, uint8_t fromPeerId)
{
	auto msg = world->deserializeMessage(msgId, data);
	msg->fromPeerId = fromPeerId;
	doSendMessage(target, std::move(msg), msgId);
}

void System::sendSystemMessage(const String& targetSystem, int msgId, gsl::span<const std::byte> data, SystemMessageCallback callback, uint8_t fromPeerId)
{
	SystemMessageContext context;

	context.msgId = msgId;
	context.remote = true;
	context.msg = world->deserializeSystemMessage(msgId, data);
	context.msg->fromPeerId = fromPeerId;
	context.callback = std::move(callback);
	
	doSendSystemMessage(std::move(context), targetSystem, SystemMessageDestination::Local);
}

void System::sendEntityMessageConfig(EntityId target, const String& messageType, const ConfigNode& data)
{
	auto msg = world->deserializeMessage(messageType, data);
	const auto id = msg->getId();
	doSendMessage(target, std::move(msg), id);
}

void System::sendSystemMessageConfig(const String& targetSystem, const String& messageType, const ConfigNode& data)
{
	auto msg = world->deserializeSystemMessage(messageType, data);
	auto destination = msg->getMessageDestination();

	SystemMessageContext context;

	context.msgId = msg->getId();
	context.remote = false;
	context.msg = std::move(msg);
	//context.callback = [] () {};
	
	doSendSystemMessage(std::move(context), targetSystem, SystemMessageDestination::Local);

}

void System::doUpdate(Time time) {
	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
	ProfilerEvent event(ProfilerEventType::WorldSystemUpdate, name);

	if (!messageTypesReceived.empty()) {
		processMessages();
	}
	purgeMessages();
	
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
