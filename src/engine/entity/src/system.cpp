#include "system.h"
#include <halley/data_structures/flat_map.h>
#include "halley/support/debug.h"

using namespace Halley;

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

void System::setCollectSamples(bool collect)
{
	collectSamples = collect;
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
				auto& inbox = entity->inbox;
				inbox.erase(std::remove_if(inbox.begin(), inbox.end(), [&] (const MessageEntry& e) { return e.age == systemId; }), inbox.end());
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
	outbox.emplace_back(std::make_pair(entityId, MessageEntry(std::move(msg), id, systemId)));
}

size_t System::doSendSystemMessage(SystemMessageContext context)
{
	const size_t nSystems = 0; // TODO: no idea how :D
	systemOutbox.emplace_back(std::move(context));
	return nSystems;
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

void System::doUpdate(Time time) {
	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
	if (collectSamples) {
		timer.beginSample();
	}

	purgeMessages();
	if (!messageTypesReceived.empty()) {
		processMessages();
	}
	
	updateBase(time);
	dispatchMessages();

	if (collectSamples) {
		timer.endSample();
	}
	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
}

void System::doRender(RenderContext& rc) {
	if (!initialised) {
		throw Exception("System " + name + " is being rendered before being initialised. Make sure a World::step() happens before World::render().", HalleyExceptions::Entity);
	}
	
	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
	if (collectSamples) {
		timer.beginSample();
	}

	renderBase(rc);

	if (collectSamples) {
		timer.endSample();
	}
	HALLEY_DEBUG_TRACE_COMMENT(name.c_str());
}
