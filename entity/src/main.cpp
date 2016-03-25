#include "world.h"
#include "../gen/cpp/systems/test_system.h"

using namespace Halley;

int main()
{
	World world;
	world.addSystem(std::make_unique<TestSystem>());

	auto& e0 = world.createEntity();
	e0.addComponent(new TestComponent());
	e0.addComponent(new FooComponent());
	e0.addComponent(new BarComponent());

	auto& e1 = world.createEntity();
	e1.addComponent(new TestComponent());
	e1.addComponent(new FooComponent());

	auto& e2 = world.createEntity();
	e2.addComponent(new TestComponent());
	e2.addComponent(new BarComponent());

	for (int i = 0; i < 100; i++) {
		world.step(0.016667f);
	}
	return 0;
}
