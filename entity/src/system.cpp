#include <chrono>
#include "system.h"
#include <iostream>

using namespace Halley;

System::System(std::initializer_list<FamilyBindingBase*> uninitializedFamilies, std::initializer_list<int> messageTypesReceived)
	: families(uninitializedFamilies)
	, messageTypesReceived(messageTypesReceived)
{
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
		std::vector<Message*> msg;
		std::vector<size_t> elemIdx;
	};
	std::map<int, MessageBox> inboxes;

	if (!families.empty()) {
		auto& fam = *families[0];
		size_t sz = fam.count();
		for (size_t i = 0; i < sz; i++) {
			Elem* elem = reinterpret_cast<Elem*>(fam.getElement(i));
			Entity* entity = world->tryGetEntity(elem->id);
			
			for (auto& msg: entity->inbox) {
				if (std::find(messageTypesReceived.begin(), messageTypesReceived.end(), msg.type) != messageTypesReceived.end()) {
					auto& inbox = inboxes[msg.type];
					inbox.msg.emplace_back(msg.msg.get());
					inbox.elemIdx.emplace_back(i);
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
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	purgeMessages();
	if (!messageTypesReceived.empty()) {
		processMessages();
	}
	
	updateBase(time);
	
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<nanoseconds>(end - start).count();
	nsTaken = static_cast<int>(duration);
}

void System::doRender(Painter& painter) {
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	renderBase(painter);

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<nanoseconds>(end - start).count();
	nsTaken = static_cast<int>(duration);
}
