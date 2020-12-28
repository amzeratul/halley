#pragma once

#include <atomic>
#include <thread>
#include <array>
#include <condition_variable>
#include "metadata.h"
#include "halley/text/string_converter.h"

#if defined(DEV_BUILD) && !defined(__NX_TOOLCHAIN_MAJOR__)
#define ENABLE_HOT_RELOAD
#endif

namespace Halley
{
	enum class ImportAssetType
	{
		Undefined,
		Skip,
		Codegen,
		SimpleCopy,
		Font,
		BitmapFont,
		Image,
		Texture,
		Material,
		Animation,
		Config,
		Audio,
		AudioEvent,
		Sprite,
		SpriteSheet,
		Shader,
		Mesh,
		VariableTable,
		Prefab,
		Scene
	};

	template <>
	struct EnumNames<ImportAssetType> {
		constexpr std::array<const char*, 20> operator()() const {
			return{{
				"undefined",
				"skip",
				"codegen",
				"simpleCopy",
				"font",
				"bitmapFont",
				"image",
				"texture",
				"material",
				"animation",
				"config",
				"audio",
				"audioEvent",
				"sprite",
				"spriteSheet",
				"shader",
				"mesh",
				"variableTable",
				"prefab",
				"scene"
			}};
		}
	};

	// This order matters.
	// Assets which depend on other types should show up on the list AFTER
	// e.g. since materials depend on shaders, they show after shaders
	enum class AssetType
	{
		BinaryFile,
		TextFile,
		ConfigFile,
		Texture,
		Shader,
		MaterialDefinition, // Depends on Texture and Shader
		Image,
		SpriteSheet, // Depends on MaterialDefinition
		Sprite, // Depends on SpriteSheet
		Animation, // Depends on SpriteSheet
		Font, // Depends on SpriteSheet
		AudioClip,
		AudioEvent, // Depends on AudioClip
		Mesh,
		MeshAnimation, // Depends on Mesh
		VariableTable,
		Prefab,
		Scene
	};

	template <>
	struct EnumNames<AssetType> {
		constexpr std::array<const char*, 18> operator()() const {
			return{{
				"binaryFile",
				"textFile",
				"configFile",
				"texture",
				"shader",
				"materialDefinition",
				"image",
				"spriteSheet",
				"sprite",
				"animation",
				"font",
				"audioClip",
				"audioEvent",
				"mesh",
				"meshAnimation",
				"variableTable",
				"prefab",
				"scene"
			}};
		}
	};

	class ResourceObserver;
	class Resources;

	class Resource
	{
	public:
		virtual ~Resource();

		void setMeta(Metadata meta);
		const Metadata& getMeta() const;
		bool isMetaSet() const;
		
		void setAssetId(String name);
		const String& getAssetId() const;
		virtual void onLoaded(Resources& resources);
		
		int getAssetVersion() const;
		void reloadResource(Resource&& resource);

	protected:
		virtual void reload(Resource&& resource);

	private:
		Metadata meta;
		String assetId;
		int assetVersion = 0;
		bool metaSet = false;
	};

	class ResourceObserver
	{
	public:
		ResourceObserver();
		ResourceObserver(const Resource& res);
		virtual ~ResourceObserver();

		void startObserving(const Resource& res);
		void stopObserving();
		
		const Resource* getResourceBeingObserved() const;
		bool needsUpdate() const;
		virtual void update();

	private:
		const Resource* res = nullptr;
		int assetVersion = 0;
	};

	class AsyncResource : public Resource
	{
	public:
		AsyncResource();
		virtual ~AsyncResource();

		void startLoading(); // call from main thread before spinning worker thread
		void doneLoading();  // call from worker thread when done loading
		void loadingFailed(); // Call from worker thread if loading fails
		void waitForLoad() const;

		bool isLoaded() const;

	private:
		std::atomic<bool> failed;
		std::atomic<bool> loading;
		mutable std::condition_variable loadWait;
		mutable std::mutex loadMutex;
	};
}
