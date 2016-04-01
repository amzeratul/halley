#include <chrono>
#include "system.h"
#include <iostream>

using namespace Halley;

Halley::System::System(std::initializer_list<FamilyBindingBase*> uninitializedFamilies)
	: families(uninitializedFamilies)
{
}

void Halley::System::onAddedToWorld(World& w) {
	this->world = &w;
	for (auto f : families) {
		f->bindFamily(w);
	}
}

void Halley::System::doUpdate(Time time) {
	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	
	update(time);
	
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<nanoseconds>(end - start).count();
	nsTaken = static_cast<int>(duration);
}

void Halley::System::doRender(Painter& painter) const {
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	render(painter);

	auto end = high_resolution_clock::now();
	auto duration = duration_cast<nanoseconds>(end - start).count();
	nsTaken = static_cast<int>(duration);
}
