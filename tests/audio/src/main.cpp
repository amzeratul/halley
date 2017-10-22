#include "prec.h"
#include "test_stage.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);
void initSDLSystemPlugin(IPluginRegistry &registry);
void initSDLAudioPlugin(IPluginRegistry &registry);
void initSDLInputPlugin(IPluginRegistry &registry);

namespace Stages {
	enum Type
	{
		Test
	};
}

class AudioTestGame final : public Game
{
public:
	int initPlugins(IPluginRegistry &registry) override
	{
		initSDLSystemPlugin(registry);
		initSDLAudioPlugin(registry);
		initSDLInputPlugin(registry);
		initOpenGLPlugin(registry);
		return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input;
	}

	void initResourceLocator(Path dataPath, ResourceLocator& locator) override
	{
		locator.addFileSystem(dataPath);
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
	
	String getName() const override
	{
		return "Audio test";
	}

	String getDataPath() const override
	{
		return "halley/audio-test";
	}

	bool isDevBuild() const override
	{
		return true;
	}

	std::unique_ptr<Stage> startGame(HalleyAPI* api) override
	{
		api->audio->startPlayback();
		api->video->setWindow(WindowDefinition(WindowType::Window, Vector2i(1280, 720), getName()), true);
		return std::make_unique<TestStage>();
	}
};

HalleyGame(AudioTestGame);
