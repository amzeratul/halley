#include "../../core/include/halley_main.h"

using namespace Halley;

class TestStage : public Stage
{
public:
	void onUpdate(Time) override
	{
		std::cout << "Hello! :D" << std::endl;
	}
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
