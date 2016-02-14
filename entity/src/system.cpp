#include <chrono>
#include "system.h"
#include <iostream>

System::System(std::initializer_list<FamilyMaskType>) {
	// TODO
}

void System::step()
{
	using namespace std::chrono;
	auto start = high_resolution_clock::now();
	
	tick(0.0f);
	
	auto end = high_resolution_clock::now();
	auto duration = duration_cast<nanoseconds>(end - start).count();
	nsTaken = static_cast<int>(duration);
}
