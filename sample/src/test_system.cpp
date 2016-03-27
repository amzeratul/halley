#include "../gen/cpp/systems/test_system.h"

void TestSystem::tick(Halley::Time time)
{
	for (auto& e : mainFamily) {
		e.test->test++;
		e.test->elapsed += static_cast<float>(time);
		e.foo->foo += 0.1f;
	}

	for (auto& e : auxFamily) {
		e.bar->bar += e.test->elapsed;
	}
}
