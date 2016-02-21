#include "../gen/cpp/systems/test_system.h"

static int test = 0;

void TestSystem::tick(Halley::Time)
{
	for (int i = 0; i < 10000; i++) {
		test = ((test * 3) + 17) % 2490951;
	}
}
