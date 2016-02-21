#include <chrono>
#include "system.h"
#include <iostream>

using namespace Halley;

Halley::System::System(std::initializer_list<FamilyBindingBase*> uninitializedFamilies)
	: families(uninitializedFamilies)
{
}

void Halley::System::onAddedToWorld() {
	for (auto f : families) {
		std::cout << "Family with masks: " << f->readMask << ", " << f->writeMask << std::endl;
	}
}

void Halley::System::step() {
	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	
	tick(0.0f);
	
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<nanoseconds>(end - start).count();
	nsTaken = static_cast<int>(duration);
}
