#include "halley/tools/project/project_loader.h"
#include "halley/core/game/halley_statics.h"
#include "halley/tools/file/filesystem.h"
#include "halley/support/debug.h"
using namespace Halley;


ProjectLoader::ProjectLoader(const HalleyStatics& statics, const Path& halleyPath)
	: statics(statics)
	, halleyPath(halleyPath)
{}

std::unique_ptr<Project> ProjectLoader::loadProject(const Path& path) const
{
	auto project = std::make_unique<Project>(curPlatforms, path, halleyPath, plugins);
	project->setAssetPackManifest(path / "halley_project" / "asset_manifest.yaml"); // HACK
	return std::move(project);
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

void ProjectLoader::setPlatforms(std::vector<String> platforms)
{
	plugins.clear();

	// Initialize known platforms
	std::vector<bool> knownPlatforms(platforms.size());
	for (size_t i = 0; i < platforms.size(); ++i) {
		knownPlatforms[i] = platforms[i] == "pc";
	}

	// Look for plugins
	auto pluginPath = halleyPath / "plugins";
	auto files = FileSystem::enumerateDirectory(pluginPath);
	for (auto& file: files) {
		if (file.getExtension() == getDLLExtension()) {
			auto plugin = loadPlugin(pluginPath / file);
			if (plugin && plugin->isDebug() == Debug::isDebug()) {
				// Valid plugin, check platform compatibility
				auto pluginPlatforms = plugin->getSupportedPlatforms();
				bool accepted = false;
				for (size_t i = 0; i < platforms.size(); ++i) {
					for (auto& pluginPlatform: pluginPlatforms) {
						accepted |= pluginPlatform == "*" || pluginPlatform == platforms[i];
						knownPlatforms[i] = knownPlatforms[i] || pluginPlatform == platforms[i];
					}
				}
				if (accepted) {
					plugins.emplace_back(std::move(plugin));
				}
			}
		}
	}
	
	for (size_t i = 0; i < platforms.size(); ++i) {
		if (!knownPlatforms[i]) {
			throw Exception("Unknown platform: " + platforms[i], HalleyExceptions::Tools);
		}
	}

	curPlatforms = std::move(platforms);
}

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#else
    #include <stdlib.h>
    #include <stdio.h>
    #include <dlfcn.h>
#endif

HalleyPluginPtr ProjectLoader::loadPlugin(const Path& path)
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
