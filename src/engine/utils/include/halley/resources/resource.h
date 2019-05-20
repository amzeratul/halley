#pragma once

#include <atomic>
#include <thread>
#include <array>
#include <condition_variable>
#include "metadata.h"
#include "halley/text/string_converter.h"

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
		Mesh
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
		MaterialDefinition,
		Image,
		Sprite,
		SpriteSheet,
		Animation,
		Font,
		AudioClip,
		AudioEvent,
		Mesh,
		MeshAnimation
	};

	template <>
	struct EnumNames<AssetType> {
		constexpr std::array<const char*, 15> operator()() const {
			return{{
				"binaryFile",
				"textFile",
				"configFile",
				"texture",
				"shader",
				"materialDefinition",
				"image",
				"sprite",
				"spriteSheet",
				"animation",
				"font",
				"audioClip",
				"audioEvent",
				"mesh",
				"meshAnimation"
			}};
		}
	};

	class ResourceObserver;
	class Resources;

	class Resource
	{
	public:
		virtual ~Resource();

		void setMeta(const Metadata& meta);
		const Metadata& getMeta() const;
		void setAssetId(const String& name);
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
