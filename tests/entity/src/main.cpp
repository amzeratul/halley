#include "prec.h"
#include "test_stage.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);

//#define WITH_BLAH_STAGE

namespace Stages {
	enum Type
	{
		Test
#ifdef WITH_BLAH_STAGE
		,Blah
#endif
	};
}

#ifdef WITH_BLAH_STAGE
class BlahStage : public Stage
{
public:
	void onFixedUpdate(Time) override { std::cout << "Hello"; }
};
#endif

class EntityTestGame final : public Game
{
public:
	int initPlugins(IPluginRegistry &registry) override
	{
		initOpenGLPlugin(registry);
		return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input;
	}

	void initResourceLocator(String dataPath, ResourceLocator& locator) override
	{
		locator.addFileSystem(dataPath);
	}

	std::unique_ptr<Stage> makeStage(StageID id) override
	{
		switch (id) {
		case Stages::Test:
			return std::make_unique<TestStage>();
#ifdef WITH_BLAH_STAGE
		case Stages::Blah:
			return std::make_unique<BlahStage>();
#endif
		default:
			return std::unique_ptr<Stage>();
		}
	}
	
	String getName() const override
	{
		return "Entity test";
	}

	String getDataPath() const override
	{
		return "halley/entity-test";
	}

	bool isDevBuild() const override
	{
		return true;
	}

	std::unique_ptr<Stage> startGame(HalleyAPI* api) override
	{
		api->video->setWindow(WindowDefinition(WindowType::Window, Vector2i(1280, 720), getName()), false);
		return std::make_unique<TestStage>();
	}
};

HalleyGame(EntityTestGame);
