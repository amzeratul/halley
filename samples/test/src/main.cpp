#include "prec.h"
#include "../gen/cpp/systems/test_system.h"
#include "../gen/cpp/systems/render_system.h"

using namespace Halley;

void initOpenGLPlugin();

class TestStage : public Stage
{
public:
	void init() override
	{
		world.addSystem(std::make_unique<TestSystem>(), TimeLine::FixedUpdate);
		world.addSystem(std::make_unique<RenderSystem>(), TimeLine::Render);

		id0 = world.createEntity()
			.addComponent(new TestComponent())
			.addComponent(new FooComponent())
			.addComponent(new BarComponent())
			.getEntityId();
	}

	void deInit() override
	{
		std::cout << "Final bar: " << world.getEntity(id0).getComponent<BarComponent>()->bar << std::endl;
	}

	void onVariableUpdate(Time) override
	{
	}
	
	void onFixedUpdate(Time time) override
	{
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

		world.step(TimeLine::FixedUpdate, time);
		i++;

		if (i == 100) {
			getAPI().core->quit();
		}
	}

	void onRender(Painter& painter) const override
	{
		painter.clear(Colour(0.2f, 0.2f, 0.3f));
		world.render(painter);

		auto texture = getResource<Texture>("sprites/test.png");
		auto shader = getResource<Shader>("shaders/sprite.yaml");
		Material material(shader, getAPI().video);
		material["main"] = texture;
		painter.drawSprite(material, Vector2f(100, 100));
	}

private:
	Halley::World world;
	int i = 0;
	EntityId id0;
	EntityId id2;
};

namespace Stages {
	enum Type
	{
		Test
	};
}

class SampleGame : public Game
{
public:
	int initPlugins() override
	{
		initOpenGLPlugin();
		return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input;
	}

	void initResourceLocator(ResourceLocator& locator) override
	{
		locator.addStandardFileSystem();
	}

	std::unique_ptr<Stage> makeStage(StageID id) override
	{
		switch (id) {
		case Stages::Test:
			return std::make_unique<TestStage>();
		default:
			return std::unique_ptr<Stage>();
		}
	}

	StageID getInitialStage() const override
	{
		return Stages::Test;
	}

	String getName() const override
	{
		return "Sample game";
	}

	String getDataPath() const override
	{
		return "halley/sample";
	}

	bool isDevBuild() const override
	{
		return true;
	}

	void init(HalleyAPI* api) override
	{
		api->video->setVideo(WindowType::Window, Vector2i(1280, 720), Vector2i(1280, 720));
	}
};

HalleyGame(SampleGame);
