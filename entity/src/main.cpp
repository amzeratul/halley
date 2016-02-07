#include "world.h"
#include "../gen/cpp/systems/test_system.h"

int main()
{
	World world;
	world.addSystem(std::make_unique<TestSystem>());

	for (int i = 0; i < 100; i++) {
		world.step();
	}
	return 0;
}
