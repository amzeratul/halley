#include "../../core/include/halley_main.h"

using namespace Halley;

class SampleGame : public Game
{
public:
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
