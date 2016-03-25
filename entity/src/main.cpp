#include "world.h"
#include "../gen/cpp/systems/test_system.h"

using namespace Halley;

int main()
{
	World world;
	world.addSystem(std::make_unique<TestSystem>());

	auto e = world.createEntity()
		.addComponent(new TestComponent())
		.addComponent(new FooComponent())
		.addComponent(new BarComponent());

	EntityId id2 = 0;
	
	for (int i = 0; i < 100; i++) {
		if (i == 20) {
			world.createEntity()
				.addComponent(new TestComponent())
				.addComponent(new FooComponent());
		}
		if (i == 40) {
			id2 = world.createEntity()
				.addComponent(new TestComponent())
				.addComponent(new BarComponent())
				.getEntityId();
		}
		if (i == 60) {
			world.getEntity(id2).removeComponent<TestComponent>();
		}
		if (i == 80) {
			world.destroyEntity(id2);
		}
		world.step(TimeLine::VariableUpdate, 0.016667f);
		std::cout << "Took " << world.getLastStepLength() << " ms.\n";
	}

	std::cout << "Final bar: " << e.getComponent<BarComponent>()->bar << std::endl;

	return 0;
}
