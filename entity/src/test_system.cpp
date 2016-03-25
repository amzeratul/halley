#include "../gen/cpp/systems/test_system.h"

static int test = 0;

void TestSystem::tick(Halley::Time time)
{
	for (size_t i = 0; i < mainFamily.count(); i++) {
		auto& e = mainFamily[i];
		e.test->test++;
		e.test->elapsed += static_cast<float>(time);
		e.foo->foo += 0.1f;
	}

	for (size_t i = 0; i < auxFamily.count(); i++) {
		auto& e = auxFamily[i];
		e.bar->bar += e.test->elapsed;
	}
}
