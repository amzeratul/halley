#include "prec.h"
#include "test_stage.h"

using namespace Halley;

void initOpenGLPlugin();

namespace Stages {
	enum Type
	{
		Test
	};
}

class SampleGame final : public Game
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
		api->video->setVideo(WindowType::Window, Vector2i(1280, 720), Vector2i(1280, 720), Vector2f(), false);
	}
};

HalleyGame(SampleGame);
