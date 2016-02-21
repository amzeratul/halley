#include <iostream>
#include "world.h"
#include "system.h"
#include <chrono>

using namespace Halley;

Halley::World::World() = default;

Halley::World::~World() = default;

Halley::System& Halley::World::addSystem(std::unique_ptr<System> system)
{
	auto& ref = *system.get();
	systems.emplace_back(std::move(system));
	ref.onAddedToWorld();
	return ref;
}

void Halley::World::step()
{
	using namespace std::chrono;
	auto start = high_resolution_clock::now();

	for (auto& system : systems) {
		system->step();
	}

	auto end = high_resolution_clock::now();
	auto length = duration_cast<duration<double, std::milli>>(end - start).count();
	std::cout << "Step took " << length << " milliseconds." << std::endl;
}
