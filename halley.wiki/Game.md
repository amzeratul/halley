The entry point to a game made in Halley is a class derived from `Halley::Game`. This represents the unique instance of your game, and controls the running of the engine.

# Setup

In your **.cpp** file corresponding to your implementation of the game class, you'll need to implement the following magical macro, which initialises Halley with an instance of your game class:

```c++
class MyGame final : public Halley::Game
{
    // ...
}

HalleyGame(MyGame);
```

In a nutshell, this function defines a **main()** function, and creates a **Halley::Core** and instantiates **MyGame** with it.

# Initialisation order

When the game engine runs, it will call the Game methods in the following order:

1. Game Constructor
1. Game::init()
1. Game::initPlugins()
1. Game::initResourceLocator()
1. Game::startGame()
1. Halley Main Loop
1. Game::endGame()
1. Game Destructor

Some const methods (such as Game::getName()) will be called inbetween those.

# Understanding the game methods
## init
```c++
virtual void init(const Environment&, const Vector<String>& args);
```
This method allows you to parse the command line arguments passed to the game and store them in your instance.

## initPlugins
```c++
virtual int initPlugins(IPluginRegistry& registry) = 0;
```

This method allows you to initialise the plugins (see [[API]]) needed for this version of the game. Because plugins are linked as headerless libraries, you'll often want to declare a prototype for the plugin you're invoking here, and call it at this point, so Halley registers it. This method also returns which types of plugins Halley should attempt to initialise.

For a typical example of a game on PC:

```c++
void initOpenGLPlugin(IPluginRegistry &registry);
void initSDLSystemPlugin(IPluginRegistry &registry);
void initSDLAudioPlugin(IPluginRegistry &registry);
void initSDLInputPlugin(IPluginRegistry &registry);
void initDX11Plugin(IPluginRegistry &registry);
void initWinRTPlugin(IPluginRegistry &registry);
void initMFPlugin(IPluginRegistry &registry);

int MyGame::initPlugins(IPluginRegistry &registry)
{
#ifdef WITH_SDL2
	initSDLSystemPlugin(registry);
	initSDLInputPlugin(registry);
	initSDLAudioPlugin(registry);
#endif

#ifdef WITH_WINRT
	initWinRTPlugin(registry);
#endif

#ifdef WITH_MEDIA_FOUNDATION
	initMFPlugin(registry);
#endif

#ifdef WITH_OPENGL
	initOpenGLPlugin(registry);
#endif

#ifdef WITH_DX11
	initDX11Plugin(registry);
#endif

	return HalleyAPIFlags::Video | HalleyAPIFlags::Audio | HalleyAPIFlags::Input | HalleyAPIFlags::Movie;
}
```

## initResourceLocator
```c++
virtual void initResourceLocator(Path path, ResourceLocator& locator);
```

Allows you to configure the resource locator, used for loading assets. A typical implementation is simply:

```c++
void MyGame::initResourceLocator(Path dataPath, ResourceLocator& locator)
{
	locator.addFileSystem(dataPath);
}
```

## startGame
```c++
virtual std::unique_ptr<Stage> startGame(const HalleyAPI* api) = 0;
```

This method is called after the [[API]] has been initialised. It has two primary functions:
1. Initialise any parts of the API, such as [[Graphics|Graphics Setup]] and [[Audio|Audio Setup]]
2. Return the initial [[Stage|Stages]]

You may also want to perform other initialisation here, before the game starts. At this point, most of the engine is fully functional, and it's generally safe to use the various APIs (provided you've initialised anything you needed).

A simple example:

```c++
std::unique_ptr<Stage> MyGame::startGame(const HalleyAPI* api)
{
	api->video->setWindow(WindowDefinition(WindowType::BorderlessWindow, Vector2i(1280, 720), getName()), true);
	api->audio->startPlayback(audioDeviceN);

	return std::make_unique<MyStage>();
}
```

## endGame
```
virtual void endGame();
```

This method provides you a chance to do clean-up before exiting. You don't typically need to do anything here.