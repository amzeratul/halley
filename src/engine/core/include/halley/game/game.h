#pragma once

#include "halley/stage/stage.h"
#include "halley/game/scene_editor_interface.h"
#include "frame_data.h"

namespace Halley
{
	class UIDebugConsoleCommands;
	class SceneEditorContext;
	class IPluginRegistry;
	class ResourceLocator;
	class Stage;
	class Environment;
	class GameConsole;
	class ISceneEditor;
	class IEditorCustomTools;
	class AssetPreviewGenerator;
	
	class Game
	{
	public:
		struct ConsoleInfo {
			String name;
			std::optional<int> monitor;
			Vector2f monitorAlign;
		};
		
		virtual ~Game();

		virtual void init(const Environment&, const Vector<String>& /*args*/);
		virtual int initPlugins(IPluginRegistry &registry) = 0;
		virtual ResourceOptions initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator);

		virtual String getName() const = 0;
		virtual String getDataPath() const = 0;
        virtual String getLogFileName() const;
		virtual bool isDevMode() const = 0;
		virtual bool shouldCreateSeparateConsole() const;
		virtual ConsoleInfo getConsoleInfo() const;

		virtual std::unique_ptr<Stage> startGame() = 0;
		virtual void endGame();

		virtual std::unique_ptr<Stage> makeStage(StageID id);
		virtual std::unique_ptr<BaseFrameData> makeFrameData();

		virtual double getTargetFPS() const;
		virtual double getTargetBackgroundFPS() const;
		virtual double getFixedUpdateFPS() const;
		virtual size_t getMaxThreads() const;
		virtual bool shouldProcessEventsOnFixedUpdate() const;

		virtual String getDevConAddress() const;
		virtual int getDevConPort() const;

		virtual std::shared_ptr<GameConsole> getGameConsole() const;
		virtual void attachToEditorDebugConsole(UIDebugConsoleCommands& commands, Resources& gameResources, IProject& project);

		virtual void onUncaughtException(const Exception& exception, TimeLine timeLine);

		virtual std::unique_ptr<ISceneEditor> createSceneEditorInterface();
		virtual std::unique_ptr<IEditorCustomTools> createEditorCustomToolsInterface();
		virtual std::unique_ptr<AssetPreviewGenerator> createAssetPreviewGenerator(const HalleyAPI& api, Resources& resources);
		virtual std::unique_ptr<UIFactory> createUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n);
		virtual std::unique_ptr<ScriptNodeTypeCollection> createScriptNodeTypeCollection();
		virtual Vector<std::unique_ptr<IComponentEditorFieldFactory>> createCustomEditorFieldFactories(Resources& resources);

		virtual String getDefaultColourScheme();

		const HalleyAPI& getAPI() const;
		Resources& getResources() const;

	private:
		friend class Core;
		
		const HalleyAPI* api = nullptr;
		Resources* resources = nullptr;
	};
}
