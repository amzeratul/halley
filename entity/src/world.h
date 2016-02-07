#pragma once

#include <memory>
#include <vector>

class System;

class World
{
public:
	World();
	~World();

	System& addSystem(std::unique_ptr<System> system);
	void step();

private:
	std::vector<std::unique_ptr<System>> systems;
};
