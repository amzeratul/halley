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

void Halley::System::step() {
	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	
	tick(0.016667f); // TODO: get correct time
	
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<nanoseconds>(end - start).count();
	nsTaken = static_cast<int>(duration);
}
