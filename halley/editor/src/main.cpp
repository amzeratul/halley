#include "prec.h"
#include "editor_root_stage.h"

using namespace Halley;

void initOpenGLPlugin(IPluginRegistry &registry);

namespace Stages {
	enum Type
	{
		Root
	};
}

class HalleyEditor final : public Game
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
		case Stages::Root:
			return std::make_unique<EditorRootStage>();
		default:
			return std::unique_ptr<Stage>();
		}
	}

	StageID getInitialStage() const override
	{
		return Stages::Root;
	}

	String getName() const override
	{
		return "Halley Editor";
	}

	String getDataPath() const override
	{
		return "halley/editor";
	}

	bool isDevBuild() const override
	{
		return true;
	}

	void init(HalleyAPI* api) override
	{
		Vector2i winSize(1280, 720);
		api->video->setWindow(Window(WindowType::ResizableWindow, api->video->getCenteredWindow(winSize, 0), winSize, getName()), false);
	}
};

HalleyGame(HalleyEditor);
