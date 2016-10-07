#include "system.h"
#include <halley/data_structures/flat_map.h>
#include "halley/support/debug.h"

using namespace Halley;

System::System(std::initializer_list<FamilyBindingBase*> uninitializedFamilies, std::initializer_list<int> messageTypesReceived)
	: families(uninitializedFamilies)
	, messageTypesReceived(messageTypesReceived)
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

void System::onAddedToWorld(World& w, int id) {
	world = &w;
	systemId = id;
	for (auto f : families) {
		f->bindFamily(w);
	}
}

void System::purgeMessages()
{
	if (messagesSentTo.size() > 0) {
		for (auto& target: messagesSentTo) {
			// Purge all messages of this age
			Entity* entity = world->tryGetEntity(target);

			if (entity) {
				auto& inbox = entity->inbox;
				size_t inboxSize = inbox.size();
				for (size_t i = 0; i < inboxSize; i++) {
					if (inbox[i].age == systemId) {
						inbox.erase(inbox.begin() + i);
						i--;
						inboxSize--;
					}
				}
			}
		}
		messagesSentTo.clear();
	}
}

void System::processMessages()
{
	// This whole method is probably very inefficient.
	struct Elem
	{
		EntityId id;
	};

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
			Elem* elem = reinterpret_cast<Elem*>(fam.getElement(i));
			Entity* entity = world->tryGetEntity(elem->id);
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

void System::doSendMessage(EntityId entityId, std::unique_ptr<Message> msg, size_t, int id)
{
	Entity* entity = world->tryGetEntity(entityId);
	if (entity) {
		entity->inbox.emplace_back(MessageEntry(std::move(msg), id, systemId));
		messagesSentTo.push_back(entityId);
	}
}

void System::doUpdate(Time time) {
	Debug::trace("Updating " + name);
	timer.beginSample();

	purgeMessages();
	if (!messageTypesReceived.empty()) {
		processMessages();
	}
	
	updateBase(time);
	
	timer.endSample();
	Debug::trace("Done updating " + name);
}

void System::doRender(Painter& painter) {
	Debug::trace("Rendering " + name);
	timer.beginSample();

	renderBase(painter);

	timer.endSample();
	Debug::trace("Done rendering " + name);
}
