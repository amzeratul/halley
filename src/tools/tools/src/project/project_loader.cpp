#include "halley/tools/project/project_loader.h"
#include "halley/core/game/halley_statics.h"
#include "halley/tools/file/filesystem.h"
#include "halley/support/debug.h"
#include "halley/support/logger.h"
using namespace Halley;


ProjectLoader::ProjectLoader(const HalleyStatics& statics, const Path& halleyPath, std::vector<String> disabledPlatforms)
	: statics(statics)
	, halleyPath(halleyPath)
	, disabledPlatforms(std::move(disabledPlatforms))
{
	loadPlugins();
}

void ProjectLoader::setDisabledPlatforms(std::vector<String> platforms)
{
	disabledPlatforms = std::move(platforms);
}

std::unique_ptr<Project> ProjectLoader::loadProject(const Path& path) const
{
	auto proj = std::make_unique<Project>(path, halleyPath);
	selectPlugins(*proj);	
	return proj;
}

void ProjectLoader::selectPlugins(Project& project) const
{
	auto platforms = project.getPlatforms();
	platforms.erase(std::remove_if(platforms.begin(), platforms.end(), [&] (const String& platform)
	{
		return std::find(disabledPlatforms.begin(), disabledPlatforms.end(), platform) != disabledPlatforms.end();
	}), platforms.end());
	
	project.setPlugins(getPlugins(platforms));
}

static String getDLLExtension()
{
#if defined (_WIN32)
	return ".dll";
#elif defined(__APPLE__)
	return ".dylib";
#else
	return ".so";
#endif	
}

void ProjectLoader::loadPlugins()
{
	knownPlatforms.clear();

	// Look for plugins
	auto pluginPath = halleyPath / "plugins";
	auto files = FileSystem::enumerateDirectory(pluginPath);
	for (auto& file: files) {
		if (file.getExtension() == getDLLExtension()) {
			auto plugin = loadPlugin(pluginPath / file);

			// Don't mix debug and release plugin/editor
			if (plugin && plugin->isDebug() == Debug::isDebug()) {
				for (auto& plat: plugin->getSupportedPlatforms()) {
					if (plat != "*" && std::find(knownPlatforms.begin(), knownPlatforms.end(), plat) == knownPlatforms.end()) {
						knownPlatforms.push_back(plat);
					}
				}
				plugins.push_back(std::move(plugin));
			}
		}
	}
}

std::vector<HalleyPluginPtr> ProjectLoader::getPlugins(std::vector<String> platforms) const
{
	std::vector<HalleyPluginPtr> result;

	// Initialize known platforms
	std::vector<bool> foundPlatforms(platforms.size());
	for (size_t i = 0; i < platforms.size(); ++i) {
		foundPlatforms[i] = platforms[i] == "pc";
	}

	for (auto& plugin: plugins) {
		bool accepted = false;
		for (size_t i = 0; i < platforms.size(); ++i) {
			for (auto& pluginPlatform: plugin->getSupportedPlatforms()) {
				accepted |= pluginPlatform == "*" || pluginPlatform == platforms[i];
				foundPlatforms[i] = foundPlatforms[i] || pluginPlatform == platforms[i];
			}
		}
		if (accepted) {
			result.emplace_back(plugin);
		}
	}
	
	for (size_t i = 0; i < platforms.size(); ++i) {
		if (!foundPlatforms[i]) {
			Logger::logError("Unknown platform: \"" + platforms[i] + "\".");
		}
	}

	return result;
}

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#else
    #include <stdlib.h>
    #include <stdio.h>
    #include <dlfcn.h>
#endif

HalleyPluginPtr ProjectLoader::loadPlugin(const Path& path) const
{
	using CreateHalleyPluginFn = IHalleyPlugin*(HalleyStatics*);
	using DestroyHalleyPluginFn = void(IHalleyPlugin*);

#ifdef _WIN32

	String nativePath = path.getString().replaceAll("/", "\\");
	auto module = LoadLibrary(nativePath.c_str());
	if (!module) {
		return {};
	}

	auto createHalleyPlugin = reinterpret_cast<CreateHalleyPluginFn*>(GetProcAddress(module, "createHalleyPlugin"));
	auto destroyHalleyPlugin = reinterpret_cast<DestroyHalleyPluginFn*>(GetProcAddress(module, "destroyHalleyPlugin"));
	if (!createHalleyPlugin || !destroyHalleyPlugin) {
		FreeLibrary(module);
		return {};
	}

	return HalleyPluginPtr(createHalleyPlugin(const_cast<HalleyStatics*>(&statics)), [=] (IHalleyPlugin* plugin)
	{
		destroyHalleyPlugin(plugin);
		FreeLibrary(module);
	});

#else

	// Warning: untested on Mac/Linux :)
	auto module = dlopen(path.getString().c_str(), RTLD_LAZY);
	if (!module) {
		return {};
	}

	auto createHalleyPlugin = reinterpret_cast<CreateHalleyPluginFn*>(dlsym(module, "createHalleyPlugin"));
	auto destroyHalleyPlugin = reinterpret_cast<DestroyHalleyPluginFn*>(dlsym(module, "destroyHalleyPlugin"));
	if (!createHalleyPlugin || !destroyHalleyPlugin) {
		dlclose(module);
		return {};
	}

	return HalleyPluginPtr(createHalleyPlugin(const_cast<HalleyStatics*>(&statics)), [=] (IHalleyPlugin* plugin)
	{
		destroyHalleyPlugin(plugin);
		dlclose(module);
	});

#endif
}
