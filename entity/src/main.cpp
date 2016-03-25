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

	EntityId id = 0;
	
	for (int i = 0; i < 100; i++) {
		if (i == 20) {
			auto& e1 = world.createEntity();
			e1.addComponent(new TestComponent());
			e1.addComponent(new FooComponent());
		}
		if (i == 40) {
			auto& e2 = world.createEntity();
			e2.addComponent(new TestComponent());
			e2.addComponent(new BarComponent());
			id = e2.getUID();
		}
		if (i == 80) {
			world.destroyEntity(id);
		}
		world.step(0.016667f);
	}

	std::cout << "Final bar: " << e0.getComponent<BarComponent>()->bar << std::endl;

	return 0;
}
