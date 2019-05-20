#include "prec.h"
#include "test_stage.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);

class NetworkTestGame final : public Game
{
public:
	int initPlugins(IPluginRegistry &registry) override
	{
		initOpenGLPlugin(registry);
		return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input;
	}

	void initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator) override
	{
		locator.addFileSystem(unpackedAssetsPath);
	}

	String getName() const override
	{
		return "Network Test";
	}

	String getDataPath() const override
	{
		return "halley/network-test";
	}

	bool isDevMode() const override
	{
		return true;
	}

	std::unique_ptr<Stage> startGame(const HalleyAPI* api) override
	{
		api->video->setWindow(WindowDefinition(WindowType::Window, Vector2i(1280, 720), getName()));
		return std::make_unique<TestStage>();
	}
};

HalleyGame(NetworkTestGame);
