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

void System::processMessages()
{
}

void System::doProcessMessages(FamilyBindingBase& family, gsl::span<const int> typesAccepted)
{
	for (const int type: typesAccepted) {
		if (auto* inbox = world->getEntityMessageInbox(type); inbox && !inbox->empty()) {
			doProcessMessages(family, type, *inbox);
		}
	}
}

void System::doProcessMessages(FamilyBindingBase& family, int messageType, Vector<std::pair<MessageEntry, EntityId>>& messages)
{
	auto targetIds = VectorTemp<EntityId>(world->getUpdateMemoryPool());
	targetIds.reserve(messages.size());
	for (auto& msg: messages) {
		targetIds.push_back(msg.second);
	}

	auto msgPtrs = VectorTemp<Message*>(world->getUpdateMemoryPool());
	auto elemIdx = VectorTemp<size_t>(world->getUpdateMemoryPool());

	const size_t sz = family.count();
	for (size_t i = 0; i < sz; i++) {
		const FamilyBase* elem = static_cast<FamilyBase*>(family.getElement(i));
		if (std_ex::contains(targetIds, elem->entityId)) {
			for (auto& msg: messages) {
				if (msg.second == elem->entityId) {
					msgPtrs.emplace_back(msg.first.msg.get());
					elemIdx.emplace_back(i);
				}
			}
		}
	}

	if (!msgPtrs.empty()) {
		onMessagesReceived(messageType, msgPtrs.data(), elemIdx.data(), msgPtrs.size(), family);
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
		outbox.emplace_back(std::make_pair(MessageEntry(std::move(msg), id, systemId), entityId));
	}
}

void System::dispatchMessages()
{
	if (!outbox.empty()) {
		for (auto& o: outbox) {
			const int type = o.first.type;
			world->sendEntityMessage(o.second, std::move(o.first));
			if (!std_ex::contains(messageTypesSentThisUpdate, type)) {
				messageTypesSentThisUpdate.push_back(type);
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
	if (!messageTypesSentThisUpdate.empty()) {
		world->purgeMessages(systemId, messageTypesSentThisUpdate);
		messageTypesSentThisUpdate.clear();
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
