#include "launcher.h"

#include "launcher_stage.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);
void initSDLSystemPlugin(IPluginRegistry &registry, std::optional<String> cryptKey);
void initSDLAudioPlugin(IPluginRegistry &registry);
void initSDLInputPlugin(IPluginRegistry &registry);
void initAsioPlugin(IPluginRegistry &registry);
void initDX11Plugin(IPluginRegistry &registry);
void initMetalPlugin(IPluginRegistry &registry);

HalleyLauncher::HalleyLauncher()
{}

HalleyLauncher::~HalleyLauncher()
{}

void HalleyLauncher::init(const Environment& environment, const Vector<String>& args)
{}

int HalleyLauncher::initPlugins(IPluginRegistry& registry)
{
	initSDLSystemPlugin(registry, {});
	initAsioPlugin(registry);
	initSDLAudioPlugin(registry);
	initSDLInputPlugin(registry);

#ifdef _WIN32
	initDX11Plugin(registry);
#elif __APPLE__
	initMetalPlugin(registry);
#else
	initOpenGLPlugin(registry);
#endif
	
	return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input | HalleyAPIFlags::Network;
}

Resources::Options HalleyLauncher::initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator)
{
	locator.addFileSystem(unpackedAssetsPath);
	return {};
}

std::unique_ptr<Stage> HalleyLauncher::startGame()
{
	auto& api = getAPI();

	api.video->setWindow(WindowDefinition(WindowType::Window, Vector2i(1000, 500), "Halley Launcher"));
	api.video->setVsync(true);

	return std::make_unique<LauncherStage>();
}

String HalleyLauncher::getName() const
{
	return "Halley Launcher";
}

String HalleyLauncher::getDataPath() const
{
	return "halley/launcher";
}

bool HalleyLauncher::isDevMode() const
{
	return false;
}

bool HalleyLauncher::shouldCreateSeparateConsole() const
{
	return true;
}

HalleyGame(HalleyLauncher);
