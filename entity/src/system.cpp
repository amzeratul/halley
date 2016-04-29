#include <chrono>
#include "system.h"
#include <iostream>

using namespace Halley;

System::System(std::initializer_list<FamilyBindingBase*> uninitializedFamilies)
	: families(uninitializedFamilies)
{
}

void System::onAddedToWorld(World& w) {
	this->world = &w;
	for (auto f : families) {
		f->bindFamily(w);
	}
}

void System::doSendMessage(EntityId, const Message&, size_t, int)
{
	// TODO
}

void System::doUpdate(Time time) {
	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	
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
